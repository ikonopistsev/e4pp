#pragma once

#include "e4pp/uri.hpp"
#include "e4pp/vhost.hpp"
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

// using hreq = evhttp_request*;

// namespace detail {

// struct req_ref_allocator final
// {
//     constexpr static inline hreq allocate() noexcept
//     {
//         return nullptr;
//     }

//     constexpr static inline void free(hreq) noexcept
//     {   }
// };

// struct req_allocator final
// {
//     static auto allocate()
//     {
//         return e4pp::detail::check_pointer("evhttp_request_new",
//             evhttp_request_new(nullptr, nullptr));
//     }

//     static void free(hreq ptr) noexcept
//     {
//         evhttp_request_free(ptr);
//     }
// };

// } // detail

// template<class A>
// class basic_request;

// using request_ref = basic_buffer<detail::req_ref_allocator>;
// using request = basic_buffer<detail::req_allocator>;

// template<class A>
// class basic_request
// {
//     using this_type = basic_request<A>;

// private:
//     hreq hreq_{A::allocate()};

//     auto assert_handle() const noexcept
//     {
//         auto h = handle();
//         assert(h);
//         return h;
//     }

// public:
//     auto handle() const noexcept
//     {
//         return hreq_;
//     }

//     basic_request() = default;

//     ~basic_request() noexcept
//     {
//         A::free(handle());
//     }

//     // only for ref
//     // this delete copy ctor for buffer&
//     basic_request(const basic_request& other) noexcept
//         : handle_{other.handle()}
//     {
//         // copy only for refs
//         static_assert(std::is_same<this_type, request_ref>::value);
//     }

//     // only for ref
//     // this delete copy ctor for buffer&
//     basic_request& operator=(const basic_request& other) noexcept
//     {
//         // copy only for refs
//         static_assert(std::is_same<this_type, request_ref>::value);
//         handle_ = other.handle();
//         return *this;
//     }

//     basic_request(basic_request&& that) noexcept
//     {
//         std::swap(handle_, that.handle_);
//     }

//     basic_request& operator=(basic_request&& that) noexcept
//     {
//         std::swap(handle_, that.handle_);
//         return *this;
//     }

//     explicit basic_request(hreq ptr) noexcept
//         : handle_{ptr}
//     {
//         static_assert(std::is_same<this_type, request_ref>::value);
//         assert(ptr);
//     }

//     explicit basic_request() noexcept
//         : handle_{evhttp_request_new()}
//     {
//         static_assert(std::is_same<this_type, request>::value);
//         assert(ptr);
//     }


//     explicit basic_request(hreq ptr) noexcept
//         : handle_{ptr}
//     {
//         static_assert(std::is_same<this_type, request_ref>::value);
//         assert(ptr);
//     }

// };

using connection_handle_type = evhttp_connection*;

class connection
{
public:
    using handle_type = connection_handle_type;

private:
    struct free_evhttp_connection
    {
        void operator()(connection_handle_type h) noexcept 
        { 
            evhttp_connection_free(h); 
        }
    };

    struct cleanup_ref
    {
        void operator()(connection_handle_type h) noexcept 
        {   }
    };

    std::unique_ptr<evhttp_connection, free_evhttp_connection> handle_{};
    //evhttp_connection_base_bufferevent_new();
    // hz

    // struct evhttp_connection *evhttp_connection_base_new(
	//     struct event_base *base, struct evdns_base *dnsbase,
	//     const char *address, ev_uint16_t port);

    static inline auto new_connection(event_base *base, evdns_base *dns, 
        const char *addr, ev_uint16_t port)
    {
        assert(base);
        assert(addr);
        return detail::check_pointer("evhttp_connection_base_new",
            evhttp_connection_base_new(base, dns, addr, port));
    }

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    connection() = default;

    connection(event_base *base, evdns_base *dns, 
        const std::string& addr, ev_uint16_t port)
        : handle_{new_connection(base, dns, addr.c_str(), port)}
    {  }

    void create(event_base *base, evdns_base *dns, 
        const std::string& addr, ev_uint16_t port)
    {
        handle_.reset(new_connection(base, dns, addr.c_str(), port));        
    }

    void create(event_base *base, const std::string& addr, ev_uint16_t port)
    {
        create(base, nullptr, addr, port);   
    }

    handle_type handle() const noexcept
    {
        return handle_.get();
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    operator bool() const noexcept
    {
        return nullptr != handle();
    }

    // ??? evhttp_connection_free_on_completion(struct evhttp_connection *evcon)s
    void free_on_completion() 
    {
        handle_.release();
    }   

    void close() 
    {
        handle_.reset();
    }

    void make(evhttp_request *req,
        enum evhttp_cmd_type type, const char *uri)
    {
        assert(req);
        detail::check_result("evhttp_make_request", 
            evhttp_make_request(assert_handle(), req, type, uri));
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
        auto hdr = assert_handle()->output_headers;
        detail::check_result("evhttp_add_header",
            evhttp_add_header(hdr, key, value));
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
