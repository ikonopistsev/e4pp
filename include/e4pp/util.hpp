#pragma once

#include <chrono>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <functional>

namespace e4pp {
namespace util {
namespace detail {

static auto inline std_output() {
    return [](std::ostream& os) noexcept -> std::ostream& {
        return os;
    };
} 

static auto inline std_timestamp_output() {
    return [](std::ostream& os) noexcept -> std::ostream& {
        namespace c = std::chrono;
        auto n = c::system_clock::now();
        auto d = n.time_since_epoch();
        auto ms = c::duration_cast<c::milliseconds>(d).count() % 1000;
        auto t = static_cast<std::time_t>(
            c::duration_cast<c::seconds>(d).count());
        return os << std::put_time(std::gmtime(&t), "%FT%T")
            << '.' << std::setfill('0') << std::setw(3) << ms << 'Z' << ' ';
    };
}

} // namespace detail

struct output final
{
    using output_type = std::function<std::ostream&(std::ostream&)>;
    output_type stream{detail::std_output()};
    bool verbose{false};
    std::ostream* outptr{&std::cout};
    std::ostream* errptr{outptr};
    
    std::ostream& cerr() 
    {
        assert(outptr);
        return stream(*outptr);
    };

    std::ostream& cout() 
    {
        assert(errptr);
        return stream(*errptr);
    };

    template<class F>
    void do_trace(F fn) noexcept
    {
        try
        {
            if (verbose)
                fn();
        }
        catch (const std::exception& e)
        {
            cerr() << e.what() << std::endl;
        }
    }

    template<class F>
    void trace(F fn) noexcept
    {
        do_trace([&]{
            cout() << fn() << std::endl;
        });
    }
};

} // namespace util
} // namespace e4pp
