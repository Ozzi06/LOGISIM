#include "vector_tools.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

std::string num_toString(double number, int precision)
{
    std::ostringstream oss;
    oss << std::scientific << std::setprecision(precision) << number;
    return oss.str();
}

//bool operator==(const Vector2& v1, const Vector2& v2)
//{
//    return (v1.x == v2.x) && (v1.y == v2.y);
//}
//
//Vector2 operator+(const Vector2& v1, const Vector2& v2)
//{
//    return { v1.x + v2.x, v1.y + v2.y };
//}
//
//Vector2 operator-(const Vector2& v1, const Vector2& v2)
//{
//    return { v1.x - v2.x, v1.y - v2.y };
//}

Vector2 operator*(const Vector2& v, float scalar)
{
    return { v.x * scalar, v.y * scalar };
}

Vector2 operator/(const Vector2& v, float divisor)
{
    // Check for division by zero to avoid undefined behavior
    if (divisor != 0.0f) {
        return { v.x / divisor, v.y / divisor };
    }
    else {
        // Handle division by zero gracefully (you may want to customize this)
        return { 0.0f, 0.0f };
    }
}

float abs(const Vector2& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Rectangle RectFrom2Points(Vector2 a, Vector2 b)
{
    float x = std::fmin(a.x, b.x);
    float y = std::fmin(a.y, b.y);
    float size_x = std::abs(a.x - b.x);
    float size_y = std::abs(a.y - b.y);
    return Rectangle{ x, y, size_x, size_y };
}
