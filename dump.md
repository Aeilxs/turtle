# FILES

## src

- `main.cpp`
```cpp
#include "backends/sfml_renderer.hpp"
#include "core/styles.hpp"
#include "geom/path.hpp"
#include "gfx/frame.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>
#include <vector>

// ---- utils ----
static mu::Mat3f composeTRS(mu::Vec2f t, float rotDeg, mu::Vec2f s) {
    return mu::Mat3f::translation(t.x, t.y) * mu::Mat3f::rotation(rotDeg) *
           mu::Mat3f::scale(s.x, s.y);
}
static inline mu::Vec2f polar(float r, float th) {
    return { r * std::cos(th), r * std::sin(th) };
}

// ---- Archimède r = a + bθ, échantillonnage ds ≈ constant ----
struct Spiral {
    gfx::PathId id{};
    float t{ 1e-4f };  // démarre > 0 pour éviter le point (0,0)
    float a{ 0.f };
    float b{ 0.01f };      // spacing entre spires = 2πb (en unités monde)
    float phi{ 0.f };      // phase
    float scale{ 100.f };  // monde -> pixels (appliqué dans la transform)
    float stepPx{ 0.9f };  // longueur d’arc visée par point (px)
    size_t maxPoints{ 20000 };
    gfx::Pen pen{ Color{ 255, 255, 255, 255 }, 1.0f };  // width corrigée plus bas
};

static void stepSpiral(gfx::PathStore& store, gfx::Frame& frame, Spiral& s, int samples) {
    auto* P = store.getMutable(s.id);
    if (!P) return;

    bool changed = false;
    for (int i = 0; i < samples; ++i) {
        const float r = s.a + s.b * s.t;
        const float th = s.t + s.phi;
        P->add(polar(r, th));
        changed = true;

        // ds = sqrt(r^2 + b^2) dθ  =>  dθ = ds_world / sqrt(r^2 + b^2)
        const float ds_world = s.stepPx / s.scale;
        const float dtheta = ds_world / std::sqrt(r * r + s.b * s.b);
        s.t += dtheta;

        if (P->pts.size() > s.maxPoints) {
            geom::Path tail;
            tail.closed = false;
            tail.pts.reserve(s.maxPoints);
            for (size_t k = P->pts.size() - s.maxPoints; k < P->pts.size(); ++k)
                tail.add(P->pts[k]);
            *P = std::move(tail);
        }
    }
    if (changed) frame.markPathDirty(s.id);
}

int main() {
    // Fenêtre
    sf::ContextSettings ctx;
    ctx.antiAliasingLevel = 16;
    sf::RenderWindow win(
        sf::VideoMode({ 1280u, 800u }), "Spirales 4", sf::Style::Default, sf::State::Windowed, ctx
    );
    win.setFramerateLimit(144);
    backends::SfmlRenderer renderer(win);

    // Pipeline
    gfx::AABB vp;
    vp.min = { 0, 0 };
    vp.max = { 1280, 800 };
    gfx::PathStore store;
    gfx::Frame frame(vp, store);
    frame.setArcTolerancePx(0.14f);

    // Paramètres — spacing voulu (en px) => b en monde
    const float scale = 105.f;     // px par unité monde
    const float spacingPx = 20.f;  // distance entre spires à l’écran
    const float base_b = spacingPx / (scale * 2.f * mu::PI);

    auto makeEmptyPath = [&]() {
        geom::Path P;
        P.closed = false;
        return store.add(std::move(P));
    };

    // Stylos (width en unités monde = 1/scale => 1 px à l’écran)
    auto mkPen = [&](Color c) {
        gfx::Pen p{ c, 1.0f / scale };
        p.cap = gfx::LineCap::Round;
        p.join = gfx::LineJoin::Round;
        return p;
    };

    // 4 spirales déphasées + b légèrement différents
    f32 base_acc = 1.00f;
    u8 base_color = 0;
    std::vector<Spiral> S;
    for (size_t i = 0; i < 20; i++) {
        Spiral a = Spiral{ makeEmptyPath(), 1e-4f, 0.f,   base_b * base_acc,           0.f,
                           scale,           0.9f,  22000, mkPen({ 20, 200, 160, 255 }) };
        base_acc += 0.10f;
        base_color += 20;
        S.push_back(a);
    }

    sf::Clock clock, step;
    float samplesPerSec = 250.f;

    auto resetAll = [&]() {
        for (auto& s : S) {
            if (auto* P = store.getMutable(s.id)) {
                P->clear();
                P->closed = false;
            }
            s.t = 1e-4f;
            frame.markPathDirty(s.id);
        }
    };

    while (win.isOpen()) {
        while (const std::optional ev = win.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) win.close();
            if (const auto* r = ev->getIf<sf::Event::Resized>()) {
                (void)r;
                auto sz = win.getSize();
                vp.max = { (float)sz.x, (float)sz.y };
            }
            if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
                if (k->scancode == sf::Keyboard::Scancode::R) resetAll();
            }
        }

        const float dt = step.restart().asSeconds();
        const int emit = std::max(1, (int)std::floor(samplesPerSec * dt));
        for (auto& s : S)
            stepSpiral(store, frame, s, emit);

        // Rendu
        win.clear(sf::Color(18, 18, 22));
        frame.clear();

        const float t = clock.getElapsedTime().asSeconds();
        const float breath = 1.0f + 0.01f * std::sin(t * 0.7f);
        const float rot = 5.f * std::sin(t * 0.15f);

        for (const auto& s : S) {
            frame.addStroke(
                s.id,
                s.pen,
                composeTRS(
                    { vp.max.x * 0.5f, vp.max.y * 0.52f },
                    rot,
                    { s.scale * breath, s.scale * breath }
                )
            );
        }

        frame.rasterize(renderer);
        win.display();
    }
}

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
    Pen(Color c, f32 w) : width(w), color(c) {
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

- `turtle/turtle.hpp`
```cpp
#pragma once
#include "core/styles.hpp"
#include "core/types.hpp"
#include "geom/path.hpp"

