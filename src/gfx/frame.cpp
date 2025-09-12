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
