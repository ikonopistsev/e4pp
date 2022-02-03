#pragma once

#include "e4pp/config.hpp"
#include "e4pp/functional.hpp"
#include <type_traits>
#include <string>

namespace e4pp {

class queue final
{
public:
    using handle_type = queue_handle_type;

private:
    struct deallocate 
    {
        void operator()(handle_type ptr) noexcept 
        { 
            event_base_free(ptr); 
        };
    };
    using ptr_type = std::unique_ptr<event_base, deallocate>;
    ptr_type handle_{detail::check_pointer("event_base_new", 
        event_base_new())};

public:
    queue() = default;
    queue(queue&&) = default;
    queue& operator=(queue&&) = default;

    explicit queue(handle_type handle) noexcept
        : handle_{handle}
    {
        assert(handle);
    }

    explicit queue(const config& conf) 
        : queue{detail::check_pointer("event_base_new_with_config",
            event_base_new_with_config(conf))}
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

    void destroy() noexcept
    {
        handle_.reset();
    }

    std::string method() const noexcept
    {
        auto res = event_base_get_method(assert_handle(handle()));
        return res ? std::string(res) : std::string();
    }

    static inline std::string version() noexcept
    {
        auto res = event_get_version();
        return (res) ? std::string(res) : std::string();
    }

    int features() const noexcept
    {
        return event_base_get_features(assert_handle(handle()));
    }

    /* true - has events */
    /* false - no events */
    bool dispatch()
    {
        return 0 == detail::check_result("event_base_dispatch",
            event_base_dispatch(assert_handle(handle())));
    }

    template<class Rep, class Period>
    bool dispatch(std::chrono::duration<Rep, Period> timeout)
    {
        loopexit(timeout);
        return dispatch();
    }

    bool loop(int flags)
    {
        return 0 == detail::check_result("event_base_loop",
            event_base_loop(assert_handle(handle()), flags));
    }

    void loopexit(timeval tv)
    {
        detail::check_result("event_base_loopexit",
            event_base_loopexit(assert_handle(handle()), &tv));
    }

    template<class Rep, class Period>
    void loopexit(std::chrono::duration<Rep, Period> timeout)
    {
        loopexit(make_timeval(timeout));
    }    

    void loop_break()
    {
        detail::check_result("event_base_loopbreak",
            event_base_loopbreak(assert_handle(handle())));
    }

    bool stopped() const noexcept
    {
        return 0 != event_base_got_break(assert_handle(handle()));
    }

#ifdef EVENT_MAX_PRIORITIES
    void priority_init(int level)
    {
        detail::check_result("event_base_priority_init",
            event_base_priority_init(assert_handle(handle()), level));
    }
#endif // EVENT_MAX_PRIORITIES

    timeval gettimeofday_cached() const
    {
        timeval tv;
        detail::check_result("event_base_gettimeofday_cached",
            event_base_gettimeofday_cached(assert_handle(handle()), &tv));
        return tv;
    }

    void update_cache_time() const
    {
        detail::check_result("event_base_update_cache_time",
            event_base_update_cache_time(assert_handle(handle())));
    }

    operator timeval() const
    {
        return gettimeofday_cached();
    }

    void once(evutil_socket_t fd, event_flag ef, timeval tv,
        event_callback_fn fn, void *arg)
    {
        detail::check_result("event_base_once",
            event_base_once(assert_handle(handle()), fd, ef, fn, arg, &tv));
    }

    template<class T>
    void once(evutil_socket_t fd, event_flag ef, timeval tv, T&& fn)
    {
        auto p = proxy_call(std::forward<T>(fn));
        once(fd, ef, tv, p.second, p.first);
    }

    template<class T, class Rep, class Period>
    void once(evutil_socket_t fd, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, T&& fn)
    {
        once(fd, ef, make_timeval(timeout), std::forward<T>(fn));
    }

    template<class T>
    void once(timeval tv, T&& fn)
    {
        once(-1, EV_TIMEOUT, tv, std::forward<T>(fn));
    }

    template<class T, class Rep, class Period>
    void once(std::chrono::duration<Rep, Period> timeout, T&& fn)
    {
        once(make_timeval(timeout), std::forward<T>(fn));
    }

    template<class T>
    void once(T&& fn)
    {
        once(timeval{}, std::forward<T>(fn));
    }

    template<class T>
    void once(queue& other, timeval tv, T&& fn)
    {
        once(tv, timer_requeue(other, std::forward<T>(fn)));

    }

    template<class T, class Rep, class Period>
    void once(queue& other, 
        std::chrono::duration<Rep, Period> timeout, T&& fn)
    {
        once(timeout, timer_requeue(other, std::forward<T>(fn)));
    }

    template<class T>
    void once(queue& other, T&& fn)
    {
        once(timeval{}, timer_requeue(other, std::forward<T>(fn)));
    }

    template<class T>
    void once(queue& other, evutil_socket_t fd, 
        event_flag ef, timeval tv, T&& fn)
    {
        once(fd, ef, tv, 
            generic_requeue(other, std::forward<T>(fn)));
    }

    template<class T, class Rep, class Period>
    void once(queue& other, evutil_socket_t the_fd, event_flag the_ef, 
        std::chrono::duration<Rep, Period> timeout, T&& fn)
    {
        once(the_fd, the_ef, timeout,
            generic_requeue(other, std::forward<T>(fn)));
    }

    template<class T>
    void once(queue& other, evutil_socket_t fd, 
        event_flag ef, T&& fn)
    {
        once(fd, ef, timeval{}, 
            generic_requeue(other, std::forward<T>(fn)));
    }

    void error(std::exception_ptr) const noexcept
    {   }

private:
    template<class T>
    auto timer_requeue(queue& queue, T&& fn)
    {
        return [&]{
            try {
                queue.once([&, f = std::forward<T>(fn)]{
                    try {
                        f();
                    } catch (...) {
                        queue.error(std::current_exception());
                    }
                });
            } catch (...) {
                error(std::current_exception());
            }
        };
    }

    template<class T>
    auto generic_requeue(queue& queue, T&& fn)
    {
        return [&](evutil_socket_t fd, event_flag ef){
            try {
                queue.once([&, fd, ef, f = std::forward<T>(fn)]{
                    try {
                        f(fd, ef);
                    } catch (...) {
                        queue.error(std::current_exception());
                    }
                });
            } catch (...) {
                error(std::current_exception());
            }
        };
    }    
};

} // namespace e4pp
