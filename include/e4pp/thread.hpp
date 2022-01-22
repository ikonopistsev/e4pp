#pragma once

#include "e4pp/e4pp.hpp"

#include "event2/thread.h"

namespace e4pp {

#ifdef _WIN32
static inline void use_threads()
{
    detail::check_result("evthread_use_pthreads",
        evthread_use_windows_threads());
}
#else
static inline void use_threads()
{
    detail::check_result("evthread_use_pthreads",
        evthread_use_pthreads());
}
#endif // _WIN32

} // namespace e4pp

