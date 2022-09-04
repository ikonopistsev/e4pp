#include "e4pp/uri.hpp"
#include <map>

namespace e4pp {

class query final
{
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

    ~query() noexcept
    {
        evhttp_clear_headers(&hdr_);
    }
};

} // namespace e4pp
    