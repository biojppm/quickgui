#ifndef QUICKGUI_MEM_HPP_
#define QUICKGUI_MEM_HPP_

#include <cstring>
#include <c4/language.hpp>
#include <c4/memory_resource.hpp>
#include <c4/memory_util.hpp>
#include <c4/error.hpp>
#include <c4/cpu.hpp>


#define QUICKGUI_L1_CACHE_LINE_SIZE 64 // FIXME


#if (defined(C4_CPU_ARM64) || defined(C4_CPU_ARM))
    #if (defined(__ARM_NEON__) || defined(__ARM_NEON))
         #define QUICKGUI_USE_NEON
    #endif
#elif (defined(C4_CPU_X86_64) || defined(C4_CPU_X86))
    #if defined(__AVX2__)
        #define QUICKGUI_USE_AVX2
    #endif
    #if defined(__AVX__)
        #define QUICKGUI_USE_AVX
    #endif
    #if defined(__SSE4_2__)
        #define QUICKGUI_USE_SSE4_2
    #endif
    #if defined(__SSE4_1__)
        #define QUICKGUI_USE_SSE4_1
    #endif
    #if defined(__SSE3__)
        #define QUICKGUI_USE_SSE3
    #endif
    #if defined(__SSE2__) || (defined(_MSC_VER) && defined(C4_CPU_X86_64))
        #define QUICKGUI_USE_SSE2
    #endif
    #if defined(__SSE__)
        #define QUICKGUI_USE_SSE
    #endif
#else
    #error "unknown cpu"
#endif

#if (!defined(QUICKGUI_USE_NEON) && !defined(QUICKGUI_USE_SSE) && !defined(QUICKGUI_USE_AVX2))
    #define QUICKGUI_USE_SCALAR
#endif

#if defined(QUICKGUI_USE_AVX2) || defined(QUICKGUI_USE_AVX)
    #include <immintrin.h>
    #define QUICKGUI_SIMD_CHUNK 256
    #define QUICKGUI_ALIGNMENT 32
#elif (defined(QUICKGUI_USE_SSE4_2)   \
    || defined(QUICKGUI_USE_SSE4_1)   \
    || defined(QUICKGUI_USE_SSE3)     \
    || defined(QUICKGUI_USE_SSE2)     \
    || defined(QUICKGUI_USE_SSE))
    #include <immintrin.h>
    #define QUICKGUI_SIMD_CHUNK 128
    #define QUICKGUI_ALIGNMENT 16
#endif

#if defined(QUICKGUI_USE_NEON)
    #include <arm_neon.h>
    #define QUICKGUI_SIMD_CHUNK 128
    #define QUICKGUI_ALIGNMENT 16
#endif

namespace quickgui::simd {
inline constexpr const size_t chunkbits = QUICKGUI_SIMD_CHUNK;
inline constexpr const size_t alignment = QUICKGUI_ALIGNMENT;
inline constexpr const size_t chunkbytes = chunkbits / 8;
template<class T>
inline constexpr const size_t chunkelms_v = chunkbytes / sizeof(T);
} // namespace quickgui::simd


#define QUICKGUI_CACHE_PAD(T, var)                                         \
    T var;                                                            \
    char C4_XCAT(___pad___, __LINE__)[c4::mult_remainder(sizeof(T), QUICKGUI_L1_CACHE_LINE_SIZE)];

#define QUICKGUI_INLINE C4_ALWAYS_INLINE
#define QUICKGUI_RESTRICT C4_RESTRICT

#define QUICKGUI_ALIGNED alignas(QUICKGUI_ALIGNMENT)

#define QUICKGUI_ASSERT_ALIGNED(ptr) C4_ASSERT(quickgui::is_aligned(ptr));
#define QUICKGUI_ASSERT_ALIGNED_TO(ptr, alignment) C4_ASSERT(quickgui::is_aligned(ptr C4_COMMA alignment));

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0886r0.pdf
#ifndef _MSC_VER
#define QUICKGUI_ASSUME_ALIGNED(ptrty, ptr) (ptrty)(__builtin_assume_aligned((ptr), QUICKGUI_ALIGNMENT)); QUICKGUI_ASSERT_ALIGNED(ptr)
#define QUICKGUI_ASSUME_ALIGNED_TO(ptrty, ptr, alignment) (ptrty)(__builtin_assume_aligned((ptr), alignment)); QUICKGUI_ASSERT_ALIGNED_TO(ptr, alignment)
#else
#define QUICKGUI_ASSUME_ALIGNED(ptrty, ptr) (ptrty)ptr; QUICKGUI_ASSERT_ALIGNED(ptr)
#define QUICKGUI_ASSUME_ALIGNED_TO(ptrty, ptr, alignment) (ptrty)ptr; QUICKGUI_ASSERT_ALIGNED_TO(ptr, alignment)
#endif

