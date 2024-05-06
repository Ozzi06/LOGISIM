#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include "nlohmann/json.hpp"
#include <limits>

#define INFINITY (std::numeric_limits<double>::infinity())

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
    size_t size;
    size_t current_size = 0;
    size_t index;
public:
    RollingAverage(int windowSize) : size(windowSize), index(0) {
        window.resize(size, 0);
    }

    void add(double num) {
        window[index] = num;

        // Move to the next index
        index = (index + 1) % size;

        if (current_size < size) current_size++;
    }

    void reset() {
        current_size = 0;
        index = 0;
    }

    double getAverage() const {
        double sum = 0;
        for (size_t i = 0; i < current_size; i++) {
            sum += window[i];
        }
        return sum / double(current_size);
    }

    double getMax() const {
        double max = -INFINITY;
        for (size_t i = 0; i < current_size; i++) {
            if (max < window[i]) max = window[i];
        }
        return max;
    }
    double getSoftmax(size_t average_window) const {
        double max = -INFINITY;
        for (size_t i = 0; i < current_size; i++) {
            double sum = 0;
            for (size_t j = 0; j < average_window && i < current_size; i++, j++) {
                sum += window[i];
            }
            if (max < sum) max = sum;
        }
        return max;
    }

    double getMin() const {
        double min = INFINITY;
        for (size_t i = 0; i < current_size; i++) {
            if (min > window[i]) min = window[i];
        }
        return min;
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