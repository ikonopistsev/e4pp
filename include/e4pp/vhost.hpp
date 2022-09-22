#pragma once

#include "e4pp/queue.hpp"
#include "event2/http.h"

namespace e4pp {

using http_handle_type = evhttp*;

class vhost
{
public:
    using handle_type = http_handle_type;

private:
    struct deallocate final
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
        : vhost{detail::check_pointer("evhttp_new", 
            evhttp_new(queue))}
    {   }

    vhost(const queue& queue, vhost& parent, const char *pattern)
        : vhost(queue)
    {
        parent.add_virtual_host(*this, pattern);
    }

    void create(const queue& queue)
    {
        handle_.reset(detail::check_pointer("evhttp_new", 
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
        detail::check_result("evhttp_add_virtual_host",
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
        detail::check_result("evhttp_add_server_alias",
            evhttp_add_server_alias(assert_handle(), alias));
    }

    void remove_server_alias(const char *alias)
    {
        assert(alias);
        detail::check_result("evhttp_add_server_alias",
            evhttp_remove_server_alias(assert_handle(), alias));
    }
};

} // namespace e4pp 