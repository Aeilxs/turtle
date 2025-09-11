# FILES

## src

- `main.cpp`
```cpp
#include "core/types.hpp"
#include <SFML/Graphics.hpp>

i32 main() {
    auto window = sf::RenderWindow(sf::VideoMode({ 1920u, 1080u }), "CMake SFML Project");
    window.setFramerateLimit(144);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear();
        window.display();
    }
}

```

- `core/geom.hpp`
```cpp

```

- `core/mu.hpp`
```cpp
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

```

- `core/styles.hpp`
```cpp
#pragma once
#include "types.hpp"
#include <vector>

namespace gfx {

/// @brief Line ending style used when rendering strokes.
/// @note Only stored here; actual rasterization is renderer-specific.
enum class LineCap {
    Butt,   /// Flat end, no extension beyond endpoint.
    Round,  /// Semicircular end.
    Square  /// Flat end extended by half stroke width.
};

/// @brief Join style at polyline corners.
/// @note Only stored here; actual rasterization is renderer-specific.
enum class LineJoin {
    Miter,  /// Sharp corner.
    Round,  /// Rounded corner (arc with radius = half stroke width).
    Bevel   /// Flat cut corner.
};

/// @brief Stroke style (outline) describing how lines/paths should be drawn.
/// @details This is a *logical* style: it holds parameters only
struct Pen {
    f32 width{ 1.0f };            /// Stroke width in logical units (must be > 0 to be visible).
    Color color{ 0, 0, 0, 255 };  /// RGBA color (default opaque black).
    bool visible{ true };         /// Quick toggle for drawing.

    LineCap cap{ LineCap::Butt };      /// Line cap style (renderer may approximate).
    LineJoin join{ LineJoin::Miter };  /// Join style for corners.
    f32 miterLimit{ 4.0f };            /// Max miter length / half-width (used if join = Miter).

    /// @brief Dash pattern as alternating on/off lengths (in stroke units).
    ///        Example: {10, 5} -> 10 on, 5 off, repeat.
    /// @note Empty means solid line. Interpretation is renderer-specific.
    std::vector<f32> dash;

    /// @brief Convenience constructor for a solid pen.
    constexpr Pen(Color c, f32 w) : width(w), color(c) {
    }

    /// @brief Default constructor.
    Pen() = default;

    /// @brief Equality (strict byte-wise; no float epsilon).
    constexpr bool operator==(const Pen& o) const {
        return width == o.width && color == o.color && visible == o.visible && cap == o.cap &&
               join == o.join && miterLimit == o.miterLimit && dash == o.dash;
    }
    constexpr bool operator!=(const Pen& o) const {
        return !(*this == o);
    }
};

}  // namespace gfx

```

- `core/geom.cpp`
```cpp

```

- `core/types.hpp`
```cpp
#pragma once
#include <cstdint>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

struct Color {
    u8 r{ 0 }, g{ 0 }, b{ 0 }, a{ 255 };
    constexpr Color() = default;
    constexpr Color(u8 r, u8 g, u8 b, u8 a = 255) : r(r), g(g), b(b), a(a) {
    }

    constexpr bool operator==(const Color& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    constexpr bool operator!=(const Color& o) const {
        return !(*this == o);
    }
};

```

- `core/mu.cpp`
```cpp
#include "mu.hpp"
#include <cassert>
#include <cmath>

namespace mu {

// ===== Vec2f =====

Vec2f Vec2f::operator+(const Vec2f& o) const noexcept {
    return { x + o.x, y + o.y };
}

Vec2f Vec2f::operator-(const Vec2f& o) const noexcept {
    return { x - o.x, y - o.y };
}

Vec2f Vec2f::operator-() const noexcept {
    return { -x, -y };
}

Vec2f Vec2f::operator*(f32 s) const noexcept {
    return { x * s, y * s };
}

Vec2f Vec2f::operator/(f32 s) const {
    assert(std::fabs(s) > EPS && "Vec2f division by zero");
    return { x / s, y / s };
}

Vec2f& Vec2f::operator+=(const Vec2f& o) noexcept {
    x += o.x;
    y += o.y;
    return *this;
}

Vec2f& Vec2f::operator-=(const Vec2f& o) noexcept {
    x -= o.x;
    y -= o.y;
    return *this;
}

Vec2f& Vec2f::operator*=(f32 s) noexcept {
    x *= s;
    y *= s;
    return *this;
}

Vec2f& Vec2f::operator/=(f32 s) {
    assert(std::fabs(s) > EPS && "Vec2f division by zero");
    x /= s;
    y /= s;
    return *this;
}

bool Vec2f::operator==(const Vec2f& o) const noexcept {
    return std::fabs(x - o.x) < EPS && std::fabs(y - o.y) < EPS;
}

bool Vec2f::operator!=(const Vec2f& o) const noexcept {
    return !(*this == o);
}

Vec2f operator*(f32 s, const Vec2f& v) noexcept {
    return { s * v.x, s * v.y };
}

f32 dot(const Vec2f& a, const Vec2f& b) noexcept {
    return a.x * b.x + a.y * b.y;
}

f32 length(const Vec2f& v) noexcept {
    return std::sqrt(dot(v, v));
}

Vec2f normalized(const Vec2f& v) noexcept {
    f32 len = length(v);
    return (len > EPS) ? (v / len) : Vec2f{ 0.f, 0.f };
}

Vec2f rotate(const Vec2f& v, f32 deg) noexcept {
    f32 r = deg2rad(deg);
    f32 c = std::cos(r), s = std::sin(r);
    return { c * v.x - s * v.y, s * v.x + c * v.y };
}

// ===== Mat3f =====

Mat3f Mat3f::identity() noexcept {
    // clang-format off
    return Mat3f{ { 1, 0, 0,
                    0, 1, 0,
                    0, 0, 1 } };
    // clang-format on
}

Mat3f Mat3f::translation(f32 tx, f32 ty) noexcept {
    // clang-format off
    return Mat3f{ { 1, 0, tx,
                    0, 1, ty,
                    0, 0, 1 } };
    // clang-format on
}

Mat3f Mat3f::rotation(f32 deg) {
    f32 r = deg2rad(deg);
    f32 c = std::cos(r), s = std::sin(r);
    // clang-format off
    return Mat3f{ { c, -s, 0,
                    s,  c, 0,
                    0,  0, 1 } };
    // clang-format on
}

Mat3f Mat3f::scale(f32 sx, f32 sy) noexcept {
    return Mat3f{ { sx, 0, 0, 0, sy, 0, 0, 0, 1 } };
}

Mat3f Mat3f::operator*(const Mat3f& o) const noexcept {
    Mat3f r{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            r.m[row * 3 + col] = m[row * 3 + 0] * o.m[0 * 3 + col] +
                                 m[row * 3 + 1] * o.m[1 * 3 + col] +
                                 m[row * 3 + 2] * o.m[2 * 3 + col];
        }
    }
    return r;
}

Vec2f Mat3f::transformPoint(const Vec2f& v) const noexcept {
    f32 xNew = m[0] * v.x + m[1] * v.y + m[2];
    f32 yNew = m[3] * v.x + m[4] * v.y + m[5];
    return { xNew, yNew };
}

}  // namespace mu

```


