#pragma once

#include "e4pp/uri.hpp"
#include "event2/http.h"
#include <functional>
#include <list>

namespace e4pp {

using kv_ptr = evkeyvalq*;

namespace detail {

struct kv_ref_allocator final
{
    constexpr static inline kv_ptr allocate() noexcept
    {
        return nullptr;
    }

    constexpr static inline void free(kv_ptr) noexcept
    {   }
};

struct kv_allocator final
{
    static auto allocate()
    {
        auto ptr = new evkeyvalq{};
        return ptr;
    }

    static void free(kv_ptr ptr) noexcept
    {
        if (ptr) {
            evhttp_clear_headers(ptr);
            delete ptr;
        }
    }
};

} // detail

template<class A>
class basic_keyval;

using keyval_ref = basic_keyval<detail::kv_ref_allocator>;
using keyval = basic_keyval<detail::kv_allocator>;

template<class A>
class basic_keyval final
{
    using this_type = basic_keyval<A>;

public:
    using handle_type = kv_ptr;

private:
    struct free_evkeyvalq
    {
        void operator()(handle_type ptr) noexcept 
        { 
            A::free(ptr);
        }
    };
    
    std::unique_ptr<evkeyvalq, free_evkeyvalq> kv_{A::allocate()};

    auto assert_handle() const noexcept
    {
        auto h = handle();
        assert(h);
        return h;
    }

public:
    auto handle() const noexcept
    {
        return kv_.get();
    }

    operator kv_ptr() const noexcept
    {
        return handle();
    }

    explicit operator keyval_ref() const noexcept
    {
        return ref();
    }

    keyval_ref ref() const noexcept
    {
        return keyval_ref(handle());
    }

    basic_keyval() = default;

    ~basic_keyval() = default;

    // only for ref
    basic_keyval(const basic_keyval& other) noexcept
        : kv_{other.handle()}
    {
        // copy only for refs
        static_assert(std::is_same<this_type, keyval_ref>::value);
    }

    // only for ref
    basic_keyval& operator=(const basic_keyval& other) noexcept
    {
        // copy only for refs
        static_assert(std::is_same<this_type, keyval_ref>::value);
        kv_.reset(other.handle());
        return *this;
    }

    basic_keyval(basic_keyval&& that) noexcept
    {
        std::swap(kv_, that.kv_);
    }

    basic_keyval& operator=(basic_keyval&& that) noexcept
    {
        std::swap(kv_, that.kv_);
        return *this;
    }

    explicit basic_keyval(kv_ptr ptr) noexcept
        : kv_{ptr}
    {
        static_assert(std::is_same<this_type, keyval_ref>::value);
        assert(ptr);
    }

    void reset(keyval other)
    {
        *this = std::move(other);
    }

    template<class Tuple>
    void parse(const Tuple& tuple_pairs) 
    {
        if constexpr (std::tuple_size_v<Tuple> > 0)
        {
            constexpr auto tuple_size = std::tuple_size_v<Tuple>;
            static_assert(tuple_size <= 64, "Tuple size must not exceed 64 elements");
            
            // Вычисляем маску "все биты установлены" на compile-time
            constexpr std::uint64_t all_processed_mask = 
                tuple_size == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << tuple_size) - 1;
            
            std::uint64_t processed_mask = 0;
            
            for (auto h = assert_handle()->tqh_first; 
                h && processed_mask != all_processed_mask; h = h->next.tqe_next)
            {
                // Один проход по tuple с проверкой битовой маски
                std::apply([&](const auto&... pairs) {
                    std::size_t current_idx = 0;
                    (void)((!(processed_mask & (1ULL << current_idx)) && std::get<0>(pairs) == h->key ? 
                        (std::get<1>(pairs)(std::get<0>(pairs), std::string_view{h->value}) ? 
                            (processed_mask |= (1ULL << current_idx), true) : true)
                        : (++current_idx, false)) || ...);
                }, tuple_pairs);
            }
        }
    }

    template<class F>
    bool find(const char* key, F fn) const
    {
        assert(key);
        auto val = evhttp_find_header(assert_handle(), key);
        if (val)
        {
            fn(std::string_view{key}, std::string_view{val});
            return true;
        }
        return false;
    }

    void clear_headers() noexcept
    {
        if (handle())
            evhttp_clear_headers(assert_handle());
    }

    void push(const char *key, const char *value)
    {
        assert(key && value);
        auto hdr = assert_handle();
        detail::check_result("evhttp_add_header",
            evhttp_add_header(hdr, key, value));
    }
   
    void push(std::string_view key, std::string_view value)
    {
        return push(key.data(), value.data());
    }

private:
    template<class... Pairs>
    static auto make_tuple_pairs(std::string_view key1, auto&& fn1, auto&&... rest)
    {
        if constexpr (sizeof...(rest) == 0) {
            return std::make_tuple(std::make_pair(key1, std::forward<decltype(fn1)>(fn1)));
        } else {
            auto current_pair = std::make_pair(key1, std::forward<decltype(fn1)>(fn1));
            auto rest_tuple = make_tuple_pairs(std::forward<decltype(rest)>(rest)...);
            return std::tuple_cat(std::make_tuple(current_pair), rest_tuple);
        }
    }

public:
    template<class ...Args>
    static auto make(Args... args) 
    {
        constexpr auto count = sizeof...(args);
        static_assert(!(count % 2), "Arguments must come in key-value pairs");
        
        if constexpr (count == 0) {
            return std::tuple<>{};
        } else {
            return make_tuple_pairs(args...);
        }
    }
};

class query final
{
    keyval kv_{};
    
public:
    query() = default;
    query(const query&) = delete;
    query& operator=(const query&) = delete;

    explicit query(const char *query_str)
    {   
        assert(query_str);
        detail::check_result("evhttp_parse_query_str", 
            evhttp_parse_query_str(query_str, kv_));
    }

    explicit query(std::string_view query_str)
        : query{query_str.empty() ? "": query_str.data()}
    {   }

    template<class Tuple>
    explicit query(const char *query_str, const Tuple& tuple_pairs) 
        : query(query_str)
    {
        parse(tuple_pairs);
    }

    template<class Tuple>
    explicit query(std::string_view query_str, const Tuple& tuple_pairs) 
        : query{query_str.empty() ? "": query_str.data(), tuple_pairs}
    {   }

    template<class Tuple>
    void parse(const Tuple& tuple_pairs) {
        kv_.parse(tuple_pairs);
    }

    template<class F>
    bool find(const char* key, F fn) const
    {
        return kv_.find(key, std::move(fn));
    }    

    ~query() = default;
};

} // namespace e4pp
    
