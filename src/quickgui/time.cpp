#include "quickgui/time.hpp"
#include <c4/platform.hpp>

#ifdef C4_UNIX
#include <time.h>
#include <errno.h>
#else
#include <thread>
#endif

namespace quickgui {

time_point start_time = {};

time_point set_start_time(time_point tp)
{
    start_time = tp;
    return start_time;
}

time_point get_start_time()
{
    return start_time;
}

duration uptime()
{
    return now() - start_time;
}

time_point busy_wait_until(time_point tp, bool pause)
{
    time_point curr = now();
    if(pause)
    {
        while(curr < tp)
        {
            pause_spin();
            curr = now();
        }
    }
    else
    {
        while(curr < tp)
        {
            curr = now();
        }
    }
    return curr;
    //
    //time_point start = now();
    //time_point curr = start;
    //duration half_tick = {};
    //size_t num_ticks = 0;
    //while(curr + half_tick < tp)
    //{
    //    curr = now();
    //    half_tick = (curr - start) / (2u * ++num_ticks);
    //}
    //return curr + half_tick;
}

void sleep_until(time_point tp)
{
#ifdef C4_UNIX
    C4_SUPPRESS_WARNING_GCC_WITH_PUSH("-Wuseless-cast")
    time_point curr = now();
    if(tp < curr)
        return;
    auto dt = tp - curr;
	auto s = std::chrono::duration_cast<std::chrono::seconds>(dt);
	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(dt - s);
	struct timespec __ts = {
	    static_cast<std::time_t>(s.count()),
	    static_cast<long>(ns.count())
    };
	while (::nanosleep(&__ts, &__ts) == -1 && errno == EINTR)
        ;
    C4_SUPPRESS_WARNING_GCC_POP
#else
    std::this_thread::sleep_until(tp);
#endif
}

} // namespace quickgui
