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
