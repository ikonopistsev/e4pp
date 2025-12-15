#pragma once

#include <array>
#include <stdexcept>
#include <openssl/sha.h>

namespace e4ppx {
namespace openssl {

struct sha1
{
    constexpr static auto size{SHA_DIGEST_LENGTH};
    std::array<unsigned char, size> data{};

    void operator()(const void *buf, std::size_t size) noexcept
    {
        SHA1(reinterpret_cast<const unsigned char*>(buf), size, data.data());
    }

    template<class T>
    void operator()(const T& text) noexcept
    {
        this->operator()(text.data(), text.size());
    }
};

struct sha256
{
    constexpr static auto size{SHA256_DIGEST_LENGTH};
    std::array<unsigned char, size> data{};

    void operator()(const void *buf, std::size_t size) noexcept
    {
        SHA256(reinterpret_cast<const unsigned char*>(buf), size, data.data());
    }

    template<class T>
    void operator()(const T& text) noexcept
    {
        this->operator()(text.data(), text.size());
    }
};

} // namepsace ssl
} // namespace btpro
