#include "e4pp/ev.hpp"
#include "e4pp/http.hpp"
#include "e4pp/util.hpp"
#include "e4pp/thread.hpp"
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/signal?view=msvc-140
#include <signal.h>
#ifdef _WIN32
namespace {

struct wsa
{
    wsa(unsigned char h, unsigned char l)
    {
        WSADATA w;
        auto err = ::WSAStartup(MAKEWORD(h, l), &w);
        if (0 != err)
            throw std::runtime_error("WSAStartup");
    }

    ~wsa() noexcept
    {
        ::WSACleanup();
    }
};

}
#endif // _WIN32

int main()
{
    e4pp::util::verbose = true;

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

        e4pp::server srv{queue};
        e4pp::vhost localhost{queue, srv, "localhost"};
        auto bind_addr = "0.0.0.0";
        auto bind_port = 27321;

        cout() << "listen: "sv << bind_addr << ":" << bind_port << std::endl;

        srv.bind_socket(bind_addr, bind_port);
        srv.set_default_content_type("application/json");
        evhttp_set_cb(localhost, "/123", [] (evhttp_request *req, void *) {
            e4pp::buffer b;
            b.append("{\"code\":200,\"message\":\"ok\"}"sv);
            evhttp_send_reply(req, 200, "ok", b);
        }, nullptr);

        // test: curl -v http://localhost:27321/123
        queue.dispatch(std::chrono::seconds{10});

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
