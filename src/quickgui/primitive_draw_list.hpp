#ifndef QUICKGUI_PRIMITIVE_DRAW_HPP_
#define QUICKGUI_PRIMITIVE_DRAW_HPP_

#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <c4/error.hpp>


namespace quickgui {

//! to avoid the need to include imgui.h, we define a compatible vector type
struct ddvec { float x, y; };

//! to avoid the need to include imgui.h, we define a compatible rectangle type
struct ddrect { ddvec min, max; };

/** this is meant for use in non-gui facing code, so it doesn't bring imgui.h */
struct PrimitiveDrawList
{
    typedef enum { text, point, line, rect, rect_filled, poly, circle, ring_filled } PrimitiveType_e;
    struct Primitive
    {
        Primitive() : point(), color(), thickness(), type() {}
        union
        {
            struct { ddvec p; uint32_t first_char, num_chars; } text;
            struct { ddvec p;    } point;
            struct { ddvec p, q; } line;
            struct { ddrect r;   } rect;
            struct { ddrect r;   } rect_filled;
            struct { uint32_t first_point, num_points; } poly;
            struct { ddvec center; float radius; } circle;
        };
        uint32_t color;
        float thickness;
        PrimitiveType_e type;
    };

    std::vector<Primitive> m_primitives;
    std::vector<ddvec>     m_points;
    std::vector<char>      m_characters;

public:

    PrimitiveDrawList()
        : m_primitives()
        , m_points()
        , m_characters()
    {
        m_primitives.reserve(64);
        m_points.reserve(1024);
        m_characters.reserve(256);
    }

    void clear()
    {
        m_primitives.clear();
        m_points.clear();
        m_characters.clear();
    }

    size_t curr() const
    {
        return m_primitives.size();
    }

public:

    void draw_text(ddvec p, const char* txt, uint32_t color, float thickness) noexcept
    {
        draw_text(p, txt, (uint32_t)strlen(txt), color, thickness);
    }

    void draw_text(ddvec p, const char* txt, uint32_t txt_len, uint32_t color, float thickness) noexcept
    {
        const size_t first = m_characters.size();
        m_characters.resize(m_characters.size() + txt_len);
        memcpy(&m_characters[first], txt, (size_t)txt_len);
        auto& prim = m_primitives.emplace_back();
        prim.type = text;
        prim.text.p = p;
        prim.text.first_char = (uint32_t)first;
        prim.text.num_chars = txt_len;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_point(ddvec p, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = point;
        prim.point.p = p;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_line(ddvec p, ddvec q, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = line;
        prim.line.p = p;
        prim.line.q = q;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_rect(ddrect r, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = rect;
        prim.rect.r = r;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_rect_filled(ddrect r, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = rect_filled;
        prim.rect_filled.r = r;
        prim.color = color;
        prim.thickness = thickness;
    }

    /** returns a buffer where the poly points can be written */
    ddvec* draw_poly(uint32_t num_points, uint32_t color, float thickness) noexcept
    {
        const uint32_t first = (uint32_t)m_points.size();
        auto &prim = m_primitives.emplace_back();
        prim.type = rect;
        prim.poly.first_point = first;
        prim.poly.num_points = num_points;
        prim.color = color;
        prim.thickness = thickness;
        m_points.resize(first + num_points);
        return &m_points[first];
    }

    void draw_circle(ddvec center, float radius, uint32_t color, float thickness) noexcept
    {
        auto& prim = m_primitives.emplace_back();
        prim.type = circle;
        prim.circle.center = center;
        prim.circle.radius = radius;
        prim.color = color;
        prim.thickness = thickness;
    }
};

} // namespace quickgui

#endif /* QUICKGUI_PRIMITIVE_DRAW_HPP_ */
