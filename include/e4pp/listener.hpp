#pragma once

#include "e4pp/e4pp.hpp"
#include "e4pp/functional.hpp"
#include "event2/listener.h"
#include <cassert>

namespace e4pp {

using listener_handle_type = evconnlistener*;

class listener final
{
public:
    using handle_type = listener_handle_type;

private:
    struct deallocate 
    {
        void operator()(handle_type ptr) noexcept 
        { 
            evconnlistener_free(ptr); 
        };
    };
    using ptr_type = std::unique_ptr<evconnlistener, deallocate>;
    ptr_type handle_{};

public:
    constexpr static auto lev_opt = int{
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE
    };

    listener() = default;
    listener(listener&&) = default;
    listener& operator=(listener&&) = default;

    listener(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        evconnlistener_cb cb, void *arg, int backlog = 1024, flag fl = { lev_opt })
        : handle_{detail::check_pointer("evconnlistener_new_bind",
            evconnlistener_new_bind(queue, cb, arg,
                fl, backlog, sa, static_cast<int>(salen)))}
    {   }

    template<class F, class P>
    listener(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        std::pair<F, P> p, int backlog = 1024, flag fl = { lev_opt })
        : listener{queue, sa, salen, p.second, p.first, backlog, fl}
    {   }

    template<class F>
    listener(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        F& fn, int backlog = 1024, flag fl = { lev_opt })
        : listener{queue, sa, salen, proxy_call(fn), backlog, fl}
    {   }  

    void listen(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        evconnlistener_cb cb, void *arg, int backlog = 1024, flag fl = { lev_opt })
    {
        handle_.reset(detail::check_pointer("evconnlistener_new_bind",
            evconnlistener_new_bind(queue, cb, arg,
                fl, backlog, sa, static_cast<int>(salen))));
    }

    template<class F, class P>
    void listen(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        std::pair<F, P> p, int backlog = 1024, flag fl = { lev_opt })
    {   
        listen(queue, sa, salen, p.second, p.first, backlog, fl);
    }

    template<class F>
    void listen(queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        F& fn, int backlog = 1024, flag fl = { lev_opt })
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
        auto rc = evconnlistener_enable(assert_handle(handle()));
        if (-1 == rc)
            throw std::runtime_error("evconnlistener_enable");
    }

    void disable()
    {
        auto rc = evconnlistener_disable(assert_handle(handle()));
        if (-1 == rc)
            throw std::runtime_error("evconnlistener_disable");
    }
};

} // namespace e4pp
