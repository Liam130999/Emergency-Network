#include "SummaryManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

SummaryManager::SummaryManager() : channelData(), summaryLock() {}

SummaryManager::~SummaryManager() {}

void SummaryManager::addEvent(const std::string& channel, const std::string& user, const Event& event) {
    std::lock_guard<std::mutex> lock(summaryLock);
    channelData[channel][user].push_back(event);
}

void SummaryManager::generateSummary(const std::string& channel, const std::string& user, const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(summaryLock);

    // Check if channel and user data exists
    if (channelData.find(channel) == channelData.end() || channelData.at(channel).find(user) == channelData.at(channel).end()) {
        std::cout << "No data found for channel: " << channel << ", user: " << user << std::endl;
        return;
    }

    auto events = channelData.at(channel).at(user); // Copy events to sort

    if (events.empty()) {
        std::cout << "No events to summarize for channel: " << channel << ", user: " << user << std::endl;
        return;
    }

    // Sort events by date_time, and then by event name lexicographically
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.get_date_time() != b.get_date_time()) {
            return a.get_date_time() < b.get_date_time(); // Sort by date_time
        }
        return a.get_name() < b.get_name(); // If date_time is the same, sort by event name
    });

    // Open the output file
    std::ofstream outFile(filePath, std::ios::trunc); // Truncate if the file exists
    if (!outFile.is_open()) {
        std::cout << "Could not open or create file: " << filePath << std::endl;
        return;
    }

    // Calculate statistics
    int totalReports = events.size();
    int activeCount = 0;
    int forcesArrivalCount = 0;

    for (const auto& event : events) {
        if (event.get_general_information().at("active") == "true") activeCount++;
        if (event.get_general_information().at("forces_arrival_at_scene") == "true") forcesArrivalCount++;
    }

    // Write summary header
    outFile << "Channel " << channel << "\n";
    outFile << "Stats:\n";
    outFile << "Total: " << totalReports << "\n";
    outFile << "active: " << activeCount << "\n";
    outFile << "forces arrival at scene: " << forcesArrivalCount << "\n\n";
    outFile << "Event Reports:\n\n";

    // Write event details
    for (size_t i = 0; i < events.size(); ++i) {
        const auto& event = events[i];
        outFile << "Report_" << (i + 1) << ":\n";
        outFile << "city: " << event.get_city() << "\n";
        outFile << "date time: " << epochToDate(event.get_date_time()) << "\n";
        outFile << "event name: " << event.get_name() << "\n";

        // Truncate description for summary
        std::string description = event.get_description();
        if (description.size() > 27) {
            description = description.substr(0, 27) + "...";
        }
        outFile << "summary: " << description << "\n\n";
    }

    outFile.close();
    //std::cout << "Summary saved to: " << filePath << std::endl;
}


std::string SummaryManager::epochToDate(int epochTime) const {
    std::time_t time = static_cast<std::time_t>(epochTime);
    std::tm* tm = std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(tm, "%d/%m/%Y %H:%M");
    return oss.str();
}

void SummaryManager::clear() {
    std::lock_guard<std::mutex> lock(summaryLock);
    channelData.clear();
}

void SummaryManager::clearClientData(const std::string& clientName) {
    std::lock_guard<std::mutex> lock(summaryLock);

    // Iterate through all channels and remove the client's data
    for (auto it = channelData.begin(); it != channelData.end(); ++it) {
        it->second.erase(clientName);
    }
}

