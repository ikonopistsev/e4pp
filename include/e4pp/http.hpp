#pragma once

#include "e4pp/vhost.hpp"

namespace e4pp {

using evhttp_flags = detail::ev_mask_flag<evhttp, EVHTTP_SERVER_LINGERING_CLOSE>;
constexpr detail::ev_flag_tag<evhttp, EVHTTP_SERVER_LINGERING_CLOSE> lingering_close{};

class server
    : public vhost
{
public:
    server() = default;
    server(server&&) = default;
    server& operator=(server&&) = default;
    virtual ~server() = default;

    explicit server(handle_type handle) noexcept
        : vhost(handle)
    {   }

    explicit server(const queue& queue)
        : vhost(queue)
    {   }

    void bind_socket(const char *address, ev_uint16_t port)
    {
        assert(address);
        detail::check_result("evhttp_bind_socket",
            evhttp_bind_socket(assert_handle(), address, port));
    }

    void set_timeout(int timeout_in_secs) noexcept
    {
        evhttp_set_timeout(assert_handle(), timeout_in_secs);
    }

    void set_default_content_type(const char *content_type) noexcept
    {
        assert(content_type);
        evhttp_set_default_content_type(assert_handle(), content_type);
    }

    void set_max_headers_size(ev_ssize_t max_headers_size)
    {
        evhttp_set_max_headers_size(assert_handle(), max_headers_size);
    }

    void set_max_body_size(ev_ssize_t max_body_size)
    {
        evhttp_set_max_body_size(assert_handle(), max_body_size);
    }

    void set_allowed_methods(http::cmd_type methods)
    {
        evhttp_set_allowed_methods(assert_handle(), 
            static_cast<uint16_t>(methods));
    }

    void set_timeout(const timeval& tv)
    {
        evhttp_set_timeout_tv(assert_handle(), &tv);
    }

    template<class Rep, class Period>
    void set_timeout(std::chrono::duration<Rep, Period> timeout)
    {
        set_timeout(make_timeval(timeout));
    }

    void set_flags(evhttp_flags f)
    {
        detail::check_result("evhttp_set_flags",
            evhttp_set_flags(assert_handle(), f));
    }
};

using request_handle_type = evhttp_request*;

namespace detail {

struct req_ref_allocator final
{
    constexpr static inline request_handle_type allocate() noexcept
    {
        return nullptr;
    }

    constexpr static inline void free(request_handle_type) noexcept
    {   }
};

struct req_allocator final
{
    static auto allocate()
    {
        return detail::check_pointer("evhttp_request_new",
            evhttp_request_new(nullptr, nullptr));
    }

    static void free(request_handle_type ptr) noexcept
    {
        evhttp_request_free(ptr);
    }
};

} // detail

template<class A>
class basic_request;

using request_ref = basic_buffer<detail::req_ref_allocator>;
using request = basic_buffer<detail::req_allocator>;

template<class A>
class basic_request
{
    using this_type = basic_request<A>;

private:
    request_handle_type handle_{A::allocate()};

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    auto handle() const noexcept
    {
        return handle_;
    }

    operator evbufer_ptr() const noexcept
    {
        return handle();
    }

    operator buffer_ref() const noexcept
    {
        return ref();
    }

    request_ref ref() const noexcept
    {
        return request_ref(handle());
    }

    basic_request() = default;

    ~basic_request() noexcept
    {
        A::free(handle());
    }

    // only for ref
    // this delete copy ctor for buffer&
    basic_request(const basic_request& other) noexcept
        : handle_{other.handle()}
    {
        // copy only for refs
        static_assert(std::is_same<this_type, request_ref>::value);
    }

    // only for ref
    // this delete copy ctor for buffer&
    basic_request& operator=(const basic_request& other) noexcept
    {
        // copy only for refs
        static_assert(std::is_same<this_type, request_ref>::value);
        handle_ = other.handle();
        return *this;
    }

    basic_request(basic_request&& that) noexcept
    {
        std::swap(handle_, that.handle_);
    }

    basic_request& operator=(basic_request&& that) noexcept
    {
        std::swap(handle_, that.handle_);
        return *this;
    }

    explicit basic_request(request_handle_type ptr) noexcept
        : handle_{ptr}
    {
        static_assert(std::is_same<this_type, request_ref>::value);
        assert(ptr);
    }
};

} // namespace e4pp
