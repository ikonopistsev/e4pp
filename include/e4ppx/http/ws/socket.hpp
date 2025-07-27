#pragma once

#include "e4pp/queue.hpp"
#include "e4ppx/http/ws/handshake.hpp"
#include "e4ppx/http/ws/evwslay.hpp"

namespace e4ppx {
namespace http {
namespace ws {

struct socket {
    e4pp::queue& queue_;
    e4ppx::http::ws::handshake hs_;
    std::function<void(wslay_opcode opcode, 
        const char *ptr, std::size_t len)> fn_{};
    std::function<void(short what)> error_{};

    struct wslay_free {
        void operator()(wslay_event_context_ptr ptr) {
            wslay_event_context_free(ptr);
        }
    };

    void close_connection(e4pp::http::connection_ref conn) {
        on_close();
    }
    e4pp::http::closecb_fn<websocket> closecb_fn_{&websocket::close_connection, *this};

    void request_error(evhttp_request_error error) {
        on_close();
    }
    void on_request(e4pp::http::request_ref req) {
        auto http_code = evhttp_request_get_response_code(req);
        if (http_code == HTTP_SWITCH_PROTOCOLS) {
            connected_ = true;
            on_http(req);
        }
        else
        {
            cerr() << "WebSocket connection failed with HTTP code: "
                   << http_code << std::endl;

            e4pp::http::request req = e4pp::http::create_request(request_fn_);

            auto kv = e4pp::keyval_ref{req.output_headers()};
            kv.push("Upgrade"sv, "websocket"sv);
            kv.push("Sec-WebSocket-Version"sv, "13"sv);
            kv.push("Sec-WebSocket-Key"sv, hs_.create_key());
            kv.push("Sec-WebSocket-Protocol"sv, "SnapshotFullRefresh2"sv);
            kv.push("Connection"sv, "Upgrade"sv);
            kv.push("Pragma"sv, "no-cache"sv);
            kv.push("Cache-Control"sv, "no-cache"sv);
            kv.push("Host"sv, "b2pm.bt02-dev.rsitedirect.com"sv);
            kv.push("Origin"sv, "http://b2pm.bt02-dev.rsitedirect.com"sv);
        
            try_connect_ = true;

            conn_.make_request(std::move(req), EVHTTP_REQ_GET, "/prime-dev/");                   
        }
    }
    e4pp::http::request_fn<websocket> request_fn_{&websocket::on_request, 
        &websocket::request_error, *this};

    struct adapter {
        websocket& ws;
        bufferevent* bev{nullptr};
        wslay_event_context_ptr wslay{nullptr};
        ssize_t read(uint8_t *data, size_t len) {
            auto buf = e4pp::buffer_ref{
                bufferevent_get_input(e4pp::assert_handle(bev))};
            auto size = buf.copyout(data, len);
            buf.drain(size);
            return size;
        }
        ssize_t write(const uint8_t *data, size_t len) {
            assert(bev);
            auto buf = e4pp::buffer_ref{
                bufferevent_get_output(e4pp::assert_handle(bev))};
            buf.append(data, len);
            io_update();
            return len;
        }
        void setup(bufferevent* b) {
            assert(b);
            bufferevent_setcb(b, adapter::readcb, adapter::writecb, adapter::eventcb, this);
            bev = b;
        }
        void forward_msg(wslay_opcode opcode, const uint8_t *ptr, std::size_t len) {
            ws.message(opcode, reinterpret_cast<const char*>(ptr), len);
            io_update();
        }
        static ssize_t recv_cb(wslay_event_context_ptr ctx,
            uint8_t *data, size_t len, int flags, void *user_data) {
            assert(user_data);
            auto a = static_cast<adapter*>(user_data);
            return a->read(data, len);
        }
        static ssize_t send_cb(wslay_event_context_ptr ctx, 
            const uint8_t *data, size_t len, int flags, void *user_data) {
            assert(user_data);
            return static_cast<adapter*>(user_data)->write(data, len);
        }
        static void on_msg_recv_cb(wslay_event_context_ptr ctx,
            const struct wslay_event_on_msg_recv_arg *arg, void *user_data) {
            assert(user_data);
            static_cast<adapter*>(user_data)->forward_msg(
                static_cast<wslay_opcode>(arg->opcode),
                    arg->msg, arg->msg_length);
        }
        void event_read() {
            wslay_event_recv(wslay);
        }
        void event_write() {
            wslay_event_send(wslay);
        }
        void event_network(short what) {
            ws.error(what);
            ws.on_close();
        }
        static void readcb(bufferevent*, void *user_data) {
            assert(user_data);
            static_cast<adapter*>(user_data)->event_read(); 
        }
        static void writecb(bufferevent*, void *user_data) {
            assert(user_data);
            static_cast<adapter*>(user_data)->event_write();             
        }
        static void eventcb(bufferevent*, short what, void *user_data) {
            assert(user_data);
            static_cast<adapter*>(user_data)->event_network(what);    
        }
        void io_update() {
            assert(bev);
            short ev = wslay_event_want_read(wslay) ? EV_READ : 0;
                ev |= wslay_event_want_write(wslay) ? EV_WRITE : 0;
            bufferevent_enable(bev, ev);
        }
    };

