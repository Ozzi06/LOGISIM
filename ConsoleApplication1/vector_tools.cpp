#include "vector_tools.h"
std::string num_toString(double number, int precision)
{
    std::ostringstream oss;
    oss << std::scientific << std::setprecision(precision) << number;
    return oss.str();
}

bool operator==(const Vector2& v1, const Vector2& v2)
{
    return (v1.x == v2.x) && (v1.y == v2.y);
}

Vector2 operator+(const Vector2& v1, const Vector2& v2)
{
    return { v1.x + v2.x, v1.y + v2.y };
}

Vector2 operator-(const Vector2& v1, const Vector2& v2)
{
    return { v1.x - v2.x, v1.y - v2.y };
}

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
    float size_x = abs(a.x - b.x);
    float size_y = abs(a.y - b.y);
    return Rectangle{ x, y, size_x, size_y };
}

// Define how to serialize Vector2
void to_json(nlohmann::json& j, const Vector2& v) {
    j = nlohmann::json{ {"x", v.x}, {"y", v.y} };
}

// Define how to deserialize Vector2
void from_json(const nlohmann::json& j, Vector2& v) {
    v.x = j.at("x").get<float>();
    v.y = j.at("y").get<float>();
}

// Define how to serialize Camera2D
void to_json(nlohmann::json& j, const Camera2D& camera) {
    j = nlohmann::json{
        {"offset", camera.offset},
        {"target", camera.target},
        {"rotation", camera.rotation},
        {"zoom", camera.zoom}
    };
}

// Define how to deserialize Camera2D
void from_json(const nlohmann::json& j, Camera2D& camera) {
    camera.offset = j.at("offset").get<Vector2>();
    camera.target = j.at("target").get<Vector2>();
    camera.rotation = j.at("rotation").get<float>();
    camera.zoom = j.at("zoom").get<float>();
}