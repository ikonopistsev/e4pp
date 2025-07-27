#pragma once

#include <openssl/sha.h>
#include <stdexcept>

namespace e4ppx {
namespace openssl {

struct sha1
{
    constexpr static std::size_t size{SHA_DIGEST_LENGTH};
    unsigned char data[size];

    void operator()(const void *buf, std::size_t size) noexcept
    {
        SHA1(reinterpret_cast<const unsigned char*>(buf), size, data);
    }

    template<class T>
    void operator()(const T& text) noexcept
    {
        this->operator()(text.data(), text.size());
    }
};

struct sha256
{
    constexpr static std::size_t size{SHA256_DIGEST_LENGTH};
    unsigned char data[size];

    void operator()(const void *buf, std::size_t size) noexcept
    {
        SHA256(reinterpret_cast<const unsigned char*>(buf), size, data);
    }

    template<class T>
    void operator()(const T& text) noexcept
    {
        this->operator()(text.data(), text.size());
    }
};

} // namepsace ssl
} // namespace btpro