#define QUICKGUI_STATIC_ERROR(ty, msg) static_assert(always_false_v<ty>, msg)
namespace quickgui {
template<class> inline constexpr bool always_false_v = false;
} // namespace quickgui



// bit_cast is c++20
#if C4_CPP < 20
namespace std {
template <class To, class From>
C4_ALWAYS_INLINE auto constexpr bit_cast(From src) noexcept
    -> std::enable_if_t<sizeof(To) == sizeof(From) &&
                        std::is_fundamental_v<From> &&
                        std::is_fundamental_v<To>,
                        To>
{
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}
} // namespace std
#endif

namespace quickgui {

template<class T>
using cache_pad = c4::MultSized<T, QUICKGUI_L1_CACHE_LINE_SIZE>;


/** return true if n is a power of two */
template<class I>
C4_ALWAYS_INLINE C4_CONST constexpr bool is_pot(I n) noexcept
{
    return (n > 0) && !(n & (n - I(1)));
}


/** get the remainder of integer division */
template<class I, I denominator>
constexpr C4_CONST C4_ALWAYS_INLINE I rem(I numerator) noexcept
{
    if constexpr (is_pot(denominator))
        return numerator & (denominator - 1);
    else
        return numerator % denominator;
}


/** true if ptr is aligned */
template<class T>
constexpr C4_CONST C4_ALWAYS_INLINE bool is_aligned(T const* ptr, size_t alignment=QUICKGUI_ALIGNMENT) noexcept
{
    C4_ASSERT(is_pot(alignment));
    return (reinterpret_cast<uintptr_t>(ptr) & (alignment - size_t(1))) == 0;
}


template<class I>
constexpr C4_CONST C4_ALWAYS_INLINE I next_multiple(I val, I base) noexcept
{
    C4_ASSERT(base > 0);
    return base * ((val + base - I(1)) / base);
}
template<class I, I base>
constexpr C4_CONST C4_ALWAYS_INLINE I next_multiple(I val) noexcept
{
    static_assert(base > 0);
    return base * ((val + base - I(1)) / base);
}

template<class I>
constexpr C4_CONST C4_ALWAYS_INLINE I prev_multiple(I val, I base) noexcept
{
    const I next = next_multiple(val, base);
    return next != val ? next - base : val;
}
template<class I, I base>
constexpr C4_CONST C4_ALWAYS_INLINE I prev_multiple(I val) noexcept
{
    const I next = next_multiple<I, base>(val);
    return next != val ? next - base : val;
}


/** eXTend to ALignment: make sure that the given number of elements is
 * extended such that they cover a multiple of the alignment block.
 * @return the number of elements required to span the full alignment block.
 * @precondition: elm_size and alignment must be multiples of each other.
 */
constexpr C4_ALWAYS_INLINE size_t xtal(const size_t num_elms, const size_t elm_size, const size_t alignment=QUICKGUI_ALIGNMENT) noexcept
{
    C4_ASSERT(is_pot(alignment));
    C4_ASSERT((elm_size % alignment) == 0);
    if(elm_size >= alignment)
        return num_elms;
    // TODO simplify this by using c4::msb()
    // or https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
    return num_elms + ((alignment - ((num_elms * elm_size) % alignment)) % alignment) / elm_size;
}


template<class I> constexpr C4_ALWAYS_INLINE C4_CONST I all_bits() noexcept;
template<> constexpr C4_ALWAYS_INLINE C4_CONST  uint8_t all_bits() noexcept { return  UINT8_C(0xff); }
template<> constexpr C4_ALWAYS_INLINE C4_CONST uint16_t all_bits() noexcept { return UINT16_C(0xffff); }
template<> constexpr C4_ALWAYS_INLINE C4_CONST uint32_t all_bits() noexcept { return UINT32_C(0xffffffff); }
template<> constexpr C4_ALWAYS_INLINE C4_CONST uint64_t all_bits() noexcept { return UINT64_C(0xffffffffffffffff); }
template<> constexpr C4_ALWAYS_INLINE C4_CONST   int8_t all_bits() noexcept { return   INT8_C(-1); }
template<> constexpr C4_ALWAYS_INLINE C4_CONST  int16_t all_bits() noexcept { return  INT16_C(-1); }
template<> constexpr C4_ALWAYS_INLINE C4_CONST  int32_t all_bits() noexcept { return  INT32_C(-1); }
template<> constexpr C4_ALWAYS_INLINE C4_CONST  int64_t all_bits() noexcept { return  INT64_C(-1); }

template<class I>
constexpr C4_ALWAYS_INLINE C4_CONST I all_bits(I last_bit) noexcept
{
    return last_bit < (I)8 * (I)sizeof(I) ? (I)((I(1) << last_bit) - I(1)) : all_bits<I>();
}

template<class I>
constexpr C4_ALWAYS_INLINE C4_CONST I contiguous_mask(I first_bit, I last_bit) noexcept
{
    C4_ASSERT(first_bit <= last_bit);
    return all_bits<I>(last_bit) & ~all_bits<I>(first_bit);
}


enum : int {
    PrologueEnabled = 1,
    LoopEnabled = 1<<1,
    EpilogueEnabled = 1<<2
};


template<class MaskType, int num_elms>
struct range_aligner
{
    int prologue_delta;
    MaskType prologue_mask;
    int loop_first;
    int loop_last;
    int epilogue_delta;
    MaskType epilogue_mask;

