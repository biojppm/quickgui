#ifndef QUICKGUI_PRIMITIVE_DRAW_LIST_MULTITHREAD_HPP_
#define QUICKGUI_PRIMITIVE_DRAW_LIST_MULTITHREAD_HPP_

#include <atomic>

#include "quickgui/primitive_draw_list.hpp"

namespace quickgui {

struct PrimitiveDrawListMultithread
{
    struct PrimitiveData
    {
        size_t m_thread;
        size_t m_position;
    };

    std::vector<PrimitiveDrawList> m_thread_lists = {};
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
        for(PrimitiveDrawList &thread_list : m_thread_lists)
            thread_list.clear();
        m_curr = 0;
    }

    PrimitiveDrawList& _claim(size_t thread_id)
    {
        C4_CHECK(thread_id < m_thread_lists.size());
        size_t id = ++m_curr;
        // since this is called concurrently, we cannot resize the
        // primitive data array:
        C4_CHECK_MSG(id < m_primitive_data.size(), "number of primitives to draw exceeds the limit: %zu > %zu", id, m_primitive_data.size());
        // in here we are safe:
        PrimitiveData &primitive_data = m_primitive_data[id];
        PrimitiveDrawList &thread_list = m_thread_lists[thread_id];
        primitive_data.m_thread = thread_id;
        primitive_data.m_position = thread_list.curr();
        return thread_list;
    }

    PrimitiveDrawList const& get_list(PrimitiveData const& pd) const
    {
        C4_ASSERT(pd.m_thread < m_thread_lists.size());
        return m_thread_lists[pd.m_thread];
    }

    PrimitiveDrawList & get_list(PrimitiveData const& pd)
    {
        C4_ASSERT(pd.m_thread < m_thread_lists.size());
        return m_thread_lists[pd.m_thread];
    }

    PrimitiveDrawList::Primitive const& get_prim(PrimitiveData const& pd) const
    {
        C4_ASSERT(pd.m_thread < m_thread_lists.size());
        C4_ASSERT(pd.m_position < m_thread_lists[pd.m_thread].m_primitives.size());
        return m_thread_lists[pd.m_thread].m_primitives[pd.m_position];
    }

public:

    #define _C4_DEFINE_DRAW_FN(name)                                    \
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

    #undef _C4_DEFINE_DRAW_FN
};

} // namespace quickgui

#endif /* QUICKGUI_PRIMITIVE_DRAW_HPP_ */