namespace turtle {

using geom::Path;
using mu::f32;
using mu::Vec2f;

class Turtle {
   public:
    void setPos(Vec2f p) {
        pos_ = p;
    }
    Vec2f pos() const {
        return pos_;
    }

    void setHeading(f32 deg) {
        heading_ = deg;
    }
    f32 heading() const {
        return heading_;
    }

    void penUp() {
        down_ = false;
    }
    void penDown() {
        down_ = true;
    }
    bool isDown() const {
        return down_;
    }

    void setPen(const gfx::Pen& p) {
        pen_ = p;
    }
    const gfx::Pen& pen() const {
        return pen_;
    }

    void beginPath() {
        path_.clear();
        path_.add(pos_);
    }
    Path endPath(bool close = false) {
        path_.closed = close;
        return path_;
    }

    void forward(f32 d) {
        auto dir = mu::rotate(Vec2f{ 1.f, 0.f }, heading_);
        Vec2f dst{ pos_.x + dir.x * d, pos_.y + dir.y * d };
        pos_ = dst;
        if (down_) path_.add(pos_);
    }
    void left(f32 deg) {
        heading_ += deg;
    }
    void right(f32 deg) {
        heading_ -= deg;
    }

   private:
    Vec2f pos_{ 0.f, 0.f };
    f32 heading_{ 0.f };
    bool down_{ true };
    gfx::Pen pen_{ Color{ 255, 255, 255, 255 }, 2.f };
    Path path_;
};

}  // namespace turtle

```

- `backends/sfml_renderer.hpp`
```cpp
#pragma once
#include "gfx/draw.hpp"
#include <SFML/Graphics.hpp>

namespace backends {

class SfmlRenderer final : public gfx::IRenderer {
   public:
    explicit SfmlRenderer(sf::RenderTarget& tgt) : tgt_(tgt) {
    }

    void drawMeshes(const std::vector<gfx::Mesh>& meshes, std::optional<gfx::Pen> overridePen)
        override {
        if (!overridePen) return;
        sf::Color col(
            overridePen->color.r, overridePen->color.g, overridePen->color.b, overridePen->color.a
        );

        for (const auto& m : meshes) {
            if (m.verts.empty()) continue;
            sf::VertexArray va(sf::PrimitiveType::Triangles, m.verts.size());
            for (size_t i = 0; i < m.verts.size(); ++i) {
                va[i].position = { m.verts[i].x, m.verts[i].y };
                va[i].color = col;
            }
            tgt_.draw(va);
        }
    }

