#pragma once

#include "e4pp/uri.hpp"
#include "e4pp/vhost.hpp"
#include "e4pp/query.hpp"
#include <event2/http_struct.h>

namespace e4pp {
namespace http {

using evhttp_flags = e4pp::detail::ev_mask_flag<evhttp,
    EVHTTP_SERVER_LINGERING_CLOSE>;

constexpr e4pp::detail::ev_flag_tag<evhttp, 
    EVHTTP_SERVER_LINGERING_CLOSE> lingering_close{};

class server
    : public vhost
{
public:
    server() = default;
    server(server&&) = default;
    server& operator=(server&&) = default;
    virtual ~server() = default;

    explicit server(handle_type handle) noexcept
        : vhost{handle}
    {   }

    explicit server(const queue& queue)
        : vhost{queue}
    {   }

    void bind_socket(const char *address, ev_uint16_t port)
    {
        assert(address);
        e4pp::detail::check_result("evhttp_bind_socket",
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
        e4pp::detail::check_result("evhttp_set_flags",
            evhttp_set_flags(assert_handle(), f));
    }
};

using request_handle_type = evhttp_request*;

class request
{
public:
    using handle_type = request_handle_type;

private:
    handle_type handle_{};
    evhttp_cmd_type type_{};
    e4pp::uri uri_{};

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    request(handle_type handle)
        : handle_{handle}
    {   }

    request(evhttp_cmd_type type, const std::string& uri)
        : handle_{evhttp_request_new([](auto...){}, nullptr)}
        , type_{type}
        , uri_{uri}
    {   }

    auto handle() const noexcept -> handle_type 
    {
        return handle_;
    }

    void push(const char* key, const char* value)
    {   
        assert(key && value);
        auto kv = keyval_ref{assert_handle()->output_headers};
        kv.push(key, value);
    }

    keyval_ref input_headers() const noexcept
    {
        return keyval_ref{assert_handle()->input_headers};
    }

    keyval_ref output_headers() const noexcept
    {
        return keyval_ref{assert_handle()->output_headers};
    }

    auto type() const noexcept
    {
        return type_;
    }
    
    auto release() noexcept
    {
        return std::exchange(handle_, nullptr);
    }
};

} // namespace http
} // namespace e4pp
