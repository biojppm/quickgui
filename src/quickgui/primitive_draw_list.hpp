#ifndef QUICKGUI_PRIMITIVE_DRAW_HPP_
#define QUICKGUI_PRIMITIVE_DRAW_HPP_

#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <c4/error.hpp>
#include <c4/substr.hpp>
#include "quickgui/imgui.hpp"

namespace quickgui {

struct PrimitiveDrawList
{
    typedef enum { text, point, line, rect, rect_filled, poly, circle } PrimitiveType_e;
    struct Primitive
    {
        C4_SUPPRESS_WARNING_MSVC_PUSH
        C4_SUPPRESS_WARNING_MSVC(4582) // constructor is not implicitly called
        Primitive()
            : rect_filled()
            , color()
            , thickness()
            , type()
        {}
        C4_SUPPRESS_WARNING_MSVC_POP
        union
        {
            struct { ImVec2 p; uint32_t first_char, num_chars; } text;
            struct { ImVec2 p;    } point;
            struct { ImVec2 p, q; } line;
            struct { ImRect r;    } rect;
            struct { ImRect r;    } rect_filled;
            struct { uint32_t first_point, num_points; } poly;
            struct { ImVec2 center; float radius; } circle;
        };
        uint32_t color;
        float thickness;
        PrimitiveType_e type;
    };

    std::vector<Primitive> m_primitives;
    std::vector<ImVec2>    m_points;
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

    void draw_text(c4::csubstr txt, ImVec2 p, uint32_t color, float thickness) noexcept
    {
        const size_t first = m_characters.size();
        m_characters.resize(m_characters.size() + txt.size());
        memcpy(&m_characters[first], txt.data(), txt.size());
        auto& prim = m_primitives.emplace_back();
        prim.type = text;
        prim.text.p = p;
        prim.text.first_char = (uint32_t)first;
        prim.text.num_chars = (uint32_t)txt.size();
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_point(ImVec2 p, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = point;
        prim.point.p = p;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_line(ImVec2 p, ImVec2 q, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = line;
        prim.line.p = p;
        prim.line.q = q;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_rect(ImRect r, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = rect;
        prim.rect.r = r;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_rect_filled(ImRect r, uint32_t color, float thickness) noexcept
    {
        auto &prim = m_primitives.emplace_back();
        prim.type = rect_filled;
        prim.rect_filled.r = r;
        prim.color = color;
        prim.thickness = thickness;
    }

    /** returns a buffer where the poly points can be written */
    ImVec2* draw_poly(uint32_t num_points, uint32_t color, float thickness) noexcept
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

    void draw_circle(ImVec2 center, float radius, uint32_t color, float thickness) noexcept
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
