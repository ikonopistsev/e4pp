#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <functional>

namespace e4pp {
namespace util {

static bool verbose = false;

using stream_fn = std::function<std::ostream&()>;

static stream_fn stdcerr = []() noexcept -> std::ostream& {
    return std::cerr;
};
static stream_fn stdcout = []() noexcept -> std::ostream& {
    return std::cout;
};

using output_fn = std::function<std::ostream&(std::ostream&)>;

static output_fn stdoutput = [](std::ostream& os) noexcept -> std::ostream& {
    namespace c = std::chrono;
    auto n = c::system_clock::now();
    auto d = n.time_since_epoch();
    auto ms = c::duration_cast<c::milliseconds>(d).count() % 1000;
    auto t = static_cast<std::time_t>(
        c::duration_cast<c::seconds>(d).count());
    return os << std::put_time(std::gmtime(&t), "%FT%T") << '.'
        << static_cast<char>(((ms / 100) % 10) + '0')
        << static_cast<char>(((ms / 10) % 10) + '0')
        << static_cast<char>((ms % 10) + '0') << 'Z' << ' ';
};

auto& endl2(std::ostream& os) noexcept
{
    return std::endl(std::endl(os));
}

auto& cerr() noexcept
{
    return stdoutput(stdcerr());
}

auto& cout() noexcept
{
    return stdoutput(stdcout());
}

template<class F>
void trace(F fn) noexcept
{
    try
    {
        if (verbose) 
            fn();
    }
    catch(const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }
    catch(...)
    {
        cerr() << "trace" << std::endl;
    }
}

} // namespace util
} // namespace e4pp
