package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {

    private int connectionId;
    private Connections<String> connections;
    private boolean terminate;

    public StompMessagingProtocolImpl() {
        this.terminate = false;
    }

    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(String message) {
        // Trim the null character
        message = message.substring(0, message.length() - 1);

        // Split headers and body
        String[] lines = message.split("\n");
        String command = lines[0]; // The first line is the command
        String receiptId = extractHeader(lines, "receipt");

        switch (command) {
            case "CONNECT":
                handleConnect(lines, receiptId, message);
                break;
            case "SUBSCRIBE":
                handleSubscribe(lines, receiptId, message);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(lines, receiptId, message);
                break;
            case "SEND":
                handleSend(lines, receiptId, message);
                break;
            case "DISCONNECT":
                handleDisconnect(lines, receiptId, message);
                break;
            default:
                sendError("Unknown command",message, "Invalid STOMP command: " + command, receiptId);
        }
    }

    @Override
    public boolean shouldTerminate() {
        return terminate;
    }

    private void handleConnect(String[] lines, String receiptId, String frame) {
        String login = extractHeader(lines, "login");
        String passcode = extractHeader(lines, "passcode");
        // Ensure both login and passcode are provided
        if (login == null || passcode == null) {

            sendError("Malformed frame",frame, "CONNECT frame missing login or passcode.", receiptId);
            return;
        }

        ConnectionsImpl<String> connectionsImpl = getConnectionsImpl();
        if (connectionsImpl == null) {
            sendError("Internal Server Error",frame, "Connections object is not compatible.", receiptId);
            return;
        }

        if (connectionsImpl.isUserLoggedIn(login)) {
            sendError("User already logged in",frame, "The user is already connected.", receiptId);
            return;
        }

        if (!connectionsImpl.validateOrRegisterUser(login, passcode)) {
            sendError("Invalid credentials",frame, "The provided login or passcode is incorrect.", receiptId); //important
            return;
        }

        connectionsImpl.associateUserWithConnection(connectionId, login);
        connectionsImpl.send(connectionId, "CONNECTED\nversion:1.2\n\n");

        if (receiptId != null) {
            sendReceipt(receiptId);
        }
    }

    private void handleSubscribe(String[] lines, String receiptId, String frame) {
        String destination = extractHeader(lines, "destination");
        String id = extractHeader(lines, "id");

        // Validate required headers
        if (destination == null || id == null) {
           // System.out.println("destination == null || id == null");
            sendError("Malformed frame", frame, "SUBSCRIBE frame missing destination or id.", receiptId);
            return;
        }

        ConnectionsImpl<String> connectionsImpl = getConnectionsImpl();
        if (connectionsImpl == null) {
           // System.out.println("connectionsImpl == null");
            sendError("Internal Server Error", frame, "Connections object is not compatible.", receiptId);
            return;
        }

        if (connectionsImpl.isSubscribed(connectionId,destination)){
            sendError("Already signed in", frame, "You are already signed in to this channel", receiptId);
            return;
        }

        connectionsImpl.subscribe(connectionId, destination, id);

        if (receiptId != null) {
            sendReceipt(receiptId);
        }
    }

    private void handleUnsubscribe(String[] lines, String receiptId, String frame) {
        String subscriptionID = extractHeader(lines, "id");

        // Validate required header
        if (subscriptionID == null) {
            sendError("Malformed frame",frame, "UNSUBSCRIBE frame missing id.", receiptId);
            return;
        }

        ConnectionsImpl<String> connectionsImpl = getConnectionsImpl();
        if (connectionsImpl == null) {
            sendError("Internal Server Error",frame, "Connections object is not compatible.", receiptId);
            return;
        }

        if (!connectionsImpl.isSubscribed(connectionId,connectionsImpl.findDestination(connectionId, subscriptionID))){
            sendError("Wrong channel request", frame, "You are not subscribed to this channel", receiptId);
            return;
        }

        connectionsImpl.unsubscribe(connectionId, subscriptionID);

        if (receiptId != null) {
            sendReceipt(receiptId);
        }
    }

    private void handleSend(String[] lines, String receiptId, String frame) {
        String destination = extractHeader(lines, "destination");

        // Validate required header
        if (destination == null) {
            sendError("Malformed frame",frame, "SEND frame missing destination.", receiptId);
            return;
        }

        String body = extractBody(lines);

        ConnectionsImpl<String> connectionsImpl = getConnectionsImpl();
        if (connectionsImpl == null) {
            sendError("Internal Server Error",frame, "Connections object is not compatible.", receiptId);
            return;
        }

        // Construct the message frame to send to all subscribers
        String messageFrame = "MESSAGE\nsubscription:" + connectionsImpl.getSubscriptionId(connectionId, destination)
         +"\nmessage-id:" +getConnectionsImpl().getMesageID()+
          "\ndestination:" + destination + "\n\n" + body + "\n\n";


        if (!connectionsImpl.isSubscribed(connectionId, destination)){
            sendError("Cannot send report",frame, "You are not subscribed to this channel", receiptId);
            return;
        }

        // Use the optimized send method in ConnectionsImpl
        connections.send(destination, messageFrame, connectionId);
    }

    private void handleDisconnect(String[] lines, String receiptId, String frame) {
        if (receiptId != null) {
            sendReceipt(receiptId);
        }
        ConnectionsImpl<String> connectionsImpl = getConnectionsImpl();
        if (connectionsImpl != null) {
            connectionsImpl.disconnect(connectionId);
        }
       
        terminate = true;

    }

    private void sendReceipt(String receiptId) {
        if(receiptId==null){
            return;
        }
        connections.send(connectionId, "RECEIPT\nreceipt-id:" + receiptId + "\n\n");
    }

    private void sendError(String message, String frame, String body, String receiptID) {
        String txt = "";
        if(receiptID!=null){
            txt = "ERROR\nreceipt-id: "+receiptID+"\nmessage: " + message + "\n\nThe message:\n-----\n" + frame
         + "\n-----\n" + body +"\n\n";
        }
        else{
            txt = "ERROR\nmessage:" + message + "\n\nThe message:\n-----\n" + frame
         + "\n-----\n" + body +"\n\n";
        }

        connections.send(connectionId,txt);

        ConnectionsImpl<String> connectionsImpl = getConnectionsImpl();
        if (connectionsImpl != null) {
            connectionsImpl.disconnect(connectionId);
        }
    }

    private String extractHeader(String[] lines, String header) {
        for (String line : lines) {
            if (line.startsWith(header + ":")) {
                return line.substring(header.length() + 1).trim();
            }
        }
        return null;
    }

    private String extractBody(String[] lines) {
        StringBuilder eventDetails = new StringBuilder();
        boolean startCollecting = false;
    
        for (String line : lines) {
            // Start collecting after the "destination:" line
            if (line.startsWith("destination:")) {
                startCollecting = true;
                continue; // Skip the "destination:" line itself
            }
    
            // Collect all lines after "destination:"
            if (startCollecting) {
                eventDetails.append(line).append("\n");
            }
        }
    
        // Return the extracted section as a string
        return eventDetails.toString().trim();
    }
    

    private ConnectionsImpl<String> getConnectionsImpl() {
        return (ConnectionsImpl<String>) connections;
    }
}
