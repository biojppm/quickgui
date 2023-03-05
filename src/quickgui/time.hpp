#ifndef QUICKGUI_TIME_HPP_
#define QUICKGUI_TIME_HPP_

#include <chrono>
#include <c4/error.hpp>

namespace quickgui {


using clock_type = std::chrono::steady_clock;
static_assert(clock_type::period::den >= clock_type::period::num * 1'000'000'000, "clock period resolution must be at least nanoseconds");

using time_point = std::chrono::time_point<clock_type, std::chrono::nanoseconds>;

using duration = std::chrono::nanoseconds;

using fsecs  = std::chrono::duration<float>;
using fmsecs = std::chrono::duration<float, std::milli>;
using fusecs = std::chrono::duration<float, std::micro>;
using fnsecs = std::chrono::duration<float, std::nano>;

using dsecs  = std::chrono::duration<double>;
using dmsecs = std::chrono::duration<double, std::milli>;
using dusecs = std::chrono::duration<double, std::micro>;
using dnsecs = std::chrono::duration<double, std::nano>;


inline constexpr const duration thirty_hz = std::chrono::nanoseconds(33'333'333);
inline constexpr const duration sixty_hz = std::chrono::nanoseconds(16'666'667);

template<class TimePoint>
quickgui::time_point to_time_point(TimePoint tp)
{
    return std::chrono::time_point_cast<quickgui::time_point>(tp);
}

template<class Duration>
quickgui::duration to_duration(Duration dt)
{
    return std::chrono::duration_cast<quickgui::duration>(dt);
}
/*
    template <class Rep, class Period>
    auto ms(const std::chrono::duration<Rep, Period> &d)
    {
        return std::chrono::duration_cast<std::chrono::duration<Rep, std::milli>>(d);
    }

    template <class Rep>
    auto ms(Rep d) -> std::enable_if_t<std::is_fundamental_v<Rep>, std::chrono::duration<Rep, std::milli>>
    {
        return std::chrono::duration<Rep, std::milli>(d);
    }
*/
/** create a duration of seconds */
template<class Rep>
constexpr duration secs(Rep dt) noexcept
{
    static_assert(std::is_fundamental_v<Rep>);
    namespace sc = std::chrono;
    return sc::duration_cast<quickgui::duration>(sc::duration<Rep>(dt));
}

/** create a duration of milliseconds */
template<class Rep>
constexpr duration msecs(Rep dt) noexcept
{
    static_assert(std::is_fundamental_v<Rep>);
    namespace sc = std::chrono;
    return sc::duration_cast<quickgui::duration>(sc::duration<Rep, std::milli>(dt));
}

/** create a duration of microseconds */
template<class Rep>
constexpr duration usecs(Rep dt) noexcept
{
    static_assert(std::is_fundamental_v<Rep>);
    namespace sc = std::chrono;
    return sc::duration_cast<quickgui::duration>(sc::duration<Rep, std::micro>(dt));
}

/** create a duration of nanoseconds */
template<class Rep>
constexpr duration nsecs(Rep dt) noexcept
{
    static_assert(std::is_fundamental_v<Rep>);
    namespace sc = std::chrono;
    return sc::duration_cast<quickgui::duration>(sc::duration<Rep, std::nano>(dt));
}
template<class Rep, class Ratio>
constexpr duration nsecs(std::chrono::duration<Rep, Ratio> dt) noexcept
{
    return std::chrono::duration_cast<quickgui::duration>(dt);
}

/** returns nanosecond-precision time with the quickgui clock type */
inline quickgui::time_point now()
{
    namespace sc = std::chrono;
    auto dur = clock_type::now().time_since_epoch();
    return quickgui::time_point(sc::duration_cast<quickgui::duration>(dur));
}

time_point set_start_time(time_point tp);
time_point get_start_time();

inline time_point set_start_time()
{
    return set_start_time(now());
}

duration uptime();

/** be nice, and spare the load/store units
 * @see https://www.felixcloutier.com/x86/pause
 * @see https://rigtorp.se/spinlock/ */
inline void pause_spin()
{
#if defined(C4_CPU_X86_64) || defined(C4_CPU_X86)
    #if defined(C4_CLANG) || defined(C4_GCC)
        __builtin_ia32_pause();
    #elif defined(C4_MSVC)
        _mm_pause();
    #else
        #error NOT IMPLEMENTED
    #endif
#elif defined(C4_CPU_ARM64) || defined(C4_CPU_ARM)
    // https://stackoverflow.com/a/70076751/5875572
    asm volatile("yield");
#else
    #error NOT IMPLEMENTED
#endif
}

time_point busy_wait_until(time_point tp, bool pause=true);

inline time_point busy_wait_for(duration dt, bool pause=true)
{
    return busy_wait_until(now() + dt, pause);
}

template<class Duration>
time_point busy_wait_for(Duration dt_, bool pause=true)
{
    return busy_wait_until(now() + to_duration(dt_), pause);
}

template<class TimePoint>
TimePoint busy_wait_until(TimePoint tp_, bool pause=true)
{
    time_point tp = std::chrono::time_point_cast<time_point>(tp_);
    tp = busy_wait_until(tp, pause);
    return std::chrono::time_point_cast<TimePoint>(tp);
}


//-----------------------------------------------------------------------------
void sleep_until(time_point tp);

template<class TimePoint>
void sleep_until(TimePoint tp)
{
    sleep_until(to_time_point(tp));
}

template<class Duration>
void sleep_for(Duration d)
{
    sleep_until(now() + d);
}


//-----------------------------------------------------------------------------

template<class PeriodOut, typename Rep, typename PeriodIn>
constexpr std::chrono::duration<Rep, PeriodIn> next_time_unit(std::chrono::duration<Rep, PeriodIn> d)
{
    auto upcast = std::chrono::duration_cast<std::chrono::duration<Rep, PeriodOut>>(d);
    if constexpr (std::is_floating_point_v<Rep>)
        upcast = decltype(upcast)(Rep(int64_t(upcast.count())));
    return std::chrono::duration_cast<std::chrono::duration<Rep, PeriodIn>>(++upcast);
}

template<class PeriodOut, typename _Clock, typename _Dur>
constexpr std::chrono::time_point<_Clock, _Dur> next_time_unit(std::chrono::time_point<_Clock, _Dur> tp)
{
    return std::chrono::time_point<_Clock, _Dur>(next_time_unit<PeriodOut>(tp.time_since_epoch()));
}

template<class TimeOrDuration>
constexpr TimeOrDuration next_hour(TimeOrDuration t)
{
    return next_time_unit<std::ratio<3600>>(t);
}

template<class TimeOrDuration>
constexpr TimeOrDuration next_minute(TimeOrDuration t)
{
    return next_time_unit<std::ratio<60>>(t);
}

template<class TimeOrDuration>
constexpr TimeOrDuration next_second(TimeOrDuration t)
{
    return next_time_unit<std::ratio<1>>(t);
}

template<class TimeOrDuration>
constexpr TimeOrDuration next_millisecond(TimeOrDuration t)
{
    return next_time_unit<std::milli>(t);
}

template<class TimeOrDuration>
constexpr TimeOrDuration next_microsecond(TimeOrDuration t)
{
    return next_time_unit<std::micro>(t);
}

template<class TimeOrDuration>
constexpr TimeOrDuration next_nanosecond(TimeOrDuration t)
{
    return next_time_unit<std::nano>(t);
}


//-----------------------------------------------------------------------------

template<class T, class Rep, class Period>
T to_hz(std::chrono::duration<Rep, Period> dt)
{
    static_assert(std::is_floating_point_v<T>);
    return T(1) / std::chrono::duration_cast<std::chrono::duration<T>>(dt).count();
}

template<class T>
duration from_hz(T hz)
{
    static_assert(std::is_floating_point_v<T>);
    return secs((duration::rep) T(1) / hz);
}


//-----------------------------------------------------------------------------

struct PeriodicTrigger
{
    duration period; //!< the period with which the trigger fires
    time_point next; //!< the time at which the next trigger should occur

    struct Result
    {
        duration excess;
        operator bool() const { return excess.count() > 0; }
        //time_point time() const { return now() - excess; }
    };

    Result trigger() { return trigger(now()); }
    Result trigger(time_point tp)
    {
        C4_ASSERT(period.count() > 0);
        Result nfo = {tp - next};
        if(nfo)
            next += period;
        return nfo;
    }

    template<class Float> Float elapsed_fraction() const { return elapsed_fraction<Float>(now()); }
    template<class Float> Float elapsed_fraction(time_point tp) const
    {
        return Float(1) - Float((next - tp).count()) / Float(period.count());
    }

    void reset()
    {
        next = now() + period;
    }

    void reset(duration period_)
    {
        period = period_;
        next = now() + period_;
    }

    template<class Duration>
    PeriodicTrigger(Duration period_, bool trigger_immediately=false) : period(to_duration(period_)), next(trigger_immediately ? now() : now() + period) {}
    PeriodicTrigger(duration period_, bool trigger_immediately=false) : period(            period_ ), next(trigger_immediately ? now() : now() + period) {}
    PeriodicTrigger() : period(), next() {}
};


} // namespace quickgui

/*
template<class OStream, class Rep, class Ratio>
OStream& operator<< (OStream &s, std::chrono::duration<Rep, Ratio> d)
{
    if constexpr (std::is_same_v<Ratio, std::pico>)
    {
        s << d.count();
        s << "ps";
    }
    else if constexpr (std::is_same_v<Ratio, std::nano>)
    {
        s << d.count();
        s << "ns";
    }
    else if constexpr (std::is_same_v<Ratio, std::micro>)
    {
        s << d.count();
        s << "us";
    }
    else if constexpr (std::is_same_v<Ratio, std::milli>)
    {
        s << d.count();
        s << "ms";
    }
    else if constexpr (std::is_same_v<Ratio, std::ratio<1,1>>)
    {
        s << d.count();
        s << "s";
    }
    else
    {
        s << std::chrono::duration_cast<std::chrono::duration<Rep>>(d).count();
        s << "s";
    }
}
*/

#endif /* QUICKGUI_TIME_HPP_ */
