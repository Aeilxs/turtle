#include "backends/sfml_renderer.hpp"
#include "core/styles.hpp"
#include "core/types.hpp"
#include "geom/path.hpp"
#include "gfx/frame.hpp"
#include "turtle/turtle.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <vector>

// -------- helpers maths --------
static mu::Vec2f rotDeg(const mu::Vec2f& v, float deg) {
    return mu::rotate(v, deg);
}

// Equilatéral CCW centré en c
static std::vector<mu::Vec2f> makeEquilateral(mu::Vec2f c, float R) {
    std::vector<mu::Vec2f> t(3);
    t[0] = { c.x + R * std::cos(mu::deg2rad(-90.f)), c.y + R * std::sin(mu::deg2rad(-90.f)) };
    t[1] = { c.x + R * std::cos(mu::deg2rad(30.f)), c.y + R * std::sin(mu::deg2rad(30.f)) };
    t[2] = { c.x + R * std::cos(mu::deg2rad(150.f)), c.y + R * std::sin(mu::deg2rad(150.f)) };
    return t;  // CCW
}

// Subdivision Koch pour un polygone fermé CCW (bump vers l'extérieur)
static std::vector<mu::Vec2f> kochSubdivide(const std::vector<mu::Vec2f>& in) {
    std::vector<mu::Vec2f> out;
    out.reserve(in.size() * 4);
    const size_t N = in.size();
    for (size_t i = 0; i < N; ++i) {
        mu::Vec2f A = in[i];
        mu::Vec2f B = in[(i + 1) % N];
        mu::Vec2f AB = B - A;                   // utilise l’opérateur membre existant
        mu::Vec2f p1 = A + (AB / 3.f);          // idem pour / et +
        mu::Vec2f p2 = A + (AB * (2.f / 3.f));  // idem pour *
        mu::Vec2f p3 = p1 + rotDeg(p2 - p1, -60.f);
        out.push_back(A);
        out.push_back(p1);
        out.push_back(p3);
        out.push_back(p2);
    }
    return out;
}

static std::vector<mu::Vec2f> makeKochSnowflake(mu::Vec2f center, float radius, int iterations) {
    iterations = std::clamp(iterations, 0, 8);
    auto poly = makeEquilateral(center, radius);
    for (int i = 0; i < iterations; ++i)
        poly = kochSubdivide(poly);
    return poly;
}

static mu::Mat3f composeTRS(mu::Vec2f t, float rotDeg, mu::Vec2f s) {
    return mu::Mat3f::translation(t.x, t.y) * mu::Mat3f::rotation(rotDeg) *
           mu::Mat3f::scale(s.x, s.y);
}

// ---------------- main ----------------
int main() {
    sf::ContextSettings ctx;
    ctx.antiAliasingLevel = 16;
    sf::RenderWindow win(
        sf::VideoMode({ 1280u, 800u }), "koch", sf::Style::Default, sf::State::Windowed, ctx
    );
    win.setFramerateLimit(144);

    backends::SfmlRenderer renderer(win);

    gfx::AABB vp;
    vp.min = { 0, 0 };
    vp.max = { 1280, 800 };
    gfx::PathStore store;
    gfx::Frame frame(vp, store);
    frame.setArcTolerancePx(0.12f);

    int iterations = 6;
    const float baseRadius = 250.f;

    std::vector<mu::Vec2f> fullPts = makeKochSnowflake({ 0, 0 }, baseRadius, iterations);

    geom::Path dyn;
    dyn.closed = true;
    dyn.add(fullPts.front());
    auto dynId = store.add(std::move(dyn));
    size_t cursor = 1;

    gfx::Pen ink{ Color{ 230, 245, 240, 255 }, 1.6f };
    ink.join = gfx::LineJoin::Round;
    ink.cap = gfx::LineCap::Round;

    sf::Clock clock, stepClock;
    float pointsPerSecond = 2500.f;

    auto rebuildFlake = [&]() {
        fullPts = makeKochSnowflake({ 0, 0 }, baseRadius, iterations);
        if (auto* p = store.getMutable(dynId)) {
            p->clear();
            p->closed = true;
            p->add(fullPts.front());
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
                if (kp->scancode == sf::Keyboard::Scancode::R) rebuildFlake();
                if (kp->scancode == sf::Keyboard::Scancode::Up) {
                    iterations = std::min(6, iterations + 1);
                    rebuildFlake();
                }
                if (kp->scancode == sf::Keyboard::Scancode::Down) {
                    iterations = std::max(2, iterations - 1);
                    rebuildFlake();
                }
            }
        }

        float dt = stepClock.restart().asSeconds();
        if (cursor < fullPts.size()) {
            int n = std::max(1, (int)std::floor(pointsPerSecond * dt));
            n = std::min<int>(n, (int)fullPts.size() - (int)cursor);
            if (auto* p = store.getMutable(dynId)) {
                for (int i = 0; i < n; ++i)
                    p->add(fullPts[cursor++]);
                frame.markPathDirty(dynId);
            }
        }

        win.clear(sf::Color(18, 18, 22));
        frame.clear();

        float t = clock.getElapsedTime().asSeconds();
        float s = 0.985f + 0.015f * std::sin(t * 0.6f);
        float rdeg = 4.f * std::sin(t * 0.2f);
        auto M = composeTRS({ vp.max.x * 0.5f, vp.max.y * 0.52f }, rdeg, { s, s });

        frame.addStroke(dynId, ink, M);
        frame.rasterize(renderer);
        win.display();
    }
}
