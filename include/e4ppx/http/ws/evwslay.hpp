#pragma once

#include "e4ppx/ssl/rand.hpp"
#include "e4pp/buffer_event.hpp"
#include <wslay/wslay.h>

namespace e4ppx {
namespace http {
namespace ws {

using wslay_event_context_ptr = wslay_event_context*;

namespace detail {

struct wslay_free 
{
    void operator()(wslay_event_context_ptr ptr) {
        wslay_event_context_free(ptr);
    }
};

} // namespace detail

// класс адаптера для wslay
template<class T>
struct evwslay {
    using socket_type = T;
    using this_type = evwslay<T>;

    socket_type& ws;
    bufferevent* bev{nullptr};

    static wslay_event_context_ptr create_client_wslay(this_type& op) {
        auto cb = wslay_event_callbacks{
            this_type::recv_cb,
            this_type::send_cb,
            [](wslay_event_context_ptr ctx, 
                uint8_t *buf, size_t len, void*) {
                e4ppx::openssl::rand rnd;
                rnd(buf, len);
                return int{};
            },
            nullptr, /* on_frame_recv_start_callback */
            nullptr, /* on_frame_recv_callback */
            nullptr, /* on_frame_recv_end_callback */
            this_type::on_msg_recv_cb,
        };

        wslay_event_context_ptr rc = nullptr;
        wslay_event_context_client_init(&rc, &cb, &rc);
        return rc;
    }

    std::unique_ptr<wslay_event_context, detail::wslay_free>
        wslay{create_client_wslay(*this)};

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
        bufferevent_setcb(b, this_type::readcb, 
            this_type::writecb, this_type::eventcb, this);
        bev = b;
    }
    void forward_msg(wslay_opcode opcode, const uint8_t *ptr, std::size_t len) {
        ws.message(opcode, reinterpret_cast<const char*>(ptr), len);
        io_update();
    }
    static ssize_t recv_cb(wslay_event_context_ptr ctx,
        uint8_t *data, size_t len, int flags, void *user_data) {
        assert(user_data);
        auto a = static_cast<this_type*>(user_data);
        return a->read(data, len);
    }
    static ssize_t send_cb(wslay_event_context_ptr ctx, 
        const uint8_t *data, size_t len, int flags, void *user_data) {
        assert(user_data);
        return static_cast<this_type*>(user_data)->write(data, len);
    }
    static void on_msg_recv_cb(wslay_event_context_ptr ctx,
        const struct wslay_event_on_msg_recv_arg *arg, void *user_data) {
        assert(user_data);
        static_cast<this_type*>(user_data)->forward_msg(
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
        ws.on_event(what);
    }
    static void readcb(bufferevent*, void *user_data) {
        assert(user_data);
        static_cast<this_type*>(user_data)->event_read(); 
    }
    static void writecb(bufferevent*, void *user_data) {
        assert(user_data);
        static_cast<this_type*>(user_data)->event_write();             
    }
    static void eventcb(bufferevent*, short what, void *user_data) {
        assert(user_data);
        static_cast<this_type*>(user_data)->event_network(what);    
    }
    void io_update() {
        assert(bev);
        short ev = wslay_event_want_read(wslay) ? EV_READ : 0;
            ev |= wslay_event_want_write(wslay) ? EV_WRITE : 0;
        if (ev) {
            bufferevent_enable(bev, ev);
        } else {
            bufferevent_disable(bev, EV_READ|EV_WRITE);
        }
    }
};

} // namespace ws
} // namespace http
} // namespace e4ppx
