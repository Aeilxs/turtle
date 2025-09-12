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

enum class OpKind : uint8_t { StrokePath /*, FillPath TODO*/ };

struct DrawOp {
    OpKind kind{ OpKind::StrokePath };
    PathId path;
    Pen pen;
    mu::Mat3f local = mu::Mat3f::identity();
    AABB aabb;  // transformed aabb fast culling
};

struct Mesh {
    std::vector<Vec2f> verts;  // triangles: verts.size() % 3 == 0
    AABB aabb;
};

struct IRenderer {
    virtual ~IRenderer() = default;

    virtual void drawMeshes(
        const std::vector<Mesh>& meshes, std::optional<Pen> overridePen, const mu::Mat3f& local
    ) = 0;
};

}  // namespace gfx
