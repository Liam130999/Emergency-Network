#include "../include/StompProtocol.h"
#include <iostream>
#include <sstream>
#include <algorithm>
using namespace std;

StompProtocol::StompProtocol()
    : receiptCounter(0),
      subscriptionCounter(0),
      isLogicConnected(false),
      sentDisconnect(-1),
      channelToSubcriptonID(),
      summaryManager(),
      receiptIDToMessageMap()
{}

SummaryManager& StompProtocol::getSummaryManager() {
    return summaryManager;
}


int StompProtocol::getNextReceiptId() {
    return receiptCounter.fetch_add(1);
}

int StompProtocol::getNextSubscriptionId() {
    return subscriptionCounter.fetch_add(1);
}

std::string StompProtocol::constructConnectFrame(const std::string& username, const std::string& password) {
    return "CONNECT\naccept-version:1.2\nhost:stomp.cs.bgu.ac.il\nlogin:" + username +
           "\npasscode:" + password + "\n\n";
}

std::string StompProtocol::constructSubscribeFrame(const std::string& channel) {
    int receiptId = getNextReceiptId();
    int subscriptionId = getNextSubscriptionId();

    //update map
    channelToSubcriptonID.insertOrUpdate(channel, subscriptionId);

    receiptIDToMessageMap.insertOrUpdate(receiptId, "Joined channel "+channel);

    return "SUBSCRIBE\ndestination:" + channel + "\nid:" + std::to_string(subscriptionId) +
           "\nreceipt:" + std::to_string(receiptId) + "\n\n";
}

std::string StompProtocol::constructUnsubscribeFrame(const std::string& channel) {
    int receiptId = getNextReceiptId();
    int subID;
    if(!channelToSubcriptonID.contains(channel)){
        subID = -1;
    }
    else{
        //get relevant id
        subID = channelToSubcriptonID.getValue(channel);
        //delete it
        channelToSubcriptonID.remove(channel);
    }
    
    receiptIDToMessageMap.insertOrUpdate(receiptId, "Exited channel "+channel);

    return "UNSUBSCRIBE\nid:"+std::to_string(subID)+"\nreceipt:" + std::to_string(receiptId) + "\n\n";
}

std::string StompProtocol::constructDisconnectFrame() {
    int receiptId = getNextReceiptId();
    sentDisconnect.store(receiptId);
    return "DISCONNECT\nreceipt:" + std::to_string(receiptId) + "\n\n";
}

std::vector<std::string> StompProtocol::constructReportFrames(const std::string& filePath, const std::string& userNameOK) {
    // Parse the events from the file
    names_and_events parsedEvents = parseEventsFile(filePath);

    // Sort events by date_time using a defined comparator
    struct {
        bool operator()(const Event& a, const Event& b) const {
            return a.get_date_time() < b.get_date_time();
        }
    } compareByDateTime; // Inline-defined comparator

    std::sort(parsedEvents.events.begin(), parsedEvents.events.end(), compareByDateTime);

    // Build the frames
    std::vector<std::string> frames;
    for (Event& event : parsedEvents.events) {
        int receiptId = getNextReceiptId(); // Ensure receipt IDs are unique
        summaryManager.addEvent(parsedEvents.channel_name, userNameOK, event); // Add event to SummaryManager
        
        // Frame construction
        std::ostringstream frame;
        frame << "SEND\n"
              << "destination:" << parsedEvents.channel_name << "\n" 
              << "user:" << userNameOK << "\n"
              << "city:" << event.get_city() << "\n"
              << "event name:" << event.get_name() << "\n"
              << "date time:" << event.get_date_time() << "\n"
              << "general information:\n";

        for (const auto& pair : event.get_general_information()) {
            frame << " " << pair.first << ": " << pair.second << "\n";
        }

        // Add description, truncating if necessary
        std::string description = event.get_description();
        frame << "description:\n" << description << "\n";

        // Add receipt
        frame << "receipt:" << receiptId << "\n";

        // Add the frame to the vector
        frames.push_back(frame.str());
    }

    return frames;
}

