#include "quickgui/log.hpp"
#include <c4/format.hpp>
#include <cstdio>
#include "quickgui/time.hpp"

namespace quickgui {
namespace detail {

char _logbuf_[512] = {};
c4::substr _logbuf = _logbuf_;
#ifdef NDEBUG // release
LogFlags_f _logflags = kLogEnabled /*| kLogFileLine *//* | kLogTimeStamp *//* | kLogFlush */;
#else
LogFlags_f _logflags = kLogEnabled /*| kLogFileLine *//* | kLogTimeStamp */   | kLogFlush;
#endif


void _logdumper(c4::csubstr s)
{
    std::fwrite(s.str, 1, s.len, stdout);
    if(_logflags & kLogFlush)
        fflush(stdout);
}

void _logfileline(const char *file, int line)
{
    if(_logflags & kLogFileLine)
        _logf("{}:{}: ", c4::to_csubstr(file), line);
}

void _logtimestamp()
{
    if(_logflags & kLogTimeStamp)
    {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(uptime());
        _logf("{}us: ", c4::fmt::zpad(us.count(), 9));
    }
}

} // namespace detail

void logflags(LogFlags_f f)
{
    detail::_logflags = f;
}
LogFlags_f logflags()
{
    return detail::_logflags;
}

} // namespace quickgui
