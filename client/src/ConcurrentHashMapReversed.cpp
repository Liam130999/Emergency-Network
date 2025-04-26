#include "ConcurrentHashMapReversed.h"

// Constructor
ConcurrentHashMapReversed::ConcurrentHashMapReversed() : map(), mapMutex() {}

// Destructor
ConcurrentHashMapReversed::~ConcurrentHashMapReversed() {}

// Inserts or updates a key-value pair
void ConcurrentHashMapReversed::insertOrUpdate(int key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mapMutex);
    map[key] = value;
}

// Retrieves the value associated with a key
bool ConcurrentHashMapReversed::get(int key, std::string& value) const {
    std::lock_guard<std::mutex> lock(mapMutex);
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return true;
    }
    return false;
}

// Retrieves the value for a given key directly (throws if not found)
std::string ConcurrentHashMapReversed::getValue(int key) const {
    std::lock_guard<std::mutex> lock(mapMutex);
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second; // Return the value if the key exists
    }
    throw std::runtime_error("Key not found: " + std::to_string(key)); // Throw if the key does not exist
}

// Removes a key
bool ConcurrentHashMapReversed::remove(int key) {
    std::lock_guard<std::mutex> lock(mapMutex);
    return map.erase(key) > 0;
}

// Checks if the map contains a given key
bool ConcurrentHashMapReversed::contains(int key) const {
    std::lock_guard<std::mutex> lock(mapMutex);
    return map.find(key) != map.end();
}

// Returns the number of elements in the map
size_t ConcurrentHashMapReversed::size() const {
    std::lock_guard<std::mutex> lock(mapMutex);
    return map.size();
}

// Clears all elements from the map
void ConcurrentHashMapReversed::clear() {
    std::lock_guard<std::mutex> lock(mapMutex);
    map.clear();
}
