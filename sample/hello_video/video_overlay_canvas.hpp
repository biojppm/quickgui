#ifndef QUICKGUI_VIDEO_OVERLAY_CANVAS_HPP_
#define QUICKGUI_VIDEO_OVERLAY_CANVAS_HPP_

#include <quickgui/debug_draw_list.hpp>
#include <quickgui/math.hpp>
#include <quickgui/color.hpp>
#include <quickgui/imgui.hpp>

struct VideoOverlayCanvas : public quickgui::DebugDrawList
{
public:

    VideoOverlayCanvas() : quickgui::DebugDrawList() {}

public:

    void draw(ImDrawList *draw_list, ImVec2 video_size, ImVec2 canvas_cursor, ImVec2 canvas_size);

    static inline constexpr const float s_point_square_size = 0.01f; // relative to canvas size
    static inline constexpr const float s_min_point_square_size = 10.f; // min absolute pixels

    static float _half_point_size(ImVec2 canvas_size) noexcept
    {
        const float canvas_dim = quickgui::min(canvas_size.x, canvas_size.y);
        return 0.5f * quickgui::max(s_point_square_size * canvas_dim, s_min_point_square_size);
    }

};

#endif /* QUICKGUI_VIDEO_OVERLAY_CANVAS_HPP_ */
