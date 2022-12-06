#pragma once

#include "e4pp/e4pp.hpp"
#include "event2/event_struct.h"

namespace e4pp {

using ev_flag = detail::ev_mask_flag<event, EV_TIMEOUT|EV_READ|EV_WRITE|
    EV_SIGNAL|EV_PERSIST|EV_ET|EV_FINALIZE|EV_CLOSED>;

constexpr detail::ev_flag_tag<event, EV_TIMEOUT> ev_timeout{};
constexpr detail::ev_flag_tag<event, EV_READ> ev_read{};
constexpr detail::ev_flag_tag<event, EV_WRITE> ev_write{};
constexpr detail::ev_flag_tag<event, EV_SIGNAL> ev_signal{};
constexpr detail::ev_flag_tag<event, EV_PERSIST> ev_persist{};
constexpr detail::ev_flag_tag<event, EV_ET> ev_et{};
constexpr detail::ev_flag_tag<event, EV_FINALIZE> ev_finalize{};
constexpr detail::ev_flag_tag<event, EV_CLOSED> ev_closed{};

class heap_event final
{
public:
    using handle_type = event_handle_type;

private:
    struct deallocate
    {
        void operator()(handle_type ptr) noexcept 
        { 
            event_free(ptr); 
        };
    };
    std::unique_ptr<event, deallocate> handle_{};

public:
    heap_event() = default;
    heap_event(heap_event&&) = default;
    heap_event& operator=(heap_event&&) = default;

    // захват хэндла
    explicit heap_event(handle_type handle) noexcept
        : handle_{handle}
    {
        assert(handle);
    }

    // generic
    heap_event(queue_handle_type queue, evutil_socket_t fd, 
        ev_flag ef, event_callback_fn fn, void *arg)
        : heap_event{detail::check_pointer("event_new", 
            event_new(queue, fd, ef, fn, arg))}
    {
        assert(queue && fn);
    }

    // создание объекта
    void create(queue_handle_type queue, evutil_socket_t fd, 
        ev_flag ef, event_callback_fn fn, void *arg)
    {
        // если создаем повторно поверх
        // значит что-то пошло не так
        assert(queue && fn && !handle_);

        // создаем объект эвента
        handle_.reset(detail::check_pointer("event_new",
            event_new(queue, fd, ef, fn, arg)));
    }

    void destroy() noexcept
    {
        handle_.reset();
    }

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
        return handle_ == nullptr;
    }
};

class stack_event final
{
private:
    event handle_{};
    
public:
    using handle_type = event_handle_type;

    stack_event() = default;
    stack_event(stack_event&&) = delete;
    stack_event(const stack_event&) = delete;
    stack_event& operator=(stack_event&&) = delete;
    stack_event& operator=(const stack_event&) = delete;

    stack_event(queue_handle_type queue, evutil_socket_t fd,
        ev_flag ef, event_callback_fn fn, void *arg)
    {
        create(queue, fd, ef, fn, arg);
    }

    ~stack_event() noexcept
    {
        // without !empty you get [warn] event_del_
        if (!empty())
            event_del(handle());
    }

    // создание объекта
    void create(queue_handle_type queue, evutil_socket_t fd,
        ev_flag ef, event_callback_fn fn, void *arg)
    {
        // без метода не получится
        assert(queue && fn && empty());

        detail::check_result("event_assign",
            event_assign(&handle_, queue, fd, ef, fn, arg));
    }

    void destroy() noexcept
    {
        if (!empty())
        {
            event_del(handle());
            handle_ = {};
        }
    }

    handle_type handle() const noexcept
    {
        return const_cast<handle_type>(&handle_);
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return !event_initialized(handle());
    }
};

} // namespace e4pp
