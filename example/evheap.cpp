#include "e4pp/ev.hpp"
#include "e4pp/uri.hpp"
#include "e4pp/buffer.hpp"
#include "e4pp/queue.hpp"
#include "e4pp/thread.hpp"
#include "e4pp/listener.hpp"
#include "e4pp/buffer_event.hpp"

#include <thread>
#include <iostream>
#include <signal.h>
#include <cassert>
#include <chrono>
#include <arpa/inet.h>

std::ostream& output(std::ostream& os)
{
    return os;
}

inline std::ostream& cerr()
{
    return output(std::cerr);
}

inline std::ostream& cout()
{
    return output(std::cout);
}

using namespace std::literals;

class proxy_test
{
    e4pp::queue& queue_;

    e4pp::timer_fn<proxy_test> timer_{
        &proxy_test::do_timer, *this };

    e4pp::generic_fn<proxy_test> generic_{
        &proxy_test::do_generic, *this };

    e4pp::timer_fun timer_fun_{[]{
        cout() << "+timer fun" << std::endl;
    }};
    e4pp::ev_heap evh_{queue_, -1, e4pp::flag{EV_TIMEOUT}, 
        std::chrono::milliseconds{570}, timer_fun_};

    e4pp::timer_fn<proxy_test> timer_fn_{
        &proxy_test::do_fn, *this };
    e4pp::ev_stack evs_{};

    e4pp::evh::timer just_timer_{queue_, e4pp::flag::timeout(), []{
            cout() << "just timer!" << std::endl;
        }};

public:
    proxy_test(e4pp::queue& queue)
        : queue_(queue)
    {   
        evs_.create(queue_, timer_fn_);
        evs_.add(std::chrono::milliseconds{550});

        queue.once(std::chrono::milliseconds{400}, timer_);
        queue.once(std::chrono::milliseconds{500}, generic_);

        just_timer_.add(std::chrono::milliseconds{650});

    }

    void do_timer()
    {
        cout() << "do_timer" << std::endl;
    }    

    void do_generic(evutil_socket_t, short)
    {
        cout() << "do_generic" << std::endl;
    }   

    void do_fn()
    {
        cout() << "+timer fn" << std::endl;
    }       
};

void test_queue(e4pp::queue& queue)
{
    queue.once(std::chrono::milliseconds{150}, []{
        cout() << "socket lambda" << std::endl;
    });

    static e4pp::timer_fun f1 = []{
        cout() << "static fun" << std::endl;
    };
    queue.once(std::chrono::milliseconds{200}, f1);

    e4pp::timer_fun f2 = []{
        cout() << "move timer" << std::endl;
    };
    queue.once(std::chrono::milliseconds{250}, std::move(f2));    

    auto f3 = [](evutil_socket_t, short) {
        cout() << "generic lambda" << std::endl;
    };
    queue.once(std::chrono::milliseconds{300}, f3);
}

void test_requeue(e4pp::queue& q1, e4pp::queue& q2)
{
    q1.once(q2, std::chrono::milliseconds{450}, 
        []{
            cout() << "timer requeue"sv << std::endl;
    });

    q1.once(q2, -1, EV_TIMEOUT, 
        std::chrono::milliseconds{450}, 
            [](evutil_socket_t, short){
                cout() << "socket requeue"sv << std::endl;
    });

    q1.once(q2, -1, EV_TIMEOUT, 
        std::chrono::milliseconds{500}, 
            [](evutil_socket_t, short){
                cout() << "generic requeue"sv << std::endl;
    });
}

int run()
{
    e4pp::config cfg{};
    cout() << "config: " << sizeof(cfg) << std::endl;
    e4pp::buffer buf{};
    cout() << "buffer: " << sizeof(buf) << std::endl;
    e4pp::buffer_event bev{};
    cout() << "buffer_event: " << sizeof(bev) << std::endl;
    e4pp::uri u("http://bot:123@localhost");
    auto [ user, passwd ] = u.auth();
    cout() << "uri: " << sizeof(u) << std::endl
        << "scheme " << u.scheme() << std::endl
        << "user " <<  user << std::endl
        << "passwd " << passwd  << std::endl
        << "host " << u.host() << std::endl;

    e4pp::heap_event h{};
    cout() << "heap_event: " << sizeof(h) << std::endl;

    e4pp::stack_event s{};
    s.destroy();
    cout() << "stack_event: " << sizeof(s) << std::endl;

    e4pp::queue queue{};
    cout() << "queue: " << sizeof(queue) << std::endl;

    e4pp::acceptor_fun on_connect = [](auto fd, auto sa, auto salen) {
        char text[INET6_ADDRSTRLEN];
        const char* rc = text;
        auto family = sa->sa_family;
        const void* addr = (family == AF_INET) ? 
            static_cast<void*>(&reinterpret_cast<sockaddr_in*>(sa)->sin_addr) :
            static_cast<void*>(&reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr);
        rc = evutil_inet_ntop(family, addr, text, sizeof(text));
        if (rc)
            cout() << "accept connection from " << rc << std::endl;
        else
            cout() << "accept connection" << std::endl;

        evutil_closesocket(fd); 
    };

    sockaddr_storage sa{};
	int slen = sizeof(sa);
    evutil_parse_sockaddr_port("[::]:32987", 
        reinterpret_cast<sockaddr*>(&sa), &slen);

    e4pp::listener listener{queue, 
        reinterpret_cast<sockaddr*>(&sa), 
        static_cast<socklen_t>(slen), on_connect};

    cout() << "listener: " << sizeof(listener) << std::endl;

    e4pp::queue thr_queue{};
    std::thread thr([&]{
        thr_queue.loopexit(std::chrono::seconds{20});
        thr_queue.loop(EVLOOP_NO_EXIT_ON_EMPTY);
        queue.once([&]{
            queue.loop_break();
        });
    });

    test_requeue(queue, thr_queue);
    test_queue(queue);
    proxy_test p{queue};

    e4pp::timer_fun fh = []{
        cout() << "ev_heap timer!"sv << std::endl;
    };
    e4pp::ev_heap evh(queue, -1, e4pp::flag{EV_TIMEOUT}, 
        std::chrono::milliseconds{600}, fh);

    e4pp::timer_fun fs = []{
        cout() << "ev_stack timer!"sv << std::endl;
    };
    e4pp::ev_stack evs(queue, -1, e4pp::flag{EV_TIMEOUT}, 
        std::chrono::milliseconds{650}, fs);

    cout() << "run"sv << std::endl;

    queue.dispatch();

    cout() << "join"sv << std::endl;
    thr.join();

    return 0;
}

int main()
{
        //e4pp::startup();
    e4pp::use_threads();

    run();

    return 0;
}
