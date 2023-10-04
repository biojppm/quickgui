#ifndef QUICKGUI_STATIC_VECTOR_HPP_
#define QUICKGUI_STATIC_VECTOR_HPP_

#include <cstdint>
#include <c4/error.hpp>
#include "quickgui/mem.hpp"

namespace quickgui {

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

/** size is determined at creation; thereafter cannot resize, cannot
 * add or remove elements. Behaves as a unique_ptr<T[]>, but the data
 * is aligned. */
template<class T, size_t Alignment=alignof(T)>
struct fixed_vector
{
    static constexpr inline size_t alignment = Alignment;
    static_assert(sizeof(T) % alignment == 0);
    using size_type = size_t;
    using value_type = T;

private:

    T *C4_RESTRICT m_data;
    size_t m_size;

public:

    fixed_vector() : m_data(), m_size()
    {
    }

    fixed_vector(size_t sz) : m_data(), m_size()
    {
        _create(sz);
    }

    template<class ...Args>
    fixed_vector(size_t sz, Args const& C4_RESTRICT ...args) : m_data(), m_size()
    {
        _create(sz, args...);
    }

    ~fixed_vector()
    {
        _destroy();
    }

    fixed_vector(fixed_vector &&that) : m_data(that.m_data), m_size(that.m_size)
    {
        that.m_data = nullptr;
        that.m_size = 0;
    }
    fixed_vector& operator= (fixed_vector &&that)
    {
        _destroy();
        m_data = that.m_data;
        m_size = that.m_size;
        that.m_data = nullptr;
        that.m_size = 0;
        return *this;
    }

    fixed_vector(fixed_vector const&) = delete;
    fixed_vector& operator= (fixed_vector const&) = delete;

    template<class ...Args>
    void reset(size_t sz, Args const& C4_RESTRICT ...args)
    {
        if(sz != m_size)
        {
            _destroy();
            _create(sz, args...);
        }
        else
        {
            for(size_t i = 0; i < m_size; ++i)
            {
                QUICKGUI_ASSERT_ALIGNED_TO(&m_data[i], Alignment);
                m_data[i].~T();
                new ((void*)&m_data[i]) T(args...);
            }
        }
    }

    void swap(fixed_vector &that)
    {
        auto data_tmp = m_data;
        m_data = that.m_data;
        that.m_data = data_tmp;
        auto size_tmp = m_size;
        m_size = that.m_size;
        that.m_size = size_tmp;
    }

public:

    using iterator = T*;
    using const_iterator = T const*;

    bool   empty() const { return m_size == 0; }
    size_t size() const { return m_size; }

public:

    T      * data()       { return m_data; }
    T const* data() const { return m_data; }

    iterator       begin()       { return m_data; }
    const_iterator begin() const { return m_data; }

    iterator       end()       { return m_data + m_size; }
    const_iterator end() const { return m_data + m_size; }

    T      & front()       { C4_ASSERT(m_size > 0); return *m_data; }
    T const& front() const { C4_ASSERT(m_size > 0); return *m_data; }

    T      & back()       { C4_ASSERT(m_size > 0); return *((m_data + m_size) - size_t(1)); }
    T const& back() const { C4_ASSERT(m_size > 0); return *((m_data + m_size) - size_t(1)); }

    C4_ALWAYS_INLINE const T& operator[](size_t pos) const
    {
        C4_ASSERT(pos < m_size);
        return m_data[pos];
    }

    C4_ALWAYS_INLINE T& operator[](size_t pos)
    {
        C4_ASSERT(pos < m_size);
        return m_data[pos];
    }

private:

    template<class ...Args>
    void _create(size_t sz, Args const& C4_RESTRICT ...args)
    {
        C4_ASSERT(m_size == 0);
        C4_ASSERT(m_data == nullptr);
        m_size = sz;
        m_data = (T*) c4::aalloc(sz * sizeof(T), alignment);
        for(size_t i = 0; i < m_size; ++i)
        {
            QUICKGUI_ASSERT_ALIGNED_TO(&m_data[i], alignment);
            new ((void*)&m_data[i]) T(args...);
        }
    }

