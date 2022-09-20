#pragma once

#include "e4pp/buffer_event.hpp"
#include "e4pp/queue.hpp"

namespace e4pp {

template<class H>
class bev;

template<class H>
class bev final
    : buffer_event
{
    H handler_{};
    
public:
    bev() = default;
    
    //bev(event_hadnle ev)
    
    using buffer_event::set;

        // fd == -1
    // create new socket
    bev(queue_handle_type queue, 
        evutil_socket_t fd, bev_flag opt, H handler)
        : buffer_event{queue, fd, opt}
    {   
        const auto& [ ptr, rdfn, wrfn, evfn ] = handler;
        set(rdfn, wrfn, evfn, ptr);
    }

    bev(queue_handle_type queue, evutil_socket_t fd, 
        H handler, bev_flag opt = bev_close_on_free)
        : buffer_event{queue, fd, opt}
    {   }

    bev(queue_handle_type queue, bev_flag opt = bev_close_on_free)
        : buffer_event{queue, opt}
    {   }


    // bev(ev_fn)
    // connect(..., ev_fn(bev.connected()){

    // });
    
};

} // namespace e4pp