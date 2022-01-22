#pragma once

#include "e4pp/e4pp.hpp"
#include "event2/listener.h"
#include <cassert>

namespace e4pp {

using listener_handle_type = evconnlistener*;

class listener
{
public:
    using handle_type = listener_handle_type;

private:
    std::unique_ptr<evconnlistener, decltype(&evconnlistener_free)>
        handle_{ nullptr, evconnlistener_free };

    handle_type assert_handle() const noexcept
    {
        auto h = handle_.get();
        assert(h);
        return h;
    }

    constexpr static auto lev_opt = unsigned{
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE
    };

public:
    listener() = default;

    listener(int backlog, unsigned int flags,
        queue_handle_type queue, const sockaddr *sa, ev_socklen_t salen,
        evconnlistener_cb cb, void *arg)
        : handle_{ detail::check_pointer("evconnlistener_new_bind",
            evconnlistener_new_bind(queue, cb, arg,
                flags, backlog, sa, static_cast<int>(salen))), evconnlistener_free }
    {   }

    listener(listener&) = delete;
    listener& operator=(listener&) = delete;

    listener(listener&& other)
    {
        swap(other);
    }

    listener& operator=(listener&& other)
    {
        swap(other);
        return *this;
    }

    void listen(queue_handle_type queue, unsigned int flags, int backlog,
        const sockaddr *sa, ev_socklen_t salen, evconnlistener_cb cb, void *arg)
    {
        handle_.reset(detail::check_pointer("evconnlistener_new_bind",
            evconnlistener_new_bind(queue, cb, arg,
                flags, backlog, sa, static_cast<int>(salen))));
    }

    void swap(listener& other) noexcept
    {
        assert(this != &other);
        handle_.swap(other.handle_);
    }

    bool empty() const noexcept
    {
        return nullptr == handle_.get();
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
        evconnlistener_set_cb(assert_handle(), cb, arg);
    }

    void clear()
    {
        evconnlistener_set_cb(assert_handle(), nullptr, nullptr);
    }

    evutil_socket_t fd() const noexcept
    {
        return evconnlistener_get_fd(assert_handle());
    }

    // хэндл очереди
    queue_handle_type base() const noexcept
    {
        return evconnlistener_get_base(assert_handle());
    }

    void close()
    {
        handle_.reset(nullptr);
    }

    void enable()
    {
        auto rc = evconnlistener_enable(assert_handle());
        if (-1 == rc)
            throw std::runtime_error("evconnlistener_enable");
    }

    void disable()
    {
        auto rc = evconnlistener_disable(assert_handle());
        if (-1 == rc)
            throw std::runtime_error("evconnlistener_disable");
    }
};

} // namespace e4pp
