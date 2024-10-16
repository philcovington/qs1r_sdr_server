/**
 * @file Map.h
 * @brief A generic map utility class that wraps around std::map.
 *
 * The Map class provides a convenient interface for managing key-value pairs,
 * allowing for easy insertion, removal, and retrieval of elements. It supports
 * various operations such as checking for the existence of a key, getting
 * values with default fallbacks, and accessing the first and last keys and
 * values in sorted order.
 *
 * Features:
 * - Constructors for default initialization, std::map initialization, and
 *   initializer list support.
 * - Methods for inserting and removing key-value pairs.
 * - Functionality to check for the existence of keys.
 * - Overloaded operator[] for accessing values by key.
 * - Method to get the value for a key with a default fallback.
 * - Accessor methods for retrieving the first and last keys and values.
 * - Size and emptiness checks for the map.
 * - Iterator support for range-based loops, making it easy to iterate through
 *   key-value pairs.
 *
 * Usage:
 * To create and use a Map instance, you can do the following:
 *
 *   Map<std::string, int> myMap;
 *   myMap.insert("key1", 100);
 *   myMap.insert("key2", 200);
 *   int value = myMap["key1"]; // Retrieves 100
 *
 * The class automatically handles the underlying std::map operations,
 * providing a user-friendly interface for map management.
 * 
 * I have tried to provide the similar functionality to Qt's QMap.
 *
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include <iostream>
#include <map>

template <typename Key, typename Value> class Map {
  public:
    // Constructors
    Map() = default;
    Map(const std::map<Key, Value> &elements) : map(elements) {}
    Map(const std::initializer_list<std::pair<Key, Value>> &elements) : map(elements) {}

    // Insert a key-value pair
    void insert(const Key &key, const Value &value) { map[key] = value; }

    // Remove a key and its associated value
    void remove(const Key &key) { map.erase(key); }

    // Check if a key exists in the map
    bool contains(const Key &key) const { return map.find(key) != map.end(); }

    // Get value for a key
    Value &operator[](const Key &key) { return map[key]; }

    // Get value for a key (const version)
    const Value &operator[](const Key &key) const { return map.at(key); }

    // Get the value for a key (similar to QMap's value())
    Value value(const Key &key, const Value &defaultValue = Value()) const {
        auto it = map.find(key);
        return (it != map.end()) ? it->second : defaultValue;
    }

    // Get the first key (smallest key in sorted order)
    Key firstKey() const { return map.begin()->first; }

    // Get the last key (largest key in sorted order)
    Key lastKey() const { return std::prev(map.end())->first; }

    // Get the first value (smallest key's value)
    Value first() const { return map.begin()->second; }

    // Get the last value (largest key's value)
    Value last() const { return std::prev(map.end())->second; }

    // Get the number of key-value pairs
    int size() const { return static_cast<int>(map.size()); }

    // Clear the map
    void clear() { map.clear(); }

    // Check if the map is empty
    bool isEmpty() const { return map.empty(); }

    // Iterator support for ranged-based loops
    auto begin() { return map.begin(); }
    auto end() { return map.end(); }
    auto begin() const { return map.begin(); }
    auto end() const { return map.end(); }

  private:
    std::map<Key, Value> map;
};