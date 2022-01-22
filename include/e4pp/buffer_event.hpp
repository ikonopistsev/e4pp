#pragma once

#include "e4pp/dns.hpp"
#include "e4pp/buffer.hpp"

#include "event2/bufferevent.h"

#include <functional>
#include <mutex>

namespace e4pp {

using buffer_event_handle_type = bufferevent*;

class buffer_event
{
public:
    using handle_type = buffer_event_handle_type;

private:

    handle_type hbev_{ nullptr };

    handle_type assert_handle() const noexcept
    {
        auto hbev = handle();
        assert(hbev);
        return hbev;
    }

    auto output_handle() const noexcept
    {
        return bufferevent_get_output(assert_handle());
    }

    auto input_handle() const noexcept
    {
        return bufferevent_get_input(assert_handle());
    }

    // хэндл очереди
    queue_handle_type queue_handle() const noexcept
    {
        return bufferevent_get_base(assert_handle());
    }

public:
    buffer_event() = default;

    buffer_event(const buffer_event&) = delete;
    buffer_event& operator=(const buffer_event&) = delete;

    buffer_event(buffer_event&& other) noexcept
    {
        std::swap(hbev_, other.hbev_);
    }

    buffer_event& operator=(buffer_event&& other) noexcept
    {
        std::swap(hbev_, other.hbev_);
        return *this;
    }

    buffer_event(handle_type hbev) noexcept
        : hbev_(hbev)
    {
        assert(hbev);
    }

    ~buffer_event() noexcept
    {
        auto hbev = handle();
        if (nullptr != hbev)
            bufferevent_free(hbev);
    }

    void attach(handle_type hbev) noexcept
    {
        hbev_ = hbev;
    }

    void create(queue_handle_type queue,
        evutil_socket_t fd, int opt = BEV_OPT_CLOSE_ON_FREE)
    {
        assert(queue);
        hbev_ = detail::check_pointer("bufferevent_socket_new",
            bufferevent_socket_new(queue, fd, opt));
    }

    void create(queue_handle_type queue, int opt = BEV_OPT_CLOSE_ON_FREE)
    {
        create(queue, -1, opt);
    }

    void destroy() noexcept
    {
        auto hbev = handle();
        if (nullptr != hbev)
        {
            bufferevent_free(hbev);
            hbev_ = nullptr;
        }
    }

    auto input() const noexcept
    {
        return buffer_ref(input_handle());
    }

    auto output() const noexcept
    {
        return buffer_ref(output_handle());
    }

    auto queue() const noexcept
    {
        return queue_handle();
    }

    int get_dns_error() const noexcept
    {
        return bufferevent_socket_get_dns_error(assert_handle());
    }

    void connect(const sockaddr* sa, ev_socklen_t len)
    {
        assert(sa);
        detail::check_result("bufferevent_socket_connect",
            bufferevent_socket_connect(assert_handle(),
                const_cast<sockaddr*>(sa), static_cast<int>(len)));
    }

    void connect(dns_handle_type dns, int af, const std::string& hostname, int port)
    {
        assert(dns);
        detail::check_result("bufferevent_socket_connect_hostname",
            bufferevent_socket_connect_hostname(assert_handle(), dns,
                af, hostname.c_str(), port));
    }

    void connect(dns_handle_type dns, const std::string& hostname, int port)
    {
        connect(dns, AF_UNSPEC, hostname, port);
    }

    void connect4(dns_handle_type dns, const std::string& hostname, int port)
    {
        connect(dns, AF_INET, hostname, port);
    }

    void connect6(dns_handle_type dns, const std::string& hostname, int port)
    {
        connect(dns, AF_INET6, hostname, port);
    }

    handle_type handle() const noexcept
    {
        return hbev_;
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    evutil_socket_t fd() const noexcept
    {
        return bufferevent_getfd(assert_handle());
    }

    void enable(short event)
    {
        detail::check_result("bufferevent_enable",
            bufferevent_enable(assert_handle(), event));
    }

    void disable(short event)
    {
        detail::check_result("bufferevent_disable",
            bufferevent_disable(assert_handle(), event));
    }

    void set_watermark(short events, std::size_t lowmark,
                       std::size_t highmark) noexcept
    {
        bufferevent_setwatermark(assert_handle(), events, lowmark, highmark);
    }

    ev_ssize_t get_max_to_read() const noexcept
    {
        return bufferevent_get_max_to_read(assert_handle());
    }

    ev_ssize_t get_max_to_write() const noexcept
    {
        return bufferevent_get_max_to_write(assert_handle());
    }

    void lock() const noexcept
    {
        bufferevent_lock(assert_handle());
    }

    void unlock() const noexcept
    {
        bufferevent_unlock(assert_handle());
    }

    template<typename F>
    void sync(F fn)
    {
        std::lock_guard<buffer_event> l(*this);
        fn(*this);
    }

    void write(const void *data, std::size_t size)
    {
        detail::check_result("bufferevent_write",
            bufferevent_write(assert_handle(), data, size));
    }

    template<class Ref>
    void write(basic_buffer<Ref> buf)
    {
        detail::check_result("bufferevent_write_buffer",
            bufferevent_write_buffer(assert_handle(), buf));
    }

    buffer read()
    {
        buffer result;
        detail::check_result("bufferevent_read_buffer",
            bufferevent_read_buffer(assert_handle(), result));
        return result;
    }

    // assign a bufferevent to a specific event_base.
    // NOTE that only socket bufferevents support this function.
    void set(queue_handle_type queue)
    {
        assert(queue);
        detail::check_result("bufferevent_base_set",
            bufferevent_base_set(queue, assert_handle()));
    }

    void set(evutil_socket_t fd)
    {
        detail::check_result("bufferevent_setfd",
            bufferevent_setfd(assert_handle(), fd));
    }

    void set(bufferevent_data_cb rdfn, bufferevent_data_cb wrfn,
        bufferevent_event_cb evfn, void *arg) noexcept
    {
        bufferevent_setcb(assert_handle(), rdfn, wrfn, evfn, arg);
    }

    void set_timeout(timeval *timeout_read, timeval *timeout_write)
    {
        bufferevent_set_timeouts(assert_handle(), timeout_read, timeout_write);
    }
};

} // namespace e4pp
