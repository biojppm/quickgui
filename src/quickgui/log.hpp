#ifndef QUICKGUI_LOG_HPP_
#define QUICKGUI_LOG_HPP_

#include <c4/dump.hpp>
#include <c4/charconv.hpp>

namespace quickgui {

using LogFlags_f = int32_t;
typedef enum : LogFlags_f {
    kLogEnabled = 1,
    kLogFileLine = 1 << 1,
    kLogTimeStamp = 1 << 2,
    kLogFlush = 1 << 3,
} LogFlags_e;

//! get the current log flags
LogFlags_f logflags();

//! set the current log flags
void logflags(LogFlags_f f);

namespace detail {
extern LogFlags_f _logflags;
extern c4::substr _logbuf;
void _logdumper(c4::csubstr s);
void _logfileline(const char* file, int line);
void _logtimestamp();
template <class... Args>
void _logf(c4::csubstr fmt, Args const& C4_RESTRICT... args)
{
    size_t needed_size = c4::format_dump<&_logdumper>(_logbuf, fmt, args...);
    // bufsize will be that of the largest element serialized to chars.
    // Eg int(1) will require 1 byte, uint8_t(255) will require three bytes.
    C4_CHECK(needed_size <= _logbuf.len);
}
template <class... Args>
C4_NO_INLINE void _logf_full(const char* file, int line, c4::csubstr fmt, Args const& C4_RESTRICT... args)
{
    _logfileline(file, line);
    _logtimestamp();
    _logf(fmt, args...);
    _logdumper("\n");
}
} // namespace detail


#ifdef QUICKGUI_LOG_DISABLED
    #define QUICKGUI_LOGF_(...)
    #define QUICKGUI_LOGF_IF_(enabled, ...)
    #define QUICKGUI_LOGF(...)
    #define QUICKGUI_LOGF_IF(enabled, ...)
#else
    #define QUICKGUI_LOGF_(...)                      \
    {                                           \
        if(::quickgui::detail::_logflags & ::quickgui::kLogEnabled)  \
            ::quickgui::detail::_logf(__VA_ARGS__);              \
    }
    #define QUICKGUI_LOGF_IF_(enabled, ...)          \
    {                                           \
        if((::quickgui::detail::_logflags & ::quickgui::kLogEnabled) && (enabled))  \
            ::quickgui::detail::_logf(__VA_ARGS__);              \
    }
    #define QUICKGUI_LOGF(...)                                       \
    {                                                           \
        if(::quickgui::detail::_logflags & ::quickgui::kLogEnabled)      \
            ::quickgui::detail::_logf_full(__FILE__, __LINE__, __VA_ARGS__); \
    }
    #define QUICKGUI_LOGF_IF(enabled, ...)                           \
    {                                                           \
        if((::quickgui::detail::_logflags & ::quickgui::kLogEnabled) && (enabled))  \
            ::quickgui::detail::_logf_full(__FILE__, __LINE__, __VA_ARGS__); \
    }
#endif


} // namespace quickgui

#endif /* QUICKGUI_LOG_HPP_ */
