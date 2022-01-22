#pragma once

#include "e4pp/e4pp.hpp"
#include <functional>

namespace e4pp {

template<class T>
struct timer_fn
{
    using fn_type = void (T::*)();
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void call() noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)();
        } 
        catch (...)
        {   }
    }
};

template<class T>
struct generic_fn
{
    using fn_type = void (T::*)(evutil_socket_t fd, event_flag ef);
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void call(evutil_socket_t fd, event_flag ef) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(fd, ef);
        } 
        catch (...)
        {   }
    }
};

template<class T>
constexpr auto proxy_call(timer_fn<T>& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t, event_flag, void *arg){
            assert(arg);
            try {
                static_cast<timer_fn<T>*>(arg)->call();
            }
            catch (...)
            {   }
        });
}

template<class T>
constexpr auto proxy_call(generic_fn<T>& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t fd, event_flag ef, void *arg){
            assert(arg);
            try {
                static_cast<generic_fn<T>*>(arg)->call(fd, ef);
            }
            catch (...)
            {   }
        });
}

using timer_fun = std::function<void()>;
using generic_fun = 
    std::function<void(evutil_socket_t fd, event_flag ef)>;

constexpr static inline auto proxy_call(timer_fun& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t, event_flag, void *arg){
            assert(arg);
            auto fn = static_cast<timer_fun*>(arg);
            try {
                (*fn)();
            }
            catch (...)
            {   }
        });
}

static inline auto proxy_call(timer_fun&& fn)
{
    return std::make_pair(new timer_fun{std::move(fn)},
        [](evutil_socket_t, event_flag, void *arg){
            assert(arg);
            auto fn = static_cast<timer_fun*>(arg);
            try {
                (*fn)();
            }
            catch (...)
            {   }
            delete fn;
        });
}
 
constexpr static inline auto proxy_call(generic_fun& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t fd, event_flag ef, void *arg){
            assert(arg);
            auto fn = static_cast<generic_fun*>(arg);
            try {
                (*fn)(fd, ef);
            }
            catch (...)
            {   }
        });
}

static inline auto proxy_call(generic_fun&& fn)
{
    return std::make_pair(new generic_fun{std::move(fn)},
        [](evutil_socket_t fd, event_flag ef, void *arg){
            assert(arg);
            auto fn = static_cast<generic_fun*>(arg);
            try {
                (*fn)(fd, ef);
            }
            catch (...)
            {   }
            delete fn;
        });
}

} // namespace e4pp