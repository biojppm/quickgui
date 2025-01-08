#ifndef QUICKGUI_OVERLAY_CANVAS_HPP_
#define QUICKGUI_OVERLAY_CANVAS_HPP_

#include "quickgui/imgui.hpp"
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
};

} // namespace quickgui

#endif /* QUICKGUI_OVERLAY_CANVAS_HPP_ */
