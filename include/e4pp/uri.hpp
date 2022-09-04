#pragma once

#include "e4pp/e4pp.hpp"
#include <string>
#include <string_view>
#include "event2/http.h"

namespace e4pp {

using uri_handle_type = evhttp_uri*;

class uri final
{
public:
    using handle_type = uri_handle_type;

private:
    struct deallocate 
    {
        void operator()(handle_type ptr) noexcept 
        { 
            evhttp_uri_free(ptr); 
        }
    };
    using ptr_type = std::unique_ptr<evhttp_uri, deallocate>;
    ptr_type handle_{};

    using sv = std::string_view;
    constexpr static auto npos = sv::npos;

public:
    uri() = default;
    uri(uri&&) = default;
    uri& operator=(uri&&) = default;

    explicit uri(const char *source_uri)
        : handle_{detail::check_pointer("evhttp_uri_parse",
            evhttp_uri_parse(source_uri))}
    {
        assert(source_uri);
    }

    explicit uri(const std::string& source_uri)
        : uri{source_uri.c_str()}
    {   }

    handle_type handle() const noexcept
    {
        return handle_.get();
    }

    std::string_view scheme() const noexcept
    {
        auto rc = evhttp_uri_get_scheme(assert_handle(handle()));
        return (rc) ? sv{rc} : sv{};
    }

    std::string_view userinfo() const noexcept
    {
        auto rc = evhttp_uri_get_userinfo(assert_handle(handle()));
        return (rc) ? sv{rc} : sv{};
    }

    static inline auto split_auth(std::string_view auth) noexcept
    {
        auto i = auth.find(':');
        return (i == npos) ?
            std::make_pair(auth, sv{}) :
            std::make_pair(auth.substr(0, i), auth.substr(i + 1));
    }

    auto auth() noexcept
    {
        return split_auth(userinfo());
    }

    std::string_view host() const noexcept
    {
        auto rc = evhttp_uri_get_host(assert_handle(handle()));
        return (rc) ? sv{rc} : sv{};
    }

    int port() const noexcept
    {
        return evhttp_uri_get_port(assert_handle(handle()));
    }

    int port(int def) const noexcept
    {
        auto rc = port();
        return (rc < 1) ? def : rc;
    }

    std::string_view path() const noexcept
    {
        auto rc = evhttp_uri_get_path(assert_handle(handle()));
        return (rc) ? sv{rc} : sv{};
    }

    std::string_view rpath() const noexcept
    {
        auto p = path();
        return (!p.empty() && p[0] == '/') ? p.substr(1) : p;
    }

    std::string_view query() const noexcept
    {
        auto rc = evhttp_uri_get_query(assert_handle(handle()));
        return (rc) ? sv{rc} : sv{};
    }

    std::string_view fragment() const noexcept
    {
        auto rc = evhttp_uri_get_fragment(handle());
        return (rc) ? sv{rc} : sv{};
    }

    std::string addr() const
    {
        std::string rc;
        auto h = host();
        auto p = port();
        if (!h.empty())
        {
            rc += h;
            if (p > 0)
            {
                rc += ':';
                rc += std::to_string(p);
            }
        }
        return rc;
    }

    std::string addr_port(int def) const
    {
        std::string rc;
        auto h = host();
        auto p = port(def);
        if (!(h.empty() || (p <= 0)))
        {
            rc += h;
            rc += ':';
            rc += std::to_string(p);
        }
        return rc;
    }
};

} // namespace e4pp
