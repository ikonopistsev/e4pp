#pragma once

#include "e4pp/e4pp.hpp"

#include <vector>
#include <string>

namespace e4pp {

using ev_base_flag = detail::ev_mask_flag<event_base_config_flag,
    EVENT_BASE_FLAG_NOLOCK|EVENT_BASE_FLAG_IGNORE_ENV|
    EVENT_BASE_FLAG_STARTUP_IOCP|EVENT_BASE_FLAG_NO_CACHE_TIME|
    EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST|EVENT_BASE_FLAG_PRECISE_TIMER>;

constexpr detail::ev_flag_tag<event_base_config_flag,
    EVENT_BASE_FLAG_NOLOCK> ev_base_nolock{};
constexpr detail::ev_flag_tag<event_base_config_flag,
    EVENT_BASE_FLAG_IGNORE_ENV> ev_base_ignore_env{};
constexpr detail::ev_flag_tag<event_base_config_flag,
    EVENT_BASE_FLAG_STARTUP_IOCP> ev_base_startup_iocp{};
constexpr detail::ev_flag_tag<event_base_config_flag,
    EVENT_BASE_FLAG_NO_CACHE_TIME> ev_base_no_cache_time{};
constexpr detail::ev_flag_tag<event_base_config_flag,
    EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST> ev_base_epoll_use_changelist{};
constexpr detail::ev_flag_tag<event_base_config_flag,
    EVENT_BASE_FLAG_PRECISE_TIMER> ev_base_precise_timer{};

using ev_feature = detail::ev_mask_flag<event_method_feature,
    EV_FEATURE_ET|EV_FEATURE_O1|EV_FEATURE_FDS|EV_FEATURE_EARLY_CLOSE>;

constexpr detail::ev_flag_tag<event_method_feature,
    EV_FEATURE_ET> ev_feature_et{};
constexpr detail::ev_flag_tag<event_method_feature,
    EV_FEATURE_O1> ev_feature_01{};
constexpr detail::ev_flag_tag<event_method_feature,
    EV_FEATURE_FDS> ev_feature_fds{};
constexpr detail::ev_flag_tag<event_method_feature,
    EV_FEATURE_EARLY_CLOSE> ev_feature_early_close{};

using config_handle_type = event_config*;

class config final
{
public:
    using handle_type = config_handle_type;

private:
    struct free_event_config
    {
        void operator()(handle_type ptr) noexcept 
        { 
            event_config_free(ptr); 
        };
    };
    using ptr_type = std::unique_ptr<event_config, free_event_config>;
    ptr_type handle_{detail::check_pointer("event_config_new", 
            event_config_new())};

public:
    config() = default;
    config(config&&) = default;
    config& operator=(config&&) = default;

    explicit config(ev_base_flag flag)
    {
        set_flag(flag);
    }

    explicit config(ev_feature f)
    {
        require_features(f);
    }

    explicit config(ev_base_flag flag, ev_feature f)
        : config(flag)
    {
        require_features(f);
    }

    void require_features(ev_feature value)
    {
        detail::check_result("event_config_require_features",
            event_config_require_features(handle(), value));
    }

    void avoid_method(const std::string& method)
    {
        detail::check_result("event_config_avoid_method",
            event_config_avoid_method(handle(), method.c_str()));
    }

    void set_flag(ev_base_flag flag)
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
        return handle_.get();
    }

    operator handle_type() const noexcept
    {
        return handle();
    }
};

} // namespace e4pp

