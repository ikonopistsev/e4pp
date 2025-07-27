#pragma once

#include "e4pp/bev.hpp"
#include <event2/bufferevent_ssl.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <memory>

namespace e4ppx {
namespace openssl {

// SSL buffer event class
class bev : public e4pp::bev
{
    // Import types from base class
    using queue_handle_type = e4pp::queue_handle_type;
    using bev_flag = e4pp::bev_flag;
    using dns_handle_type = e4pp::dns_handle_type;
    
    // Import constants from base class  
    static constexpr auto bev_close_on_free = e4pp::bev_close_on_free;
    
private:
    auto assert_ssl() const noexcept
    {
        auto ssl_ptr = bufferevent_openssl_get_ssl(assert_handle());
        assert(ssl_ptr);
        return ssl_ptr;
    }

public:
    // Default constructor - empty SSL bev
    bev() = default;
    
    // Move constructor and assignment
    bev(bev&&) = default;
    bev& operator=(bev&&) = default;
    
    // No copy constructor/assignment
    bev(const bev&) = delete;
    bev& operator=(const bev&) = delete;

    // Constructor from raw handle (takes ownership)
    explicit bev(handle_type ptr) noexcept
        : e4pp::bev{ptr}
    {   }

    // Create SSL buffer event from existing socket
    static auto create_ssl(SSL* ssl, queue_handle_type queue, evutil_socket_t fd,
        bufferevent_ssl_state state = BUFFEREVENT_SSL_CONNECTING,
        bev_flag opt = bev_close_on_free)
    {
        assert(ssl);
        assert(queue);
        
        auto ptr = e4pp::detail::check_pointer("bufferevent_openssl_socket_new",
            bufferevent_openssl_socket_new(queue, fd, ssl, state, opt));
        
        return ptr;
    }    

    // Create SSL buffer event for accepting connections (server mode)
    static bev accept_ssl(SSL* ssl, queue_handle_type queue, evutil_socket_t fd,
        bev_flag opt = bev_close_on_free)
    {
        assert(ssl);
        assert(queue);
        assert(fd != -1); // Server must have valid connected socket
        
        auto ptr = e4pp::detail::check_pointer("bufferevent_openssl_socket_new",
            bufferevent_openssl_socket_new(queue, fd, ssl, 
                BUFFEREVENT_SSL_ACCEPTING, opt));        
        return bev{ptr};
    }    

    // Constructor from SSL* and queue (takes ownership of SSL*)
    explicit bev(SSL* ssl, queue_handle_type queue, 
        evutil_socket_t fd = -1, bev_flag opt = bev_close_on_free)
        : e4pp::bev{create_ssl(ssl, queue, fd, BUFFEREVENT_SSL_CONNECTING, opt)} 
    {   }

    // Get SSL handle from bufferevent
    SSL* ssl() const noexcept
    {
        return assert_ssl();
    }

    operator SSL*() const noexcept
    {
        return assert_ssl();
    }

    // Override shutdown to properly close SSL session
    void shutdown() noexcept override
    {
        auto ssl_ptr = assert_ssl();
        
        // If the underlying socket is closed or in error state,
        // set SSL_RECEIVED_SHUTDOWN to avoid waiting for close_notify
        // that will never come
        evutil_socket_t fd = this->fd();
        if (fd == -1) {
            // Socket is already closed - set flag to avoid blocking
            SSL_set_shutdown(ssl_ptr, SSL_RECEIVED_SHUTDOWN);
        }
        
        // Perform SSL shutdown handshake
        // We don't check return value as we're shutting down anyway
        SSL_shutdown(ssl_ptr);
        
        // Note: SSL* will be freed by bufferevent_free automatically
        // when bufferevent was created with BEV_OPT_CLOSE_ON_FREE
    }

    void connect(dns_handle_type dns, int af,
                const std::string& hostname, int port) override
    {
        assert(dns);
        
        // Set SNI hostname for SSL
        SSL_set_tlsext_host_name(assert_ssl(), hostname.c_str());
        
        e4pp::detail::check_result("bufferevent_socket_connect_hostname",
            bufferevent_socket_connect_hostname(assert_handle(), dns,
                af, hostname.c_str(), port));
    }

    // SSL-specific methods
    
    // Check if SSL handshake is complete
    bool is_ssl_connected() const noexcept
    {
        return SSL_is_init_finished(assert_ssl());
    }

    // Set SSL shutdown state explicitly
    void set_shutdown_state(int mode) noexcept
    {
        SSL_set_shutdown(assert_ssl(), mode);
    }

    // Get current SSL shutdown state
    int get_shutdown_state() const noexcept
    {
        return SSL_get_shutdown(assert_ssl());
    }

    // Check if we received shutdown from peer
    bool received_shutdown() const noexcept
    {
        return (get_shutdown_state() & SSL_RECEIVED_SHUTDOWN) != 0;
    }

    // Check if we sent shutdown to peer
    bool sent_shutdown() const noexcept
    {
        return (get_shutdown_state() & SSL_SENT_SHUTDOWN) != 0;
    }

    // Get SSL error (requires OpenSSL support in libevent)
    unsigned long get_ssl_error() const noexcept
    {
        return bufferevent_get_openssl_error(assert_handle());
    }

    // Allow dirty shutdown (requires OpenSSL support in libevent)
    void allow_dirty_shutdown() noexcept
    {
        bufferevent_openssl_set_allow_dirty_shutdown(assert_handle(), 1);
    }

    // Get peer certificate
    X509* get_peer_certificate() const noexcept
    {
        return SSL_get_peer_certificate(assert_ssl());
    }

    // Get cipher info
    auto get_cipher() const noexcept
    {
        return SSL_get_cipher(assert_ssl());
    }

    // Get SSL version
    auto get_version() const noexcept
    {
        return SSL_get_version(assert_ssl());
    }
};

} // namespace openssl
} // namespace e4pp