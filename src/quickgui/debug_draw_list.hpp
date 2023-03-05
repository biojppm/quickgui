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
    typedef enum { point, line, rect, rect_filled, poly } PrimitiveType_e;
    struct Primitive
    {
        Primitive() : point(), color(), thickness(), type() {}
        union
        {
            struct { ddvec p;    } point;
            struct { ddvec p, q; } line;
            struct { ddrect r;   } rect;
            struct { ddrect r;   } rect_filled;
            struct { uint32_t first_point, num_points; } poly;
        };
        uint32_t color;
        float thickness;
        PrimitiveType_e type;
    };

    std::vector<Primitive> m_primitives;
    std::vector<ddvec>     m_points;

public:

    DebugDrawList()
        : m_primitives()
        , m_points()
    {
        m_primitives.reserve(64);
        m_points.reserve(256);
    }

    void clear()
    {
        m_primitives.clear();
        m_points.clear();
    }

    size_t curr() const
    {
        return m_primitives.size();
    }

public:

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

    template<class ...Args>
    void draw_point(size_t thread_id, Args&& ...args) noexcept
    {
        _claim(thread_id).draw_point(std::forward<Args>(args)...);
    }

    template<class ...Args>
    void draw_line(size_t thread_id, Args&& ...args) noexcept
    {
        _claim(thread_id).draw_line(std::forward<Args>(args)...);
    }

    template<class ...Args>
    void draw_rect(size_t thread_id, Args&& ...args) noexcept
    {
        _claim(thread_id).draw_rect(std::forward<Args>(args)...);
    }

    template<class ...Args>
    void draw_rect_filled(size_t thread_id, Args&& ...args) noexcept
    {
        _claim(thread_id).draw_rect_filled(std::forward<Args>(args)...);
    }

    /** returns a buffer where the poly points can be written */
    template<class ...Args>
    ddvec* draw_poly(size_t thread_id, Args&& ...args) noexcept
    {
        return _claim(thread_id).draw_poly(std::forward<Args>(args)...);
    }
};

} // namespace quickgui

#endif /* QUICKGUI_DEBUG_DRAW_HPP_ */
