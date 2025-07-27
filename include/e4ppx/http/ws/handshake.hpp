#pragma once

#include <string>

#include "e4pp/query.hpp"
#include "e4pp/http/request.hpp"
#include "e4ppx/ssl/base64.hpp"
#include "e4ppx/ssl/rand.hpp"
#include "e4ppx/ssl/sha.hpp"
#include <event2/http.h>

namespace e4ppx {
namespace http {
namespace ws {

struct handshake 
{
    std::string key_{};

    static inline std::string create_accept_key(const std::string& client_key) 
    {
        static constexpr std::string_view wsuuid{"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"};
        std::string str;
        str.reserve(client_key.size() + wsuuid.size());
        str += client_key;
        str += wsuuid;

        openssl::sha1 f;
        f(str);

        openssl::b64enc b64;
        return b64(f.data, f.size);
    }

    static inline std::string create_client_key() 
    {
        std::array<char, 16> ch;
        openssl::rand rnd;
        rnd(reinterpret_cast<uint8_t*>(ch.data()), ch.size());
        openssl::b64enc b64;
        return b64(ch.data(), ch.size());
    }    

    std::string create_key() noexcept
    {
        auto key = create_client_key();
        key_ = create_accept_key(key);
        return key;
    }

    bool accept(const char *header) const noexcept {
        return key_ == header;
    }

    bool accept(const e4pp::http::request_ref& req) const noexcept
    {
        bool is_valid = false;

        auto code = req.get_response_code();
        if (code != HTTP_SWITCH_PROTOCOLS) {
            return is_valid;
        }

        auto kv = e4pp::keyval_ref{req.input_headers()};
        kv.find("sec-websocket-accept", [&](auto, auto val) {
            is_valid = accept(val);
        });
        
        return is_valid;
    }

    void clear() noexcept
    {
        key_ = {};
    }
};

} // namespace ws
} // namespace http
} // namespace e4pp


