#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>

namespace e4pp {
namespace util {

static bool verbose = false;
static auto& stdcerr = std::cerr;
static auto& stdcout = std::cout;
static auto stdoutput = [](std::ostream& os) noexcept -> std::ostream& {
    namespace ch = std::chrono;
    auto n = ch::system_clock::now();
    auto d = n.time_since_epoch();
    auto ms = ch::duration_cast<ch::milliseconds>(d).count() % 1000;
    auto t = static_cast<std::time_t>(
        ch::duration_cast<ch::seconds>(d).count());
    auto tm = *std::gmtime(&t);
    return os << std::put_time(&tm, "%FT%T")
        << '.' << std::setfill('0') << std::setw(3) << ms << 'Z' << ' ';
};

std::ostream& endl2(std::ostream& os) noexcept
{
    return os << std::endl << std::endl;
}

std::ostream& cerr() noexcept
{
    return stdoutput(stdcerr);
}

std::ostream& cout() noexcept
{
    return stdoutput(stdcout);
}

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

} // namespace util
} // namespace e4pp
