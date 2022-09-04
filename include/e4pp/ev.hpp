#pragma once

#include "e4pp/evcore.hpp"
#include "e4pp/functional.hpp"

namespace e4pp {

using ev_heap = evcore<heap_event>;
using ev_stack = evcore<stack_event>;

template<class H, class E>
class ev;

namespace evs {

template<class T>
using timer_fn = ev<e4pp::timer_fn<T>, ev_stack>;
using timer = ev<e4pp::timer_fun, ev_stack>;
using type = ev_stack;

} // namespace evs

namespace evh {

template<class T>
using timer_fn = ev<e4pp::timer_fn<T>, heap_event>;
using timer =  ev<e4pp::timer_fun, heap_event>;
using type = ev_heap;

} // namespace evh

template<class H, class E>
class ev final
    : evcore<E>
{
    H handler_{};
    using parent_type = evcore<E>;
    
public:
    ev() = default;
    ev(ev&&) = default;
    ev(const ev&) = default;
    ev& operator=(ev&&) = default;
    ev& operator=(const ev&) = default;

// --- socket
    ev(queue_handle_type queue, evutil_socket_t fd, 
        ev_flag ef, H handler)
        : parent_type{queue, fd, ef, handler_}
        , handler_{std::move(handler)}
    {   }

    ev(queue_handle_type queue, evutil_socket_t fd,
        ev_flag ef, const timeval& tv, H handler)
        : parent_type{queue, fd, ef, handler_}
        , handler_{std::move(handler)}
    {   }

    template<class Rep, class Period>
    ev(queue_handle_type queue, evutil_socket_t fd, ev_flag ef, 
        std::chrono::duration<Rep, Period> timeout, H handler)
        : parent_type{queue, fd, ef, timeout, handler_}
        , handler_{std::move(handler)}
    {   }

// --- timer
    ev(queue_handle_type queue, ev_flag ef, H handler)
        : parent_type{queue, ef, handler_}
        , handler_{std::move(handler)}
    {   }

    ev(queue_handle_type queue, ev_flag ef, const timeval& tv, H handler)
        : parent_type{queue, ef, tv, handler_}
        , handler_{std::move(handler)}
    {   }

    template<class Rep, class Period>
    ev(queue_handle_type queue, ev_flag ef, 
        std::chrono::duration<Rep, Period> timeout, H handler)
        : parent_type{queue, ef, timeout, handler_}
        , handler_{std::move(handler)}
    {   }

    using parent_type::empty;
    using parent_type::destroy;

    // add() - это не выполнить на следующем цикле очереди
    // это добавить без таймаута
    // допустим вечное ожидание EV_READ или сигнала
    using parent_type::add;
    using parent_type::remove;

    // метод запускает эвент напрямую
    // те может привести к бесконечному вызову калбеков activate
    // можно испольновать из разных потоков (при use_threads)
    using parent_type::active;
    using parent_type::set_priority;

    //    Checks if a specific event is pending or scheduled
    //    @param tv if this field is not NULL, and the event has a timeout,
    //           this field is set to hold the time at which the timeout will
    //       expire.
    using parent_type::pending;
    using parent_type::events;
};

} // namespace e4pp
