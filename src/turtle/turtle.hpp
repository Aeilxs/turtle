#pragma once
#include "core/styles.hpp"
#include "core/types.hpp"
#include "geom/path.hpp"

namespace turtle {

using geom::Path;
using mu::f32;
using mu::Vec2f;

class Turtle {
   public:
    void setPos(Vec2f p) {
        pos_ = p;
    }
    Vec2f pos() const {
        return pos_;
    }

    void setHeading(f32 deg) {
        heading_ = deg;
    }
    f32 heading() const {
        return heading_;
    }

    void penUp() {
        down_ = false;
    }
    void penDown() {
        down_ = true;
    }
    bool isDown() const {
        return down_;
    }

    void setPen(const gfx::Pen& p) {
        pen_ = p;
    }
    const gfx::Pen& pen() const {
        return pen_;
    }

    void beginPath() {
        path_.clear();
        path_.add(pos_);
    }
    Path endPath(bool close = false) {
        path_.closed = close;
        return path_;
    }

    void forward(f32 d) {
        auto dir = mu::rotate(Vec2f{ 1.f, 0.f }, heading_);
        Vec2f dst{ pos_.x + dir.x * d, pos_.y + dir.y * d };
        pos_ = dst;
        if (down_) path_.add(pos_);
    }
    void left(f32 deg) {
        heading_ += deg;
    }
    void right(f32 deg) {
        heading_ -= deg;
    }

   private:
    Vec2f pos_{ 0.f, 0.f };
    f32 heading_{ 0.f };
    bool down_{ true };
    gfx::Pen pen_{ Color{ 255, 255, 255, 255 }, 2.f };
    Path path_;
};

}  // namespace turtle
