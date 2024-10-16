#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class DebugLogger {
  public:
    // Log function for basic types and strings
    template <typename T> DebugLogger &operator<<(const T &value) {
        stream << value;
        return *this;
    }

    // Function to log a vector of values
    template <typename T> DebugLogger &operator<<(const std::vector<T> &vec) {
        stream << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            stream << vec[i];
            if (i < vec.size() - 1) {
                stream << ", ";
            }
        }
        stream << "]";
        return *this;
    }

    // Flush the log with timestamp and output
    ~DebugLogger() {
        std::time_t now = std::time(nullptr);
        std::tm *local_time = std::localtime(&now);
        std::cout << std::put_time(local_time, "[%Y-%m-%d %H:%M:%S] ") << stream.str() << std::endl;
    }

  private:
    std::ostringstream stream;
};

// Helper function to create a logger instance
DebugLogger _debug() { return DebugLogger(); }
