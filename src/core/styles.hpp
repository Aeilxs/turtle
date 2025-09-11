#pragma once
#include "types.hpp"
#include <vector>

namespace gfx {

/// @brief Line ending style used when rendering strokes.
/// @note Only stored here; actual rasterization is renderer-specific.
enum class LineCap {
    Butt,   /// Flat end, no extension beyond endpoint.
    Round,  /// Semicircular end.
    Square  /// Flat end extended by half stroke width.
};

/// @brief Join style at polyline corners.
/// @note Only stored here; actual rasterization is renderer-specific.
enum class LineJoin {
    Miter,  /// Sharp corner.
    Round,  /// Rounded corner (arc with radius = half stroke width).
    Bevel   /// Flat cut corner.
};

/// @brief Stroke style (outline) describing how lines/paths should be drawn.
/// @details This is a *logical* style: it holds parameters only
struct Pen {
    f32 width{ 1.0f };            /// Stroke width in logical units (must be > 0 to be visible).
    Color color{ 0, 0, 0, 255 };  /// RGBA color (default opaque black).
    bool visible{ true };         /// Quick toggle for drawing.

    LineCap cap{ LineCap::Butt };      /// Line cap style (renderer may approximate).
    LineJoin join{ LineJoin::Miter };  /// Join style for corners.
    f32 miterLimit{ 4.0f };            /// Max miter length / half-width (used if join = Miter).

    /// @brief Dash pattern as alternating on/off lengths (in stroke units).
    ///        Example: {10, 5} -> 10 on, 5 off, repeat.
    /// @note Empty means solid line. Interpretation is renderer-specific.
    std::vector<f32> dash;

    /// @brief Convenience constructor for a solid pen.
    constexpr Pen(Color c, f32 w) : width(w), color(c) {
    }

    /// @brief Default constructor.
    Pen() = default;

    /// @brief Equality (strict byte-wise; no float epsilon).
    constexpr bool operator==(const Pen& o) const {
        return width == o.width && color == o.color && visible == o.visible && cap == o.cap &&
               join == o.join && miterLimit == o.miterLimit && dash == o.dash;
    }
    constexpr bool operator!=(const Pen& o) const {
        return !(*this == o);
    }
};

}  // namespace gfx