   private:
    sf::RenderTarget& tgt_;
};

}  // namespace backends

```

- `backends/sfml_renderer.cpp`
```cpp
#include "backends/sfml_renderer.hpp"

```

- `geom/path.hpp`
```cpp
#pragma once
#include "core/mu.hpp"
#include "core/types.hpp"
#include <cstdint>
#include <vector>

namespace geom {

using mu::Mat3f;
using mu::Vec2f;

struct AABB {
    Vec2f min{ +1e9f, +1e9f };  // yes magic numbers i don't give a fuck
    Vec2f max{ -1e9f, -1e9f };
    void expand(Vec2f p) {
        if (p.x < min.x) min.x = p.x;
        if (p.y < min.y) min.y = p.y;
        if (p.x > max.x) max.x = p.x;
        if (p.y > max.y) max.y = p.y;
    }
    bool overlaps(const AABB& o) const {
        return !(max.x < o.min.x || o.max.x < min.x || max.y < o.min.y || o.max.y < min.y);
    }
};

/// Path = polyline (points consécutifs), éventuellement fermé.
struct Path {
    std::vector<Vec2f> pts;
    bool closed{ false };
    AABB bounds;
    void clear() {
        pts.clear();
        closed = false;
        bounds = {};
    }
    void add(Vec2f p) {
        pts.push_back(p);
        bounds.expand(p);
    }
};

inline Path makeRect(Vec2f p, Vec2f s, bool closed = true) {
    Path r;
    r.closed = closed;
    r.add({ p.x, p.y });
    r.add({ p.x + s.x, p.y });
    r.add({ p.x + s.x, p.y + s.y });
    r.add({ p.x, p.y + s.y });
    return r;
}

inline Path makeCircle(Vec2f c, f32 radius, int segments = 96, bool closed = true) {
    Path P;
    P.closed = closed;
    for (int i = 0; i < segments; ++i) {
        f32 t = 360.f * (f32)i / (f32)segments;
        auto v = mu::rotate({ radius, 0.f }, t);
        P.add({ c.x + v.x, c.y + v.y });
    }
    return P;
}

inline Path transform(const Path& src, const Mat3f& M) {
    Path out;
    out.closed = src.closed;
    out.pts.reserve(src.pts.size());
    for (auto& p : src.pts) {
        auto q = M.transformPoint(p);
        out.add(q);
    }
    return out;
}

}  // namespace geom

```

- `gfx/frame.cpp`
```cpp
#include "gfx/frame.hpp"

namespace gfx {

void Frame::rasterize(IRenderer& renderer) {
    if (ops_.empty()) return;

    // Batch par PenKey
    std::unordered_map<PenKey, std::vector<const DrawOp*>, PenKeyHash> buckets;
    buckets.reserve(ops_.size());
    for (auto& op : ops_) {
        if (op.kind != OpKind::StrokePath) continue;
        buckets[PenKey{ op.pen }].push_back(&op);
    }

    // Pour chaque bucket, construire des Mesh (triangulation + transform CPU)
    for (auto& [pkey, list] : buckets) {
        std::vector<Mesh> meshes;
        meshes.reserve(list.size());
        for (auto* op : list) {
            const auto* P = store_.get(op->path);
            if (!P) continue;
            const Mesh& base = tri_.stroke(*P, op->pen);
            // copier + transformer CPU (local)
            Mesh m;
            m.verts.reserve(base.verts.size());
            for (auto& v : base.verts) {
                auto q = op->local.transformPoint(v);
                m.verts.push_back(q);
                m.aabb.expand(q);
            }
            meshes.push_back(std::move(m));
        }
        renderer.drawMeshes(meshes, pkey.pen);
    }
}

}  // namespace gfx

```

- `gfx/triangulate.cpp`
```cpp
#include "gfx/triangulate.hpp"
#include <algorithm>
#include <cmath>

