#pragma once

#include "event.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>

class SummaryManager {
private:
    std::map<std::string, std::map<std::string, std::vector<Event>>> channelData; // Channel -> User -> Events
    mutable std::mutex summaryLock; // For thread-safe access

    std::string epochToDate(int epochTime) const; // Convert epoch time to DD/MM/YYYY HH:MM

public:
    SummaryManager();
    ~SummaryManager();

    void addEvent(const std::string& channel, const std::string& user, const Event& event); // Add an event
    void generateSummary(const std::string& channel, const std::string& user, const std::string& filePath) const; // Generate summary
    void clear(); // Clear all stored events
    void clearClientData(const std::string& clientName);

};
