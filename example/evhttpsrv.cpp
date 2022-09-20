#include "e4pp/http.hpp"
#include "e4pp/util.hpp"
#include "e4pp/evtype.hpp"
#include "e4pp/buffer_event.hpp"

int main()
{
    e4pp::util::verbose = true;

    using namespace e4pp::util;
    using namespace std::literals;

    try
    {
        e4pp::config cfg{e4pp::ev_feature_et};
        e4pp::queue queue{cfg};
        e4pp::server srv{queue};
        e4pp::vhost localhost{queue, srv, "localhost"};
        constexpr auto bind_addr = "0.0.0.0";
        constexpr auto bind_port = 27321;

        cout() << "listen: "sv << bind_addr << ":" << bind_port << std::endl;

        srv.bind_socket(bind_addr, bind_port);
        evhttp_set_cb(localhost, "/123", [] (evhttp_request *req, void *) {
            e4pp::buffer b;
            b.append("ok!"sv);
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