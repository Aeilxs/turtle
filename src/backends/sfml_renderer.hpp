#pragma once
#include "gfx/draw.hpp"
#include <SFML/Graphics.hpp>

namespace backends {

static inline sf::Transform toSf(const mu::Mat3f& M) {
    return sf::Transform(M.m[0], M.m[1], M.m[2], M.m[3], M.m[4], M.m[5], 0.f, 0.f, 1.f);
}

class SfmlRenderer final : public gfx::IRenderer {
   public:
    explicit SfmlRenderer(sf::RenderTarget& tgt) : tgt_(tgt) {
    }

    void drawMeshes(
        const std::vector<gfx::Mesh>& meshes,
        std::optional<gfx::Pen> overridePen,
        const mu::Mat3f& local
    ) override {
        if (!overridePen) return;
        sf::Color col(
            overridePen->color.r, overridePen->color.g, overridePen->color.b, overridePen->color.a
        );

        sf::RenderStates states;
        states.transform = toSf(local);  // << GPU transform

        for (const auto& m : meshes) {
            if (m.verts.empty()) continue;
            sf::VertexArray va(sf::PrimitiveType::Triangles, m.verts.size());
            for (size_t i = 0; i < m.verts.size(); ++i) {
                va[i].position = { m.verts[i].x, m.verts[i].y };
                va[i].color = col;
            }
            tgt_.draw(va, states);
        }
    }

   private:
    sf::RenderTarget& tgt_;
};

}  // namespace backends
