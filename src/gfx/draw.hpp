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
