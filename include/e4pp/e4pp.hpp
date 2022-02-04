#pragma once

#include "event2/event.h"

#include <memory>
#include <chrono>
#include <stdexcept>

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstring>

namespace e4pp {

using event_flag = short;
using event_handle_type = event*;
using queue_handle_type = event_base*;

template<class Rep, class Period>
timeval make_timeval(std::chrono::duration<Rep, Period> timeout) noexcept
{
    const auto sec = std::chrono::duration_cast<
        std::chrono::seconds>(timeout);
    const auto usec = std::chrono::duration_cast<
        std::chrono::microseconds>(timeout) - sec;

    // FIX: decltype fix for clang macosx
    return {
        static_cast<decltype(timeval::tv_sec)>(sec.count()),
        static_cast<decltype(timeval::tv_usec)>(usec.count())
    };
}

template<class T>
T assert_handle(T handle) noexcept
{
    assert(handle);
    return handle;
}

struct flag 
{   
    int value_{};
    
    operator int() const noexcept 
    {
        return value_;
    }

    constexpr static inline flag timeout(int ef = 0) 
    {
        return { EV_TIMEOUT|ef };
    }

    constexpr static inline flag interval(int ef = 0) 
    {
        return { EV_TIMEOUT|EV_PERSIST|ef };
    }
};

// static inline void startup(unsigned char h = 2, unsigned char l = 2)
// {
//     static const net::launch launch(h, l);
// }

namespace detail {

static inline int check_result(const char* what, int rc)
{
    assert(what);
    if (-1 == rc)
        throw std::runtime_error(what);
    return rc;
}

template<class P>
P* check_pointer(const char* what, P* value)
{
    assert(what);
    if (!value)
        throw std::runtime_error(what);
    return value;
}

template<class T>
std::size_t check_size(const char* what, T result)
{
    assert(what);
    if (static_cast<T>(-1) == result)
        throw std::runtime_error(what);
    return static_cast<std::size_t>(result);
}

} // namespace detail
} // namespace e4pp