    void _destroy()
    {
        if(m_data)
        {
            for(size_t i = 0; i < m_size; ++i)
                m_data[i].~T();
            c4::afree(m_data);
        }
        m_size = 0;
        m_data = nullptr;
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


template<class T, size_t N>
struct static_vector
{
    static constexpr inline size_t alignment = alignof(T);
    static_assert(sizeof(T) % alignment == 0);
    using size_type = size_t;

private:

#ifdef NDEBUG
    // avoid unions in release builds
    typename std::aligned_storage<sizeof(T), alignment>::type m_data[N];
#else
    // use a union to show in the debugger
    union {
        typename std::aligned_storage<sizeof(T), alignment>::type m_data[N];
        alignas(alignment) T m_arr[N];
    };
    static_assert(sizeof(m_data) == sizeof(m_arr));
    template<class U, size_t M, size_t Curr> friend struct check_alignment;
#endif
    size_t m_size;

public:

    static_vector() : m_size()
    {
        memset(m_data, 0, sizeof(m_data));
    }

    template<class... Args>
    static_vector(size_t size, Args const& ...args)
        : m_size(size)
    {
        C4_ASSERT(size <= N);
        for(size_t pos = 0; pos < m_size; ++pos)
        {
            new (&m_data[pos]) T(args...);
        }
        memset(m_data + m_size, 0, (N - m_size) * sizeof(decltype(m_data[0])));
    }

    ~static_vector()
    {
        clear();
    }

    static_vector(static_vector const& that) : static_vector()
    {
        _copy(that);
    }

    static_vector& operator= (static_vector const& that)
    {
        clear();
        _copy(that);
        return *this;
    }

    void _copy(static_vector const& that)
    {
        m_size = that.m_size;
        if constexpr(std::is_trivially_copyable_v<T>)
        {
            memcpy(m_data, that.m_data, sizeof(T) * that.m_size);
        }
        else
        {
            for(size_t pos = 0; pos < m_size; ++pos)
            {
                new (&m_data[pos]) T((T const&) that.m_data[pos]);
            }
        }
    }
public:

    using iterator = T*;
    using const_iterator = T const*;

    bool   empty() const { return m_size == 0; }
    size_t size() const { return m_size; }
    static constexpr size_t capacity() { return N; }

    T      * data()       { return m_size ? (T      *) &m_data[0] : nullptr; }
    T const* data() const { return m_size ? (T const*) &m_data[0] : nullptr; }

public:

    iterator       begin()       { return reinterpret_cast<T      *>(m_data); }
    const_iterator begin() const { return reinterpret_cast<T const*>(m_data); }

    iterator       end()       { return reinterpret_cast<T      *>(m_data + m_size); }
    const_iterator end() const { return reinterpret_cast<T const*>(m_data + m_size); }

    T      & front()       { C4_ASSERT(m_size > 0); return reinterpret_cast<T      &>(*m_data); }
    T const& front() const { C4_ASSERT(m_size > 0); return reinterpret_cast<T const&>(*m_data); }

    T      & back()       { C4_ASSERT(m_size > 0); return reinterpret_cast<T      &>(*((m_data + m_size) - size_t(1))); }
    T const& back() const { C4_ASSERT(m_size > 0); return reinterpret_cast<T const&>(*((m_data + m_size) - size_t(1))); }

    C4_ALWAYS_INLINE const T& operator[](size_t pos) const
    {
        C4_ASSERT(m_size <= N);
        C4_ASSERT(pos < m_size);
        return *reinterpret_cast<const T*>(&m_data[pos]);
    }

    C4_ALWAYS_INLINE T& operator[](size_t pos)
    {
        C4_ASSERT(m_size <= N);
        C4_ASSERT(pos < m_size);
        return *reinterpret_cast<T*>(&m_data[pos]);
    }

public:

    void clear()
    {
        for(size_t pos = 0; pos < m_size; ++pos)
        {
            reinterpret_cast<T*>(&m_data[pos])->~T();
        }
        m_size = 0;
    }

    template<class... Args>
    void resize(size_t sz, Args&&... args)
    {
        C4_ASSERT(sz <= N);
        for(size_t pos = sz; pos < m_size; ++pos)
        {
            reinterpret_cast<T*>(&m_data[pos])->~T();
        }
        for(size_t pos = m_size; pos < sz; ++pos)
        {
            new (&m_data[pos]) T(args...);
        }
        m_size = sz;
    }

    template<typename ...Args>
    T& emplace_back(Args&&... args)
    {
        C4_ASSERT(m_size < N);
        return * new(&m_data[m_size++]) T(std::forward<Args>(args)...);
    }

    T&& pop_back()
    {
        C4_ASSERT(m_size > 0);
        --m_size;
        return m_data[m_size];
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

C4_SUPPRESS_WARNING_MSVC_PUSH
C4_SUPPRESS_WARNING_MSVC(4625)  // copy constructor was implicitly defined as deleted
C4_SUPPRESS_WARNING_MSVC(4626)  // assignment operator was implicitly defined as deleted
/** a chameleon that is reusable as a collection of any types.
 * @warning does not construct or destroy */
struct reusable_buffer
{
    fixed_vector<uint8_t> mem = {1024};
    template<class T>
    T *reset(size_t num=1, size_t align=alignof(T)) { return (T*) reset(num, sizeof(T), align); }
    void *reset(size_t num_obj, size_t sz_obj, size_t align_obj)
    {
        C4_ASSERT(mem.data() != nullptr);
        uintptr_t start = (uintptr_t)mem.data();
        uintptr_t aligned = quickgui::next_multiple(start, sz_obj);
        size_t sz = (aligned - start) + num_obj * sz_obj;
        if(mem.size() < sz)
            mem.reset(2 * mem.size() < sz ? 2 * mem.size() : sz);
        memset(mem.data(), 0, mem.size());
        start = (uintptr_t)mem.data();
        aligned = quickgui::next_multiple(start, align_obj);
        return (void*)(mem.data() + (aligned - start));
    }
};
C4_SUPPRESS_WARNING_MSVC_POP

C4_SUPPRESS_WARNING_GCC_CLANG_POP

} // namespace quickgui

#endif /* QUICKGUI_STATIC_VECTOR_HPP_ */
