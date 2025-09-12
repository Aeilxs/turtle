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

}  // namespace gfx