    int enabled_parts() const
    {
        return ((prologue_delta > 0)
                | ((loop_first < loop_last) << 1)
                | ((epilogue_delta > 0) << 2));
    }

    range_aligner(int first_, int last_)
    {
        loop_first = next_multiple<int, num_elms>(first_);
        loop_last = prev_multiple<int, num_elms>(last_);
        loop_last = loop_last > loop_first ? loop_last : loop_first;
        C4_ASSERT(loop_first <= loop_last);
        C4_ASSERT(((loop_last - loop_first) & (num_elms - 1)) == 0);
        prologue_delta = 0;
        const int prologue_first_bit = first_ & (num_elms-1);
        if(loop_first > first_)
        {
            if(loop_first < last_)
                prologue_delta = loop_first - first_;
            else
                prologue_delta = last_ - first_;
        }
        epilogue_delta = last_ >= loop_last ? (last_ - loop_last) : 0;
        C4_ASSERT(prologue_delta >= 0 && prologue_delta < num_elms);
        C4_ASSERT(epilogue_delta >= 0 && epilogue_delta < num_elms);
        C4_ASSERT(prologue_delta == 0 || loop_first >= num_elms);
        C4_ASSERT(prologue_first_bit > 0 || prologue_delta == 0); // prologue can never start on the first aligned bit
        C4_ASSERT(prologue_first_bit < num_elms);
        C4_ASSERT(prologue_delta + (loop_last - loop_first) + epilogue_delta == last_ - first_);
        C4_UNUSED(prologue_first_bit);
        prologue_mask = contiguous_mask((MaskType)prologue_first_bit, (MaskType)(prologue_first_bit+prologue_delta)); // TODO
        epilogue_mask = contiguous_mask((MaskType)0, (MaskType)epilogue_delta); // TODO
    }
};



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// integer range, short and sweet

namespace detail {
template<class I>
struct _irange
{
    static_assert(std::is_integral_v<I>);
    using value_type = I;
    I first, last;
    C4_ALWAYS_INLINE _irange() noexcept {}
    C4_ALWAYS_INLINE _irange(I first_, I last_) noexcept
        : first(first_)
        , last(last_)
    {
        C4_ASSERT(first <= last);
    }
    struct iterator
    {
        I val;
        C4_ALWAYS_INLINE constexpr I operator++ () noexcept { return ++val; }
        C4_ALWAYS_INLINE C4_PURE constexpr I operator* () const noexcept { return val; }
        C4_ALWAYS_INLINE C4_PURE constexpr bool operator!= (iterator const it) const noexcept { return val != it.val; }
        C4_ALWAYS_INLINE C4_PURE constexpr bool operator<  (iterator const it) const noexcept { return val < it.val; }
        C4_ALWAYS_INLINE C4_PURE constexpr bool operator>  (iterator const it) const noexcept { return val > it.val; }
        C4_ALWAYS_INLINE C4_PURE constexpr iterator operator+ (I more) const noexcept { return iterator{val + more}; }
        C4_ALWAYS_INLINE C4_PURE constexpr iterator operator- (I more) const noexcept { return iterator{val - more}; }
    };
    C4_ALWAYS_INLINE C4_PURE constexpr iterator begin() const noexcept { return {first}; }
    C4_ALWAYS_INLINE C4_PURE constexpr iterator end() const noexcept { return {last}; }
};
} // namespace detail

//! integer range, from 0 to last
template<class I>
C4_ALWAYS_INLINE C4_CONST constexpr auto irange(I last) noexcept
    -> typename std::enable_if<std::is_integral_v<I>, detail::_irange<I>>::type
{
    C4_ASSERT(last >= 0);
    return detail::_irange<I>(I(0), last);
}
//! integer range, from first to last
template<class I>
C4_ALWAYS_INLINE C4_CONST constexpr auto irange(I first, I last) noexcept
    -> typename std::enable_if<std::is_integral_v<I>, detail::_irange<I>>::type
{
    return detail::_irange<I>(first, last);
}
//! integer range. version for containers.
template<class T>
C4_ALWAYS_INLINE C4_CONST constexpr auto irange(T const& C4_RESTRICT container) noexcept
    -> typename std::enable_if<!std::is_integral_v<T>, detail::_irange<typename T::size_type>>::type
{
    using I = typename T::size_type;
    return detail::_irange<I>(I(0), container.size());
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// non-owning memory blocks


struct Blob
{
    char * QUICKGUI_RESTRICT buf;
    size_t sz;

    constexpr QUICKGUI_INLINE bool empty() const noexcept { return sz == 0; }
    constexpr QUICKGUI_INLINE size_t size() const noexcept { return sz; }
    constexpr QUICKGUI_INLINE size_t size_bytes() const noexcept { return sz; }
    constexpr QUICKGUI_INLINE char * data() const noexcept { return buf; }

    template<class T>
    constexpr Blob(T * b, size_t num_elms)
        : buf(reinterpret_cast<char* QUICKGUI_RESTRICT>(b))
        , sz(sizeof(T) * num_elms) {}
    constexpr Blob() : buf(), sz() {}
};


template<class T>
struct Span
{
    T * QUICKGUI_RESTRICT buf;
    size_t sz;

    constexpr operator Span<const T> () const noexcept { return {buf, sz};}

    constexpr QUICKGUI_INLINE bool empty() const noexcept { return sz == 0; }
    constexpr QUICKGUI_INLINE size_t size() const noexcept { return sz; }
    constexpr QUICKGUI_INLINE size_t size_bytes() const noexcept { return sz * sizeof(T); }
    constexpr QUICKGUI_INLINE T* data() const noexcept { return buf; }

    constexpr QUICKGUI_INLINE operator Blob () const { return Blob(buf, sz); }

    constexpr Span(T* b, size_t num_elms) : buf(b), sz(num_elms) {}
    constexpr Span() : buf(), sz() {}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// owning memory blocks


/** memory is NOT initialized, and members are not destroyed */
template<class T, size_t N>
struct MemStatic
{
    C4_NO_COPY_OR_MOVE(MemStatic);

    QUICKGUI_ALIGNED T buf[N];

    MemStatic() {}
    MemStatic(size_t sz) { C4_ASSERT(sz <= N); C4_UNUSED(sz); }

    static constexpr QUICKGUI_INLINE bool empty() noexcept { return false; }
    constexpr QUICKGUI_INLINE static size_t size() noexcept { return N; }
    constexpr QUICKGUI_INLINE size_t size_bytes() const noexcept { return N * sizeof(T); }
    constexpr QUICKGUI_INLINE T *    data() const noexcept { return (T*)buf; }

    using       iterator = T      *;
    using const_iterator = T const*;
    constexpr QUICKGUI_INLINE       iterator begin()       noexcept { return buf; }
    constexpr QUICKGUI_INLINE const_iterator begin() const noexcept { return buf; }
    constexpr QUICKGUI_INLINE       iterator end  ()       noexcept { return buf + N; }
    constexpr QUICKGUI_INLINE const_iterator end  () const noexcept { return buf + N; }

    constexpr QUICKGUI_INLINE operator Span<T> () const noexcept { return {(T*)buf, N}; }
    constexpr QUICKGUI_INLINE operator Blob    () const noexcept { return Blob((T*)buf, N); }
};


/** memory is allocated, but NOT initialized */
template<class T>
struct Mem
{
    C4_NO_COPY_OR_MOVE(Mem);

    T * buf;
    size_t sz;

    Mem() : buf(), sz() {}
    Mem(size_t sz_)
        : buf((T*)c4::aalloc(sizeof(T) * sz_, QUICKGUI_ALIGNMENT))
        , sz(sz_)
    {
        QUICKGUI_ASSERT_ALIGNED(buf);
    }

    ~Mem()
    {
        if(buf)
            c4::afree(buf);
        buf = nullptr;
    }

    constexpr bool  empty() const noexcept { return sz == 0; }
    constexpr size_t size() const noexcept { return sz; }
    constexpr QUICKGUI_INLINE size_t size_bytes() const noexcept { return sz * sizeof(T); }
    constexpr QUICKGUI_INLINE T* data() const noexcept { return buf; }

    using       iterator = T      *;
    using const_iterator = T const*;
    constexpr QUICKGUI_INLINE       iterator begin()       noexcept { return buf; }
    constexpr QUICKGUI_INLINE const_iterator begin() const noexcept { return buf; }
    constexpr QUICKGUI_INLINE       iterator end  ()       noexcept { return buf + sz; }
    constexpr QUICKGUI_INLINE const_iterator end  () const noexcept { return buf + sz; }

    constexpr QUICKGUI_INLINE operator Span<T> () const noexcept { return {buf, sz}; }
    constexpr QUICKGUI_INLINE operator Blob    () const noexcept { return Blob(buf, sz); }

    /** @warning old members are NOT copied, new members are NOT
     * initialized */
    void resize(size_t sz_)
    {
        if(sz_ <= sz)
            return;
        if(buf)
            c4::afree(buf);
        buf = (T*)c4::aalloc(sizeof(T) * sz_, QUICKGUI_ALIGNMENT);
        sz = sz_;
        QUICKGUI_ASSERT_ALIGNED(buf);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace copy {

template<size_t sz>
void memcpy_naive(char *QUICKGUI_RESTRICT dst, char const *QUICKGUI_RESTRICT src)
{
    dst = QUICKGUI_ASSUME_ALIGNED(char       *QUICKGUI_RESTRICT, dst);
    src = QUICKGUI_ASSUME_ALIGNED(char const *QUICKGUI_RESTRICT, src);
    for(size_t i = 0; i < sz; ++i)
        dst[i] = src[i];
}
inline void memcpy_naive(char *QUICKGUI_RESTRICT dst, char const *QUICKGUI_RESTRICT src, size_t sz)
{
    dst = QUICKGUI_ASSUME_ALIGNED(char       *QUICKGUI_RESTRICT, dst);
    src = QUICKGUI_ASSUME_ALIGNED(char const *QUICKGUI_RESTRICT, src);
    for(size_t i = 0; i < sz; ++i)
        dst[i] = src[i];
}


//-----------------------------------------------------------------------------

template<size_t sz>
QUICKGUI_INLINE void memcpy_std(char *QUICKGUI_RESTRICT dst, char const *QUICKGUI_RESTRICT src)
{
    dst = QUICKGUI_ASSUME_ALIGNED(char       *QUICKGUI_RESTRICT, dst);
    src = QUICKGUI_ASSUME_ALIGNED(char const *QUICKGUI_RESTRICT, src);
    memcpy(dst, src, sz);
}
QUICKGUI_INLINE void memcpy_std(char *QUICKGUI_RESTRICT dst, char const *QUICKGUI_RESTRICT src, size_t sz)
{
    dst = QUICKGUI_ASSUME_ALIGNED(char       *QUICKGUI_RESTRICT, dst);
    src = QUICKGUI_ASSUME_ALIGNED(char const *QUICKGUI_RESTRICT, src);
    memcpy(dst, src, sz);
}


//-----------------------------------------------------------------------------

#if defined(QUICKGUI_USE_AVX2)
template<size_t sz>
void avx2_unroll4(char *QUICKGUI_RESTRICT dst_, char const *QUICKGUI_RESTRICT src_)
{
    auto dst = QUICKGUI_ASSUME_ALIGNED(__m256i       *QUICKGUI_RESTRICT, dst_);
    auto src = QUICKGUI_ASSUME_ALIGNED(__m256i const *QUICKGUI_RESTRICT, src_);
    enum : size_t { block_size = 32u, num_blocks = sz / block_size };
    size_t block = 0;
    for(; block + 3u < num_blocks; block += 4u)
    {
        _mm256_store_si256(dst + block     , _mm256_load_si256(src + block     ));
        _mm256_store_si256(dst + block + 1u, _mm256_load_si256(src + block + 1u));
        _mm256_store_si256(dst + block + 2u, _mm256_load_si256(src + block + 2u));
        _mm256_store_si256(dst + block + 3u, _mm256_load_si256(src + block + 3u));
    }
    for(size_t i = block * block_size; i < sz; ++i)
    {
        dst_[i] = src_[i];
    }
}
inline void avx2_unroll4(char *QUICKGUI_RESTRICT dst_, char const *QUICKGUI_RESTRICT src_, size_t sz)
{
    auto dst = QUICKGUI_ASSUME_ALIGNED(__m256i       *QUICKGUI_RESTRICT, dst_);
    auto src = QUICKGUI_ASSUME_ALIGNED(__m256i const *QUICKGUI_RESTRICT, src_);
    enum : size_t { block_size = 32u };
    const size_t num_blocks = sz / block_size;
    size_t block = 0;
    for(; block + 3u < num_blocks; block += 4u)
    {
        _mm256_store_si256(dst + block     , _mm256_load_si256(src + block     ));
        _mm256_store_si256(dst + block + 1u, _mm256_load_si256(src + block + 1u));
        _mm256_store_si256(dst + block + 2u, _mm256_load_si256(src + block + 2u));
        _mm256_store_si256(dst + block + 3u, _mm256_load_si256(src + block + 3u));
    }
    for(size_t i = block * block_size; i < sz; ++i)
    {
        dst_[i] = src_[i];
    }
}

template<size_t sz>
void avx2_unroll4_stream(char *QUICKGUI_RESTRICT dst_, char const *QUICKGUI_RESTRICT src_)
{
    auto dst = QUICKGUI_ASSUME_ALIGNED(__m256i       *QUICKGUI_RESTRICT, dst_);
    auto src = QUICKGUI_ASSUME_ALIGNED(__m256i const *QUICKGUI_RESTRICT, src_);
    enum : size_t { block_size = 32u, num_blocks = sz / block_size };
    size_t block = 0;
    for(; block + 3u < num_blocks; block += 4u)
    {
        _mm256_stream_si256(dst + block     , _mm256_stream_load_si256(src + block     ));
        _mm256_stream_si256(dst + block + 1u, _mm256_stream_load_si256(src + block + 1u));
        _mm256_stream_si256(dst + block + 2u, _mm256_stream_load_si256(src + block + 2u));
        _mm256_stream_si256(dst + block + 3u, _mm256_stream_load_si256(src + block + 3u));
    }
    for(size_t i = block * block_size; i < sz; ++i)
    {
        dst_[i] = src_[i];
    }
}
inline void avx2_unroll4_stream(char *QUICKGUI_RESTRICT dst_, char const *QUICKGUI_RESTRICT src_, size_t sz)
{
    auto dst = QUICKGUI_ASSUME_ALIGNED(__m256i       *QUICKGUI_RESTRICT, dst_);
    auto src = QUICKGUI_ASSUME_ALIGNED(__m256i const *QUICKGUI_RESTRICT, src_);
    enum : size_t { block_size = 32u };
    const size_t num_blocks = sz / 32u;
    size_t block = 0;
    for(; block + 3u < num_blocks; block += 4u)
    {
        _mm256_stream_si256(dst + block     , _mm256_stream_load_si256(src + block     ));
        _mm256_stream_si256(dst + block + 1u, _mm256_stream_load_si256(src + block + 1u));
        _mm256_stream_si256(dst + block + 2u, _mm256_stream_load_si256(src + block + 2u));
        _mm256_stream_si256(dst + block + 3u, _mm256_stream_load_si256(src + block + 3u));
    }
    for(size_t i = block * block_size; i < sz; ++i)
    {
        dst_[i] = src_[i];
    }
}
#endif

} // namespace copy
} // namespace quickgui

#endif /* QUICKGUI_MEM_HPP_ */
