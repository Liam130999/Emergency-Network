#pragma once
#include <map>
#include <string>
#include <mutex>
#include <stdexcept>

class ConcurrentHashMapReversed {
private:
    std::map<int, std::string> map; // Map from int to string
    mutable std::mutex mapMutex;    // Mutex for thread safety

public:
    // Constructor
    ConcurrentHashMapReversed();

    // Destructor
    ~ConcurrentHashMapReversed();

    // Inserts or updates a key-value pair
    void insertOrUpdate(int key, const std::string& value);

    // Retrieves the value associated with a key
    bool get(int key, std::string& value) const;

    // Retrieves the value for a given key directly (throws if not found)
    std::string getValue(int key) const;

    // Removes a key
    bool remove(int key);

    // Checks if the map contains a given key
    bool contains(int key) const;

    // Returns the number of elements in the map
    size_t size() const;

    // Clears all elements from the map
    void clear();
};
