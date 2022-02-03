#pragma once

#include "e4pp/evcore.hpp"
#include "e4pp/functional.hpp"

namespace e4pp {

template<class H, class E>
class ev;

using ev_heap = evcore<heap_event>;
using ev_stack = evcore<stack_event>;

namespace detail {

template<class H, class E>
using timer_ev_fn = ev<e4pp::timer_fn<H>, E>;

template<class T>
using timer_ev_fun = ev<e4pp::timer_fun, T>;

} // namespace ev

namespace evs {

template<class T>
using timer_fn = detail::timer_ev_fn<T, ev_stack>;
using timer = detail::timer_ev_fun<ev_stack>;
using type = ev_stack;

} // namespace evs

namespace evh {

template<class T>
using timer_fn = detail::timer_ev_fn<T, ev_heap>;
using timer = detail::timer_ev_fun<ev_heap>;
using type = ev_heap;

} // namespace evh

template<class H, class E>
class ev
{
    H handler_{};
    E ev_{};

public:
    ev() = default;
    ev(ev&&) = default;
    ev(const ev&) = default;
    ev& operator=(ev&&) = default;
    ev& operator=(const ev&) = default;

// --- socket
    ev(queue_handle_type queue, evutil_socket_t fd, 
        event_flag ef, H handler)
        : handler_{ std::move(handler) }
        , ev_{ queue, fd, ef, handler_ }
    {   }

    ev(queue_handle_type queue, evutil_socket_t fd,
        event_flag ef, timeval tv, H handler)
        : handler_{ std::move(handler) }
        , ev_{ queue, fd, ef, tv, handler_ }
    {   }

    template<class Rep, class Period>
    ev(queue_handle_type queue, evutil_socket_t fd, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, H handler)
        : handler_{std::move(handler)}
        , ev_{queue, fd, ef, timeout, handler_}
    {   }

// --- timer
    ev(queue_handle_type queue, event_flag ef, H handler)
        : handler_{ std::move(handler) }
        , ev_{ queue, ef, handler_ }
    {   }

    ev(queue_handle_type queue, event_flag ef, timeval tv, H handler)
        : handler_{ std::move(handler) }
        , ev_{ queue, ef, tv, handler_ }
    {   }

    template<class Rep, class Period>
    ev(queue_handle_type queue, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, H handler)
        : handler_{ std::move(handler) }
        , ev_{ queue, ef, timeout, handler_ }
    {   }

// api proxy from evcore
    bool empty() const noexcept
    {
        return ev_.empty();
    }

    void destroy() noexcept
    {
        ev_.destroy();
    }

    // !!! это не выполнить на следующем цикле очереди
    // это добавить без таймаута
    // допустим вечное ожидание EV_READ или сигнала
    void add(timeval* tv = nullptr)
    {
        ev_.add(tv);
    }

    void add(timeval tv)
    {
        ev_.add(tv);
    }

    template<class Rep, class Period>
    void add(std::chrono::duration<Rep, Period> timeout)
    {
        ev_.add(timeout);
    }

    void remove()
    {
        ev_.remove();
    }

    // метод запускает эвент напрямую
    // те может привести к бесконечному вызову калбеков activate
    // можно испольновать из разных потоков (при use_threads)
    void active(int res) noexcept
    {
        ev_.active(res);
    }

    void set_priority(int priority)
    {
        ev_.set_priority(priority);
    }

    //    Checks if a specific event is pending or scheduled
    //    @param tv if this field is not NULL, and the event has a timeout,
    //           this field is set to hold the time at which the timeout will
    //       expire.
    bool pending(timeval& tv, event_flag events) const noexcept
    {
        return ev_.pending(tv, events);
    }

    template<class Rep, class Period>
    void pending(std::chrono::duration<Rep, Period> timeout, 
        event_flag events) noexcept
    {
        pending(make_timeval(timeout), events);
    }

    // Checks if a specific event is pending or scheduled
    bool pending(event_flag events) const noexcept
    {
        return ev_.pending(events);
    }

    event_flag events() const noexcept
    {
        return ev_.events();
    }
};

} // namespace btpro
