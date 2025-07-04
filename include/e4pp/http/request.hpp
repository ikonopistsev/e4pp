#pragma once

#include "e4pp/e4pp.hpp"
#include "e4pp/buffer.hpp"
#include <event2/http.h>
#include <event2/http_struct.h>
#include <functional>

namespace e4pp {
namespace http {

using request_ptr = evhttp_request*;

namespace detail {

using request_native_fn = void (*)(request_ptr, void *);
using request_error_native_fn = void (*)(evhttp_request_error, void *);

struct request_ref_allocator final
{
    constexpr static inline request_ptr allocate(request_ptr other) noexcept
    {
        return other;
    }

    constexpr static inline void free(request_ptr) noexcept
    {   }
};

struct request_allocator final
{
    static auto allocate(request_native_fn fn, 
        request_error_native_fn err_fn, void *arg) noexcept
    {
        auto req = e4pp::detail::check_pointer("evhttp_request_new",
            evhttp_request_new(fn, arg));
        evhttp_request_set_error_cb(req, err_fn);
        return req;
    }

    static auto allocate(request_native_fn fn, void *arg) noexcept
    {
        return e4pp::detail::check_pointer("evhttp_request_new",
            evhttp_request_new(fn, arg));
    }    

    static void free(request_ptr ptr) noexcept
    {
        evhttp_request_free(ptr);
    }
};

} // detail

// Forward declarations
template<class A>
class basic_request;

using request_ref = basic_request<detail::request_ref_allocator>;
using request = basic_request<detail::request_allocator>;

template<class T>
struct chunked_cb_fn final
{
    using fn_type = void (T::*)(request_ptr req);
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void call(request_ptr req) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(req);
        } 
        catch (...)
        {   }
    }
};

template<class T>
auto proxy_call(chunked_cb_fn<T>& fn)
{
    return std::make_pair(&fn,
        [](evhttp_request* req, void *arg){
            assert(arg);
            try {
                static_cast<chunked_cb_fn<T>*>(arg)->call(req);
            }
            catch (...)
            {   }
        });
}

template<class A>
class basic_request final
{
    using this_type = basic_request<A>;

public:
    using handle_type = request_ptr;

private:
    struct free_evhttp_request
    {
        void operator()(handle_type ptr) noexcept 
        { 
            A::free(ptr);
        }
    };
    
    std::unique_ptr<evhttp_request, 
        free_evhttp_request> req_{};

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    basic_request()
    {
        // Ref types should not create new objects, only capture existing ones
        static_assert(!std::is_same<this_type, request_ref>::value);
    }

    explicit basic_request(request_ptr ptr) noexcept
        : req_{ptr}
    {
        // Ref types should not create new objects, only capture existing ones
        static_assert(std::is_same<this_type, request_ref>::value);
        assert(ptr);
    }
    
    ~basic_request() = default;

    // Create request with callback
    template<class ... Args>
    explicit basic_request(Args&&... args) noexcept
        : req_{A::allocate(std::forward<Args>(args)...)}
    {
        // Ref types should not create new objects, only capture existing ones
        static_assert(!std::is_same<this_type, request_ref>::value);
        assert(handle());
    }   

    // only for ref
    basic_request(const basic_request& other) noexcept
        : req_{other.handle()}
    {
        // copy only for refs
        static_assert(std::is_same<this_type, request_ref>::value);
    }

    // only for ref
    basic_request& operator=(const basic_request& other) noexcept
    {
        // copy only for refs
        static_assert(std::is_same<this_type, request_ref>::value);
        req_.reset(other.handle());
        return *this;
    }

    basic_request(basic_request&& that) noexcept
    {
        std::swap(req_, that.req_);
    }

    basic_request& operator=(basic_request&& that) noexcept
    {
        std::swap(req_, that.req_);
        return *this;
    }

    auto handle() const noexcept
    {
        return req_.get();
    }

    operator request_ptr() const noexcept
    {
        return handle();
    }

    explicit operator request_ref() const noexcept
    {
        return ref();
    }

    request_ref ref() const noexcept
    {
        return request_ref(handle());
    }

