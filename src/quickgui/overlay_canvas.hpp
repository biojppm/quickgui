#ifndef QUICKGUI_OVERLAY_CANVAS_HPP_
#define QUICKGUI_OVERLAY_CANVAS_HPP_

#include <quickgui/primitive_draw_list.hpp>
#include <quickgui/math.hpp>
#include <quickgui/color.hpp>

namespace quickgui {
struct OverlayCanvas
{
    std::vector<ImVec2> m_transformed_points;

    OverlayCanvas() : m_transformed_points() { m_transformed_points.reserve(128); }

    void draw(PrimitiveDrawList const& primitives, ImVec2 primitives_subject_size,
              ImVec2 canvas_position, ImVec2 canvas_size, ImDrawList *draw_list);

    static inline constexpr const float s_point_square_size = 0.01f; // relative to canvas size
    static inline constexpr const float s_min_point_square_size = 10.f; // min absolute pixels

    static float _half_point_size(ImVec2 canvas_size) noexcept
    {
        const float canvas_dim = quickgui::min(canvas_size.x, canvas_size.y);
        return 0.5f * quickgui::max(s_point_square_size * canvas_dim, s_min_point_square_size);
    }

};
} // namespace quickgui

#endif /* QUICKGUI_OVERLAY_CANVAS_HPP_ */
