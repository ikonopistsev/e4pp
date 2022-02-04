#pragma once

#include "e4pp/evtype.hpp"
#include "e4pp/queue.hpp"

namespace e4pp {

template<class T>
class evcore
{
public:
    using event_type = T;

private:
    event_type event_{};

    auto assert_handle() const noexcept
    {
        auto h = event_.handle();
        assert(h);
        return h;
    }

public:
    evcore() = default;
    evcore(evcore&&) = default;
    evcore(const evcore&) = default;
    evcore& operator=(evcore&&) = default;
    evcore& operator=(const evcore&) = default;

// generic
    evcore(queue_handle_type queue, evutil_socket_t fd, flag ef,
        event_callback_fn fn, void *arg)
        : event_{queue, fd, ef, fn, arg}
    {   }

    evcore(queue_handle_type queue, evutil_socket_t fd, flag ef,
        timeval tv, event_callback_fn fn, void *arg)
        : evcore{queue, fd, ef, fn, arg}
    {   
        add(tv);
    }

    template<class F, class P>
    evcore(queue_handle_type queue, evutil_socket_t fd, 
        flag ef, std::pair<F, P> p)
        : evcore{queue, fd, ef, p.second, p.first}
    {   }

    template<class F, class P>
    evcore(queue_handle_type queue, evutil_socket_t fd, 
        flag ef, timeval tv, std::pair<F, P> p)
        : evcore{queue, fd, ef, tv, p.second, p.first}
    {   }

    template<class F>
    evcore(queue_handle_type queue, evutil_socket_t fd, 
        flag ef, F& fn)
        : evcore{queue, fd, ef, proxy_call(fn)}
    {   }  

    template<class F>
    evcore(queue_handle_type queue, evutil_socket_t fd, 
        flag ef, timeval tv, F& fn)
        : evcore{queue, fd, ef, tv, proxy_call(fn)}
    {   }  

    template<class F, class Rep, class Period>
    evcore(queue_handle_type queue, evutil_socket_t fd, 
        flag ef, std::chrono::duration<Rep, Period> timeout, F& fn)
        : evcore{queue, fd, ef, make_timeval(timeout), fn}
    {   } 

    template<class F>
    evcore(queue_handle_type queue, flag ef, F& fn)
        : evcore{queue, -1, ef, fn}
    {   }  

    template<class F>
    evcore(queue_handle_type queue, flag ef, timeval tv, F& fn)
        : evcore{queue, -1, ef, tv, fn}
    {   }

    template<class F, class Rep, class Period>
    evcore(queue_handle_type queue, flag ef, 
        std::chrono::duration<Rep, Period> timeout, F& fn)
        : evcore{queue, ef, make_timeval(timeout), fn}
    {   } 

    template<class F>
    evcore(queue_handle_type queue, F& fn)
        : evcore{queue, -1, flag{EV_TIMEOUT}, fn}
    {   }  

    template<class F>
    evcore(queue_handle_type queue, timeval tv, F& fn)
        : evcore{queue, -1, flag{EV_TIMEOUT}, tv, fn}
    {   }

    template<class F, class Rep, class Period>
    evcore(queue_handle_type queue,
        std::chrono::duration<Rep, Period> timeout, F& fn)
        : evcore{queue, -1, flag{EV_TIMEOUT}, timeout, fn}
    {   }

    void create(queue_handle_type queue, evutil_socket_t fd, flag ef,
        event_callback_fn fn, void *arg)
    {
        event_.create(queue, fd, ef, fn, arg);
    }

    template<class F, class P>
    void create(queue_handle_type queue, 
        evutil_socket_t fd, flag ef, std::pair<F, P> p)
    {   
        create(queue, fd, ef, p.second, p.first);
    }

    template<class F>
    void create(queue_handle_type queue, 
        evutil_socket_t fd, flag ef, F& fn)
    {
        create(queue, fd, ef, proxy_call(fn));
    }  

    template<class F>
    void create(queue_handle_type queue, flag ef, F& fn)
    {   
        flag f{static_cast<short>(ef|EV_TIMEOUT)};
        create(queue, -1, f, fn);
    }  

    template<class F>
    void create(queue_handle_type queue, F& fn)
    {   
        create(queue, -1, flag{EV_TIMEOUT}, fn);
    } 

// api
    bool empty() const noexcept
    {
        return event_.empty();
    }

    auto handle() const noexcept
    {
        return event_.handle();
    }

    operator event_handle_type() const noexcept
    {
        return handle();
    }

    void destroy() noexcept
    {
        event_.destroy();
    }

    // !!! это не выполнить на следующем цикле очереди
    // это добавить без таймаута
    // допустим вечное ожидание EV_READ или сигнала
    void add(timeval* tv = nullptr)
    {
        detail::check_result("event_add",
            event_add(assert_handle(), tv));
    }

    void add(timeval tv)
    {
        add(&tv);
    }

    template<class Rep, class Period>
    void add(std::chrono::duration<Rep, Period> timeout)
    {
        add(make_timeval(timeout));
    }

    void remove()
    {
        detail::check_result("event_del",
            event_del(assert_handle()));
    }

    // метод запускает эвент напрямую
    // те может привести к бесконечному вызову калбеков activate
    // можно испольновать из разных потоков (при use_threads)
    void active(int res) noexcept
    {
        event_active(assert_handle(), res, 0);
    }

    void set_priority(int priority)
    {
        detail::check_result("event_priority_set",
            event_priority_set(assert_handle(), priority));
    }

    //    Checks if a specific event is pending or scheduled
    //    @param tv if this field is not NULL, and the event has a timeout,
    //           this field is set to hold the time at which the timeout will
    //       expire.
    bool pending(timeval& tv, event_flag events) const noexcept
    {
        return event_pending(assert_handle(), events, &tv) != 0;
    }

    // Checks if a specific event is pending or scheduled
    bool pending(event_flag events) const noexcept
    {
        return event_pending(assert_handle(), events, nullptr) != 0;
    }

    event_flag events() const noexcept
    {
        return event_get_events(assert_handle());
    }

    evutil_socket_t fd() const noexcept
    {
        return event_get_fd(assert_handle());
    }

    // хэндл очереди
    queue_handle_type queue_handle() const noexcept
    {
        return event_get_base(assert_handle());
    }

    operator queue_handle_type() const noexcept
    {
        return queue_handle();
    }

    event_callback_fn callback() const noexcept
    {
        return event_get_callback(assert_handle());
    }

    void* callback_arg() const noexcept
    {
        return event_get_callback_arg(assert_handle());
    }

    // выполнить обратный вызов напрямую
    void exec(event_flag ef)
    {
        auto handle = assert_handle();
        auto fn = event_get_callback(handle);
        // должен быть каллбек
        assert(fn);
        // вызываем обработчик
        (*fn)(event_get_fd(handle), ef, event_get_callback_arg(handle));
    }
};

} // namespace e4pp
