#pragma once

#include "e4pp/http/connection.hpp"

namespace e4pp {
namespace http {

template<class T>
struct closecb_fn final
{
    using fn_type = void (T::*)(connection_ref conn);
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

} // namespace http
} // namespace e4pp
