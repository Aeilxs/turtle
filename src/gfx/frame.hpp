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
