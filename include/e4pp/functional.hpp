#pragma once

#include "e4pp/buffer.hpp"
#include <functional>

struct evconnlistener;

namespace e4pp {

template<class T>
struct timer_fn final
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
struct generic_fn final
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
struct acceptor_fn final
{
    using fn_type = void (T::*)(evutil_socket_t, sockaddr*, int socklen);
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void call(evutil_socket_t fd, sockaddr* sockaddr, int socklen) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(fd, sockaddr, socklen);
        } 
        catch (...)
        {   }
    }
};

template<class T>
struct event_fn
{
    using fn_type = void (T::*)(short what);
    using data_fn_type = void (T::*)(buffer_ref, short);
    using self_type = T;

    fn_type fn_{};
    data_fn_type data_fn_{};
    T& self_;

    void call(short what) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(what);
        } 
        catch (...)
        {   }
    }

    void read(buffer_ref buf, short what) noexcept
    {
        assert(data_fn_);
        try {
            (self_.*data_fn_)(std::move(buf), what);
        } 
        catch (...)
        {   }
    }
};

template<class T>
auto proxy_call(timer_fn<T>& fn)
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
auto proxy_call(generic_fn<T>& fn)
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

template<class T>
auto proxy_call(event_fn<T>& fn)
{
    return std::make_tuple(&fn,
        [](struct bufferevent*, short what, void *arg){
            assert(arg);
            try {
                static_cast<event_fn<T>*>(arg)->call(what);
            }
            catch (...)
            {   }
        },
        [](struct bufferevent* bev, void *arg){
            assert(arg);
            try {
                auto f = bufferevent_get_input(bev);
                static_cast<event_fn<T>*>(arg)->read(buffer_ref(f), EV_READ);
            }
            catch (...)
            {   }
        },
        [](struct bufferevent*, void *arg){
            assert(arg);
            try {
                static_cast<event_fn<T>*>(arg)->call(EV_WRITE);
            }
            catch (...)
            {   }
        });
}

template<class T>
auto proxy_call_accept(acceptor_fn<T>& fn)
{
    return std::make_pair(&fn,
        [](evconnlistener*, evutil_socket_t fd, 
            sockaddr* sockaddr, int socklen, void* arg){
            assert(arg);
            try {
                static_cast<acceptor_fn<T>*>(arg)->call(fd, sockaddr, socklen);
            }
            catch (...)
            {   }
        });
}

using timer_fun = std::function<void()>;
using generic_fun = 
    std::function<void(evutil_socket_t fd, event_flag ef)>;
using acceptor_fun = std::function<void(evutil_socket_t, sockaddr*, int)>;

inline auto proxy_call(timer_fun& fn)
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

inline auto proxy_call(timer_fun&& fn)
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
 
inline auto proxy_call(generic_fun& fn)
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

inline auto proxy_call(generic_fun&& fn)
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

template<class F>
auto proxy_call_accept(F& fn)
{
    return std::make_pair(&fn,
        [](evconnlistener*, evutil_socket_t fd, 
            sockaddr* sockaddr, int socklen, void* arg){
            assert(arg);
            auto fn = static_cast<F*>(arg);
            try {
                (*fn)(fd, sockaddr, socklen);
            }
            catch (...)
            {   }
        });
}


} // namespace e4pp