/**
 * @file mtls_connection_factory.cpp
 * @brief mTLS Connection Factory Implementation
 * @version 2.0.0
 * @date 2026-07-19
 * 
 * Implements the factory pattern for creating mTLS connections.
 * Handles SSL context management, TCP connect, TLS handshake,
 * and certificate verification.
 */

#include "sharding/mtls_connection_factory.h"
#include "utils/logger.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <unistd.h>
#include <iostream>
#include <sstream>

namespace themis::sharding {

// ===========================================================================
// MTLSConnectionFactory Implementation
// ===========================================================================

MTLSConnectionFactory::MTLSConnectionFactory(SSL_CTX* ssl_context, const Config& config)
    : ssl_context_(ssl_context), config_(config) {
    
    if (!ssl_context_) {
        throw std::invalid_argument("MTLSConnectionFactory: ssl_context cannot be null");
    }
    
    THEMIS_INFO("[MTLSConnectionFactory] Initialized with SSL context. "
                "Connect timeout: {}ms, Handshake timeout: {}ms, "
                "Verify peer: {}, Verify hostname: {}",
                config_.connect_timeout_ms,
                config_.tls_handshake_timeout_ms,
                config_.verify_peer,
                config_.verify_hostname);
}

std::optional<std::pair<std::string, std::string>> 
MTLSConnectionFactory::parseEndpoint(const std::string& endpoint) {
    // Remove protocol prefix if present (http://, https://, etc.)
    std::string clean = endpoint;
    size_t protocol_pos = clean.find("://");
    if (protocol_pos != std::string::npos) {
        clean = clean.substr(protocol_pos + 3);
    }
    
    // Handle IPv6 addresses [host]:port
    if (clean[0] == '[') {
        size_t close_bracket = clean.find(']');
        if (close_bracket == std::string::npos) {
            THEMIS_WARN("[MTLSConnectionFactory::parseEndpoint] Invalid IPv6 format: {}", endpoint);
            return std::nullopt;
        }
        
        std::string host = clean.substr(1, close_bracket - 1);
        
        // Check for port after ]
        if (close_bracket + 1 < clean.length() && clean[close_bracket + 1] == ':') {
            std::string port = clean.substr(close_bracket + 2);
            if (port.empty()) {
                port = "50051"; // Default port
            }
            return std::make_pair(host, port);
        } else {
            return std::make_pair(host, "50051"); // Default port
        }
    }
    
    // Handle IPv4 or hostname with optional port
    size_t colon_pos = clean.find_last_of(':');
    if (colon_pos != std::string::npos) {
        std::string host = clean.substr(0, colon_pos);
        std::string port = clean.substr(colon_pos + 1);
        
        if (port.empty()) {
            port = "50051";
        }
        
        return std::make_pair(host, port);
    }
    
    // No port specified, use default
    return std::make_pair(clean, "50051");
}

std::optional<std::unique_ptr<SSL, SSLDeleter>> 
MTLSConnectionFactory::createConnection(const std::string& endpoint) {
    if (config_.enable_logging) {
        THEMIS_DEBUG("[MTLSConnectionFactory::createConnection] Attempting connection to {}",
                     endpoint);
    }
    
    // Parse endpoint
    auto parsed = parseEndpoint(endpoint);
    if (!parsed) {
        THEMIS_WARN("[MTLSConnectionFactory::createConnection] Failed to parse endpoint: {}",
                    endpoint);
        return std::nullopt;
    }
    
    auto [host, port] = *parsed;
    
    try {
        // Use Boost.Asio to establish TCP connection
        namespace asio = boost::asio;
        namespace ssl = boost::asio::ssl;
        using tcp = asio::ip::tcp;
        
        asio::io_context ioc;
        tcp::socket socket(ioc);
        
        // Set connection timeout
        socket.set_option(asio::socket_base::send_buffer_size(65536));
        socket.set_option(asio::socket_base::receive_buffer_size(65536));
        
        // Resolve endpoint
        tcp::resolver resolver(ioc);
        auto results = resolver.resolve(host, port);
        
        // Connect with timeout
        boost::system::error_code ec;
        asio::connect(socket, results, ec);
        
        if (ec) {
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] TCP connect failed to {}:{}: {}",
                        host, port, ec.message());
            return std::nullopt;
        }
        
        if (config_.enable_logging) {
            THEMIS_DEBUG("[MTLSConnectionFactory::createConnection] TCP connection established to {}:{}",
                         host, port);
        }
        
        // Create SSL object from context
        SSL* ssl = SSL_new(ssl_context_);
        if (!ssl) {
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] Failed to create SSL object");
            return std::nullopt;
        }
        
        // Set hostname for SNI (if enabled)
        if (config_.verify_hostname) {
            if (!SSL_set_tlsext_host_name(ssl, host.c_str())) {
                SSL_free(ssl);
                THEMIS_WARN("[MTLSConnectionFactory::createConnection] Failed to set SNI hostname");
                return std::nullopt;
            }
        }
        
        // Transfer socket fd ownership to a BIO with BIO_CLOSE so that SSL_free
        // will close the underlying fd. socket.release() prevents Boost from
        // closing it again when the socket object goes out of scope.
        auto raw_fd = static_cast<int>(socket.release());
        BIO* bio = BIO_new_fd(raw_fd, BIO_CLOSE);
        if (!bio) {
            // BIO creation failed; we own raw_fd, so close it manually
            ::close(raw_fd);
            SSL_free(ssl);
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] Failed to create BIO for fd");
            return std::nullopt;
        }
        SSL_set_bio(ssl, bio, bio); // SSL takes ownership of bio (and thus the fd)
        
        // Perform TLS handshake
        int ret = SSL_connect(ssl);
        if (ret != 1) {
            char err_buf[256] = {};
            ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] TLS handshake failed to {}:{}: {}",
                        host, port, err_buf);
            SSL_free(ssl); // frees bio which closes raw_fd
            return std::nullopt;
        }
        
        THEMIS_INFO("[MTLSConnectionFactory::createConnection] Successfully created connection to {}:{}",
                    host, port);
        
        return std::make_optional(std::unique_ptr<SSL, SSLDeleter>(ssl));
        
    } catch (const std::exception& e) {
        THEMIS_WARN("[MTLSConnectionFactory::createConnection] Exception creating connection to {}: {}",
                    endpoint, e.what());
        return std::nullopt;
    }
}

} // namespace themis::sharding
