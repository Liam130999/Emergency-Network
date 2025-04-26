#pragma once

#include <string>
#include <sstream>
#include <atomic>
#include "../include/event.h"
#include <vector>
#include "ConcurrentHashMap.h"
#include "ConcurrentHashMapReversed.h"
#include "SummaryManager.h"
#include <map>




class StompProtocol {
private:
    std::atomic<int> receiptCounter;      // Thread-safe counter for unique receipt IDs
    std::atomic<int> subscriptionCounter; // Thread-safe counter for unique subscription IDs

public:
    StompProtocol();
    std::atomic<bool> isLogicConnected;
    std::atomic<int> sentDisconnect;
    ConcurrentHashMap channelToSubcriptonID; 
    ConcurrentHashMapReversed receiptIDToMessageMap;


    // Frame constructors
    std::string constructConnectFrame(const std::string& username, const std::string& password);
    std::string constructSubscribeFrame(const std::string& channel);
    std::string constructUnsubscribeFrame(const std::string& channel);
    std::string constructDisconnectFrame();
    std::vector<std::string> constructReportFrames(const std::string& filePath, const std::string& userNameOK);
    SummaryManager& getSummaryManager(); // Access summary manager

    // Process server responses
    void processFrame(const std::string& frame);

private:
    int getNextReceiptId();       // Helper function to generate unique receipt IDs
    int getNextSubscriptionId();  // Helper function to generate unique subscription IDs
    SummaryManager summaryManager; // Summary manager instance

};
