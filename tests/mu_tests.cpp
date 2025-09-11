#include "core/mu.hpp"
#include <catch2/catch_all.hpp>

using Catch::Approx;

TEST_CASE("Vec2f basic operators") {
    mu::Vec2f a{ 1.f, 2.f };
    mu::Vec2f b{ 3.f, 4.f };

    SECTION("addition and subtraction") {
        auto c = a + b;  // {4,6}
        auto d = b - a;  // {2,2}
        REQUIRE(c == mu::Vec2f{ 4.f, 6.f });
        REQUIRE(d == mu::Vec2f{ 2.f, 2.f });
    }

    SECTION("scalar mul/div") {
        auto s = a * 2.f;  // {2,4}
        auto t = a / 2.f;  // {0.5,1}
        REQUIRE(s == mu::Vec2f{ 2.f, 4.f });
        REQUIRE(t == mu::Vec2f{ 0.5f, 1.f });
    }

    SECTION("length/dot/normalized") {
        REQUIRE(mu::dot(a, b) == Approx(11.f).margin(1e-6));
        REQUIRE(mu::length(mu::Vec2f{ 3.f, 4.f }) == Approx(5.f).margin(1e-6));

        auto n = mu::normalized(mu::Vec2f{ 3.f, 4.f });
        REQUIRE(mu::length(n) == Approx(1.f).margin(1e-6));
        // direction preserved (sign)
        REQUIRE(n.x > 0.f);
        REQUIRE(n.y > 0.f);
    }
}

TEST_CASE("Angles and clamp") {
    REQUIRE(mu::deg2rad(180.f) == Approx(mu::PI).margin(1e-6));
    REQUIRE(mu::rad2deg(mu::PI) == Approx(180.f).margin(1e-6));

    REQUIRE(mu::clamp(5.f, 0.f, 10.f) == Approx(5.f).margin(1e-6));
    REQUIRE(mu::clamp(-5.f, 0.f, 10.f) == Approx(0.f).margin(1e-6));
    REQUIRE(mu::clamp(15.f, 0.f, 10.f) == Approx(10.f).margin(1e-6));
}

TEST_CASE("Mat3f transforms") {
    using mu::Mat3f;
    using mu::Vec2f;

    SECTION("identity") {
        auto I = Mat3f::identity();
        REQUIRE(I.transformPoint(Vec2f{ 2.f, 3.f }) == Vec2f{ 2.f, 3.f });
    }

    SECTION("translation") {
        auto T = Mat3f::translation(10.f, -2.f);
        REQUIRE(T.transformPoint(Vec2f{ 1.f, 1.f }) == Vec2f{ 11.f, -1.f });
    }

    SECTION("scale") {
        auto S = Mat3f::scale(2.f, 3.f);
        REQUIRE(S.transformPoint(Vec2f{ 2.f, 2.f }) == Vec2f{ 4.f, 6.f });
    }

    SECTION("rotation 90 deg") {
        auto R = Mat3f::rotation(90.f);
        auto p = R.transformPoint(Vec2f{ 1.f, 0.f });
        REQUIRE(p.x == Approx(0.f).margin(1e-6));
        REQUIRE(p.y == Approx(1.f).margin(1e-6));
    }

    SECTION("composition R * T") {
        auto R = Mat3f::rotation(90.f);
        auto T = Mat3f::translation(2.f, 0.f);
        auto M = R * T;
        auto p = M.transformPoint(Vec2f{ 1.f, 0.f });  // (3,0) -> (0,3)
        REQUIRE(p.x == Approx(0.f).margin(1e-6));
        REQUIRE(p.y == Approx(3.f).margin(1e-6));
    }
}
