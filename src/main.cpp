#include "backends/sfml_renderer.hpp"
#include "core/styles.hpp"
#include "core/types.hpp"
#include "geom/path.hpp"
#include "gfx/frame.hpp"
#include "turtle/turtle.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <functional>

// ---------- utils ----------
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

// Rose: r = a * cos(k t). k impair -> période π ; k pair -> 2π.
static geom::Path makeRose(mu::Vec2f c, float a, int k, int samples) {
    const float period = (k % 2 != 0) ? mu::PI : 2.f * mu::PI;
    return makeParametric(
        [=](float t) {
            const float r = a * std::cos(k * t);
            return mu::Vec2f{ c.x + r * std::cos(t), c.y + r * std::sin(t) };
        },
        0.f,
        period,
        samples,
        true
    );
}

static geom::Path makeLissajous(
    mu::Vec2f c, float ax, float by, float A, float B, float delta, int samples
) {
    return makeParametric(
        [=](float t) {
            return mu::Vec2f{ c.x + A * std::sin(ax * t + delta), c.y + B * std::sin(by * t) };
        },
        0.f,
        2.f * mu::PI,
        samples,
        true
    );
}

static mu::Mat3f composeTRS(mu::Vec2f t, float rotDeg, mu::Vec2f s) {
    return mu::Mat3f::translation(t.x, t.y) * mu::Mat3f::rotation(rotDeg) *
           mu::Mat3f::scale(s.x, s.y);
}

// ---------- main ----------
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
    frame.setArcTolerancePx(0.18f);  // arrondis lisses

    // Géométrie (samples raisonnables)
    auto hypotroId = store.add(makeHypotrochoid({ 0, 0 }, 220.f, 72.f, 110.f, 5000));
    auto roseId = store.add(makeRose({ 0, 0 }, 150.f, /*k=*/27, 1200));  // fermé net à π
    auto lisId = store.add(makeLissajous({ 0, 0 }, 3.f, 2.f, 160.f, 110.f, mu::PI / 3.f, 1100));

    // Stylos (strokes simples)
    gfx::Pen teal{ Color{ 20, 200, 160, 255 }, 1.0f };
    teal.join = gfx::LineJoin::Round;
    teal.cap = gfx::LineCap::Round;
    gfx::Pen magenta{ Color{ 235, 30, 115, 255 }, 1.0f };
    magenta.join = gfx::LineJoin::Round;
    magenta.cap = gfx::LineCap::Round;
    gfx::Pen amber{ Color{ 255, 180, 40, 255 }, 1.0f };
    amber.join = gfx::LineJoin::Round;
    amber.cap = gfx::LineCap::Round;

    sf::Clock clock;

    while (win.isOpen()) {
        while (const std::optional event = win.pollEvent()) {
            if (event->is<sf::Event::Closed>()) win.close();
            if (const auto* r = event->getIf<sf::Event::Resized>()) {
                (void)r;
                auto sz = win.getSize();
                vp.max = { (float)sz.x, (float)sz.y };
            }
        }

        win.clear(sf::Color(18, 18, 22));
        frame.clear();

        const float t = clock.getElapsedTime().asSeconds();

        // Hypotrochoid — gauche
        {
            const float rot = std::sin(t * 0.35f) * 9.f;
            const float s = 0.97f + 0.02f * std::sin(t * 0.9f);
            auto M = composeTRS({ 360.f, 440.f }, rot, { s, s });
            frame.addStroke(hypotroId, teal, M);
        }
        // Rose — haut centre
        {
            const float rot = -std::sin(t * 0.25f) * 22.f;
            const float s = 0.92f + 0.04f * std::sin(t * 0.6f + 1.1f);
            auto M = composeTRS({ 800.f, 250.f + 10.f * std::sin(t * 0.7f) }, rot, { s, s });
            frame.addStroke(roseId, magenta, M);
        }
        // Lissajous — droite
        {
            const float rot = 10.f * std::sin(t * 0.5f + 0.8f);
            const float s = 1.00f + 0.03f * std::sin(t * 0.8f + 2.2f);
            auto M = composeTRS({ 980.f, 520.f }, rot, { s, s });
            frame.addStroke(lisId, amber, M);
        }

        frame.rasterize(renderer);
        win.display();
    }
}
