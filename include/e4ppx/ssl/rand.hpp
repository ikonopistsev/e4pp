#pragma once

#include <openssl/rand.h>
#include <stdexcept>

namespace e4ppx {
namespace openssl {

struct rand
{
    void operator()(unsigned char *out, std::size_t len) noexcept
    {
        RAND_bytes(out, static_cast<int>(len));
    }

    static void poll()
    {
        if (!RAND_poll())
            throw std::runtime_error("RAND_poll");
    }
};


} // namepsace ssl
} // namespace btpro
