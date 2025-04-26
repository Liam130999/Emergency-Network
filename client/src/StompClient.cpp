#include <stdlib.h>
#include "../include/ConnectionHandler.h"
#include <thread>
#include <memory>
#include <chrono>
#include "../include/StompProtocol.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <csignal>
#include "ConcurrentHashMap.h"
#include <map>


using namespace std;

// Global variables
mutex myLock, waitLock;
condition_variable var;
string user = "";
bool handlerConnected = false; // Physical connection
std::thread* listenerThreadPtr = nullptr; // Global thread pointer for signal handling


int getReceiptId(const std::string& frame){
    const std::string receiptPrefix = "receipt-id:";
    size_t pos = frame.find(receiptPrefix);

    if (pos != std::string::npos) {
        pos += receiptPrefix.length(); // Move to the start of the ID
        size_t endPos = frame.find('\n', pos); // Find the end of the line

        std::string receiptIdStr = (endPos != std::string::npos)
            ? frame.substr(pos, endPos - pos) // Extract ID until the newline
            : frame.substr(pos);              // Extract the rest if no newline

        // Convert the extracted receipt ID to an integer
        return std::stoi(receiptIdStr);
    }

    return -1;
}

//check if got the disconnection one
bool compareReceiptId(const std::string& frame, int id) {
    const std::string receiptPrefix = "receipt-id:";
    size_t pos = frame.find(receiptPrefix);

    if (pos != std::string::npos) {
        pos += receiptPrefix.length(); // Move to the start of the ID
        size_t endPos = frame.find('\n', pos); // Find the end of the line

        std::string receiptIdStr = (endPos != std::string::npos)
            ? frame.substr(pos, endPos - pos) // Extract ID until the newline
            : frame.substr(pos);              // Extract the rest if no newline

        // Convert the extracted receipt ID to an integer
        int extractedId = std::stoi(receiptIdStr);
        return extractedId == id; // Compare the extracted ID with the given ID
    }

    return false; // Return false if receipt-id is not found
}


// Function to process frames received from the server
void processFrame(const std::string& frame, StompProtocol& protocol, ConnectionHandler* handler) {
    std::istringstream response(frame);
    string command;
    response >> command;

    if (command == "CONNECTED") {
        protocol.isLogicConnected.store(true);
        std::cout << "Login successful!\n";
    } 

else if (command == "MESSAGE") {
    //std::cout << "" << frame << std::endl;

    // Parse the MESSAGE frame
    std::string channel, user, eventName, description, city;
    int dateTime = 0;
    std::map<std::string, std::string> generalInfo;
    bool inDescription = false;
    bool inGeneralInfo = false;

    std::istringstream frameStream(frame);
    std::string line;

    while (std::getline(frameStream, line)) {
        if (line.find("destination:") == 0) {
            channel = line.substr(12);
        } else if (line.find("user:") == 0) {
            user = line.substr(5);
        } else if (line.find("city:") == 0) {
            city = line.substr(5);
        } else if (line.find("event name:") == 0) {
            eventName = line.substr(11);
        } else if (line.find("date time:") == 0) {
            dateTime = std::stoi(line.substr(10));
        } else if (line.find("general information:") == 0) {
            // Start parsing the general information block
            inGeneralInfo = true;
            continue;
        } else if (line.find("description:") == 0) {
            // Start parsing the description block
            description = line.substr(12);
            inDescription = true;
            inGeneralInfo = false; // End general info block
        }

        // Handle general information block (multi-line)
        else if (inGeneralInfo) {
            if (line.empty() || line.find(":") == std::string::npos) {
                inGeneralInfo = false; // Stop parsing general information if invalid format
            } else {
                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string key = line.substr(0, colonPos);
                    std::string value = line.substr(colonPos + 1);
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    generalInfo[key] = value;
                }
            }
        }

        // Handle multi-line description
        else if (inDescription) {
            if (line.empty() || line.find(":") != std::string::npos) {
                inDescription = false; // Stop collecting description on encountering a new field
            } else {
                description += line; // Append
            }
        }
    }

    // Create and store the event
    Event event(channel, city, eventName, dateTime, description, generalInfo);
    protocol.getSummaryManager().addEvent(channel, user, event);
}


    else if (command == "RECEIPT") {
        //std::cout << "Receipt received: " << frame << std::endl;
        bool needDisconnect = compareReceiptId(frame, protocol.sentDisconnect.load());
        int id = getReceiptId(frame);
        // Find and delete the pair with the specified ID
        if(protocol.receiptIDToMessageMap.contains(id)){
            cout << protocol.receiptIDToMessageMap.getValue(id) << endl;
            protocol.receiptIDToMessageMap.remove(id);
        }
        //gracefull disconnection
        if(needDisconnect){
            protocol.getSummaryManager().clearClientData(user);
            protocol.receiptIDToMessageMap.clear();
            protocol.isLogicConnected.store(false);
            protocol.sentDisconnect.store(-1);;
            protocol.channelToSubcriptonID.clear();
            handler->close();
            {
                std::lock_guard<std::mutex> lock(myLock);
                handlerConnected = false;
            }
            var.notify_all();
        }


    } 
    else if (command == "ERROR") {
        std::cout << "" << frame << std::endl;
        protocol.getSummaryManager().clearClientData(user);
        protocol.receiptIDToMessageMap.clear();
        protocol.isLogicConnected.store(false);
        protocol.sentDisconnect.store(-1);
        protocol.channelToSubcriptonID.clear();
        handler->close();
        {
            std::lock_guard<std::mutex> lock(myLock);
            handlerConnected = false;
        }
        var.notify_all();
    }
}

