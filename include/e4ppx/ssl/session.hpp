#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <memory>
#include <stdexcept>

namespace e4ppx {
namespace openssl {

// SSL wrapper
class session
{
public:
    using handle_type = SSL*;

private:
    struct free_ssl
    {
        void operator()(handle_type ptr) noexcept 
        { 
            SSL_free(ptr); 
        }
    };
    std::unique_ptr<SSL, free_ssl> ssl_{};

public:
    session() = default;
    session(session&&) = default;
    session& operator=(session&&) = default;

    explicit session(SSL_CTX* ctx)
        : ssl_{SSL_new(ctx)}
    {
        if (!ssl_) {
            throw std::runtime_error("SSL_new failed");
        }
    }

    handle_type handle() const noexcept
    {
        return ssl_.get();
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    void set_hostname(const std::string& hostname)
    {
        SSL_set_tlsext_host_name(handle(), hostname.c_str());
        SSL_set1_host(handle(), hostname.c_str());
    }

    // Get peer certificate for verification
    std::unique_ptr<X509, void(*)(X509*)> get_peer_certificate() const
    {
        return {SSL_get_peer_certificate(handle()), X509_free};
    }

    // Verify hostname against certificate
    bool verify_hostname(const std::string& hostname) const
    {
        auto cert = get_peer_certificate();
        if (!cert) {
            return false;
        }

        return X509_check_host(cert.get(), 
            hostname.c_str(), hostname.length(), 0, nullptr) == 1;
    }

    // Release ownership for use with allocator
    auto release() noexcept
    {
        return ssl_.release();
    }
};
    
} // namespace openssl
} // namespace e4ppext

