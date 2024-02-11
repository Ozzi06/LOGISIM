#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include "nlohmann/json.hpp"

std::string num_toString(double number, int precision);


bool operator==(const Vector2& v1, const Vector2& v2);

Vector2 operator+(const Vector2& v1, const Vector2& v2);

Vector2 operator-(const Vector2& v1, const Vector2& v2);

Vector2 operator*(const Vector2& v, float scalar);

Vector2 operator/(const Vector2& v, float divisor);

float abs(const Vector2& v);

Rectangle RectFrom2Points(Vector2 a, Vector2 b);

class RollingAverage {
private:
    std::vector<double> window;
    int size;
    int index;
    double sum;

public:
    RollingAverage(int windowSize) : size(windowSize), index(0), sum(0) {
        window.resize(size, 0);
    }

    void add(double num) {
        // Subtract the oldest number from the sum
        sum -= window[index];

        // Add the new number
        window[index] = num;
        sum += num;

        // Move to the next index
        index = (index + 1) % size;
    }

    double getAverage() const {
        // Handle case where less than 'size' elements have been added
        int currentSize = std::min(index, size);
        return currentSize == 0 ? 0 : sum / currentSize;
    }
};


template<typename T>
void moveToTop(std::vector<T>& vec, size_t pos) {
    if (pos < vec.size()) {
        T element = vec[pos]; // Copy the element
        vec.erase(vec.begin() + pos); // Remove element from current position
        vec.insert(vec.begin(), element); // Insert element at the beginning
    }
}

template<typename T>
void moveToBottom(std::vector<T>& vec, size_t pos) {
    if (pos < vec.size()) {
        T element = vec[pos]; // Copy the element
        vec.erase(vec.begin() + pos); // Remove element from current position
        vec.push_back(element); // Append element at the end
    }
}

template<typename T>
void moveToBottom(std::vector<T>& vec, typename std::vector<T>::iterator it) {
    T element = *it; // Copy the element
    vec.erase(it); // Remove element from current position
    vec.push_back(element); // Append element at the end
}

// Define how to serialize Vector2
void to_json(nlohmann::json& j, const Vector2& v);

// Define how to deserialize Vector2
void from_json(const nlohmann::json& j, Vector2& v);

// Define how to serialize Camera2D
void to_json(nlohmann::json& j, const Camera2D& camera);

// Define how to deserialize Camera2D
void from_json(const nlohmann::json& j, Camera2D& camera);