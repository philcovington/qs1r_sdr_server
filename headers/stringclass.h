#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class String {
  public:
    // Constructors
    String() = default;
    String(const std::string &str) : data(str) {}
    String(const char *str) : data(str) {}

    // Static method to create a String object from std::string
    static String fromStdString(const std::string &str) { return String(str); }

    // Append another string
    void append(const std::string &str) { data.append(str); }

    // Prepend another string
    void prepend(const std::string &str) { data.insert(0, str); }

    // Convert the string to upper case
    String toUpper() const {
        String result = *this;
        std::transform(result.data.begin(), result.data.end(), result.data.begin(), ::toupper);
        return result;
    }

    // Convert the string to lower case
    String toLower() const {
        String result = *this;
        std::transform(result.data.begin(), result.data.end(), result.data.begin(), ::tolower);
        return result;
    }

    // Get a substring from index `start` with a specific `length`
    String mid(int start, int length = -1) const {
        if (start >= 0 && start < size()) {
            return length == -1 ? data.substr(start) : data.substr(start, length);
        }
        return "";
    }

    // Trim whitespace from the string
    String trimmed() const {
        size_t first = data.find_first_not_of(' ');
        size_t last = data.find_last_not_of(' ');
        if (first == std::string::npos || last == std::string::npos)
            return "";
        return data.substr(first, last - first + 1);
    }

    // Replace occurrences of `before` with `after`
    String replace(const std::string &before, const std::string &after) const {
        String result = *this;
        size_t pos = 0;
        while ((pos = result.data.find(before, pos)) != std::string::npos) {
            result.data.replace(pos, before.size(), after);
            pos += after.size();
        }
        return result;
    }

    // Split the string by a delimiter
    std::vector<String> split(char delimiter) const {
        std::vector<String> tokens;
        std::istringstream stream(data);
        std::string token;
        while (std::getline(stream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    // Check if the string contains a substring
    bool contains(const std::string &str) const { return data.find(str) != std::string::npos; }

    // Get the size of the string
    int size() const { return static_cast<int>(data.size()); }

    // Overload the `+` operator for concatenation
    String operator+(const String &other) const { return String(this->data + other.data); }

    // Overload the `==` operator for comparison
    bool operator==(const String &other) const { return this->data == other.data; }

    // Overload the `!=` operator for comparison
    bool operator!=(const String &other) const { return this->data != other.data; }

    // Overload the `[]` operator for access
    char &operator[](int index) { return data[index]; }

    const char &operator[](int index) const { return data[index]; }

    // Overload stream insertion operator for printing
    friend std::ostream &operator<<(std::ostream &os, const String &str) {
        os << str.data;
        return os;
    }

    // Static method to convert int to String
    static String number(int num) { return String(std::to_string(num)); }

    // Static method to convert double to String (with optional precision)
    static String number(double num, int precision = 6) {
        std::ostringstream out;
        out.precision(precision);
        out << std::fixed << num;
        return String(out.str());
    }

    // Static method to convert float to String (with optional precision)
    static String number(float num, int precision = 6) {
        std::ostringstream out;
        out.precision(precision);
        out << std::fixed << num;
        return String(out.str());
    }

  private:
    std::string data;
};

// // Example usage
// int main() {
//     std::string infoName = "Hello from std::string";
//     String str = String::fromStdString(infoName);

//     std::cout << "Converted from std::string: " << str << std::endl;

//     return 0;
// }
