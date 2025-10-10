#pragma once

#include "e4pp/queue.hpp"
#include "e4ppx/http/ws/handshake.hpp"
#include "e4ppx/http/ws/evwslay.hpp"

namespace e4ppx {
namespace http {
namespace ws {

template<class T>
class socket 
{
    using self_type = T;
    using protocol_fn = bool (T::*)(std::string_view);
    using data_fn = void (T::*)(wslay_opcode, const char*, std::size_t);
    using error_fn = void (T::*)(short what);

    T& self_;
    e4pp::queue& queue_;
    //evwslay adapter_{*this};
    handshake handshaker_{};
    e4pp::http::connection conn_{};

public:
    socket(self_type& self, e4pp::queue& queue)
        : self_{self}
        , queue_{queue}
    {   }

    void on_http(evhttp_request *req) 
    {
    }

    void open(const std::string& uri, 
        const std::string& protocol, auto fn) 
    {

        // auto uri = e4pp::uri{location};
        // auto host = std::string{uri.host()};
        // auto path = std::string{uri.full_path()};
        // conn_.create(queue_, host, uri.port(80));
        // conn_.on_close(closecb_fn_);
        // conn_.set_family(AF_INET);
        // conn_.set_flags(e4pp::http::con_reuse_connected_addr);

        // fn_ = fn;

        // e4pp::http::request req = e4pp::http::create_request(request_fn_);

        // auto kv = e4pp::keyval_ref{req.output_headers()};
        // kv.push("Upgrade"sv, "websocket"sv);
        // kv.push("Sec-WebSocket-Version"sv, "13"sv);
        // kv.push("Sec-WebSocket-Key"sv, hs_.create_key());
        // kv.push("Sec-WebSocket-Protocol"sv, protocol);
        // kv.push("Connection"sv, "Upgrade"sv);
        // kv.push("Pragma"sv, "no-cache"sv);
        // kv.push("Cache-Control"sv, "no-cache"sv);
        // kv.push("Host"sv, host);
        // kv.push("Origin"sv, "http://b2pm.bt02-dev.rsitedirect.com"sv);
    
        // try_connect_ = true;

        //conn_.make_request(std::move(req), EVHTTP_REQ_GET, path.c_str());
    }

    void close() 
    {
        // if (connected_) {
        //     connected_ = false;
        //     wslay_.reset();
        //     conn_.on_close(nullptr, nullptr);
        //     conn_.close();
        //     cout() << "ws closed" << std::endl;
        // }
    }
};

} // namespace ws
} // namespace http
} // namespace e4ppx
