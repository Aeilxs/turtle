#include "backends/sfml_renderer.hpp"
#include "core/styles.hpp"
#include "geom/path.hpp"
#include "gfx/frame.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>
#include <vector>

static mu::Mat3f composeTRS(mu::Vec2f t, f32 rotDeg, mu::Vec2f s) {
    return mu::Mat3f::translation(t.x, t.y) * mu::Mat3f::rotation(rotDeg) *
           mu::Mat3f::scale(s.x, s.y);
}
static inline f32 clampf(f32 v, f32 a, f32 b) {
    return (v < a) ? a : (v > b) ? b : v;
}

static geom::Path makeRegularPolygon(i32 sides) {
    geom::Path P;
    P.closed = true;
    const f32 twoPi = 2.f * mu::PI;
    for (i32 i = 0; i < sides; ++i) {
        f32 a = twoPi * (f32)i / (f32)sides;
        P.add({ std::cos(a), std::sin(a) });
    }
    return P;
}

i32 main() {
    // ===========
    // TWEAKABLES
    // ===========
    const i32 POLY_SIDES = 4;
    const i32 INSTANCES = 5000;
    const f32 BASE_SCALE_PX = 320.f;
    const f32 SCALE_RATIO = 0.950f;
    const f32 ROT_STEP_DEG = 6.0f;
    const f32 GLOBAL_ROT_SPEED = 12.f;
    const f32 STROKE_PX = 1.0f;
    const f32 ARC_TOLERANCE_PX = 0.6f;
    const bool USE_GRADIENT = false;
    const f32 BG_DARKNESS = 0.f;

    f32 zoom = 1.0f;
    const f32 ZOOM_MIN = 0.25f, ZOOM_MAX = __FLT_MAX__, ZOOM_STEP = 1.1f;

    sf::ContextSettings ctx;
    ctx.antiAliasingLevel = 16;
    sf::RenderWindow win(
        sf::VideoMode({ 1280u, 800u }),
        "RECREATIVE PROGRAMMING WINDOW",
        sf::Style::Default,
        sf::State::Windowed,
        ctx
    );
    win.setFramerateLimit(144);
    backends::SfmlRenderer renderer(win);

    gfx::AABB vp;
    vp.min = { 0, 0 };
    vp.max = { 1280, 800 };
    gfx::PathStore store;
    gfx::Frame frame(vp, store);
    frame.setArcTolerancePx(ARC_TOLERANCE_PX);

    gfx::PathId polyId = store.add(makeRegularPolygon(POLY_SIDES));

    auto mkPen = [&](Color c) {
        gfx::Pen p{ c, STROKE_PX / BASE_SCALE_PX };
        p.cap = gfx::LineCap::Butt;
        p.join = gfx::LineJoin::Miter;
        p.miterLimit = 6.0f;
        return p;
    };

    std::vector<gfx::Pen> pens;
    pens.reserve(INSTANCES);
    for (i32 i = 0; i < INSTANCES; ++i) {
        if (!USE_GRADIENT) {
            pens.push_back(mkPen({ 255, 255, 255, 255 }));
        } else {
            f32 u = (f32)i / std::max(1, INSTANCES - 1);
            u = std::pow(u, 0.7f);
            u8 v = (u8)std::round(220.f * (1.f - u));
            pens.push_back(mkPen({ v, v, v, 255 }));
        }
    }

    sf::Clock clk;

    while (win.isOpen()) {
        while (const std::optional ev = win.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) win.close();
            if (const auto* r = ev->getIf<sf::Event::Resized>()) {
                auto sz = win.getSize();
                vp.max = { (f32)sz.x, (f32)sz.y };
            }
            if (const auto* w = ev->getIf<sf::Event::MouseWheelScrolled>()) {
                f32 factor = std::pow(ZOOM_STEP, w->delta);
                zoom = mu::clamp(zoom * factor, ZOOM_MIN, ZOOM_MAX);
            }
            if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
                if (k->scancode == sf::Keyboard::Scancode::Space) {
                    zoom = 1.0f;
                    clk.restart();
                }
            }
        }

        const f32 t = clk.getElapsedTime().asSeconds();
        const f32 globRot = GLOBAL_ROT_SPEED * t;
        const mu::Vec2f center{ vp.max.x * 0.5f, vp.max.y * 0.5f };

        win.clear(sf::Color(BG_DARKNESS, BG_DARKNESS, BG_DARKNESS));
        frame.clear();

        for (i32 i = INSTANCES - 1; i >= 0; --i) {
            f32 s = BASE_SCALE_PX * std::pow(SCALE_RATIO, (f32)i) * zoom;
            f32 rot = globRot + ROT_STEP_DEG * (f32)i;

            frame.addStroke(polyId, pens[i], composeTRS(center, rot, { s, s }));
        }

        frame.rasterize(renderer);
        win.display();
    }
}
