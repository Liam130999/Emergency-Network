package bgu.spl.net.srv;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class ConnectionsImpl<T> implements Connections<T> {

    private final Map<Integer, ConnectionHandler<T>> connectionHandlers; // connectionId -> ConnectionHandler
    private final Map<String, Set<Integer>> channelSubscriptions; // channel -> Set of connectionIds
    private final Map<Integer, Map<String, String>> clientSubscriptions; // connectionId -> (channel -> subscriptionId)
    private final Map<Integer, Map<String, String>> subscriptionToChannel; // connectionId -> (subscriptionId ->
                                                                           // channel)
    private final Map<Integer, String> connectionToUser; // connectionId -> username
    private final Map<String, String> userCredentials; // username -> password
    private AtomicInteger messageIDgenerator;

    public ConnectionsImpl() {
        this.connectionHandlers = new ConcurrentHashMap<>();
        this.channelSubscriptions = new ConcurrentHashMap<>();
        this.clientSubscriptions = new ConcurrentHashMap<>();
        this.subscriptionToChannel = new ConcurrentHashMap<>();
        this.connectionToUser = new ConcurrentHashMap<>();
        this.userCredentials = new ConcurrentHashMap<>();
        this.messageIDgenerator = new AtomicInteger(-1);
    }

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = connectionHandlers.get(connectionId);
        if (handler != null) {
            //System.out.println(msg);
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg, int notYou) {
        Set<Integer> subscribers = channelSubscriptions.get(channel);
        if (subscribers != null) {
            for (int connectionId : subscribers) {
                if(connectionId!=notYou){
                    send(connectionId, msg);
                }
            }
        } 
    }

    @Override
    public void disconnect(int connectionId) {
        connectionHandlers.remove(connectionId);

        // Remove from all channels
        for (Set<Integer> subscribers : channelSubscriptions.values()) {
            subscribers.remove(connectionId);
        }

        // Remove the user association
        connectionToUser.remove(connectionId);

        // Remove all subscriptions for this client
        clientSubscriptions.remove(connectionId);
        subscriptionToChannel.remove(connectionId);

        //System.out.println("Connection " + connectionId + " disconnected and cleaned up.");
    }

    public void addConnection(int connectionId, ConnectionHandler<T> handler) {
        connectionHandlers.put(connectionId, handler);
    }

    // Prevent duplicate subscriptions to the same channe
    public boolean isSubscribed(int connectionId, String channel) {
        if(channel==null){
            return false;
        }
        Set<Integer> connectionIds = channelSubscriptions.get(channel);
        // If the channel exists, check if it contains the target integer
        return (connectionIds != null && connectionIds.contains(connectionId));
    }

    public void subscribe(int connectionId, String channel, String subscriptionId) {
        channelSubscriptions.putIfAbsent(channel, ConcurrentHashMap.newKeySet());
        channelSubscriptions.get(channel).add(connectionId);

        clientSubscriptions.putIfAbsent(connectionId, new ConcurrentHashMap<>());
        clientSubscriptions.get(connectionId).put(channel, subscriptionId);

        subscriptionToChannel.putIfAbsent(connectionId, new ConcurrentHashMap<>());
        subscriptionToChannel.get(connectionId).put(subscriptionId, channel);
    }

    public String findDestination(int connectionId, String subscriptionId) {
        Map<String, String> subs = subscriptionToChannel.get(connectionId);
        if (subs == null || subs.isEmpty()) {
            return null;
        }
        return subs.get(subscriptionId);
    }

    public void unsubscribe(int connectionId, String subscriptionId) {
        Map<String, String> subscriptions = subscriptionToChannel.get(connectionId);
        if (subscriptions != null) {
            String channel = subscriptions.remove(subscriptionId); // Direct lookup
            if (channel != null) {
                Set<Integer> subscribers = channelSubscriptions.get(channel);
                if (subscribers != null) {
                    subscribers.remove(connectionId);
                    if (subscribers.isEmpty()) {
                        channelSubscriptions.remove(channel);
                    }
                }
                clientSubscriptions.get(connectionId).remove(channel);
            } 
        }
    }

    public String getSubscriptionId(int connectionId, String channel) {
        Map<String, String> subscriptions = clientSubscriptions.get(connectionId);
        return subscriptions != null ? subscriptions.get(channel) : null;
    }

    public Set<Integer> getSubscribers(String channel) {
        return channelSubscriptions.getOrDefault(channel, Collections.emptySet());
    }

    public boolean validateOrRegisterUser(String login, String passcode) {
        if (userCredentials.containsKey(login)) {
            return userCredentials.get(login).equals(passcode);
        } else {
            userCredentials.put(login, passcode);
            return true;
        }
    }

    public boolean isUserLoggedIn(String login) {
        return connectionToUser.containsValue(login);
    }

    public void associateUserWithConnection(int connectionId, String login) {
        connectionToUser.put(connectionId, login);
    }

    public String getUserByConnectionId(int connectionId) {
        return connectionToUser.get(connectionId);
    }

    public int getMesageID() {
        return this.messageIDgenerator.incrementAndGet();
    }
}
