#include "e4pp/ev.hpp"
#include "e4pp/http.hpp"
#include "e4pp/util.hpp"
#include "e4pp/thread.hpp"
#include "e4pp/query.hpp"
#include "e4pp/buffer_event.hpp"
#include <wslay/wslay.h>
#include <event2/http_struct.h>
#include <array>
#include "base64.hpp"
#include "rand.hpp"
#include "sha.hpp"
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/signal?view=msvc-140
#include <signal.h>
#include <list>
#include <vector>
#include <algorithm>
#ifdef _WIN32
namespace {

struct wsa
{
    wsa(unsigned char h, unsigned char l)
    {
        WSADATA w;
        auto err = ::WSAStartup(MAKEWORD(h, l), &w);
        if (0 != err)
            throw std::runtime_error("::WSAStartup");
    }

    ~wsa() noexcept
    {
        ::WSACleanup();
    }
};

}
#endif // _WIN32

using namespace std::literals;
constexpr auto wsuuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"sv;

std::string create_accept_key(const std::string& client_key) {
    std::string str;
    str.reserve(client_key.size() + wsuuid.size());
    str += client_key;
    str += wsuuid;
    btpro::ssl::sha1::outbuf_type out;
    btpro::ssl::sha1 f;
    f(str, out);
    btpro::ssl::base64 b64;
    return b64.encode(out, sizeof(out));
}

std::string create_client_key() {
    std::array<char, 16> ch;
    btpro::ssl::rand rnd;
    rnd(reinterpret_cast<uint8_t*>(ch.data()), ch.size());
    btpro::ssl::base64 b64;
    return b64.encode(ch.data(), ch.size());
}

auto create_queue()
{
#ifndef _WIN32
    auto cfg = e4pp::config{e4pp::ev_feature_et|
        e4pp::ev_feature_01|e4pp::ev_feature_early_close};
#else
    wsa w{2, 2};
    e4pp::use_threads();
    auto cfg = e4pp::config{e4pp::ev_base_startup_iocp};
#endif // _WIN32

    return e4pp::queue{cfg};
}

struct websocket {
    e4pp::queue& queue_;
    std::string client_key_{};
    std::function<void(wslay_opcode opcode, 
        const char *ptr, std::size_t len)> fn_{};
    std::function<void(short what)> error_{};

    struct deallocate {
        void operator()(evhttp_connection *ptr) {
            evhttp_connection_free(ptr);
        }
        void operator()(wslay_event_context_ptr ptr) {
            wslay_event_context_free(ptr);
        }
    };

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
            return static_cast<adapter*>(user_data)->read(data, len);
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
            bufferevent_enable(bev, ev |
                wslay_event_want_write(wslay) ? EV_WRITE : 0);
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
                btpro::ssl::rand rnd;
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
    std::unique_ptr<evhttp_connection, deallocate> 
        conn_{nullptr};
    std::unique_ptr<wslay_event_context, deallocate>
        wslay_{create_client_wslay(wsbev_)};

    websocket(e4pp::queue& queue)
        : queue_{queue}
    {   }

    void on_http(evhttp_request *req) {
        auto accept_header = create_accept_key(client_key_);
        if (accept_header == evhttp_find_header(req->input_headers, "Sec-WebSocket-Accept")) {
            wsbev_.setup(evhttp_connection_get_bufferevent(conn_.get()));
            write("{\"login\":\"cylive\",\"password_hash\":\"54a79007858bbcdffd97ad4cf3d4f24645ebd467\",\"symbols\":[]}"sv);
        } else {
            e4pp::util::cout() << "ws OFF" << std::endl;
        }
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

    void open(const std::string& location, const std::string& protocol, auto fn) {

        auto uri = e4pp::uri{location};
        auto host = uri.host();
        auto path = std::string{uri.full_path()};
        conn_.reset(evhttp_connection_base_new(queue_, 
            nullptr, host.data(), uri.port(80)));

        fn_ = fn;

        auto req = evhttp_request_new([](evhttp_request *req, void *user_data) {
            auto& ws = *static_cast<websocket*>(user_data);
            ws.on_http(req);
        }, this);

        client_key_ = create_client_key();
    
        evhttp_add_header(req->output_headers, "Host", host.data());
        evhttp_add_header(req->output_headers, "Origin", "http://b2pm.bt02-dev.rsitedirect.com");
        evhttp_add_header(req->output_headers, "Sec-WebSocket-Version", "13");
        evhttp_add_header(req->output_headers, "Sec-WebSocket-Protocol", protocol.c_str());
        evhttp_add_header(req->output_headers, "Sec-WebSocket-Key", client_key_.c_str());
        evhttp_add_header(req->output_headers, "Connection", "keep-alive, Upgrade");
        evhttp_add_header(req->output_headers, "Pragma", "no-cache");
        evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
        evhttp_add_header(req->output_headers, "Upgrade", "websocket");
        evhttp_make_request(conn_.get(), req, EVHTTP_REQ_GET, path.c_str());
    }
};

int main()
{
    e4pp::util::verbose = true;

    using namespace e4pp::util;

    try
    {
        auto queue = create_queue();

        auto f = [&](auto, auto){
            queue.loop_break();
        };
        e4pp::ev_stack sint;
        sint.create(queue, SIGINT, e4pp::ev_signal|e4pp::ev_persist, f);
        sint.add();
        e4pp::ev_stack sterm;
        sterm.create(queue, SIGTERM, e4pp::ev_signal|e4pp::ev_persist, f);
        sterm.add();

        websocket ws{queue};
        ws.open("ws://b2pm.bt02-dev.rsitedirect.com/prime-dev/?u=test.client#mytest", 
            "SnapshotFullRefresh2", [&](wslay_opcode opcode, 
                const char *ptr, std::size_t len) {
                if (opcode == WSLAY_TEXT_FRAME) {
                    std::string_view text{ptr, len};
                    cout() << text << std::endl;
                    constexpr auto p = "{\"ping\":\""sv;
                    auto f = text.find(p);
                    if (f != std::string_view::npos) {
                        std::string pong{"{\"pong\":\""};
                        pong += text.substr(p.size());
                        ws.write(pong);
                    }
                } else
                    cout() << "recv: " << opcode << std::endl;
        });

        queue.dispatch();

        trace([]{
            return "bye"sv;
        });
    }
    catch (const std::exception& e)
    {
        cerr() << e.what() << std::endl;
    }

    return 0;
}
