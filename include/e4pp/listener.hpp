#pragma once

#include "e4pp/e4pp.hpp"
#include "e4pp/functional.hpp"
#include "event2/listener.h"
#include <cassert>

namespace e4pp {

using listener_handle_type = evconnlistener*;

using lev_flag = detail::ev_mask_flag<evconnlistener, 
    LEV_OPT_LEAVE_SOCKETS_BLOCKING|LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|
    LEV_OPT_REUSEABLE|LEV_OPT_THREADSAFE|LEV_OPT_DISABLED|
    LEV_OPT_DEFERRED_ACCEPT|LEV_OPT_REUSEABLE_PORT|LEV_OPT_BIND_IPV6ONLY>;

constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_LEAVE_SOCKETS_BLOCKING>
    lev_leave_sockets_blocking{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_CLOSE_ON_FREE>
    lev_close_on_free{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_CLOSE_ON_EXEC>
    lev_close_on_exec{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_REUSEABLE>
    lev_reuseable{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_THREADSAFE>
    lev_threadsafe{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_DISABLED>
    lev_disabled{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_DEFERRED_ACCEPT>
    lev_deferred_accept{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_REUSEABLE_PORT>
    lev_reuseable_port{};
constexpr detail::ev_flag_tag<evconnlistener, LEV_OPT_BIND_IPV6ONLY>
    lev_bind_ipv6only{};
constexpr auto lev_default{lev_close_on_free|lev_reuseable};

class listener final
{
public:
    using handle_type = listener_handle_type;

private:
    struct free_evconnlistener
    {
        void operator()(handle_type ptr) noexcept 
        { 
            evconnlistener_free(ptr); 
        }
    };
    using ptr_type = std::unique_ptr<evconnlistener, free_evconnlistener>;
    ptr_type handle_{};

public:
    listener() = default;
    listener(listener&&) = default;
    listener& operator=(listener&&) = default;

    listener(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        evconnlistener_cb cb, void *arg, int backlog = 1024, lev_flag fl = lev_default)
        : handle_{detail::check_pointer("evconnlistener_new_bind",
            evconnlistener_new_bind(queue, cb, arg,
                fl, backlog, sa, static_cast<int>(salen)))}
    {   }

    template<class F, class P>
    listener(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        std::pair<F, P> p, int backlog = 1024, lev_flag fl = lev_default)
        : listener{queue, sa, salen, p.second, p.first, backlog, fl}
    {   }

    template<class F>
    listener(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        F& fn, int backlog = 1024, lev_flag fl = lev_default)
        : listener{queue, sa, salen, proxy_call(fn), backlog, fl}
    {   }  

    void listen(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        evconnlistener_cb cb, void *arg, int backlog = 1024, lev_flag fl = lev_default)
    {
        handle_.reset(detail::check_pointer("evconnlistener_new_bind",
            evconnlistener_new_bind(queue, cb, arg,
                fl, backlog, sa, static_cast<int>(salen))));
    }

    template<class F, class P>
    void listen(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        std::pair<F, P> p, int backlog = 1024, lev_flag fl = lev_default)
    {   
        listen(queue, sa, salen, p.second, p.first, backlog, fl);
    }

    template<class F>
    void listen(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        F& fn, int backlog = 1024, lev_flag fl = lev_default)
    {   
        listen(queue, sa, salen, proxy_call(fn), backlog, fl);
    }  

    void swap(listener& other) noexcept
    {
        assert(this != &other);
        handle_.swap(other.handle_);
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    handle_type handle() const noexcept
    {
        return handle_.get();
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    void set(evconnlistener_cb cb, void *arg)
    {
        assert(cb);
        evconnlistener_set_cb(assert_handle(handle()), cb, arg);
    }

    void clear()
    {
        evconnlistener_set_cb(assert_handle(handle()), nullptr, nullptr);
    }

    evutil_socket_t fd() const noexcept
    {
        return evconnlistener_get_fd(assert_handle(handle()));
    }

    // хэндл очереди
    queue_handle_type base() const noexcept
    {
        return evconnlistener_get_base(assert_handle(handle()));
    }

    void close()
    {
        handle_.reset(nullptr);
    }

    void enable()
    {
        detail::check_result("evconnlistener_enable", 
            evconnlistener_enable(assert_handle(handle())));
    }

    void disable()
    {
        detail::check_result("evconnlistener_disable", 
            evconnlistener_disable(assert_handle(handle())));
    }
};

} // namespace e4pp
