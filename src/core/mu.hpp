#pragma once
#include <cstdint>

namespace mu {

using f32 = float;

/// @brief Common math constants
constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 EPS = 1e-6f;  /// Epsilon for float comparisons

/// @brief Return true if |a - b| < eps
constexpr bool nearlyEqual(f32 a, f32 b, f32 eps = EPS) noexcept {
    return (a > b ? a - b : b - a) < eps;
}

///
/// Vec2f
///

/// @brief 2D vector (float)
struct Vec2f {
    f32 x{ 0 };
    f32 y{ 0 };

    constexpr Vec2f() = default;
    Vec2f(f32 x, f32 y) : x(x), y(y) {
    }

    /// @brief Vector addition
    Vec2f operator+(const Vec2f& o) const noexcept;
    /// @brief Vector subtraction
    Vec2f operator-(const Vec2f& o) const noexcept;
    /// @brief Unary minus
    Vec2f operator-() const noexcept;

    /// @brief Scalar multiplication
    Vec2f operator*(f32 s) const noexcept;
    /// @brief Scalar division (undefined if s == 0)
    Vec2f operator/(f32 s) const;

    /// @brief In-place vector addition
    Vec2f& operator+=(const Vec2f& o) noexcept;
    /// @brief In-place vector subtraction
    Vec2f& operator-=(const Vec2f& o) noexcept;
    /// @brief In-place scalar multiplication
    Vec2f& operator*=(f32 s) noexcept;
    /// @brief In-place scalar division (undefined if s == 0)
    Vec2f& operator/=(f32 s);

    /// @brief Epsilon-based equality
    bool operator==(const Vec2f& o) const noexcept;
    /// @brief Epsilon-based inequality
    bool operator!=(const Vec2f& o) const noexcept;
};

/// @brief Left-scalar multiply
Vec2f operator*(f32 s, const Vec2f& v) noexcept;

/// @brief Dot product of two vectors
f32 dot(const Vec2f& a, const Vec2f& b) noexcept;

/// @brief Length (magnitude) of a vector
f32 length(const Vec2f& v) noexcept;

/// @brief Normalized vector (length = 1), or {0,0} if too small
Vec2f normalized(const Vec2f& v) noexcept;

/// @brief Rotate a vector by angle in degrees (around origin)
Vec2f rotate(const Vec2f& v, f32 deg) noexcept;

///
/// Mat3f
///

/// @brief 3x3 matrix (float) for 2D affine transforms (row-major)
struct Mat3f {
    f32 m[9];

    /// @brief Identity matrix
    static Mat3f identity() noexcept;
    /// @brief Translation matrix
    static Mat3f translation(f32 tx, f32 ty) noexcept;
    /// @brief Rotation matrix (angle in degrees)
    static Mat3f rotation(f32 deg);
    /// @brief Scale matrix
    static Mat3f scale(f32 sx, f32 sy) noexcept;

    /// @brief Matrix multiplication
    Mat3f operator*(const Mat3f& o) const noexcept;

    /// @brief Transform a 2D point
    Vec2f transformPoint(const Vec2f& v) const noexcept;
};

///
/// Global math functions
///

/// @brief Convert degrees to radians
constexpr f32 deg2rad(f32 d) noexcept {
    return d * (PI / 180.f);
}

/// @brief Convert radians to degrees
constexpr f32 rad2deg(f32 r) noexcept {
    return r * (180.f / PI);
}

/// @brief Clamp a value between lo and hi
constexpr f32 clamp(f32 v, f32 lo, f32 hi) noexcept {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

}  // namespace mu
