#pragma once

#include "e4pp/e4pp.hpp"
#include "event2/event_struct.h"

namespace e4pp {

class heap_event
{
public:
    using handle_type = event_handle_type;

private:
    handle_type handle_{ nullptr };

public:
    heap_event() = default;
    heap_event(const heap_event&) = delete;
    heap_event& operator=(const heap_event&) = delete;

    heap_event(heap_event&& that) noexcept
    {
        assert(this != &that);
        std::swap(handle_, that.handle_);
    }

    heap_event& operator=(heap_event&& that) noexcept
    {
        assert(this != &that);
        std::swap(handle_, that.handle_);
        return *this;
    }

    // generic
    heap_event(queue_handle_type queue, evutil_socket_t fd, 
        event_flag ef, event_callback_fn fn, void *arg)
        : handle_{ detail::check_pointer("event_new", 
            event_new(queue, fd, ef, fn, arg)) }
    {
        assert(queue && fn);
    }

    ~heap_event() noexcept
    {
        if (handle_)
            event_free(handle_);
    }

    // захват хэндла
    explicit heap_event(handle_type handle) noexcept
        : handle_{handle}
    {
        assert(handle);
    }

    void attach(handle_type handle) noexcept
    {
        handle_ = handle;
    }

    // создание объекта
    void create(queue_handle_type queue, evutil_socket_t fd, 
        event_flag ef, event_callback_fn fn, void *arg)
    {
        // если создаем повторно поверх
        // значит что-то пошло не так
        assert(queue && fn && !handle_);

        // создаем объект эвента
        handle_ = detail::check_pointer("event_new",
            event_new(queue, fd, ef, fn, arg));
    }

    void destroy() noexcept
    {
        if (handle_)
        {
            event_free(handle_);
            handle_ = nullptr;
        }
    }

    handle_type handle() const noexcept
    {
        return handle_;
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

class stack_event
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
        event_flag ef, event_callback_fn fn, void *arg)
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
        event_flag ef, event_callback_fn fn, void *arg)
    {
        // без метода не получится
        assert(queue && fn && empty());

        detail::check_result("event_assign",
            event_assign(&handle_, queue, fd, ef, fn, arg));
    }

    void destroy() noexcept
    {
        event_del(handle());
        handle_ = event{};
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
