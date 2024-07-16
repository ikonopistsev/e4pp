#include "e4pp/uri.hpp"
#include <functional>
#include <list>

namespace e4pp {

class query final
{
public:
    using fn_type = std::function<void(std::string_view, std::string_view)>;
    using pair_type = std::pair<std::string_view, fn_type>;

private:
    evkeyvalq hdr_{};
    
public:
    query() = default;
    query(const uri&) = delete;
    query& operator=(const uri&) = delete;

    explicit query(const char *query_str)
    {   
        assert(query_str);
        detail::check_result("evhttp_parse_query_str", 
            evhttp_parse_query_str(query_str, &hdr_));
    }

    explicit query(std::string_view query_str)
        : query{query_str.empty() ? "": query_str.data()}
    {   }

    template<class C>
    explicit query(const char *query_str, C& val) 
        : query(query_str)
    {
        parse(std::begin(val), std::end(val));
    }

    template<class C>
    explicit query(std::string_view query_str, C& val)
        : query{query_str.empty() ? "": query_str.data(), val}
    {   }

    template<class Iterator>
    void parse(Iterator b, Iterator e) {
        if (b != e)
        {
            for (auto h = hdr_.tqh_first; h; h = h->next.tqe_next)
            {
                const auto f = std::find_if(b, e, [=](auto& pair){
                    return pair.first == h->key;
                });
                if (f != e)
                {
                    auto& [name, fn] = *f;
                    fn(name, std::string_view{h->value});
                    std::advance(e, -1);
                    if (b == e)
                        break;
                    std::swap(*f, *e);
                }
            }
        }
    }


private:
    template<int N, int I>
    struct fill
    {
        std::array<pair_type, N>& val;

        template<class... A>
        void operator()(A... args) 
        {
            set(std::move(args)...);
        }

        void set(std::string_view key, fn_type fn) 
        {
            val[I] = std::make_pair(key, std::move(fn));
        }

        template<class... A>
        void set(std::string_view key, fn_type fn, A... args) 
        {
            set(key, std::move(fn));

            fill<N, I + 1> f{val};
            f(std::move(args)...);
        }
    };

public:
    template<class ...Args>
    static auto make(Args... A) 
    {
        constexpr auto count = sizeof...(A);
        static_assert(!(count % 2));
        constexpr auto size = count / 2;
        std::array<pair_type, size> val{};
        fill<size, 0> f{val};
        f(A...);
        return val;
    }

    ~query() noexcept
    {
        evhttp_clear_headers(&hdr_);
    }
};

} // namespace e4pp
    
