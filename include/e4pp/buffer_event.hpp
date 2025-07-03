#pragma once

#include "e4pp/dns.hpp"
#include "e4pp/buffer.hpp"
#include <event2/bufferevent.h>
#include <functional>
#include <mutex>

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

namespace detail {

struct buffer_event_ref_allocator final
{
    constexpr static inline buffer_event_ptr allocate(buffer_event_ptr other) noexcept
    {
        return other;
    }

    constexpr static inline void free(buffer_event_ptr) noexcept
    {   }
};

struct buffer_event_allocator final
{
    static auto allocate(queue_handle_type queue,
        evutil_socket_t fd, bev_flag opt) noexcept
    {
        assert(queue);
        return e4pp::detail::check_pointer("bufferevent_socket_new",
            bufferevent_socket_new(queue, fd, opt));
    }

    static auto allocate(queue_handle_type queue, bev_flag opt) noexcept
    {
        return allocate(queue, -1, opt);
    }

    static auto allocate(buffer_event_ptr handle) noexcept
    {
        assert(handle);
        return handle;
    }

    static void free(buffer_event_ptr ptr) noexcept
    {
        if (ptr) {
            bufferevent_free(ptr);
        }
    }
};

} // detail

// Forward declarations
template<class A>
class basic_buffer_event;

using buffer_event_ref = basic_buffer_event<detail::buffer_event_ref_allocator>;
using buffer_event = basic_buffer_event<detail::buffer_event_allocator>;

template<class A>
class basic_buffer_event final
{
    using this_type = basic_buffer_event<A>;

public:
    using handle_type = buffer_event_ptr;

private:
    struct free_bufferevent
    {
        void operator()(handle_type ptr) noexcept 
        { 
            A::free(ptr);
        }
    };
    
    std::unique_ptr<bufferevent, free_bufferevent> handle_{};

    auto output_handle() const noexcept
    {
        return bufferevent_get_output(assert_handle());
    }

    auto input_handle() const noexcept
    {
        return bufferevent_get_input(assert_handle());
    }

    queue_handle_type queue_handle() const noexcept
    {
        return bufferevent_get_base(assert_handle());
    }

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    basic_buffer_event()
    {
        // Ref types should not create new objects, only capture existing ones  
        static_assert(!std::is_same<this_type, buffer_event_ref>::value, 
                      "buffer_event_ref should not create new objects - use explicit constructor with existing pointer");
    }

    ~basic_buffer_event() = default;

    // Create buffer event
    template<class ... Args>
    explicit basic_buffer_event(Args&&... args) noexcept
        : handle_{A::allocate(std::forward<Args>(args)...)}
    {
        // Ref types should not create new objects, only capture existing ones
        static_assert(!std::is_same<this_type, buffer_event_ref>::value, 
                      "buffer_event_ref should not create new objects - use explicit constructor with existing pointer");
        assert(handle());
    }

    // only for ref
    basic_buffer_event(const basic_buffer_event& other) noexcept
        : handle_{other.handle()}
    {
        // copy only for refs
        static_assert(std::is_same<this_type, buffer_event_ref>::value);
    }

    // only for ref
    basic_buffer_event& operator=(const basic_buffer_event& other) noexcept
    {
        // copy only for refs
        static_assert(std::is_same<this_type, buffer_event_ref>::value);
        handle_.reset(other.handle());
        return *this;
    }

    basic_buffer_event(basic_buffer_event&& that) noexcept
    {
        std::swap(handle_, that.handle_);
    }

    basic_buffer_event& operator=(basic_buffer_event&& that) noexcept
    {
        std::swap(handle_, that.handle_);
        return *this;
    }

    auto handle() const noexcept
    {
        return handle_.get();
    }

    operator buffer_event_ptr() const noexcept
    {
        return handle();
    }

    explicit operator buffer_event_ref() const noexcept
    {
        return ref();
    }

    buffer_event_ref ref() const noexcept
    {
        return buffer_event_ref(handle());
    }

    void create(queue_handle_type queue,
        evutil_socket_t fd, bev_flag opt = bev_close_on_free)
    {
        // Ref types should not create new objects, only capture existing ones
        static_assert(!std::is_same<this_type, buffer_event_ref>::value, 
                      "buffer_event_ref should not create new objects - use explicit constructor with existing pointer");
        handle_.reset(A::allocate(queue, fd, opt));
    }

    void create(queue_handle_type queue, bev_flag opt = bev_close_on_free)
    {
        create(queue, -1, opt);
    }

    operator bool() const noexcept
    {
        return nullptr != handle();
    }

    void close() 
    {
        handle_.reset();
    }

    // Legacy alias for close()
    void destroy() noexcept
    {
        close();
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
        return bufferevent_getfd(assert_handle());
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

    void connect(dns_handle_type dns, int af,
        const std::string& hostname, int port)
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
        std::lock_guard<basic_buffer_event> l(*this);
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

    auto read()
    {
        buffer result;
        detail::check_result("bufferevent_read_buffer",
            bufferevent_read_buffer(assert_handle(), result));
        return result;
    }

    auto input_buffer() noexcept
    {
        return buffer_ref{detail::check_pointer("bufferevent_get_input",
            bufferevent_get_input(assert_handle()))};
    }

    auto output_buffer() noexcept
    {
        return buffer_ref{detail::check_pointer("bufferevent_get_output",
            bufferevent_get_output(assert_handle()))};
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

    // Release ownership of the underlying bufferevent pointer
    auto release() noexcept
    {
        // Only owning types can release ownership, not refs
        static_assert(!std::is_same<this_type, buffer_event_ref>::value, 
            "Cannot release() from buffer_event_ref - only owning buffer_event can transfer ownership");
        return handle_.release();
    }
};

} // namespace e4pp
