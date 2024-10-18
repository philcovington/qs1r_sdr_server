#pragma once

#include <algorithm> // For std::remove
#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept> // For std::invalid_argument
#include <string>

class UUID {
  public:
    // Default constructor generates a random UUID
    UUID() { generateRandomUUID(); }

    // Constructor from a string
    UUID(const std::string &uuidString) { fromString(uuidString); }

    // Generate a random UUID
    void generateRandomUUID() {
        static std::random_device rd;
        static std::mt19937 mt(rd());
        static std::uniform_int_distribution<int> dist(0, 255);

        for (int i = 0; i < 16; ++i) {
            data[i] = static_cast<uint8_t>(dist(mt));
        }

        // Set the version and variant
        data[6] = (data[6] & 0x0f) | 0x40; // Version 4
        data[8] = (data[8] & 0x3f) | 0x80; // Variant 1
    }

    // Convert to string representation
    std::string toString() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (size_t i = 0; i < data.size(); ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                oss << '-';
            }
            oss << std::setw(2) << static_cast<int>(data[i]);
        }

        return oss.str();
    }

    // Convert from string representation
    bool fromString(const std::string &uuidString) {
        // Remove hyphens from the UUID string
        std::string cleaned = uuidString;
        cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '-'), cleaned.end());

        // Check if the cleaned string is of valid UUID length
        if (cleaned.size() != 32) {
            return false; // Invalid UUID length
        }

        // Convert each pair of hex characters to a byte
        for (size_t i = 0; i < 16; ++i) {
            std::string byteString = cleaned.substr(i * 2, 2);
            try {
                data[i] = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
            } catch (const std::invalid_argument &) {
                return false; // Invalid hex string
            } catch (const std::out_of_range &) {
                return false; // Out of range for uint8_t
            }
        }

        return true;
    }

    // Equality operator
    bool operator==(const UUID &other) const { return data == other.data; }

    // Inequality operator
    bool operator!=(const UUID &other) const { return !(*this == other); }

  private:
    std::array<uint8_t, 16> data;
};
