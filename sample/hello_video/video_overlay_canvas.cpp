#include "video_overlay_canvas.hpp"

using ddvec = quickgui::ddvec;

C4_CONST C4_ALWAYS_INLINE ImVec2 const& conv(ddvec  const& vi) noexcept { return (ImVec2 const&)vi; }
C4_CONST C4_ALWAYS_INLINE ImVec2      & conv(ddvec       & vi) noexcept { return (ImVec2      &)vi; }
C4_CONST C4_ALWAYS_INLINE ddvec  const& conv(ImVec2 const& vi) noexcept { return (ddvec  const&)vi; }
C4_CONST C4_ALWAYS_INLINE ddvec       & conv(ImVec2      & vi) noexcept { return (ddvec       &)vi; }

void VideoOverlayCanvas::draw(ImDrawList *draw_list, ImVec2 video_size, ImVec2 canvas_cursor, ImVec2 canvas_size)
{
    // https://github.com/ocornut/imgui/issues/1415
    // a lambda to transform from video coordinates to canvas coordinates
    auto tr = [vsize=video_size, csize=canvas_size, cursor=canvas_cursor](ddvec v){
        return cursor + csize * conv(v) / vsize;
    };
    const float hps = _half_point_size(canvas_size);
    for(DebugDrawList::Primitive const& prim : m_primitives)
    {
        switch(prim.type)
        {
        case DebugDrawList::point:
        {
            ImVec2 p = tr(prim.point.p);
            ImVec2 topl = p + ImVec2{-hps, -hps};
            ImVec2 topr = p + ImVec2{ hps, -hps};
            ImVec2 botr = p + ImVec2{ hps,  hps};
            ImVec2 botl = p + ImVec2{-hps,  hps};
            // draw the square
            draw_list->AddLine(topl, topr, prim.color, prim.thickness);
            draw_list->AddLine(topr, botr, prim.color, prim.thickness);
            draw_list->AddLine(botr, botl, prim.color, prim.thickness);
            draw_list->AddLine(botl, topl, prim.color, prim.thickness);
            // draw diagonals across the square
            draw_list->AddLine(topl, botr, prim.color, prim.thickness);
            draw_list->AddLine(botl, topr, prim.color, prim.thickness);
            break;
        }
        case DebugDrawList::rect_filled:
        {
            draw_list->AddRectFilled(tr(prim.rect.r.min), tr(prim.rect.r.max), prim.color);
            break;
        }
        case DebugDrawList::rect:
        {
            draw_list->AddRect(tr(prim.rect.r.min), tr(prim.rect.r.max), prim.color);
            break;
        }
        case DebugDrawList::line:
        {
            draw_list->AddLine(tr(prim.line.p), tr(prim.line.q), prim.color, prim.thickness);
            break;
        }
        case DebugDrawList::poly:
        {
            // need to transform all the points; we do it in place
            for(uint32_t p = prim.poly.first_point; p < prim.poly.num_points; ++p)
                m_points[p] = conv(tr(m_points[p]));
            draw_list->AddPolyline(&conv(m_points[prim.poly.first_point]), (int)prim.poly.num_points, prim.color, /*flags*/{}, prim.thickness);
            break;
        }
        default:
            C4_NOT_IMPLEMENTED();
            break;
        }
    }
}
