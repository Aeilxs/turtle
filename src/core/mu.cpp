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
