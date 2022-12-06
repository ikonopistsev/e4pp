#pragma once

#include "e4pp/queue.hpp"
#include "event2/http.h"

namespace e4pp {
namespace http {

using http_handle_type = evhttp*;

using cmd_type = e4pp::detail::ev_mask_flag<evhttp_cmd_type,
    EVHTTP_REQ_GET|EVHTTP_REQ_POST|EVHTTP_REQ_HEAD|EVHTTP_REQ_PUT|
    EVHTTP_REQ_DELETE|EVHTTP_REQ_OPTIONS|EVHTTP_REQ_TRACE|
    EVHTTP_REQ_CONNECT|EVHTTP_REQ_PATCH>;

namespace method {

constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type, EVHTTP_REQ_GET> get{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type, EVHTTP_REQ_POST> post{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type, EVHTTP_REQ_HEAD> head{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type, EVHTTP_REQ_PUT> put{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type, EVHTTP_REQ_DELETE> del{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type,
    EVHTTP_REQ_OPTIONS> options{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type, EVHTTP_REQ_TRACE> trace{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type,
    EVHTTP_REQ_CONNECT> connect{};
constexpr e4pp::detail::ev_flag_tag<evhttp_cmd_type, EVHTTP_REQ_PATCH> patch{};

} // namespace method

class vhost
{
public:
    using handle_type = http_handle_type;

private:
    struct deallocate
    {
        void operator()(handle_type ptr) noexcept
        {
            evhttp_free(ptr); 
        }
    };
    using ptr_type = std::unique_ptr<evhttp, deallocate>;
    ptr_type handle_{};
    vhost* parent_{};

protected:
    handle_type assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

    void set_parent(vhost* parent) noexcept
    {
        parent_ = parent;
    }

public:
    vhost() = default;
    vhost(vhost&&) = default;
    vhost& operator=(vhost&&) = default;

    virtual ~vhost() noexcept
    {
        remove_virtual_host();
    }

    explicit vhost(handle_type handle) noexcept
        : handle_{handle}
    {
        assert(handle);
    }

    explicit vhost(const queue& queue)
        : vhost{e4pp::detail::check_pointer("evhttp_new", 
            evhttp_new(queue))}
    {   }

    vhost(const queue& queue, vhost& parent, const char *pattern)
        : vhost(queue)
    {
        parent.add_virtual_host(*this, pattern);
    }

    void create(const queue& queue)
    {
        handle_.reset(e4pp::detail::check_pointer("evhttp_new", 
            evhttp_new(queue)));
    }

    handle_type handle() const noexcept
    {
        return handle_.get();
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    void destroy() noexcept
    {
        remove_virtual_host();
        handle_.reset();
    }

    void add_virtual_host(vhost& other, const char *pattern)
    {
        e4pp::detail::check_result("evhttp_add_virtual_host",
            evhttp_add_virtual_host(assert_handle(), pattern, other));
        other.set_parent(this);
    }

    void remove_virtual_host() noexcept
    {
        if (parent_) 
        {
            evhttp_remove_virtual_host(parent_->handle(), assert_handle());
            parent_ = nullptr;
        }
    }

    void add_server_alias(const char *alias)
    {
        assert(alias);
        e4pp::detail::check_result("evhttp_add_server_alias",
            evhttp_add_server_alias(assert_handle(), alias));
    }

    void remove_server_alias(const char *alias)
    {
        assert(alias);
        e4pp::detail::check_result("evhttp_add_server_alias",
            evhttp_remove_server_alias(assert_handle(), alias));
    }
};

} // namespace http
} // namespace e4pp 
