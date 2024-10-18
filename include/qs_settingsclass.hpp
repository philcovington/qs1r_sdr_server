#pragma once

#include <../include/json.hpp> // Include the nlohmann/json library for JSON handling
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

class Settings {
  public:
    // Constructor that takes a filename for saving and loading settings
    Settings(const std::string &filename) : m_filename(filename) {
        load(); // Load settings from file upon instantiation
    }

    // Set a value for a given key
    template <typename T> void setValue(const std::string &key, const T &value) {
        m_settings[key] = value;
        save(); // Save settings after modification
    }

    // Get a value for a given key with a default fallback
    template <typename T> T value(const std::string &key, const T &defaultValue = T()) const {
        auto it = m_settings.find(key);
        if (it != m_settings.end()) {
            return it->second.get<T>(); // Return the value if found
        }
        return defaultValue; // Return default value if key not found
    }

    // Remove a key from the settings
    void remove(const std::string &key) {
        m_settings.erase(key);
        save(); // Save settings after removal
    }

    // Clear all settings
    void clear() {
        m_settings.clear();
        save(); // Save settings after clearing
    }

  private:
    std::string m_filename; // File to store settings
    json m_settings;        // Internal storage for settings

    // Load settings from the JSON file
    void load() {
        std::ifstream file(m_filename);
        if (file) {
            file >> m_settings; // Load settings from file
        }
    }

    // Save settings to the JSON file
    void save() const {
        std::ofstream file(m_filename);
        if (file) {
            file << m_settings.dump(4); // Save settings with indentation
        }
    }
};
