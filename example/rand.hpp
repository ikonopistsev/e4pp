#pragma once

#include <openssl/rand.h>
#include <stdexcept>

namespace btpro {
namespace ssl {

class rand
{
public:
    rand() = default;

    int operator()(uint8_t *buf, size_t len) noexcept
    {
        return RAND_bytes(static_cast<unsigned char *>(buf),
            static_cast<int>(len));
    }

    static void poll()
    {
        if (!RAND_poll())
            throw std::runtime_error("RAND_poll");
    }
};


} // namepsace ssl
} // namespace btpro
