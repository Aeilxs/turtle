#include "gfx/frame.hpp"

namespace gfx {

void Frame::rasterize(IRenderer& renderer) {
    if (ops_.empty()) return;

    // Ancien batching par PenKey supprimé ici pour passer la transform propre à chaque op.
    // (Si tu veux re-batcher plus tard, il faudra grouper par (Pen, Mat3f) ou gérer une liste de
    // transforms.)
    for (auto& op : ops_) {
        if (op.kind != OpKind::StrokePath) continue;

        const auto* P = store_.get(op.path);
        if (!P) continue;

        const Mesh& base = tri_.stroke(*P, op.pen);
        // IMPORTANT: plus de transform CPU ici — on dessine en local et on passe la Mat3f au
        // backend
        renderer.drawMeshes({ base }, op.pen, op.local);
    }
}

}  // namespace gfx
