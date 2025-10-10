#pragma once

#include "e4pp/dns.hpp"
#include "e4pp/queue.hpp"
#include <event2/bufferevent.h>
#include <memory>

namespace e4pp {

using buffer_event_handle_type = bufferevent*;
using buffer_event_ptr = bufferevent*;

using bev_flag = detail::ev_mask_flag<bufferevent, BEV_OPT_CLOSE_ON_FREE|
    BEV_OPT_THREADSAFE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_UNLOCK_CALLBACKS>;

constexpr detail::ev_flag_tag<bufferevent, BEV_OPT_CLOSE_ON_FREE>
    bev_close_on_free{};
constexpr detail::ev_flag_tag<bufferevent, BEV_OPT_THREADSAFE>
    bev_threadsafe{};
constexpr detail::ev_flag_tag<bufferevent, BEV_OPT_DEFER_CALLBACKS>
    bev_defer_callbacks{};
constexpr detail::ev_flag_tag<bufferevent, BEV_OPT_UNLOCK_CALLBACKS>
    bev_unlock_callbacks{};

class bev
{
public:
    using handle_type = buffer_event_ptr;

private:
    struct free_bufferevent
    {
        void operator()(handle_type ptr) noexcept 
        { 
            bufferevent_free(ptr);
        }
    };
    
    std::unique_ptr<bufferevent, free_bufferevent> handle_{};

protected:
    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    // Default constructor - empty bev
    bev() = default;
    
    // Move constructor and assignment
    bev(bev&&) = default;
    bev& operator=(bev&&) = default;
    
    // No copy constructor/assignment (like unique_ptr)
    bev(const bev&) = delete;
    bev& operator=(const bev&) = delete;
    
    // Virtual destructor
    virtual ~bev() 
    {
        shutdown();
    }

    // Create TCP buffer event
    static auto create_tcp(queue_handle_type queue, 
        evutil_socket_t fd = -1, bev_flag opt = bev_close_on_free)
    {
        assert(queue);
        auto ptr = detail::check_pointer("bufferevent_socket_new",
            bufferevent_socket_new(queue, fd, opt));
        return ptr;
    }

    // Constructor from raw handle (takes ownership)
    explicit bev(handle_type ptr) noexcept
        : handle_{ptr}
    {
        assert(ptr);
    }

    // Create TCP buffer event
    bev(queue_handle_type queue, evutil_socket_t fd = -1, 
        bev_flag opt = bev_close_on_free)
        : handle_{create_tcp(queue, fd, opt)}
    {   }    

    // Get raw handle
    handle_type handle() const noexcept
    {
        return handle_.get();
    }

    // Check if valid
    explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    // Reset to empty state
    void reset() noexcept
    {
        handle_.reset();
    }

    // Reset with new handle
    void reset(handle_type ptr) noexcept
    {
        handle_.reset(ptr);
    }

    // Release ownership (like unique_ptr::release)
    handle_type release() noexcept
    {
        return handle_.release();
    }

    // Swap with another bev
    void swap(bev& other) noexcept
    {
        handle_.swap(other.handle_);
    }

    // Virtual shutdown method - override in derived classes
    virtual void shutdown() noexcept
    {
        // Base implementation does nothing
        // Derived classes (like SSL) should override
    }

    auto input_handle() const noexcept
    {
        return bufferevent_get_input(assert_handle());
    }    

    auto output_handle() const noexcept
    {
        return bufferevent_get_output(assert_handle());
    }

    // Buffer operations
    auto input() const noexcept
    {
        return buffer_ref{input_handle()};
    }

    auto output() const noexcept
    {
        return buffer_ref{output_handle()};
    }

    auto queue() const noexcept
    {
        return bufferevent_get_base(assert_handle());
    }

    evutil_socket_t fd() const noexcept
    {
        return bufferevent_getfd(assert_handle());
    }

    // Virtual connection operations
    virtual void connect(const sockaddr* sa, ev_socklen_t len)
    {
        assert(sa);
        detail::check_result("bufferevent_socket_connect",
            bufferevent_socket_connect(assert_handle(),
                const_cast<sockaddr*>(sa), static_cast<int>(len)));
    }

    virtual void connect(dns_handle_type dns, int af,
        const std::string& hostname, int port)
    {
        assert(dns);
        detail::check_result("bufferevent_socket_connect_hostname",
            bufferevent_socket_connect_hostname(assert_handle(), dns,
                af, hostname.c_str(), port));
    }

    virtual void connect(dns_handle_type dns, const std::string& hostname, int port)
    {
        connect(dns, AF_UNSPEC, hostname, port);
    }

    // Event control
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

    // I/O operations
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

    auto read()
    {
        buffer result;
        detail::check_result("bufferevent_read_buffer",
            bufferevent_read_buffer(assert_handle(), result));
        return result;
    }

    // Callbacks
    void set_callbacks(bufferevent_data_cb rdfn, bufferevent_data_cb wrfn,
                      bufferevent_event_cb evfn, void *arg) noexcept
    {
        bufferevent_setcb(assert_handle(), rdfn, wrfn, evfn, arg);
    }

    // Timeouts
    void set_timeouts(timeval *timeout_read, timeval *timeout_write)
    {
        bufferevent_set_timeouts(assert_handle(), timeout_read, timeout_write);
    }

    // Watermarks
    void set_watermark(short events, std::size_t lowmark,
                       std::size_t highmark) noexcept
    {
        bufferevent_setwatermark(assert_handle(), events, lowmark, highmark);
    }

    // Thread safety
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
        std::lock_guard<bev> l(*this);
        fn(*this);
    }

    // DNS error (for socket bufferevents)
    int get_dns_error() const noexcept
    {
        return bufferevent_socket_get_dns_error(assert_handle());
    }

    // Set file descriptor
    void set_fd(evutil_socket_t fd)
    {
        detail::check_result("bufferevent_setfd",
            bufferevent_setfd(assert_handle(), fd));
    }

    // Set event base
    void set_base(queue_handle_type queue)
    {
        assert(queue);
        detail::check_result("bufferevent_base_set",
            bufferevent_base_set(queue, assert_handle()));
    }

    // Get limits
    ev_ssize_t get_max_to_read() const noexcept
    {
        return bufferevent_get_max_to_read(assert_handle());
    }

    ev_ssize_t get_max_to_write() const noexcept
    {
        return bufferevent_get_max_to_write(assert_handle());
    }
};

// Free function for swap (ADL)
inline void swap(bev& a, bev& b) noexcept
{
    a.swap(b);
}

// Comparison operators (like unique_ptr)
inline bool operator==(const bev& a, const bev& b) noexcept
{
    return a.handle() == b.handle();
}

inline bool operator!=(const bev& a, const bev& b) noexcept
{
    return !(a == b);
}

inline bool operator==(const bev& a, std::nullptr_t) noexcept
{
    return !a;
}

inline bool operator==(std::nullptr_t, const bev& a) noexcept
{
    return !a;
}

inline bool operator!=(const bev& a, std::nullptr_t) noexcept
{
    return static_cast<bool>(a);
}

inline bool operator!=(std::nullptr_t, const bev& a) noexcept
{
    return static_cast<bool>(a);
}

} // namespace e4pp