// Thread responsible for listening to the server
void listen(ConnectionHandler *&handler, StompProtocol &protocol) {
    while (true) {
        std::unique_lock<std::mutex> varLock(waitLock);
        var.wait(varLock, [] { return handlerConnected; });

        if (!handler) {
            std::cout << "Handler is null. Exiting listener thread.\n";
            return;
        }

        while (handlerConnected) {
            std::string response;
            if (handlerConnected && !handler->getLine(response)) {
                std::cout << "Connection closed by server or error occurred. Disconnecting listener.\n";
                {
                    std::lock_guard<std::mutex> lock(myLock);
                    handlerConnected = false;
                }
                var.notify_all();
                break;
            }

            processFrame(response, protocol, handler);
            //std::cout << response << std::endl;
        }
    }
}


int main(int argc, char *argv[]) {

    // Run listener thread
    ConnectionHandler *handler = nullptr;
    StompProtocol protocol;

    std::thread listenerThread([&]() { listen(handler, protocol); });
    listenerThreadPtr = &listenerThread;

    // Read from keyboard
    while (1) {
        const short bufsize = 1024;
        char buf[bufsize];
        std::cin.getline(buf, bufsize); // From keyboard
        std::string userInput(buf);
        std::istringstream input(userInput);
        string command;
        input >> command;

        // Handle not logged in
        if (command != "login" && (!protocol.isLogicConnected.load() || !handlerConnected)) {
            std::cout << "Please login first. Format - login {host:port} {username} {password}\n";
            continue;
        }

        // Handle login command
        else if (command == "login") {
            std::string hostPort, username, password;
            input >> hostPort >> username >> password;

            // Validate login input
            if (protocol.isLogicConnected.load()) {
                std::cout << "The client is already logged in, log out before trying again\n";
                continue;
            }

            if (hostPort.empty() || username.empty() || password.empty()) {
                std::cout << "Wrong login input. Format - login {host:port} {username} {password}\n";
                continue;
            }

            size_t colonPos = hostPort.find(':');
            if (colonPos == std::string::npos) {
                std::cout << "Wrong login input. Format - login {host:port} {username} {password}\n";
                continue;
            }

            std::string host = hostPort.substr(0, colonPos);
            std::string portStr = hostPort.substr(colonPos + 1);

             if (host.empty() || portStr.empty()) {
                std::cout << "Wrong login input. Format - login {host:port} {username} {password}\n";
                continue;
            }

            short port;
            try {
                // Wrap port parsing in a try-catch block to handle invalid port numbers
                port = std::stoi(portStr); // Convert port string to an integer
            } catch (std::exception& e) {
                std::cout << "Invalid port number in host:port\n";
                continue;
            }

            // Establish connection
            if (handler != nullptr) {
                delete handler;
            }
            handler = new ConnectionHandler(host, port);
            if (!handler->connect()) {
                std::cout << "Cannot connect to " << host << ":" << port << std::endl;
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(myLock);
                handlerConnected = true;
            }
            var.notify_all();

            string connectFrame = protocol.constructConnectFrame(username, password);
            if (!handler->sendLine(connectFrame)) {
                std::cout << "Couldn't send frame\n";
                continue;
            }
            user = username;
        }
        else if (command == "logout") {
            string disconnectFrame = protocol.constructDisconnectFrame();
            if (!handler->sendLine(disconnectFrame)) {
                std::cout << "Couldn't send frame\n";
                continue;
            }
        }
        else if (command == "join") {
            string channel;
            input >> channel;

            if (channel.empty()) {
                std::cout << "Wrong join input. Format - join {channel}\n";
                continue;
            }

            string joinFrame = protocol.constructSubscribeFrame(channel);
            if (!handler->sendLine(joinFrame)) {
                std::cout << "Couldn't send frame\n";
                continue;
            }
        }
        else if (command == "exit") {
            string channel;
            input >> channel;

            if (channel.empty()) {
                std::cout << "Wrong exit input. Format - exit {channel}\n";
                continue;
            }

            string unsubscribeFrame = protocol.constructUnsubscribeFrame(channel);
            if (!handler->sendLine(unsubscribeFrame)) {
                std::cout << "Couldn't send frame\n";
                continue;
            }
        }
        else if (command == "report") {
            string path;
            input >> path;
            std::vector<std::string> reportFrames = protocol.constructReportFrames(path, user);

            for (string& frame : reportFrames) {
                if (!handler->sendLine(frame)) {
                    std::cout << "Couldn't send frame\n";
                    continue;
                }
            }
        }
        
    else if (command == "summary") {
        std::string channelName, clientName, summaryFile;

        // Parse the command arguments
        input >> channelName >> clientName;
        std::getline(input, summaryFile);

        // Remove leading space from the summaryFile (if any)
        if (!summaryFile.empty() && summaryFile[0] == ' ') {
            summaryFile = summaryFile.substr(1);
        }

        if (channelName.empty() || clientName.empty() || summaryFile.empty()) {
            std::cout << "Bad format: summary {channelName} {sender} {summaryFile}" << std::endl;
            continue;
        }

        // Check if the client is subscribed to the requested channel
        if (!protocol.channelToSubcriptonID.contains(channelName)) {
            std::cout << "You are not subscribed to the channel: " << channelName << std::endl;
            continue;
        }

        // Generate the summary
        protocol.getSummaryManager().generateSummary(channelName, clientName, summaryFile);
        }

        else {
            std::cout << "Unknown request\n";
            continue;
        }
    }

    // Join listener thread and cleanup
    listenerThread.join();
    if (handler) {
        handler->close();
        delete handler;
    }

    std::cout << "Client shutdown complete.\n";
    return 0;
}