    void create(detail::request_native_fn fn, void *arg)
    {
        // Ref types should not create new objects, only capture existing ones
        static_assert(!std::is_same<this_type, request_ref>::value);
        req_.reset(A::allocate(fn, arg));        
    }

    operator bool() const noexcept
    {
        return nullptr != handle();
    }

    void close() 
    {
        req_.reset();
    }

    // Core request methods
    const char* get_uri() const noexcept
    {
        return evhttp_request_get_uri(assert_handle());
    }

    enum evhttp_cmd_type get_command() const noexcept
    {
        return evhttp_request_get_command(assert_handle());
    }

    int get_response_code() const noexcept
    {
        return evhttp_request_get_response_code(assert_handle());
    }

    const char* get_response_code_line() const noexcept
    {
        return evhttp_request_get_response_code_line(assert_handle());
    }

    const char* get_host() const noexcept
    {
        return evhttp_request_get_host(assert_handle());
    }

    auto input_headers() const noexcept
    {
        return evhttp_request_get_input_headers(assert_handle());
    }

    auto output_headers() const noexcept
    {
        return evhttp_request_get_output_headers(assert_handle());
    }

    // Buffer access
    auto input_buffer() const noexcept
    {
        return evhttp_request_get_input_buffer(assert_handle());
    }

    auto output_buffer() const noexcept
    {
        return evhttp_request_get_output_buffer(assert_handle());
    }

    // Connection access
    auto get_connection() const noexcept
    {
        return evhttp_request_get_connection(assert_handle());
    }

    operator evhttp_connection*() const noexcept
    {
        return get_connection();
    }

    // Request ownership
    void own()
    {
        evhttp_request_own(assert_handle());
    }

    bool is_owned() const noexcept
    {
        return evhttp_request_is_owned(assert_handle()) != 0;
    }

    // Callback setters
    template<class F>
    void on_chunked(F& fn)
    {
        auto pair = proxy_call(fn);
        evhttp_request_set_chunked_cb(assert_handle(), 
            pair.second, pair.first);
    }

    void cancel() noexcept
    {
        evhttp_cancel_request(assert_handle());
    }

    template<class F>
    void on_header(F& fn)
    {
        auto pair = proxy_call(fn);        
        evhttp_request_set_header_cb(assert_handle(), 
            pair.second, pair.first);
    }

    template<class F>
    void on_complete(F& fn)
    {
        auto pair = proxy_call(fn);
        evhttp_request_set_on_complete_cb(assert_handle(), 
            pair.second, pair.first);
    }

    auto release() noexcept
    {
        // Only owning types can release ownership, not refs
        static_assert(!std::is_same<this_type, request_ref>::value);
        return req_.release();
    }
};

using request_fun = std::function<void(request_ref)>;

// Callback wrappers
template<class T>
struct request_fn final
{
    using fn_type = void (T::*)(request_ref req);
    using self_type = T;
    using err_fn_type = void (T::*)(enum evhttp_request_error error);

    fn_type fn_{};
    err_fn_type error_fn_{};
    T& self_;

    void call(request_ptr req) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(request_ref{req});
        } 
        catch (...)
        {   }
    }

    void call(enum evhttp_request_error error) noexcept
    {
        assert(error_fn_);
        try {
            (self_.*error_fn_)(error);
        } 
        catch (...)
        {   }
    }
};

// Helper functions for creating requests
template<class T>
request create_request(request_fn<T>& cb)
{
    // Используем функции-шаблоны вместо static лямбд
    auto cb_fn = [](evhttp_request *req, void *arg) {
        assert(arg);
        try {
            static_cast<request_fn<T>*>(arg)->call(req);
        }
        catch (...)
        {   }
    };

    auto err_fn = [](enum evhttp_request_error error, void *arg) {
        assert(arg);
        try {
            static_cast<request_fn<T>*>(arg)->call(error);
        }
        catch (...)
        {   }
    };

    return (!cb.error_fn_) ? request{+cb_fn, &cb} :
        request{+cb_fn, +err_fn, &cb};
}

} // namespace http
} // namespace e4pp