namespace gfx {

static StrokeParams toParams(const Pen& pen, float arcTolPx) {
    StrokeParams sp;
    sp.width = std::max(0.f, pen.width);
    sp.cap = pen.cap;
    sp.join = pen.join;
    sp.miterLimit = std::max(1.f, pen.miterLimit);
    sp.arcTolPx = std::max(0.05f, arcTolPx);
    return sp;
}

// Nombre de segments pour approximer un arc de rayon r et angle theta
// avec une erreur de flèche (sagitta) <= tol (pixels).
static int segmentsForArc(float r, float theta, float tol, int minSeg = 6, int maxSeg = 256) {
    theta = std::fabs(theta);
    r = std::max(r, 1e-6f);
    tol = std::max(tol, 1e-6f);
    float x = 1.f - std::min(tol / r, 1.9f);
    x = std::clamp(x, -1.f, 1.f);
    float phi = 2.f * std::acos(x);
    if (phi <= 1e-6f) phi = theta;
    int n = int(std::ceil(theta / phi));
    return std::max(minSeg, std::min(maxSeg, n));
}

const Mesh& Triangulator::stroke(const Path& P, const Pen& pen) {
    MeshKey key{ &P, toParams(pen, arcTolPx_) };
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;
    auto mesh = buildStrokeMesh(P, key.sp);
    auto [it2, _] = cache_.emplace(key, std::move(mesh));
    return it2->second;
}

Mesh Triangulator::buildStrokeMesh(const Path& P, const StrokeParams& sp) {
    Mesh M{};
    if (P.pts.size() < 2 || sp.width <= 0.f) return M;

    const f32 hw = sp.width * 0.5f;
    const size_t N = P.pts.size();

    std::vector<Vec2f> dir(N);
    std::vector<Vec2f> nrm(N);
    for (size_t i = 0; i < N - 1; ++i) {
        auto d = Vec2f{ P.pts[i + 1].x - P.pts[i].x, P.pts[i + 1].y - P.pts[i].y };
        auto nd = norm(d);
        dir[i] = nd;
        nrm[i] = perp(nd);
    }
    if (P.closed) {
        auto d = Vec2f{ P.pts[0].x - P.pts[N - 1].x, P.pts[0].y - P.pts[N - 1].y };
        auto nd = norm(d);
        dir[N - 1] = nd;
        nrm[N - 1] = perp(nd);
    } else {
        dir[N - 1] = dir[N - 2];
        nrm[N - 1] = nrm[N - 2];
    }

    auto emitCap = [&](Vec2f p, Vec2f d, Vec2f n, bool start) {
        if (sp.cap == LineCap::Butt) return;

        if (sp.cap == LineCap::Square) {
            Vec2f off = { d.x * hw * (start ? -1.f : +1.f), d.y * hw * (start ? -1.f : +1.f) };
            Vec2f a = { p.x + off.x + n.x * hw, p.y + off.y + n.y * hw };
            Vec2f b = { p.x + n.x * hw, p.y + n.y * hw };
            Vec2f c = { p.x - n.x * hw, p.y - n.y * hw };
            Vec2f d0 = { p.x + off.x - n.x * hw, p.y + off.y - n.y * hw };
            addTri(M, a, b, c);
            addTri(M, a, c, d0);
            return;
        }
        if (sp.cap == LineCap::Round) {
            int segs = segmentsForArc(hw, mu::PI, sp.arcTolPx, 10, 256);
            float base = std::atan2(n.y, n.x) + (start ? mu::PI : 0.f);
            Vec2f center = p;
            Vec2f prev = { center.x + std::cos(base) * hw, center.y + std::sin(base) * hw };
            float dirSign = start ? -1.f : +1.f;
            for (int i = 1; i <= segs; ++i) {
                float ang = base + dirSign * (mu::PI * (float)i / (float)segs);
                Vec2f cur = { center.x + std::cos(ang) * hw, center.y + std::sin(ang) * hw };
                addTri(M, center, prev, cur);
                prev = cur;
            }
        }
    };

    auto emitJoin = [&](Vec2f p, Vec2f n0, Vec2f n1, Vec2f d0, Vec2f d1) {
        float cross = d0.x * d1.y - d0.y * d1.x;
        bool leftTurn = cross > 0.f;

        if (sp.join == LineJoin::Bevel) {
            Vec2f o0 = { p.x + n0.x * hw, p.y + n0.y * hw };
            Vec2f o1 = { p.x + n1.x * hw, p.y + n1.y * hw };
            addTri(M, p, o0, o1);
            return;
        }

        if (sp.join == LineJoin::Round) {
            float a0 = std::atan2(n0.y, n0.x);
            float a1 = std::atan2(n1.y, n1.x);
            auto normA = [&](float a) {
                while (a < 0)
                    a += 2 * mu::PI;
                while (a >= 2 * mu::PI)
                    a -= 2 * mu::PI;
                return a;
            };
            a0 = normA(a0);
            a1 = normA(a1);
            float diff = leftTurn ? ((a1 >= a0) ? a1 - a0 : (a1 + 2 * mu::PI) - a0)
                                  : ((a0 >= a1) ? a0 - a1 : (a0 + 2 * mu::PI) - a1);
            int segs = segmentsForArc(hw, diff, sp.arcTolPx, 8, 192);
            Vec2f prev = { p.x + n0.x * hw, p.y + n0.y * hw };
            for (int i = 1; i <= segs; ++i) {
                float ang =
                    leftTurn ? (a0 + diff * (float)i / segs) : (a0 - diff * (float)i / segs);
                Vec2f cur = { p.x + std::cos(ang) * hw, p.y + std::sin(ang) * hw };
                addTri(M, p, prev, cur);
                prev = cur;
            }
            return;
        }

        if (sp.join == LineJoin::Miter) {
            Vec2f m = norm(Vec2f{ n0.x + n1.x, n0.y + n1.y });
            float denom = dot(m, (leftTurn ? n1 : n0));
            if (std::fabs(denom) < 1e-5f) {
                Vec2f o0 = { p.x + n0.x * hw, p.y + n0.y * hw };
                Vec2f o1 = { p.x + n1.x * hw, p.y + n1.y * hw };
                addTri(M, p, o0, o1);
                return;
            }
            float miterLen = hw / denom;
            if (miterLen > sp.miterLimit * hw) {
                Vec2f o0 = { p.x + n0.x * hw, p.y + n0.y * hw };
                Vec2f o1 = { p.x + n1.x * hw, p.y + n1.y * hw };
                addTri(M, p, o0, o1);
                return;
            }
            Vec2f outer = { p.x + m.x * miterLen, p.y + m.y * miterLen };
            Vec2f o0 = { p.x + n0.x * hw, p.y + n0.y * hw };
            Vec2f o1 = { p.x + n1.x * hw, p.y + n1.y * hw };
            addTri(M, o0, outer, o1);
            Vec2f i0 = { p.x - n0.x * hw, p.y - n0.y * hw };
            Vec2f i1 = { p.x - n1.x * hw, p.y - n1.y * hw };
            addTri(M, p, i1, i0);
            return;
        }
    };

    auto emitSegment = [&](Vec2f a, Vec2f b, Vec2f nA, Vec2f nB) {
        Vec2f aL = { a.x + nA.x * hw, a.y + nA.y * hw };
        Vec2f aR = { a.x - nA.x * hw, a.y - nA.y * hw };
        Vec2f bL = { b.x + nB.x * hw, b.y + nB.y * hw };
        Vec2f bR = { b.x - nB.x * hw, b.y - nB.y * hw };
        addTri(M, aL, bL, aR);
        addTri(M, aR, bL, bR);
    };

    if (!P.closed) emitCap(P.pts.front(), dir[0], nrm[0], /*start*/ true);

    for (size_t i = 0; i < N - 1; ++i) {
        emitSegment(P.pts[i], P.pts[i + 1], nrm[i], nrm[i]);
        if (i < N - 2) {
            emitJoin(P.pts[i + 1], nrm[i], nrm[i + 1], dir[i], dir[i + 1]);
        } else if (P.closed) {
            emitJoin(P.pts[0], nrm[N - 1], nrm[0], dir[N - 1], dir[0]);
        }
    }

    if (!P.closed) emitCap(P.pts.back(), dir[N - 2], nrm[N - 2], /*start*/ false);
    return M;
}

void Triangulator::invalidate(const Path* p) {
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (it->first.path == p)
            it = cache_.erase(it);
        else
            ++it;
    }
}

}  // namespace gfx

