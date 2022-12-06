#include "e4pp/uri.hpp"
#include <functional>

namespace e4pp {

class query final
{
public:
    using pair_type = std::pair<std::string_view,
        std::function<void(std::string_view, std::string_view)>>;

private:
    evkeyvalq hdr_{};
    
public:
    query() = default;
    query(const uri&) = delete;
    query& operator=(const uri&) = delete;

    explicit query(const char *query_str, std::initializer_list<pair_type> val)
    {
        assert(query_str);
        detail::check_result("evhttp_parse_query_str", 
            evhttp_parse_query_str(query_str, &hdr_));
        parse(std::move(val));
    }

    explicit query(const char *query_str)
        : query{query_str, {}}
    {   }

    void parse(std::initializer_list<pair_type> val) {
        if (val.size())
        {
            for (auto h = hdr_.tqh_first; h; h = h->next.tqe_next)
            {
                auto end = val.end();
                auto f = std::find_if(val.begin(), end, [=](auto& pair){
                    return pair.first == h->key;
                });
                if (f != end)
                {
                    auto& [name, fn] = *f;
                    fn(name, std::string_view{h->value});
                }
            }
        }
    }

    ~query() noexcept
    {
        evhttp_clear_headers(&hdr_);
    }
};

} // namespace e4pp
    
