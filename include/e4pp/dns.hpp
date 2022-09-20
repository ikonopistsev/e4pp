#pragma once

#include "e4pp/e4pp.hpp"
#include "event2/dns.h"

namespace e4pp {

using dns_handle_type = evdns_base*;

class dns final
{
public:
    using handle_type = dns_handle_type;
    constexpr static int def_opt =
        { EVDNS_BASE_INITIALIZE_NAMESERVERS|
            EVDNS_BASE_DISABLE_WHEN_INACTIVE };

private:
    struct deallocate final
    {
        void operator()(handle_type ptr) noexcept 
        { 
            evdns_base_free(ptr, DNS_ERR_SHUTDOWN);
        }
    };
    using ptr_type = std::unique_ptr<evdns_base, deallocate>;
    ptr_type handle_{};

public:
    dns() = default;
    dns(dns&&) = default;
    dns& operator=(dns&&) = default;

    explicit dns(handle_type handle)
        : handle_{handle}
    {
        assert(handle);
        randomize_case("0");
    }

    dns(queue_handle_type queue, int opt = def_opt) 
        : dns{detail::check_pointer("evdns_base_new", 
            evdns_base_new(queue, opt))}
    {   }

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

    dns& set(const char *key, const char *val)
    {
        assert(key && val);
        assert((key[0] != '\0') && (val[0] != '\0'));
        detail::check_result("evdns_base_set_option",
            evdns_base_set_option(assert_handle(handle()), key, val));
        return *this;
    }

    dns& randomize_case(const char *val)
    {
        return set("randomize-case", val);
    }

    dns& timeout(const char *val)
    {
        return set("timeout", val);
    }

    dns& max_timeouts(const char *val)
    {
        return set("max-timeouts", val);
    }
};

} // namespace e4pp