```

- `gfx/draw.hpp`
```cpp
#pragma once
#include "core/styles.hpp"
#include "geom/path.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace gfx {

using geom::AABB;
using geom::Path;
using mu::f32;
using mu::Vec2f;

using PathId = std::uint64_t;

/// Opérations de dessin retenues
enum class OpKind : uint8_t { StrokePath /*, FillPath (plus tard)*/ };

struct DrawOp {
    OpKind kind{ OpKind::StrokePath };
    PathId path;
    Pen pen;
    // Transform locale (optionnel) — pour SFML on appliquera CPU
    mu::Mat3f local = mu::Mat3f::identity();
    AABB aabb;  // aabb transformée (pour culling rapide)
};

/// Maillage triangulé (positions + couleur uniforme pour le lot)
struct Mesh {
    std::vector<Vec2f> verts;  // triangles: verts.size() % 3 == 0
    AABB aabb;
};

/// Interface backend (retenue simple)
struct IRenderer {
    virtual ~IRenderer() = default;
    virtual void drawMeshes(const std::vector<Mesh>& meshes, std::optional<Pen> overridePen) = 0;
};

}  // namespace gfx

```

- `gfx/frame.hpp`
```cpp
#pragma once
#include "gfx/draw.hpp"
#include "gfx/triangulate.hpp"
#include <unordered_map>

