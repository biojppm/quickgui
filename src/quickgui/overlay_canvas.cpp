#include "overlay_canvas.hpp"

namespace quickgui {


C4_CONST C4_ALWAYS_INLINE ImVec2 const& conv(ddvec  const& vi) noexcept { return (ImVec2 const&)vi; }
C4_CONST C4_ALWAYS_INLINE ImVec2      & conv(ddvec       & vi) noexcept { return (ImVec2      &)vi; }
C4_CONST C4_ALWAYS_INLINE ddvec  const& conv(ImVec2 const& vi) noexcept { return (ddvec  const&)vi; }
C4_CONST C4_ALWAYS_INLINE ddvec       & conv(ImVec2      & vi) noexcept { return (ddvec       &)vi; }

void OverlayCanvas::draw(PrimitiveDrawList const& primitives, ImVec2 primitives_subject_size,
                         ImVec2 canvas_position, ImVec2 canvas_size, ImDrawList *draw_list)
{
    // https://github.com/ocornut/imgui/issues/1415
    // a lambda to transform from video coordinates to canvas coordinates
    auto tr = [vsize=primitives_subject_size, csize=canvas_size, cursor=canvas_position](ddvec v){
        return cursor + csize * conv(v) / vsize;
    };
    const float hps = _half_point_size(canvas_size);
    for(PrimitiveDrawList::Primitive const& prim : primitives.m_primitives)
    {
        switch(prim.type)
        {
        case PrimitiveDrawList::text:
        {
            const char *first = &primitives.m_characters[prim.text.first_char];
            const char *last = first + prim.text.num_chars;
            draw_list->AddText(tr(prim.text.p), prim.color, first, last);
            break;
        }
        case PrimitiveDrawList::point:
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
        case PrimitiveDrawList::line:
        {
            draw_list->AddLine(tr(prim.line.p), tr(prim.line.q), prim.color, prim.thickness);
            break;
        }
        case PrimitiveDrawList::rect:
        {
            draw_list->AddRect(tr(prim.rect.r.min), tr(prim.rect.r.max), prim.color);
            break;
        }
        case PrimitiveDrawList::rect_filled:
        {
            draw_list->AddRectFilled(tr(prim.rect.r.min), tr(prim.rect.r.max), prim.color);
            break;
        }
        case PrimitiveDrawList::poly:
        {
            // need to transform all the points; we do it in place
            m_transformed_points.resize(prim.poly.num_points);
            for(uint32_t p = prim.poly.first_point; p < prim.poly.num_points; ++p)
                m_transformed_points[p] = conv(tr(primitives.m_points[p]));
            draw_list->AddPolyline(&conv(m_transformed_points[0]), (int)prim.poly.num_points, prim.color, /*flags*/{}, prim.thickness);
            break;
        }
        case PrimitiveDrawList::circle:
        {
            draw_list->AddCircle(tr(prim.circle.center), prim.circle.radius, prim.color);
            break;
        }
        case PrimitiveDrawList::ring_filled:
        {
            draw_list->AddCircle(tr(prim.circle.center), prim.circle.radius, prim.color);
            break;
        }
        default:
            C4_NOT_IMPLEMENTED();
            break;
        }
    }
}

} // namespace quickgui
