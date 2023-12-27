#include "overlay_canvas.hpp"

namespace quickgui {

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

namespace {
C4_ALWAYS_INLINE C4_CONST ucolor shadow(ucolor c) noexcept
{
    ucolor ret;
    ret.r = (uint8_t)(UINT8_C(0xff) - c.r);
    ret.g = (uint8_t)(UINT8_C(0xff) - c.g);
    ret.b = (uint8_t)(UINT8_C(0xff) - c.b);
    ret.a = UINT8_C(0xff);
    return ret;
}
}

void OverlayCanvas::draw(PrimitiveDrawList const& primitives, ImVec2 primitives_subject_size,
                         ImVec2 canvas_position, ImVec2 canvas_size, ImDrawList *draw_list)
{
    // https://github.com/ocornut/imgui/issues/1415
    // tr (ie transform): a lambda to transform from subject coordinates to canvas coordinates
    auto tr = [ssize=primitives_subject_size, csize=canvas_size, cpos=canvas_position](ImVec2 v){
        return cpos + csize * (v / ssize);
    };
    // transform a length
    auto trlen = [ssize=minof(primitives_subject_size), csize=minof(canvas_size)](float len){
        return csize * (len / ssize);
    };
    // shadow offset position
    auto sh = [offs=_shadow_offset(canvas_size)](ImVec2 p){
        return p + offs;
    };
    for(PrimitiveDrawList::Primitive const& prim : primitives.m_primitives)
    {
        switch(prim.type)
        {
        case PrimitiveDrawList::text:
        {
            const char *first = &primitives.m_characters[prim.text.first_char];
            const char *last = first + prim.text.num_chars;
            const ImVec2 pos = tr(prim.text.p);
            const ucolor shad = shadow(prim.color);
            if(s_draw_shadow > 0.f)
                draw_list->AddText(sh(pos), shad, first, last);
            draw_list->AddText(pos, prim.color, first, last);
            break;
        }
        case PrimitiveDrawList::point:
        {
            const float hps = _half_point_size(canvas_size);
            const ImVec2 p = tr(prim.point.p);
            const ImVec2 topl = p + ImVec2{-hps, -hps};
            const ImVec2 topr = p + ImVec2{ hps, -hps};
            const ImVec2 botr = p + ImVec2{ hps,  hps};
            const ImVec2 botl = p + ImVec2{-hps,  hps};
            const ucolor shad = shadow(prim.color);
            if(s_draw_shadow > 0.f)
            {
                // draw the square
                draw_list->AddLine(sh(topl), sh(topr), shad, prim.thickness);
                draw_list->AddLine(sh(topr), sh(botr), shad, prim.thickness);
                draw_list->AddLine(sh(botr), sh(botl), shad, prim.thickness);
                draw_list->AddLine(sh(botl), sh(topl), shad, prim.thickness);
                // draw diagonals across the square
                draw_list->AddLine(sh(topl), sh(botr), shad, prim.thickness);
                draw_list->AddLine(sh(botl), sh(topr), shad, prim.thickness);
            }
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
            const ImVec2 p = tr(prim.line.p);
            const ImVec2 q = tr(prim.line.q);
            const ucolor shad = shadow(prim.color);
            if(s_draw_shadow > 0.f)
                draw_list->AddLine(sh(p), sh(q), shad, prim.thickness);
            draw_list->AddLine(p, q, prim.color, prim.thickness);
            break;
        }
        case PrimitiveDrawList::rect:
        {
            const ImVec2 min = tr(prim.rect.r.Min);
            const ImVec2 max = tr(prim.rect.r.Max);
            const ucolor shad = shadow(prim.color);
            if(s_draw_shadow > 0.f)
                draw_list->AddRect(sh(min), sh(max), shad);
            draw_list->AddRect(min, max, prim.color);
            break;
        }
        case PrimitiveDrawList::rect_filled:
        {
            const ImVec2 min = tr(prim.rect.r.Min);
            const ImVec2 max = tr(prim.rect.r.Max);
            // it is filled - no shadow needed
            draw_list->AddRectFilled(min, max, prim.color);
            break;
        }
        case PrimitiveDrawList::poly:
        {
            if(prim.poly.num_points)
            {
                // need to transform all the points; we do it in place
                m_transformed_points.resize(prim.poly.num_points);
                if(s_draw_shadow > 0.f)
                {
                    const ucolor shad = shadow(prim.color);
                    for(uint32_t p = prim.poly.first_point; p < prim.poly.num_points; ++p)
                        m_transformed_points[p] = sh(tr(primitives.m_points[p]));
                    draw_list->AddPolyline(&m_transformed_points[0], (int)prim.poly.num_points, shad, /*flags*/{}, prim.thickness);
                }
                for(uint32_t p = prim.poly.first_point; p < prim.poly.num_points; ++p)
                    m_transformed_points[p] = tr(primitives.m_points[p]);
                draw_list->AddPolyline(&m_transformed_points[0], (int)prim.poly.num_points, prim.color, /*flags*/{}, prim.thickness);
            }
            break;
        }
        case PrimitiveDrawList::circle:
        {
            const ImVec2 center = tr(prim.circle.center);
            const float radius = trlen(prim.circle.radius);
            const ucolor shad = shadow(prim.color);
            if(s_draw_shadow > 0.f)
                draw_list->AddCircle(sh(center), radius, shad);
            draw_list->AddCircle(center, radius, prim.color);
            break;
        }
        default:
            C4_NOT_IMPLEMENTED();
            break;
        }
    }
}

C4_SUPPRESS_WARNING_GCC_CLANG_POP

} // namespace quickgui
