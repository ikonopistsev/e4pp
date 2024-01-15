#include "e4pp/ev.hpp"
#include "e4pp/http.hpp"
#include "e4pp/util.hpp"
#include "e4pp/thread.hpp"
#include "e4pp/query.hpp"
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/signal?view=msvc-140
#include <signal.h>
#include <list>
#include <vector>
#include <algorithm>

namespace {

#ifdef _WIN32

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

#endif // _WIN32

e4pp::util::output u;

inline std::ostream& cerr() noexcept
{
    return u.cerr();
}

inline std::ostream& cout() noexcept
{
    return u.cout();
}

template<class F>
void do_trace(F fn) noexcept
{
    u.do_trace(std::move(fn));
}

template<class F>
void trace(F fn) noexcept
{
    u.trace(std::move(fn));
}

}

int main()
{
    u.verbose = true;

    using namespace e4pp::util;
    using namespace std::literals;

    try
    {
#ifndef _WIN32
        e4pp::config cfg{e4pp::ev_feature_et|
            e4pp::ev_feature_01|e4pp::ev_feature_early_close};
#else
        wsa w{2, 2};
        e4pp::use_threads();
        e4pp::config cfg{e4pp::ev_base_startup_iocp};
#endif // _WIN32

        e4pp::query q{"key1=val1&key2=val2&key3=val3", {
            { "key1"sv, [&](auto, auto val) {
                  cout() << val << std::endl;
            }},
            { "key2"sv, [&](auto key, auto val) {
                  cout() << key << ':' << val << std::endl;
            }}
        }};

        e4pp::queue queue{cfg};
        auto f = [&](auto, auto){
            queue.loop_break();
        };
        e4pp::ev_stack sint;
        sint.create(queue, SIGINT, e4pp::ev_signal|e4pp::ev_persist, f);
        sint.add();
        e4pp::ev_stack sterm;
        sterm.create(queue, SIGTERM, e4pp::ev_signal|e4pp::ev_persist, f);
        sterm.add();

        e4pp::http::server srv{queue};
        e4pp::http::vhost localhost{queue, srv, "localhost"};
        auto bind_addr = "0.0.0.0";
        auto bind_port = 27321;

        cout() << "listen: "sv << bind_addr << ":" << bind_port << std::endl;

        srv.bind_socket(bind_addr, bind_port);
        srv.set_default_content_type("application/json");
        srv.set_allowed_methods(e4pp::http::method::post);
        srv.set_timeout(std::chrono::seconds{80});
        srv.set_flags(e4pp::http::lingering_close);
        evhttp_set_cb(localhost, "/123", [](evhttp_request *req, void* q) {
            cout() << "reply_start"sv << std::endl;
            evhttp_send_reply_start(req, HTTP_OK, "ok");
            auto& queue = *reinterpret_cast<e4pp::queue*>(q);
            queue.once(std::chrono::seconds{1}, [&, req]{
                e4pp::buffer b;
                b.append("{\"code\":200,\"message\":\"ok\"}"sv);
                cout() << "reply_chunk"sv << std::endl;
                evhttp_send_reply_chunk(req, b);
                queue.once(std::chrono::seconds{1}, [&, req]{
                    cout() << "reply_end"sv << std::endl;
                    evhttp_send_reply_end(req);
                });
            });
        }, &queue);

        // test: curl -v http://localhost:27321/123
        // http post 1 byte per sec
        // curl -v http://localhost:27321/123 -d '1234567890' --limit-rate 1
        queue.dispatch(std::chrono::seconds{80});

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
