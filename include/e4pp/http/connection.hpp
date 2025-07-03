#pragma once

#include "e4pp/vhost.hpp"
#include "e4pp/buffer_event.hpp"
#include "e4pp/http/request.hpp"
#include <event2/http_struct.h>

namespace e4pp {
namespace http {

using connection_ptr = evhttp_connection*;

using connection_flags = e4pp::detail::ev_mask_flag<evhttp_connection, EVHTTP_CON_REUSE_CONNECTED_ADDR|
    EVHTTP_CON_READ_ON_WRITE_ERROR|EVHTTP_CON_LINGERING_CLOSE>;
constexpr e4pp::detail::ev_flag_tag<evhttp_connection, EVHTTP_CON_REUSE_CONNECTED_ADDR>
    con_reuse_connected_addr{};
constexpr e4pp::detail::ev_flag_tag<evhttp_connection, EVHTTP_CON_READ_ON_WRITE_ERROR>
    con_read_on_write_error{};
constexpr e4pp::detail::ev_flag_tag<evhttp_connection, EVHTTP_CON_LINGERING_CLOSE>
    con_lingering_close{};

namespace detail {

struct connection_ref_allocator final
{
    constexpr static inline connection_ptr allocate(connection_ptr other) noexcept
    {
        return other;
    }

    constexpr static inline void free(connection_ptr) noexcept
    {   }
};

struct connection_allocator final
{
    static auto allocate(event_base *base, evdns_base *dns,
        bufferevent* bev, const char *addr, ev_uint16_t port) noexcept
    {
        assert(base);
        assert(addr);
        assert(bev);
        return e4pp::detail::check_pointer("evhttp_connection_base_bufferevent_new",
            evhttp_connection_base_bufferevent_new(base, dns, bev, addr, port));
    }

    static auto allocate(event_base *base, evdns_base *dns, 
        const char *addr, ev_uint16_t port) noexcept
    {
        assert(base);
        assert(addr);
        return e4pp::detail::check_pointer("evhttp_connection_base_new",
            evhttp_connection_base_new(base, dns, addr, port));
    }

    static void free(connection_ptr ptr) noexcept
    {
        evhttp_connection_free(ptr);
    }
};

} // detail

template<class A>
class basic_connection;

using connection_ref = basic_connection<detail::connection_ref_allocator>;
using connection = basic_connection<detail::connection_allocator>;

template<class A>
class basic_connection final
{
    using this_type = basic_connection<A>;

public:
    using handle_type = connection_ptr;

private:
    struct free_evhttp_connection
    {
        void operator()(handle_type ptr) noexcept 
        { 
            A::free(ptr);
        }
    };
    
    std::unique_ptr<evhttp_connection, free_evhttp_connection> conn_{};

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    basic_connection() = default;

    ~basic_connection() = default;

    // захват указателя на evhttp_connection
    // мы не владеем этим объектом
    template<class ... Args>
    explicit basic_connection(Args&&... args) noexcept
        : conn_{A::allocate(std::forward<Args>(args)...)}
    {
        assert(handle());
    }   

    // only for ref
    basic_connection(const basic_connection& other) noexcept
        : conn_{other.handle()}
    {
        // copy only for refs
        static_assert(std::is_same<this_type, connection_ref>::value);
    }

    // only for ref
    basic_connection& operator=(const basic_connection& other) noexcept
    {
        // copy only for refs
        static_assert(std::is_same<this_type, connection_ref>::value);
        conn_.reset(other.handle());
        return *this;
    }

    basic_connection(basic_connection&& that) noexcept
    {
        std::swap(conn_, that.conn_);
    }

    basic_connection& operator=(basic_connection&& that) noexcept
    {
        std::swap(conn_, that.conn_);
        return *this;
    }

    auto handle() const noexcept
    {
        return conn_.get();
    }

    operator connection_ptr() const noexcept
    {
        return handle();
    }

    explicit operator connection_ref() const noexcept
    {
        return ref();
    }

    connection_ref ref() const noexcept
    {
        return connection_ref(handle());
    }

    void create(event_base *base, evdns_base *dns, 
        const std::string& addr, ev_uint16_t port)
    {
        conn_.reset(A::allocate(base, dns, addr.c_str(), port));        
    }

    void create(event_base *base, const std::string& addr, ev_uint16_t port)
    {
        create(base, nullptr, addr, port);   
    }

    operator bool() const noexcept
    {
        return nullptr != handle();
    }

    void close() 
    {
        conn_.reset();
    }

    auto buffer_event() const noexcept
    {
        return evhttp_connection_get_bufferevent(assert_handle());
    }

    template<class F>
    void set_closecb(F& fn)
    {
        auto pair = proxy_call(fn);
        evhttp_connection_set_closecb(assert_handle(), pair.second, pair.first);
    } 

    void set_family(int family)
    {
        evhttp_connection_set_family(assert_handle(), family);
    }

    void set_flags(connection_flags f)
    {
        e4pp::detail::check_result("evhttp_connection_set_flags",
            evhttp_connection_set_flags(assert_handle(), f));
    }    

    void make_request(evhttp_request *req,
        enum evhttp_cmd_type type, const char *uri)
    {
        assert(req);
        e4pp::detail::check_result("evhttp_make_request", 
            evhttp_make_request(assert_handle(), req, type, uri));
    }

    void make_request(request req,
        enum evhttp_cmd_type type, const char *uri)
    {
        make_request(req.release(), type, uri);
    }
};

} // namespace http
} // namespace e4pp