namespace gfx {

class PathStore {
   public:
    PathId add(geom::Path p) {
        PathId id = nextId_++;
        paths_.emplace(id, std::move(p));
        return id;
    }
    const geom::Path* get(PathId id) const {
        auto it = paths_.find(id);
        return it == paths_.end() ? nullptr : &it->second;
    }

    geom::Path* getMutable(PathId id) {
        auto it = paths_.find(id);
        return it == paths_.end() ? nullptr : &it->second;
    }

   private:
    PathId nextId_{ 1 };
    std::unordered_map<PathId, geom::Path> paths_;
};

class Frame {
   public:
    explicit Frame(const AABB& viewport, PathStore& store) : vp_(viewport), store_(store) {
    }

    void addStroke(PathId id, const Pen& pen, const mu::Mat3f& local = mu::Mat3f::identity()) {
        const auto* P = store_.get(id);
        if (!P) return;
        AABB taabb{};
        auto add = [&](mu::Vec2f v) { taabb.expand(v); };
        mu::Vec2f c1{ P->bounds.min.x, P->bounds.min.y };
        mu::Vec2f c2{ P->bounds.max.x, P->bounds.min.y };
        mu::Vec2f c3{ P->bounds.max.x, P->bounds.max.y };
        mu::Vec2f c4{ P->bounds.min.x, P->bounds.max.y };
        add(local.transformPoint(c1));
        add(local.transformPoint(c2));
        add(local.transformPoint(c3));
        add(local.transformPoint(c4));
        if (!vp_.overlaps(taabb)) return;

        DrawOp op;
        op.kind = OpKind::StrokePath;
        op.path = id;
        op.pen = pen;
        op.local = local;
        op.aabb = taabb;
        ops_.push_back(std::move(op));
    }

    void markPathDirty(PathId id) {
        if (const auto* P = store_.get(id)) tri_.invalidate(P);
    }

    void rasterize(IRenderer& renderer);
    void clear() {
        ops_.clear();
    }

    // ---- contrôle finesse des arrondis (en pixels) ----
    void setArcTolerancePx(float px) {
        tri_.setArcTolerancePx(px);
    }

   private:
    AABB vp_;
    PathStore& store_;
    std::vector<DrawOp> ops_;
    Triangulator tri_;

