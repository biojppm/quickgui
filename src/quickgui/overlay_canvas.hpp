#ifndef QUICKGUI_OVERLAY_CANVAS_HPP_
#define QUICKGUI_OVERLAY_CANVAS_HPP_

#include "quickgui/imgui.hpp"
#include "quickgui/math.hpp"
#include <vector>

namespace quickgui {
struct PrimitiveDrawList;
struct OverlayCanvas
{
    std::vector<ImVec2> m_transformed_points;

    OverlayCanvas() : m_transformed_points() { m_transformed_points.reserve(128); }

    void clear() { m_transformed_points.clear(); m_transformed_points.shrink_to_fit(); }
    void draw(PrimitiveDrawList const& primitives, ImVec2 primitives_subject_size,
              ImVec2 canvas_position, ImVec2 canvas_size, ImDrawList *draw_list);

    static inline constexpr const float s_point_square_size = 0.01f; // relative to canvas size
    static inline constexpr const float s_min_point_square_size = 10.f; // min absolute pixels
    static inline constexpr const float s_draw_shadow = false; // fixme
    static inline constexpr const float s_draw_shadow_offset = 0.0015f; // fixme

    static float _half_point_size(ImVec2 canvas_size) noexcept
    {
        const float canvas_dim = quickgui::min(canvas_size.x, canvas_size.y);
        return 0.5f * quickgui::max(s_point_square_size * canvas_dim, s_min_point_square_size);
    }

    static ImVec2 _shadow_offset(ImVec2 canvas_size) noexcept
    {
        const float canvas_dim = quickgui::max(canvas_size.x, canvas_size.y);
        return ImVec2{canvas_dim * s_draw_shadow_offset, canvas_dim * s_draw_shadow_offset};
    }

};
} // namespace quickgui

#endif /* QUICKGUI_OVERLAY_CANVAS_HPP_ */
