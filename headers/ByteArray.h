#include <vector>
#include <cstdint>

class ByteArray {
public:
    ByteArray() = default;

    // Construct from a byte vector
    ByteArray(const std::vector<uint8_t>& data) : data_(data) {}

    // Get the size of the byte array
    size_t size() const {
        return data_.size();
    }

    // Access byte at a given index
    uint8_t operator[](size_t index) const {
        return data_.at(index);
    }

    // Add a byte to the end
    void append(uint8_t byte) {
        data_.push_back(byte);
    }

    // Get raw data pointer
    const uint8_t* data() const {
        return data_.data();
    }

private:
    std::vector<uint8_t> data_; // Internal storage
};
