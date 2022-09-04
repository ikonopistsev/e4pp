#pragma once

#include "e4pp/e4pp.hpp"
#include "event2/event_struct.h"

namespace e4pp {

struct ev_flag 
{   
    int value_{};
    
    operator int() const noexcept 
    {
        return value_;
    }

    constexpr static inline ev_flag timeout(int ef = 0) 
    {
        return { EV_TIMEOUT|ef };
    }

    constexpr static inline ev_flag interval(int ef = 0) 
    {
        return { EV_TIMEOUT|EV_PERSIST|ef };
    }
};

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
