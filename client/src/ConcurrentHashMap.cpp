#include "ConcurrentHashMap.h"

// Constructor
ConcurrentHashMap::ConcurrentHashMap() : map(), mapMutex() {}

// Destructor
ConcurrentHashMap::~ConcurrentHashMap() {}

// Inserts or updates a key-value pair
void ConcurrentHashMap::insertOrUpdate(const std::string& key, int value) {
    std::lock_guard<std::mutex> lock(mapMutex);
    map[key] = value;
}

// Retrieves the value associated with a key
bool ConcurrentHashMap::get(const std::string& key, int& value) const {
    std::lock_guard<std::mutex> lock(mapMutex);
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return true;
    }
    return false;
}

// Retrieves the value for a given key directly (throws if not found)
int ConcurrentHashMap::getValue(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mapMutex);
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second; // Return the value if the key exists
    }
    throw std::runtime_error("Key not found: " + key); // Throw if the key does not exist
}

// Removes a key
bool ConcurrentHashMap::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mapMutex);
    return map.erase(key) > 0;
}

// Checks if the map contains a given key
bool ConcurrentHashMap::contains(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mapMutex);
    return map.find(key) != map.end();
}

// Returns the number of elements in the map
size_t ConcurrentHashMap::size() const {
    std::lock_guard<std::mutex> lock(mapMutex);
    return map.size();
}

// Clears all elements from the map
void ConcurrentHashMap::clear() {
    std::lock_guard<std::mutex> lock(mapMutex);
    map.clear();
}
