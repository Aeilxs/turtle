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
