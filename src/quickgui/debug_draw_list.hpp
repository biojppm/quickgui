#ifndef QUICKGUI_DEBUG_DRAW_HPP_
#define QUICKGUI_DEBUG_DRAW_HPP_

#include <vector>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <c4/error.hpp>


namespace quickgui {

//! to avoid the need to include imgui.h, we define a compatible vector type
struct ddvec { float x, y; };

//! to avoid the need to include imgui.h, we define a compatible rectangle type
struct ddrect { ddvec min, max; };

struct DebugDrawList
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
            struct { ddvec center; float radius; uint32_t num_segments; } circle;
            struct { ddvec center; float inner_radius, outer_radius; uint32_t inner_color, outer_color; uint32_t num_segments; } ring_filled;
        };
        uint32_t color;
        float thickness;
        PrimitiveType_e type;
    };

    std::vector<Primitive> m_primitives;
    std::vector<ddvec>     m_points;
    std::vector<char>      m_characters;

public:

    DebugDrawList()
        : m_primitives()
        , m_points()
        , m_characters()
    {
        m_primitives.reserve(64);
        m_points.reserve(256);
        m_points.reserve(1024);
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
        draw_text(p, txt, strlen(txt), color, thickness);
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

    void draw_circle(ddvec center, float radius, uint32_t num_segments, uint32_t color, float thickness) noexcept
    {
        auto& prim = m_primitives.emplace_back();
        prim.type = circle;
        prim.circle.center = center;
        prim.circle.radius = radius;
        prim.circle.num_segments = num_segments;
        prim.color = color;
        prim.thickness = thickness;
    }

    void draw_ring_filled(ddvec center, float inner_radius, float outer_radius, uint32_t inner_color, uint32_t outer_color, uint32_t num_segments, uint32_t shade_color, float thickness) noexcept
    {
        auto& prim = m_primitives.emplace_back();
        prim.type = ring_filled;
        prim.ring_filled.center = center;
        prim.ring_filled.inner_radius = inner_radius;
        prim.ring_filled.outer_radius = outer_radius;
        prim.ring_filled.inner_color = inner_color;
        prim.ring_filled.outer_color = outer_color;
        prim.ring_filled.num_segments = num_segments;
        prim.color = shade_color;
        prim.thickness = thickness;
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct MultithreadDebugDrawList
{
    struct PrimitiveData
    {
        size_t m_thread;
        size_t m_position;
    };

    std::vector<DebugDrawList> m_thread_lists = {};
    std::vector<PrimitiveData> m_primitive_data = {};
    std::atomic_size_t m_curr = 0;

    void init(size_t num_threads, size_t max_primitives)
    {
        m_thread_lists.resize(num_threads);
        m_primitive_data.resize(max_primitives);
        clear();
    }

    void ensure_master_id(size_t master_id)
    {
        if(master_id >= m_thread_lists.size())
            m_thread_lists.resize(master_id + 1);
    }

    void clear()
    {
        for(DebugDrawList &thread_list : m_thread_lists)
            thread_list.clear();
        m_curr = 0;
    }

    DebugDrawList& _claim(size_t thread_id)
    {
        C4_CHECK(thread_id < m_thread_lists.size());
        size_t id = ++m_curr;
        // since this is called concurrently, we cannot resize the
        // primitive data array:
        C4_CHECK_MSG(id < m_primitive_data.size(), "number of primitives to draw exceeds the limit: %zu > %zu", id, m_primitive_data.size());
        // in here we are safe:
        PrimitiveData &primitive_data = m_primitive_data[id];
        DebugDrawList &thread_list = m_thread_lists[thread_id];
        primitive_data.m_thread = thread_id;
        primitive_data.m_position = thread_list.curr();
        return thread_list;
    }

    DebugDrawList const& get_list(PrimitiveData const& pd) const
    {
        C4_ASSERT(pd.m_thread < m_thread_lists.size());
        return m_thread_lists[pd.m_thread];
    }

    DebugDrawList & get_list(PrimitiveData const& pd)
    {
        C4_ASSERT(pd.m_thread < m_thread_lists.size());
        return m_thread_lists[pd.m_thread];
    }

    DebugDrawList::Primitive const& get_prim(PrimitiveData const& pd) const
    {
        C4_ASSERT(pd.m_thread < m_thread_lists.size());
        C4_ASSERT(pd.m_position < m_thread_lists[pd.m_thread].m_primitives.size());
        return m_thread_lists[pd.m_thread].m_primitives[pd.m_position];
    }

public:

#define _C4_DEFINE_DRAW_FN(name) \                                      \
    template<class ...Args>                                             \
    auto draw_##name(size_t thread_id, Args&& ...args) noexcept         \
    {                                                                   \
        return _claim(thread_id).draw_##name(std::forward<Args>(args)...); \
    }

    _C4_DEFINE_DRAW_FN(text)
    _C4_DEFINE_DRAW_FN(point)
    _C4_DEFINE_DRAW_FN(line)
    _C4_DEFINE_DRAW_FN(rect)
    _C4_DEFINE_DRAW_FN(rect_filled)
    _C4_DEFINE_DRAW_FN(poly)
    _C4_DEFINE_DRAW_FN(circle)
    _C4_DEFINE_DRAW_FN(ring_filled)

#undef _C4_DEFINE_DRAW_FN
};

} // namespace quickgui

#endif /* QUICKGUI_DEBUG_DRAW_HPP_ */
