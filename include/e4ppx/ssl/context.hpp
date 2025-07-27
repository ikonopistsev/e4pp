#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory>
#include <stdexcept>

namespace e4ppx {
namespace openssl {

// SSL initialization helper
class engine final    
{
public:
    engine()
    {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }

    ~engine()
    {
        EVP_cleanup();
        ERR_free_strings();
    }
};

// SSL context wrapper
class context final
{
public:
    using handle_type = SSL_CTX*;

private:
    struct free_ssl_ctx
    {
        void operator()(handle_type ptr) noexcept 
        { 
            SSL_CTX_free(ptr); 
        }
    };
    std::unique_ptr<SSL_CTX, free_ssl_ctx> ctx_{};

public:
    context() = default;
    context(context&&) = default;
    context& operator=(context&&) = default;

    // Create client context
    static context create_client()
    {
        context ctx;
        ctx.ctx_.reset(SSL_CTX_new(TLS_client_method()));
        if (!ctx.ctx_) {
            throw std::runtime_error("SSL_CTX_new failed");
        }
        
        // Set verification mode
        SSL_CTX_set_verify(ctx.ctx_.get(), SSL_VERIFY_PEER, nullptr);
        
        // Load default CA certificates
        if (SSL_CTX_set_default_verify_paths(ctx.ctx_.get()) != 1) {
            throw std::runtime_error("SSL_CTX_set_default_verify_paths failed");
        }
        
        return ctx;
    }

    handle_type handle() const noexcept
    {
        return ctx_.get();
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    void set_verify_callback(int (*callback)(int, X509_STORE_CTX*))
    {
        SSL_CTX_set_verify(handle(), SSL_VERIFY_PEER, callback);
    }
};
    
} // namespace openssl
} // namespace e4ppext