    void message(wslay_opcode opcode, const char *ptr, std::size_t len) {
        if (fn_)
            fn_(opcode, ptr, len);
    }

    void error(short what) {
        if (error_)
            error_(what);
    }

    static wslay_event_context_ptr create_client_wslay(adapter& op) {
        auto cb = wslay_event_callbacks{
            adapter::recv_cb,
            adapter::send_cb,
            [](wslay_event_context_ptr ctx, 
                uint8_t *buf, size_t len, void*) {
                e4ppx::openssl::rand rnd;
                rnd(buf, len);
                return int{};
            },
            nullptr, /* on_frame_recv_start_callback */
            nullptr, /* on_frame_recv_callback */
            nullptr, /* on_frame_recv_end_callback */
            adapter::on_msg_recv_cb,
        };

        wslay_event_context_client_init(&op.wslay, &cb, &op);
        return op.wslay;
    }
    adapter wsbev_{*this};
    e4pp::http::connection conn_{};
    std::unique_ptr<wslay_event_context, wslay_free>
        wslay_{create_client_wslay(wsbev_)};
    bool try_connect_{false};
    bool connected_{false};
    bool closed_on_connect_{false};

    websocket(e4pp::queue& queue)
        : queue_{queue}
    {   }

    void on_http(evhttp_request *req) {
        if (closed_on_connect_) {
            return;
        }

        if (hs_.accept(e4pp::http::request_ref{req})) {
            cout() << "ws handshake successful!" << std::endl;

            wsbev_.setup(conn_.buffer_event());                
            write("{\"login\":\"cylive\",\"password_hash\":\"54a79007858bbcdffd97ad4cf3d4f24645ebd467\",\"symbols\":[]}"sv);
        } else {
            cout() << "ws handshake failed" << std::endl;
            
            on_close();
        }

        hs_.clear();
    }

    void write(uint8_t opcode, const char *data, std::size_t len) {
        auto msg = wslay_event_msg{
            opcode, reinterpret_cast<const uint8_t*>(data), len};

        auto wsh = wslay_.get();
        wslay_event_queue_msg(wsh, &msg);
        wsbev_.io_update();
    }

    void write(std::string_view text) {
        write(WSLAY_TEXT_FRAME, text.data(), text.size());
    }

    void on_close() {
        if (try_connect_) {
            try_connect_ = false;
            closed_on_connect_ = true;
        }
        cout() << "close conn!" << std::endl;
        close();
    }

    void open(const std::string& location, const std::string& protocol, auto fn) {

        auto uri = e4pp::uri{location};
        auto host = std::string{uri.host()};
        auto path = std::string{uri.full_path()};
        conn_.create(queue_, host, uri.port(80));
        conn_.on_close(closecb_fn_);
        conn_.set_family(AF_INET);
        conn_.set_flags(e4pp::http::con_reuse_connected_addr);

        fn_ = fn;

        e4pp::http::request req = e4pp::http::create_request(request_fn_);

        auto kv = e4pp::keyval_ref{req.output_headers()};
        kv.push("Upgrade"sv, "websocket"sv);
        kv.push("Sec-WebSocket-Version"sv, "13"sv);
        kv.push("Sec-WebSocket-Key"sv, hs_.create_key());
        kv.push("Sec-WebSocket-Protocol"sv, protocol);
        kv.push("Connection"sv, "Upgrade"sv);
        kv.push("Pragma"sv, "no-cache"sv);
        kv.push("Cache-Control"sv, "no-cache"sv);
        kv.push("Host"sv, host);
        kv.push("Origin"sv, "http://b2pm.bt02-dev.rsitedirect.com"sv);
    
        try_connect_ = true;

        conn_.make_request(std::move(req), EVHTTP_REQ_GET, path.c_str());
    }

    void close() {
        if (connected_) {
            connected_ = false;
            wslay_.reset();
            conn_.on_close(nullptr, nullptr);
            conn_.close();
            cout() << "ws closed" << std::endl;
        }
    }
};

} // namespace ws
} // namespace http
} // namespace e4ppx
