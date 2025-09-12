#include "gfx/frame.hpp"

namespace gfx {

void Frame::rasterize(IRenderer& renderer) {
    if (ops_.empty()) return;

    for (auto& op : ops_) {
        if (op.kind != OpKind::StrokePath) continue;

        const auto* P = store_.get(op.path);
        if (!P) continue;

        const Mesh& base = tri_.stroke(*P, op.pen);
        renderer.drawMeshes({ base }, op.pen, op.local);
    }
}

}  // namespace gfx
