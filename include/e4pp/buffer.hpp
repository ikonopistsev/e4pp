#pragma once

#include "e4pp/e4pp.hpp"
#include "event2/buffer.h"

#include <mutex>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <type_traits>

namespace e4pp {

using evbufer_ptr = evbuffer*;

namespace detail {

struct ref_allocator
{
    constexpr static inline evbufer_ptr allocate() noexcept
    {
        return nullptr;
    }

    constexpr static inline void free(evbufer_ptr) noexcept
    {   }
};

struct buf_allocator
{
    static auto allocate()
    {
        return detail::check_pointer("evbuffer_new", 
            evbuffer_new());
    }

    static void free(evbufer_ptr ptr) noexcept
    {
        evbuffer_free(ptr);
    }
};

} // detail

template<class A>
class basic_buffer;

using buffer_ref = basic_buffer<detail::ref_allocator>;
using buffer = basic_buffer<detail::buf_allocator>;

template<class A>
class basic_buffer
{
    using this_type = basic_buffer<A>;

private:
    evbufer_ptr handle_{A::allocate()};

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    auto handle() const noexcept
    {
        return handle_;
    }

    operator evbufer_ptr() const noexcept
    {
        return handle();
    }

    operator buffer_ref() const noexcept
    {
        return ref();
    }

    buffer_ref ref() const noexcept
    {
        return buffer_ref(handle());
    }

    basic_buffer() = default;

    ~basic_buffer() noexcept
    {
        A::free(handle());
    }

    // only for ref
    // this delete copy ctor for buffer&
    basic_buffer(const basic_buffer& other) noexcept
        : handle_{other.handle()}
    {
        // copy only for refs
        static_assert(std::is_same<this_type, buffer_ref>::value);
    }

    // only for ref
    // this delete copy ctor for buffer&
    basic_buffer& operator=(const basic_buffer& other) noexcept
    {
        // copy only for refs
        static_assert(std::is_same<this_type, buffer_ref>::value);
        handle_ = other.handle();
        return *this;
    }

    basic_buffer(basic_buffer&& that) noexcept
    {
        std::swap(handle_, that.handle_);
    }

    basic_buffer& operator=(basic_buffer&& that) noexcept
    {
        std::swap(handle_, that.handle_);
        return *this;
    }

    explicit basic_buffer(evbufer_ptr ptr) noexcept
        : handle_{ptr}
    {
        static_assert(std::is_same<this_type, buffer_ref>::value);
        assert(ptr);
    }

    void reset(buffer other)
    {
        *this = std::move(other);
    }

    void add_file(int fd, ev_off_t offset, ev_off_t length)
    {
        detail::check_result("evbuffer_add_file",
            evbuffer_add_file(assert_handle(), fd, offset, length));
    }

    void append_ref(const void *data, std::size_t len,
        evbuffer_ref_cleanup_cb cleanupfn, void *cleanupfn_arg)
    {
        assert(data && len);
        detail::check_result("evbuffer_add_reference",
            evbuffer_add_reference(assert_handle(), data, len,
                cleanupfn, cleanupfn_arg));
    }

    void append_ref(const void *data, std::size_t len)
    {
        append_ref(data, len, nullptr, nullptr);
    }

    template<std::size_t N>
    void append_ref(std::reference_wrapper<const char[N]> data_ref)
    {
        append_ref(data_ref, N - 1);
    }

    template<template<class, class...>
        class Str, class Ch, class ... R>
    void append_ref(const Str<Ch, R...>& text)
    {
        append_ref(text.data(), text.size());
    }

    template<template<class, std::size_t>
        class Str, class Ch, std::size_t Sz>
    void append_ref(const Str<Ch, Sz>& text)
    {
        append_ref(text.data(), text.size());
    }

    void append(buffer_ref) = delete;
    void append(buffer buf)
    {
        detail::check_result("evbuffer_add_buffer",
            evbuffer_add_buffer(assert_handle(), buf));
    }

    void append(const void *data, std::size_t len)
    {
        assert(data && len);
        detail::check_result("evbuffer_add",
            evbuffer_add(assert_handle(), data, len));
    }

    void append(const void *data, std::size_t len, bool ref)
    {
        (ref) ?
            append_ref(data, len) : append(data, len);
    }

    template<std::size_t N>
    void append(std::reference_wrapper<const char[N]> data_ref)
    {
        append_ref(data_ref.get(), N - 1);
    }

    template<template<class, class...>
        class Str, class Ch, class ... R>
    void append(const Str<Ch, R...>& text)
    {
        append(text.data(), text.size());
    }

    template<template<class, std::size_t>
        class Str, class Ch, std::size_t Sz>
    void append(const Str<Ch, Sz>& text)
    {
        append(text.data(), text.size());
    }

    template<class T>
    void append(std::reference_wrapper<const T> data_ref)
    {
        const auto& str_buf = data_ref.get();
        append_ref(str_buf.data(), str_buf.size());
    }

    // Prepends data to the beginning of the evbuffer
    void prepend(buffer_ref) = delete;
    void prepend(buffer buf)
    {
        detail::check_result("evbuffer_prepend_buffer",
            evbuffer_prepend_buffer(assert_handle(), buf));
    }

