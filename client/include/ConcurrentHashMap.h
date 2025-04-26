#ifndef CONCURRENT_HASH_MAP_H
#define CONCURRENT_HASH_MAP_H

#include <map>
#include <string>
#include <mutex>
#include <stdexcept>

class ConcurrentHashMap {
private:
    std::map<std::string, int> map; // The underlying map
    mutable std::mutex mapMutex;    // Mutex for thread safety

public:
    // Constructor
    ConcurrentHashMap();

    // Destructor
    ~ConcurrentHashMap();

    // Inserts or updates a key-value pair
    void insertOrUpdate(const std::string& key, int value);

    // Retrieves the value associated with a key
    // Returns true if the key exists, false otherwise
    bool get(const std::string& key, int& value) const;

    // Retrieves the value for a given key directly (throws if not found)
    int getValue(const std::string& key) const;

    // Removes a key
    bool remove(const std::string& key);

    // Checks if a key exists
    bool contains(const std::string& key) const;

    // Returns the size of the map
    size_t size() const;

    // Clears all elements from the map
    void clear();
};

#endif // CONCURRENT_HASH_MAP_H