## tests

- `mu_tests.cpp`
```cpp
#include "core/mu.hpp"
#include <catch2/catch_all.hpp>

using Catch::Approx;

TEST_CASE("Vec2f basic operators") {
    mu::Vec2f a{ 1.f, 2.f };
    mu::Vec2f b{ 3.f, 4.f };

    SECTION("addition and subtraction") {
        auto c = a + b;  // {4,6}
        auto d = b - a;  // {2,2}
        REQUIRE(c == mu::Vec2f{ 4.f, 6.f });
        REQUIRE(d == mu::Vec2f{ 2.f, 2.f });
    }

    SECTION("scalar mul/div") {
        auto s = a * 2.f;  // {2,4}
        auto t = a / 2.f;  // {0.5,1}
        REQUIRE(s == mu::Vec2f{ 2.f, 4.f });
        REQUIRE(t == mu::Vec2f{ 0.5f, 1.f });
    }

    SECTION("length/dot/normalized") {
        REQUIRE(mu::dot(a, b) == Approx(11.f).margin(1e-6));
        REQUIRE(mu::length(mu::Vec2f{ 3.f, 4.f }) == Approx(5.f).margin(1e-6));

        auto n = mu::normalized(mu::Vec2f{ 3.f, 4.f });
        REQUIRE(mu::length(n) == Approx(1.f).margin(1e-6));
        // direction preserved (sign)
        REQUIRE(n.x > 0.f);
        REQUIRE(n.y > 0.f);
    }
}

TEST_CASE("Angles and clamp") {
    REQUIRE(mu::deg2rad(180.f) == Approx(mu::PI).margin(1e-6));
    REQUIRE(mu::rad2deg(mu::PI) == Approx(180.f).margin(1e-6));

    REQUIRE(mu::clamp(5.f, 0.f, 10.f) == Approx(5.f).margin(1e-6));
    REQUIRE(mu::clamp(-5.f, 0.f, 10.f) == Approx(0.f).margin(1e-6));
    REQUIRE(mu::clamp(15.f, 0.f, 10.f) == Approx(10.f).margin(1e-6));
}

TEST_CASE("Mat3f transforms") {
    using mu::Mat3f;
    using mu::Vec2f;

    SECTION("identity") {
        auto I = Mat3f::identity();
        REQUIRE(I.transformPoint(Vec2f{ 2.f, 3.f }) == Vec2f{ 2.f, 3.f });
    }

    SECTION("translation") {
        auto T = Mat3f::translation(10.f, -2.f);
        REQUIRE(T.transformPoint(Vec2f{ 1.f, 1.f }) == Vec2f{ 11.f, -1.f });
    }

    SECTION("scale") {
        auto S = Mat3f::scale(2.f, 3.f);
        REQUIRE(S.transformPoint(Vec2f{ 2.f, 2.f }) == Vec2f{ 4.f, 6.f });
    }

    SECTION("rotation 90 deg") {
        auto R = Mat3f::rotation(90.f);
        auto p = R.transformPoint(Vec2f{ 1.f, 0.f });
        REQUIRE(p.x == Approx(0.f).margin(1e-6));
        REQUIRE(p.y == Approx(1.f).margin(1e-6));
    }

    SECTION("composition R * T") {
        auto R = Mat3f::rotation(90.f);
        auto T = Mat3f::translation(2.f, 0.f);
        auto M = R * T;
        auto p = M.transformPoint(Vec2f{ 1.f, 0.f });  // (3,0) -> (0,3)
        REQUIRE(p.x == Approx(0.f).margin(1e-6));
        REQUIRE(p.y == Approx(3.f).margin(1e-6));
    }
}

```