    // Prepends data to the beginning of the evbuffer
    void prepend(const void *data, std::size_t len)
    {
        assert((nullptr != data) && len);
        detail::check_result("evbuffer_prepend",
            evbuffer_prepend(assert_handle(), data, len));
    }

    // Prepends data to the beginning of the evbuffer
    template<std::size_t N>
    void prepend(std::reference_wrapper<const char[N]> data_ref)
    {
        prepend(data_ref.get(), N - 1);
    }

    template<template<class, class...>
        class Str, class Ch, class ... R>
    void prepend(const Str<Ch, R...>& text)
    {
        prepend(text.data(), text.size());
    }

    template<template<class, std::size_t>
        class Str, class Ch, std::size_t Sz>
    void prepend(const Str<Ch, Sz>& text)
    {
        prepend(text.data(), text.size());
    }

    // Remove a specified number of bytes data from the beginning of an evbuffer.
    std::size_t drain(std::size_t len)
    {
        detail::check_result("evbuffer_drain",
            evbuffer_drain(assert_handle(), len));
        return size();
    }

    // Remove a specified number of bytes data from the beginning of an evbuffer.
    std::size_t drain(std::string& text, std::size_t max_len)
    {
        std::size_t total = (std::min)(size(), max_len);
        if (total)
        {
            text.resize(total);
            copyout(text.data(), total);
            drain(total);
        }
        return total;
    }

    // Expands the available space in the evbuffer to at least datlen,
    // so that appending datlen additional bytes
    // will not require any new allocations
    void expand(std::size_t size)
    {
        detail::check_result("evbuffer_expand",
            evbuffer_expand(assert_handle(), size));
    }

    std::size_t drain(std::string& text)
    {
        return drain(text, size());
    }

    // Read data from an evbuffer, and leave the buffer unchanged.
    std::size_t copyout(void *out, std::size_t len) const
    {
        assert(out && len);
        return detail::check_size("evbuffer_copyout",
            evbuffer_copyout(assert_handle(), out, len));
    }

    // Makes the data at the beginning of an evbuffer contiguous.
    unsigned char* pullup(ev_ssize_t len)
    {
        // @return a pointer to the contiguous memory array, or NULL
        // if param size requested more data than is present in the buffer.
        assert(len <= static_cast<ev_ssize_t>(size()));

        unsigned char *result = evbuffer_pullup(assert_handle(), len);
        if (!result)
        {
            if (empty())
            {
                static unsigned char nil;
                result = &nil;
            }
            else
                throw std::runtime_error("evbuffer_pullup");
        }
        return result;
    }

    // Returns the total number of bytes stored in the evbuffer
    std::size_t size() const noexcept
    {
        return evbuffer_get_length(assert_handle());
    }

    // Returns the number of contiguous available bytes in the first buffer chain.
    std::size_t contiguous_space() const noexcept
    {
        return evbuffer_get_contiguous_space(assert_handle());
    }

    void enable_locking()
    {
        detail::check_result("evbuffer_enable_locking",
            evbuffer_enable_locking(assert_handle(), nullptr));
    }

    void lock() const noexcept
    {
        evbuffer_lock(assert_handle());
    }

    void unlock() const noexcept
    {
        evbuffer_unlock(assert_handle());
    }

    template<typename T>
    void sync(T&& func)
    {
        std::lock_guard<basic_buffer> l(*this);
        func(*this);
    }

    std::vector<char> vector() const
    {
        std::vector<char> vec;
        std::size_t len = size();
        if (len)
        {
            vec.resize(len);
            copyout(vec.data(), len);
        }
        return vec;
    }

    std::string str() const
    {
        std::string str;
        std::size_t len = size();
        if (len)
        {
            str.resize(len);
            copyout(str.data(), len);
        }
        return str;
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    // Read data from an evbuffer and drain the bytes read.
    std::size_t remove(void *out, std::size_t len) const
    {
        assert(out && len);
        return detail::check_size("evbuffer_remove",
            evbuffer_remove(assert_handle(), out, len));
    }

    // Read data from an evbuffer and drain the bytes read.
    std::size_t remove(basic_buffer& out, std::size_t len) const
    {
        return detail::check_size("evbuffer_remove_buffer",
            evbuffer_remove_buffer(assert_handle(), out, len));
    }

    // Read data from an evbuffer and drain the bytes read.
    std::size_t remove(basic_buffer& out) const
    {
        auto len = size();
        return (len) ?
            remove(out, len) : len;
    }

    int peek(evbuffer_iovec* vec_out, int n_vec) noexcept
    {
        return evbuffer_peek(assert_handle(), -1, nullptr, vec_out, n_vec);
    }

    int read(evutil_socket_t fd, int howmuch)
    {
        return evbuffer_read(assert_handle(), fd, howmuch);
    }

    int write(evutil_socket_t fd)
    {
        return evbuffer_write(assert_handle(), fd);
    }

    int write(buffer buf)
    {
        auto size = buf.size();
        buf.write(std::move(*this));
        return size;
    }
};

} // namespace e4pp
