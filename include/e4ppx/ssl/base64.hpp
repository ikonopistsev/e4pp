#pragma once

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdint.h>
#include <string_view>
#include <stdexcept>

namespace e4ppx {
namespace openssl {

struct b64enc
{
    int encode(const void* in, int in_len, char *out, int out_len)
    {
        int ret = 0;

        BIO *b64 = BIO_new(BIO_f_base64());
        BIO *bio = BIO_new(BIO_s_mem());
        if (b64 && bio)
        {
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            BIO_push(b64, bio);

            ret = BIO_write(b64, in, in_len);
            BIO_flush(b64);

            if (ret > 0)
                ret = BIO_read(bio, out, out_len);

            BIO_free(b64);
        }

        return ret;
    }

    std::string operator()(const void* in, int in_len)
    {
        std::string rc;
        rc.resize(2 * in_len);

        auto ret = encode(in, in_len, rc.data(), rc.size());
        if (ret > 0)
            rc.resize(static_cast<std::size_t>(ret));
        else
            rc.clear();

        return rc;
    }
};

struct b64dec
{
    int decode(const char* in, int in_len, char *out, int out_len) noexcept
    {
        int ret = 0;

        BIO *b64 = BIO_new(BIO_f_base64());
        BIO *bio = BIO_new(BIO_s_mem());
        if (b64 && bio)
        {
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            BIO_push(b64, bio);

            ret = BIO_write(bio, in, in_len);
            BIO_flush(bio);

            if (ret)
                ret = BIO_read(b64, out, out_len);

            BIO_free(b64);
        }

        return ret;
    }

    int operator()(std::string_view msg, char *out, int out_len) noexcept
    {
        return decode(msg.data(), msg.size(), out, out_len);
    }
};

} // namepsace ssl
} // namespace btpro
