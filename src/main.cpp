#include "backends/sfml_renderer.hpp"
#include "core/styles.hpp"
#include "core/types.hpp"
#include "geom/path.hpp"
#include "gfx/frame.hpp"
#include "turtle/turtle.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <functional>
#include <vector>

// ----------------- helpers -----------------
static int gcd_int(int a, int b) {
    a = a < 0 ? -a : a;
    b = b < 0 ? -b : b;
    while (b) {
        int t = a % b;
        a = b;
        b = t;
    }
    return a ? a : 1;
}

static geom::Path makeParametric(
    const std::function<mu::Vec2f(float)>& f, float t0, float t1, int samples, bool closed = true
) {
    geom::Path P;
    P.closed = closed;
    P.pts.reserve(samples);
    const float dt = (t1 - t0) / (float)samples;
    for (int i = 0; i < samples; ++i)
        P.add(f(t0 + dt * i));
    return P;
}

static geom::Path makeHypotrochoid(mu::Vec2f c, float R, float r, float d, int samples) {
    const float k = (R - r) / r;
    const int g = gcd_int((int)std::lround(R), (int)std::lround(r));
    const float period = 2.f * mu::PI * (r / (float)g);
    return makeParametric(
        [=](float t) {
            const float c1 = std::cos(t), s1 = std::sin(t);
            const float c2 = std::cos(k * t), s2 = std::sin(k * t);
            return mu::Vec2f{ c.x + (R - r) * c1 + d * c2, c.y + (R - r) * s1 - d * s2 };
        },
        0.f,
        period,
        samples,
        true
    );
}

static mu::Mat3f composeTRS(mu::Vec2f t, float rotDeg, mu::Vec2f s) {
    return mu::Mat3f::translation(t.x, t.y) * mu::Mat3f::rotation(rotDeg) *
           mu::Mat3f::scale(s.x, s.y);
}

int main() {
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
    frame.setArcTolerancePx(0.14f);

    const float R = 260.f, r = 73.f, d = 110.f;
    const int samples = 7200;
    auto full = makeHypotrochoid({ 0, 0 }, R, r, d, samples);
    std::vector<mu::Vec2f> curvePts = full.pts;  // source list

    geom::Path dyn;
    dyn.closed = false;
    dyn.add(curvePts.front());
    auto dynId = store.add(std::move(dyn));
    size_t cursor = 1;

    // Style
    gfx::Pen ink{ Color{ 230, 245, 240, 255 }, 1.8f };
    ink.join = gfx::LineJoin::Round;
    ink.cap = gfx::LineCap::Round;

    // Animation
    sf::Clock clock, stepClock;
    const float pxStep = 2.2f;  // ~distance par point (approx)
    float speedPx = 420.f;      // pixels/s (vitesse de révélation)

    auto resetDrawing = [&]() {
        if (auto* p = store.getMutable(dynId)) {
            p->clear();
            p->closed = false;
            p->add(curvePts.front());
            cursor = 1;
            frame.markPathDirty(dynId);
        }
    };

    while (win.isOpen()) {
        while (const std::optional ev = win.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) win.close();
            if (const auto* rsz = ev->getIf<sf::Event::Resized>()) {
                (void)rsz;
                auto sz = win.getSize();
                vp.max = { (float)sz.x, (float)sz.y };
            }
            if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
                if (kp->scancode == sf::Keyboard::Scancode::R) resetDrawing();  // relance
            }
        }

        // --- progression
        float dt = stepClock.restart().asSeconds();
        if (cursor < curvePts.size()) {
            int ptsToAdd = std::max(1, (int)std::floor((speedPx / pxStep) * dt));
            if (auto* p = store.getMutable(dynId)) {
                int n = std::min<int>(ptsToAdd, (int)curvePts.size() - (int)cursor);
                for (int i = 0; i < n; ++i)
                    p->add(curvePts[cursor++]);
                frame.markPathDirty(dynId);
                if (cursor >= curvePts.size()) {
                    p->closed = true;
                    frame.markPathDirty(dynId);
                }
            }
        }

        // --- rendu
        win.clear(sf::Color(18, 18, 22));
        frame.clear();

        const float t = clock.getElapsedTime().asSeconds();
        const float s = 0.98f + 0.02f * std::sin(t * 0.7f);  // breathe
        const float rdeg = std::sin(t * 0.25f) * 6.f;        // rotate
        auto M = composeTRS({ vp.max.x * 0.5f, vp.max.y * 0.52f }, rdeg, { s, s });

        frame.addStroke(dynId, ink, M);

        frame.rasterize(renderer);
        win.display();
    }
}
