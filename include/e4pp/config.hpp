#pragma once

#include "e4pp/e4pp.hpp"

#include <vector>
#include <string>

namespace e4pp {

using config_handle_type = event_config*;

class config
{
public:
    using handle_type = config_handle_type;

private:
    std::unique_ptr<event_config, decltype(&event_config_free)>
        hconf_{ detail::check_pointer("event_config_new", 
            event_config_new()), event_config_free };

public:
    config() = default;

    config(config&) = delete;
    config& operator=(config&) = delete;

    explicit config(int flag)
    {
        set_flag(flag);
    }

    void require_features(int value)
    {
        detail::check_result("event_config_require_features",
            event_config_require_features(handle(), value));
    }

    void avoid_method(const std::string& method)
    {
        detail::check_result("event_config_avoid_method",
            event_config_avoid_method(handle(), method.c_str()));
    }

    void set_flag(int flag)
    {
        detail::check_result("event_config_set_flag",
            event_config_set_flag(handle(), flag));
    }

    static inline std::vector<std::string> supported_methods()
    {
        std::vector<std::string> res;
        auto method = event_get_supported_methods();
        for (std::size_t i = 0; method[i] != nullptr; ++i)
            res.emplace_back(method[i], std::strlen(method[i]));

        return res;
    }

    handle_type handle() const noexcept
    {
        return hconf_.get();
    }

    operator handle_type() const noexcept
    {
        return handle();
    }
};

} // namespace e4pp

