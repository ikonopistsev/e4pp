#pragma once

#include "e4pp/vhost.hpp"

namespace e4pp {

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
};

using request_handle_type = evhttp_request*;

namespace detail {

struct req_ref_allocator
{
    constexpr static inline request_handle_type allocate() noexcept
    {
        return nullptr;
    }

    constexpr static inline void free(request_handle_type) noexcept
    {   }
};

struct req_allocator
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
