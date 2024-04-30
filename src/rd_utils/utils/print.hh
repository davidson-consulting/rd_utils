#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <unordered_map>
#include <cstdint>

template <typename T>
std::ostream& operator<< (std::ostream& stream, const std::vector<T> & vec) {
    stream << "[";
    for (uint32_t i = 0 ; i < vec.size () ;i++) {
        if (i != 0) stream << ", ";
        stream << vec[i];
    }
    stream << "]";
    return stream;
}


template <typename T>
std::ostream& operator<< (std::ostream& stream, const std::list<T> & vec) {
    stream << "(";
    uint64_t i = 0;
    for (auto & it : vec) {
        if (i != 0) stream << ";";
        else i += 1;
        stream << it;
    }
    stream << ")";
    return stream;
}


std::ostream& operator<< (std::ostream& stream, const std::vector<char> & vec);

template <typename K, typename V>
std::ostream& operator<<(std::ostream& stream, const std::map <K, V>& map) {
    stream << "{";
    int i = 0;
    for (auto & it : map) {
        if (i != 0) stream << ", ";
        stream << it.first << " : " << it.second;
        i += 1;
    }
    stream << "}";
    return stream;
}

template <typename K>
std::ostream& operator<<(std::ostream& stream, const std::set <K>& map) {
    stream << "{";
    int i = 0;
    for (auto & it : map) {
        if (i != 0) stream << ", ";
        stream << it;
        i += 1;
    }
    stream << "}";
    return stream;
}


template <typename K, typename V>
std::ostream& operator<<(std::ostream& stream, const std::pair <K, V>& p) {
    stream << "{";
    stream << p.first << "," << p.second;
    stream << "}";
    return stream;
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& stream, const std::unordered_map <K, V>& map) {
    stream << "{";
    int i = 0;
    for (auto & it : map) {
        if (i != 0) stream << ", ";
        stream << it.first << " : " << it.second;
        i += 1;
    }
    stream << "}";
    return stream;
}
