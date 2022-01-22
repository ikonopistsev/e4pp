#pragma once

#include "e4pp/e4pp.hpp"
#include <string>
#include <string_view>
#include "event2/http.h"

namespace e4pp {

using uri_handle_type = evhttp_uri*;

class uri
{
public:
    using handle_type = uri_handle_type;
    using ptr_type = std::unique_ptr<evhttp_uri, 
         decltype(&evhttp_uri_free)>;

private:
    ptr_type handle_{ nullptr, evhttp_uri_free };
    std::string_view user_{};
    std::string_view passcode_{};

    using sv = std::string_view;
    constexpr static auto npos = sv::npos;

    auto split_auth(std::string_view auth) noexcept
    {
        auto i = auth.find(':');
        return (i == npos) ?
            std::make_pair(auth, sv{}) :
            std::make_pair(auth.substr(0, i), auth.substr(i + 1));
    }

    handle_type assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    uri() = default;
    uri(uri&&) = default;
    uri& operator=(uri&&) = default;

    explicit uri(const char *source_uri)
        : handle_{ detail::check_pointer("evhttp_uri_parse",
            evhttp_uri_parse(source_uri)), evhttp_uri_free }
    {
        assert(source_uri);
        auto ui = userinfo();
        if (!ui.empty())
        {
            auto p = split_auth(ui);
            user_ = std::get<0>(p);
            passcode_ = std::get<1>(p);
        }
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
        auto rc = evhttp_uri_get_scheme(assert_handle());
        return (rc) ? sv{rc} : sv{};
    }

    std::string_view userinfo() const noexcept
    {
        auto rc = evhttp_uri_get_userinfo(assert_handle());
        return (rc) ? sv{rc} : sv{};
    }

    std::string_view user() const noexcept
    {
        return user_;
    }

    std::string_view passcode() const noexcept
    {
        return passcode_;
    }

    std::string_view host() const noexcept
    {
        auto rc = evhttp_uri_get_host(assert_handle());
        return (rc) ? sv{rc} : sv{};
    }

    int port() const noexcept
    {
        return evhttp_uri_get_port(assert_handle());
    }

    int port(int def) const noexcept
    {
        auto rc = port();
        return (rc < 1) ? def : rc;
    }

    std::string_view path() const noexcept
    {
        auto rc = evhttp_uri_get_path(assert_handle());
        return (rc) ? sv{rc} : sv{};
    }

    std::string_view rpath() const noexcept
    {
        auto p = path();
        return (!p.empty() && p[0] == '/') ? p.substr(1) : p;
    }

    std::string_view query() const noexcept
    {
        auto rc = evhttp_uri_get_query(assert_handle());
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
