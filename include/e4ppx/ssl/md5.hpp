#pragma once

#include <array>
#include <stdexcept>
#include <openssl/md5.h>

namespace e4ppx {
namespace openssl {

struct md5
{
    constexpr static auto size{MD5_DIGEST_LENGTH};
    std::array<unsigned char, size> hash{};

    void operator()(const void *buf, std::size_t size) noexcept
    {
        MD5(reinterpret_cast<const unsigned char*>(buf), size, hash.data());
    }

    template<class T>
    void operator()(const T& text) noexcept
    {
        this->operator()(text.data(), text.size());
    }    

    auto begin() const noexcept
    {
        return hash.begin();
    }

    auto data() const noexcept
    {
        return hash.data();
    }

    auto end() const noexcept
    {
        return hash.end();
    }
};

} // namespace ssl
} // namespace e4ppx
