#pragma once

#include "e4pp/bev.hpp"
#include "e4pp/buffer.hpp"
#include <functional>
#include <string>
#include <mutex>

namespace e4pp {
namespace detail {

struct buffer_event_base
{
    virtual buffer_event_ptr release() noexcept = 0;
    virtual buffer_event_ptr handle() const noexcept = 0;
    virtual ~buffer_event_base() = default;

    auto output_handle() const noexcept
    {
        return bufferevent_get_output(handle());
    }

    auto input_handle() const noexcept
    {
        return bufferevent_get_input(handle());
    }

    queue_handle_type queue_handle() const noexcept
    {
        return bufferevent_get_base(handle());
    }

    operator buffer_event_ptr() const noexcept
    {
        return handle();
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

    evutil_socket_t fd() const noexcept
    {
        return bufferevent_getfd(handle());
    }

    int get_dns_error() const noexcept
    {
        return bufferevent_socket_get_dns_error(handle());
    }

    void connect(const sockaddr* sa, ev_socklen_t len)
    {
        assert(sa);
        detail::check_result("bufferevent_socket_connect",
            bufferevent_socket_connect(handle(),
                const_cast<sockaddr*>(sa), static_cast<int>(len)));
    }

    void connect(dns_handle_type dns, int af,
        const std::string& hostname, int port)
    {
        assert(dns);
        detail::check_result("bufferevent_socket_connect_hostname",
            bufferevent_socket_connect_hostname(handle(), dns,
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

    void enable(short event)
    {
        detail::check_result("bufferevent_enable",
            bufferevent_enable(handle(), event));
    }

    void disable(short event)
    {
        detail::check_result("bufferevent_disable",
            bufferevent_disable(handle(), event));
    }

    void set_watermark(short events, std::size_t lowmark,
                       std::size_t highmark) noexcept
    {
        bufferevent_setwatermark(handle(), events, lowmark, highmark);
    }

    ev_ssize_t get_max_to_read() const noexcept
    {
        return bufferevent_get_max_to_read(handle());
    }

    ev_ssize_t get_max_to_write() const noexcept
    {
        return bufferevent_get_max_to_write(handle());
    }

    void lock() noexcept
    {
        bufferevent_lock(handle());
    }

    void unlock() noexcept
    {
        bufferevent_unlock(handle());
    }

    template<typename F>
    void sync(F fn)
    {
        std::lock_guard<buffer_event_base> l(*this);
        fn(*this);
    }

    void write(const void *data, std::size_t size)
    {
        detail::check_result("bufferevent_write",
            bufferevent_write(handle(), data, size));
    }

    template<class Ref>
    void write(basic_buffer<Ref> buf)
    {
        detail::check_result("bufferevent_write_buffer",
            bufferevent_write_buffer(handle(), buf));
    }

    auto read()
    {
        buffer result;
        detail::check_result("bufferevent_read_buffer",
            bufferevent_read_buffer(handle(), result));
        return result;
    }

    auto input_buffer() noexcept
    {
        return buffer_ref{detail::check_pointer("bufferevent_get_input",
            bufferevent_get_input(handle()))};
    }

    auto output_buffer() noexcept
    {
        return buffer_ref{detail::check_pointer("bufferevent_get_output",
            bufferevent_get_output(handle()))};
    }

    // assign a bufferevent to a specific event_base.
    // NOTE that only socket bufferevents support this function.
    void set(queue_handle_type queue)
    {
        assert(queue);
        detail::check_result("bufferevent_base_set",
            bufferevent_base_set(queue, handle()));
    }

    void set(evutil_socket_t fd)
    {
        detail::check_result("bufferevent_setfd",
            bufferevent_setfd(handle(), fd));
    }

    void set(bufferevent_data_cb rdfn, bufferevent_data_cb wrfn,
        bufferevent_event_cb evfn, void *arg) noexcept
    {
        bufferevent_setcb(handle(), rdfn, wrfn, evfn, arg);
    }

    void set_timeout(timeval *timeout_read, timeval *timeout_write)
    {
        bufferevent_set_timeouts(handle(), timeout_read, timeout_write);
    }    
};

} // namespace detail

// Forward declarations
template<class T>
class basic_buffer_event;

using buffer_event_ref = basic_buffer_event<buffer_event_ptr>;
using buffer_event = basic_buffer_event<bev>;

// Non-owning buffer event specialization (reference to existing bufferevent)
template<>
class basic_buffer_event<buffer_event_ptr> final
    : public detail::buffer_event_base
{
    buffer_event_ptr handle_;
    using this_type = basic_buffer_event<buffer_event_ptr>; 

public:
    // Default constructor (null reference)
    basic_buffer_event() noexcept
        : handle_(nullptr)
    {
    }
    
    // Constructor from raw pointer
    explicit basic_buffer_event(buffer_event_ptr ptr) noexcept
        : handle_(ptr)
    {
        assert(ptr);
    }
    
    // Copy constructor and assignment (ref types can be copied)
    basic_buffer_event(const basic_buffer_event& other) noexcept
        : handle_(other.handle_)
    {
    }
    
    basic_buffer_event& operator=(const basic_buffer_event& other) noexcept
    {
        handle_ = other.handle_;
        return *this;
    }
    
    // Move constructor and assignment
    basic_buffer_event(basic_buffer_event&& other) noexcept
        : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }
    
    basic_buffer_event& operator=(basic_buffer_event&& other) noexcept
    {
        handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }
    
    // Reset to different pointer
    void reset(buffer_event_ptr ptr = nullptr) noexcept
    {
        handle_ = ptr;
    }
    
    // buffer_event_base interface
    buffer_event_ptr handle() const noexcept override
    {
        return handle_;
    }
    
    // Check if valid
    explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }
    
    // Get raw pointer
    buffer_event_ptr get() const noexcept
    {
        return handle_;
    }
    
    // Swap
    void swap(basic_buffer_event& other) noexcept
    {
        std::swap(handle_, other.handle_);
    }

    buffer_event_ptr release() noexcept override
    {
        return std::exchange(handle_, nullptr);
    }
};

// Generic template for bev-derived types
template<class T>
class basic_buffer_event final
    : public detail::buffer_event_base
{
    static_assert(std::is_base_of_v<bev, T>);
    using bev_type = T;

    bev_type bev_;
    using this_type = basic_buffer_event<T>; 

public:
    // Default constructor
    basic_buffer_event() = default;
    
    // Move constructor and assignment
    basic_buffer_event(basic_buffer_event&&) = default;
    basic_buffer_event& operator=(basic_buffer_event&&) = default;
    
    // No copy constructor/assignment (owning type)
    basic_buffer_event(const basic_buffer_event&) = delete;
    basic_buffer_event& operator=(const basic_buffer_event&) = delete;
    
    // Constructor from T (takes ownership)
    explicit basic_buffer_event(T&& h) noexcept
        : bev_(std::move(h))
    {   }
    
    explicit basic_buffer_event(queue_handle_type queue, 
        evutil_socket_t fd = -1, bev_flag opt = bev_close_on_free)
        : bev_{queue, fd, opt}
    {   }
    
    // Create from existing T
    void create(T&& h) noexcept
    {
        bev_ = std::move(h);
    }
    
    void create(queue_handle_type queue,
        evutil_socket_t fd = -1, bev_flag opt = bev_close_on_free)
    {
        bev_ = bev{queue, fd, opt};
    }
    
    // buffer_event_base interface
    buffer_event_ptr handle() const noexcept override
    {
        return bev_.handle();
    }
    
    // Check if valid
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(bev_);
    }
    
    // Close/reset
    void close() noexcept
    {
        bev_.reset();
    }
    
    // Legacy alias for close()
    void destroy() noexcept
    {
        close();
    }
    
    // Convert to ref
    explicit operator buffer_event_ref() const noexcept
    {
        return ref();
    }
    
    buffer_event_ref ref() const noexcept
    {
        return buffer_event_ref{handle()};
    }
    
    // Release ownership
    buffer_event_ptr release() noexcept override
    {
        return bev_.release();
    }
    
    // Swap
    void swap(basic_buffer_event& other) noexcept
    {
        bev_.swap(other.bev_);
    }
};

// Free functions for swap (ADL)
template<class T>
inline void swap(basic_buffer_event<T>& a, 
    basic_buffer_event<T>& b) noexcept
{
    a.swap(b);
}

static inline void swap(basic_buffer_event<buffer_event_ptr>& a, 
    basic_buffer_event<buffer_event_ptr>& b) noexcept
{
    a.swap(b);
}

} // namespace e4pp
