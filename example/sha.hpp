#pragma once

#include <openssl/sha.h>
#include <stdexcept>

namespace btpro {
namespace ssl {

class sha1
{
public:
    using outbuf_type = unsigned char[SHA_DIGEST_LENGTH];

    sha1() = default;

    void operator()(const void *buf, size_t size, outbuf_type& out) noexcept
    {
        SHA1(reinterpret_cast<const unsigned char *>(buf), size, out);
    }

    template<class T>
    void operator()(const T& text, outbuf_type& out) noexcept
    {
        this->operator()(text.data(), text.size(), out);
    }
};

class sha256
{
public:
    using outbuf_type = unsigned char[SHA256_DIGEST_LENGTH];

    sha256() = default;

    void operator()(const void *buf, size_t size, outbuf_type& out) noexcept
    {
        SHA256(reinterpret_cast<const unsigned char *>(buf), size, out);
    }

    template<class T>
    void operator()(const T& text, outbuf_type& out) noexcept
    {
        this->operator()(text.data(), text.size(), out);
    }
};

} // namepsace ssl
} // namespace btpro