    struct PenKey {
        Pen pen;
        bool operator==(const PenKey& o) const {
            return pen.width == o.pen.width && pen.color == o.pen.color && pen.cap == o.pen.cap &&
                   pen.join == o.pen.join && pen.miterLimit == o.pen.miterLimit;
        }
    };
    struct PenKeyHash {
        size_t operator()(const PenKey& k) const noexcept {
            size_t h = 0;
            auto mix = [&](size_t v) { h ^= v + 0x9e37 + (h << 6) + (h >> 2); };
            mix((size_t)k.pen.color.r | ((size_t)k.pen.color.g << 8) |
                ((size_t)k.pen.color.b << 16) | ((size_t)k.pen.color.a << 24));
            mix(std::hash<int>{}(int(k.pen.cap)));
            mix(std::hash<int>{}(int(k.pen.join)));
            mix(std::hash<int>{}(int(k.pen.miterLimit * 1024)));
            mix(std::hash<int>{}(int(k.pen.width * 1024)));
            return h;
        }
    };
};

}  // namespace gfx

```

- `gfx/triangulate.hpp`
```cpp
#pragma once
#include "gfx/draw.hpp"
#include <cmath>
#include <unordered_map>

namespace gfx {

struct StrokeParams {
    f32 width{ 1.f };
    LineCap cap{ LineCap::Butt };
    LineJoin join{ LineJoin::Bevel };
    f32 miterLimit{ 4.f };
    // Tolérance d’arc en pixels (plus petit = plus de segments)
    f32 arcTolPx{ 0.3f };

    bool operator==(const StrokeParams& o) const {
        return width == o.width && cap == o.cap && join == o.join && miterLimit == o.miterLimit &&
               arcTolPx == o.arcTolPx;
    }
};

struct StrokeParamsHash {
    size_t operator()(const StrokeParams& p) const noexcept {
        size_t h = 0x9e3779b97f4a7c15ull;
        auto mix = [&](size_t v) { h ^= v + 0x9e37 + (h << 6) + (h >> 2); };
        mix(std::hash<int>{}(int(p.cap)));
        mix(std::hash<int>{}(int(p.join)));
        mix(std::hash<int>{}(int(p.miterLimit * 1024)));
        mix(std::hash<int>{}(int(p.width * 1024)));
        mix(std::hash<int>{}(int(p.arcTolPx * 1024)));
        return h;
    }
};

struct MeshKey {
    const Path* path{ nullptr };
    StrokeParams sp;
    bool operator==(const MeshKey& o) const noexcept {
        return path == o.path && sp == o.sp;
    }
};
struct MeshKeyHash {
    size_t operator()(const MeshKey& k) const noexcept {
        size_t h = std::hash<const void*>{}(k.path);
        StrokeParamsHash sph;
        h ^= sph(k.sp) + 0x9e37 + (h << 6) + (h >> 2);
        return h;
    }
};

class Triangulator {
   public:
    void setArcTolerancePx(f32 px) {
        arcTolPx_ = std::max(0.05f, px);
    }
    f32 arcTolerancePx() const {
        return arcTolPx_;
    }

    const Mesh& stroke(const Path& P, const Pen& pen);
    void clear() {
        cache_.clear();
    }
    void invalidate(const Path* p);

   private:
    std::unordered_map<MeshKey, Mesh, MeshKeyHash> cache_;
    f32 arcTolPx_{ 0.3f };

    Mesh buildStrokeMesh(const Path& P, const StrokeParams& sp);

    static inline Vec2f perp(const Vec2f& v) {
        return { -v.y, v.x };
    }
    static inline f32 dot(const Vec2f& a, const Vec2f& b) {
        return a.x * b.x + a.y * b.y;
    }
    static inline f32 len(const Vec2f& v) {
        return std::sqrt(dot(v, v));
    }
    static inline Vec2f norm(const Vec2f& v) {
        f32 L = len(v);
        return (L > mu::EPS) ? Vec2f{ v.x / L, v.y / L } : Vec2f{ 0, 0 };
    }

    void addTri(Mesh& m, Vec2f a, Vec2f b, Vec2f c) {
        m.verts.push_back(a);
        m.verts.push_back(b);
        m.verts.push_back(c);
        m.aabb.expand(a);
        m.aabb.expand(b);
        m.aabb.expand(c);
    }
};

}  // namespace gfx

```

