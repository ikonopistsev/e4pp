#pragma once

#include "e4pp/http/connection.hpp"

namespace e4pp {
namespace http {

template<class T>
struct closecb_fn final
{
    using fn_type = void (T::*)(connection_ref);
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void call(connection_ptr conn) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(connection_ref{conn});
        } 
        catch (...)
        {   }
    }
};

template<class T>
auto proxy_call(closecb_fn<T>& fn)
{
    return std::make_pair(&fn,
        [](evhttp_connection* conn, void *arg){
            assert(arg);
            try {
                static_cast<closecb_fn<T>*>(arg)->call(conn);
            }
            catch (...)
            {   }
        });
}

using closecb_fun = std::function<void(connection_ref)>;

inline auto proxy_call(closecb_fun& fn)
{
    return std::make_pair(&fn,
        [](evhttp_connection* conn, void *arg){
            assert(arg);
            auto fn = static_cast<closecb_fun*>(arg);
            try {
                (*fn)(connection_ref{conn});
            }
            catch (...)
            {   }
        });
}

inline auto proxy_call(closecb_fun&& fn)
{
    return std::make_pair(new closecb_fun{std::move(fn)},
        [](evhttp_connection* conn, void *arg){
            assert(arg);
            auto fn = static_cast<closecb_fun*>(arg);
            try {
                (*fn)(connection_ref{conn});
            }
            catch (...)
            {   }
            delete fn;
        });
}

} // namespace http
} // namespace e4pp
