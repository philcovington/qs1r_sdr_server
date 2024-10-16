#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

template <typename T> class List {
  public:
    // Constructors
    List() = default;
    List(const std::vector<T> &elements) : list(elements) {}
    List(const std::initializer_list<T> &elements) : list(elements) {}

    // Add an element to the end of the list
    void append(const T &value) { list.push_back(value); }

    // Add another list of elements to this list
    void append(const List<T> &other) { list.insert(list.end(), other.list.begin(), other.list.end()); }

    // Insert an element at a specific index
    void insert(int index, const T &value) {
        if (index >= 0 && index <= size()) {
            list.insert(list.begin() + index, value);
        }
    }

    // Remove an element at a specific index
    void removeAt(int index) {
        if (index >= 0 && index < size()) {
            list.erase(list.begin() + index);
        }
    }

    // Remove all occurrences of an element
    void removeAll(const T &value) { list.erase(std::remove(list.begin(), list.end(), value), list.end()); }

    // Check if the list contains an element
    bool contains(const T &value) const { return std::find(list.begin(), list.end(), value) != list.end(); }

    // Get the size of the list
    int size() const { return static_cast<int>(list.size()); }

    // Access element at a specific index
    T &at(int index) { return list.at(index); }

    const T &at(int index) const { return list.at(index); }

    // Get the first element
    T &first() { return list.front(); }

    const T &first() const { return list.front(); }

    // Get the last element
    T &last() { return list.back(); }

    const T &last() const { return list.back(); }

    // Clear the list
    void clear() { list.clear(); }

    // Overload operator[] for access
    T &operator[](int index) { return list.at(index); }

    const T &operator[](int index) const { return list.at(index); }

    // Static method to create a List from a std::vector of any type
    template <typename U> static List<U> fromVector(const std::vector<U> &vec) {
        List<U> list;
        for (const auto &item : vec) {
            list.append(item);
        }
        return list;
    }

  private:
    std::vector<T> list;
};
