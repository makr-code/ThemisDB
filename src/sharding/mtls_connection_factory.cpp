/**
 * @file mtls_connection_factory.cpp
 * @brief mTLS Connection Factory Implementation
 * @version 2.1.0
 * @date 2026-07-19
 *
 * Implements the factory pattern for creating mTLS connections using
 * Boost.Asio (async TCP connect with configurable deadline) and raw
 * OpenSSL (TLS handshake with configurable socket-level timeout, peer
 * certificate verification, and optional hostname verification).
 *
 * **Cross-platform notes:**
 * - TCP socket BIO is created via `BIO_new_socket()` (the correct
 *   OpenSSL socket API) rather than the POSIX-only `BIO_new_fd()`.
 * - Platform-specific socket-level timeout is set using `setsockopt`
 *   with `SO_RCVTIMEO`/`SO_SNDTIMEO` (POSIX `timeval`, Windows `DWORD`).
 * - Socket handle cleanup on BIO-creation failure uses `::close()` on
 *   POSIX and `::closesocket()` on Windows.
 */

#include "sharding/mtls_connection_factory.h"
#include "utils/logger.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509v3.h>
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#  include <winsock2.h>  // closesocket
#else
#  include <unistd.h>    // ::close
#  include <sys/socket.h>
#  include <sys/time.h>
#endif

namespace themis::sharding {

// ===========================================================================
// Internal helpers
// ===========================================================================

namespace {

/**
 * @brief Close a native socket handle in a platform-portable way.
 *
 * On POSIX, delegates to `::close()`; on Windows, to `::closesocket()`.
 *
 * @param native_sock Platform-native socket descriptor.
 */
inline void closeNativeSocket([[maybe_unused]] int native_sock) noexcept {
#ifdef _WIN32
    ::closesocket(static_cast<SOCKET>(native_sock));
#else
    ::close(native_sock);
#endif
}

/**
 * @brief Apply a per-operation timeout to an already-connected socket.
 *
 * Sets both `SO_RCVTIMEO` and `SO_SNDTIMEO` so that blocking TLS
 * operations (SSL_connect, SSL_read, SSL_write) return with an error
 * rather than blocking indefinitely.
 *
 * @param native_sock Platform-native socket descriptor.
 * @param timeout_ms  Timeout in milliseconds (0 = no timeout).
 * @return `true` on success; `false` if either setsockopt call fails.
 */
inline bool setSocketTimeout(int native_sock, uint32_t timeout_ms) noexcept {
    if (timeout_ms == 0) {
        return true;
    }
#ifdef _WIN32
    DWORD ms = static_cast<DWORD>(timeout_ms);
    bool ok = (::setsockopt(native_sock, SOL_SOCKET, SO_RCVTIMEO,
                             reinterpret_cast<const char*>(&ms), sizeof(ms)) == 0);
    ok &= (::setsockopt(native_sock, SOL_SOCKET, SO_SNDTIMEO,
                        reinterpret_cast<const char*>(&ms), sizeof(ms)) == 0);
    return ok;
#else
    struct timeval tv;
    tv.tv_sec  = static_cast<long>(timeout_ms / 1000u);
    tv.tv_usec = static_cast<long>((timeout_ms % 1000u) * 1000u);
    bool ok = (::setsockopt(native_sock, SOL_SOCKET, SO_RCVTIMEO,
                             &tv, sizeof(tv)) == 0);
    ok &= (::setsockopt(native_sock, SOL_SOCKET, SO_SNDTIMEO,
                        &tv, sizeof(tv)) == 0);
    return ok;
#endif
}

} // anonymous namespace

// ===========================================================================
// SSLDeleter
// ===========================================================================

/**
 * @brief Release an OpenSSL SSL object and its associated resources.
 *
 * `SSL_free()` also calls `BIO_free_all()` on the BIO chain attached to
 * the SSL object.  When the BIO was created with `BIO_CLOSE`, this closes
 * the underlying socket file descriptor / SOCKET handle as well.
 *
 * @param ptr SSL object to free.  No-op when `ptr` is `nullptr`.
 */
void SSLDeleter::operator()(SSL* ptr) const {
    if (ptr) {
        SSL_free(ptr);
    }
}

// ===========================================================================
// MTLSConnectionFactory
// ===========================================================================

/**
 * @brief Construct the factory with a borrowed SSL context and configuration.
 *
 * Validates that @p ssl_context is non-null and logs the effective settings
 * for audit purposes.
 *
 * @param ssl_context OpenSSL SSL_CTX that defines certificate, key, and CA
 *        chain for this factory.  The context must remain valid for the entire
 *        lifetime of this factory (and any pools that use it).
 * @param config      Factory configuration.
 *
 * @throws std::invalid_argument if @p ssl_context is null.
 */
MTLSConnectionFactory::MTLSConnectionFactory(SSL_CTX* ssl_context, const Config& config)
    : ssl_context_(ssl_context), config_(config) {
    
    if (!ssl_context_) {
        throw std::invalid_argument("MTLSConnectionFactory: ssl_context cannot be null");
    }
    
    THEMIS_INFO("[MTLSConnectionFactory] Initialized. "
                "Connect timeout: {}ms, Handshake timeout: {}ms, "
                "verify_peer: {}, verify_hostname: {}",
                config_.connect_timeout_ms,
                config_.tls_handshake_timeout_ms,
                config_.verify_peer,
                config_.verify_hostname);
}

/**
 * @brief Parse a "host:port" endpoint string into its components.
 *
 * Supported formats:
 * - `hostname:port`
 * - `hostname` (port defaults to `"50051"`)
 * - `[ipv6address]:port`
 * - `protocol://hostname:port` (protocol prefix is stripped)
 *
 * @param endpoint Endpoint string to parse.
 * @return Pair of (host, port) strings, or `nullopt` on parse error.
 */
std::optional<std::pair<std::string, std::string>> 
MTLSConnectionFactory::parseEndpoint(const std::string& endpoint) {
    // Remove protocol prefix if present (http://, https://, etc.)
    std::string clean = endpoint;
    size_t protocol_pos = clean.find("://");
    if (protocol_pos != std::string::npos) {
        clean = clean.substr(protocol_pos + 3);
    }
    
    // Handle IPv6 addresses [host]:port
    if (!clean.empty() && clean[0] == '[') {
        size_t close_bracket = clean.find(']');
        if (close_bracket == std::string::npos) {
            THEMIS_WARN("[MTLSConnectionFactory::parseEndpoint] Invalid IPv6 format: {}", endpoint);
            return std::nullopt;
        }
        
        std::string host = clean.substr(1, close_bracket - 1);
        
        if (close_bracket + 1 < clean.length() && clean[close_bracket + 1] == ':') {
            std::string port = clean.substr(close_bracket + 2);
            if (port.empty()) {
                port = "50051";
            }
            return std::make_pair(host, port);
        }
        return std::make_pair(host, "50051");
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
    
    // No port specified — use default
    return std::make_pair(clean, "50051");
}

/**
 * @brief Create a fully authenticated mTLS connection to @p endpoint.
 *
 * The function performs the following steps in order:
 *
 * 1. **DNS resolution** — resolves the host part of @p endpoint.
 * 2. **TCP connect with deadline** — uses an async `boost::asio::async_connect`
 *    with a `steady_timer` so the call returns after `config_.connect_timeout_ms`
 *    milliseconds even if no peer is reachable.
 * 3. **Socket-level timeout** — calls `setsockopt(SO_RCVTIMEO/SO_SNDTIMEO)`
 *    so the subsequent TLS handshake cannot block longer than
 *    `config_.tls_handshake_timeout_ms` milliseconds.
 * 4. **SSL object creation** — creates an SSL object from the shared
 *    `SSL_CTX`.  Peer verification mode is set here:
 *    - `config_.verify_peer == false` → `SSL_VERIFY_NONE`
 *    - `config_.verify_peer == true` (default) → inherits context verify mode
 *      (which must include `SSL_VERIFY_PEER`).
 * 5. **SNI + hostname verification setup** — when `config_.verify_hostname` is
 *    true, sets both the SNI extension (`SSL_set_tlsext_host_name`) and the
 *    expected hostname on the X.509 verify params (`X509_VERIFY_PARAM_set1_host`)
 *    so the peer certificate's CN/SAN is verified against the target host.
 * 6. **BIO attachment** — uses `BIO_new_socket()` (the correct cross-platform
 *    OpenSSL socket BIO) with `BIO_CLOSE` so that `SSL_free()` also closes the
 *    underlying socket handle.
 * 7. **TLS handshake** — calls `SSL_connect()`.  On failure, logs the OpenSSL
 *    error and returns `nullopt`.
 * 8. **Post-handshake peer verification** — when `config_.verify_peer` is true,
 *    calls `SSL_get_verify_result()` and rejects the connection if the result is
 *    anything other than `X509_V_OK`.
 *
 * @param endpoint Target endpoint string, e.g. `"localhost:50051"` or
 *                 `"[::1]:8443"`.
 *
 * @return A ready-to-use `unique_ptr<SSL, SSLDeleter>` on success, or
 *         `nullopt` on any failure (TCP, TLS, certificate verification).
 *         Errors are logged; no exceptions are thrown.
 */
std::optional<std::unique_ptr<SSL, SSLDeleter>> 
MTLSConnectionFactory::createConnection(const std::string& endpoint) {
    if (config_.enable_logging) {
        THEMIS_DEBUG("[MTLSConnectionFactory::createConnection] Connecting to {}", endpoint);
    }
    
    auto parsed = parseEndpoint(endpoint);
    if (!parsed) {
        THEMIS_WARN("[MTLSConnectionFactory::createConnection] Failed to parse endpoint: {}",
                    endpoint);
        return std::nullopt;
    }
    
    auto [host, port] = *parsed;
    
    try {
        namespace asio = boost::asio;
        using tcp = asio::ip::tcp;
        
        asio::io_context ioc;
        tcp::socket socket(ioc);
        
        // Tune kernel-side buffer sizes for throughput.
        socket.set_option(asio::socket_base::send_buffer_size(65536));
        socket.set_option(asio::socket_base::receive_buffer_size(65536));
        
        // ── Step 1: DNS resolution ───────────────────────────────────────
        tcp::resolver resolver(ioc);
        boost::system::error_code resolve_ec;
        auto results = resolver.resolve(host, port, resolve_ec);
        if (resolve_ec) {
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                        "DNS resolution failed for {}:{}: {}",
                        host, port, resolve_ec.message());
            return std::nullopt;
        }
        
        // ── Step 2: TCP async connect with deadline ──────────────────────
        bool          timed_out  = false;
        boost::system::error_code connect_ec;
        
        asio::steady_timer connect_timer(ioc);
        connect_timer.expires_after(
            std::chrono::milliseconds(config_.connect_timeout_ms));
        
        // Timer fires → cancel the pending connect.
        connect_timer.async_wait(
            [&socket, &timed_out](const boost::system::error_code& ec) {
                if (!ec) {
                    timed_out = true;
                    boost::system::error_code cancel_ec;
                    socket.cancel(cancel_ec);
                }
            });
        
        // Async connect → cancel the timer on completion.
        asio::async_connect(socket, results,
            [&connect_ec, &connect_timer](
                const boost::system::error_code& ec, const tcp::endpoint&) {
                connect_ec = ec;
                connect_timer.cancel();
            });
        
        ioc.run();
        
        if (timed_out) {
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                        "TCP connect timed out ({}ms) to {}:{}",
                        config_.connect_timeout_ms, host, port);
            return std::nullopt;
        }
        if (connect_ec) {
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                        "TCP connect failed to {}:{}: {}",
                        host, port, connect_ec.message());
            return std::nullopt;
        }
        
        if (config_.enable_logging) {
            THEMIS_DEBUG("[MTLSConnectionFactory::createConnection] "
                         "TCP connected to {}:{}", host, port);
        }
        
        // ── Step 3: TLS handshake timeout via socket options ─────────────
        // Apply SO_RCVTIMEO / SO_SNDTIMEO so SSL_connect() cannot block
        // indefinitely.  The socket is still owned by Boost at this point;
        // we read native_handle() without releasing it yet.
        auto native_sock = static_cast<int>(socket.native_handle());
        if (!setSocketTimeout(native_sock, config_.tls_handshake_timeout_ms)) {
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                        "Failed to set TLS handshake timeout on socket");
            // Non-fatal: continue without enforced timeout
        }
        
        // ── Step 4: Create SSL object ─────────────────────────────────────
        SSL* ssl_obj = SSL_new(ssl_context_);
        if (!ssl_obj) {
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                        "SSL_new() failed");
            return std::nullopt;
        }
        
        // Override per-connection peer verification if caller requests none.
        if (!config_.verify_peer) {
            SSL_set_verify(ssl_obj, SSL_VERIFY_NONE, nullptr);
        }
        
        // ── Step 5: SNI + hostname verification ───────────────────────────
        if (config_.verify_hostname) {
            // Set SNI extension so the server can select the right certificate.
            if (!SSL_set_tlsext_host_name(ssl_obj, host.c_str())) {
                SSL_free(ssl_obj);
                THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                            "Failed to set SNI hostname: {}", host);
                return std::nullopt;
            }
            
            // Also set the expected hostname on the X.509 verify parameters
            // so that the peer certificate CN / SAN is checked against `host`.
            X509_VERIFY_PARAM* param = SSL_get0_param(ssl_obj);
            X509_VERIFY_PARAM_set_hostflags(param,
                X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            if (!X509_VERIFY_PARAM_set1_host(param, host.c_str(), 0)) {
                SSL_free(ssl_obj);
                THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                            "X509_VERIFY_PARAM_set1_host() failed for: {}", host);
                return std::nullopt;
            }
        }
        
        // ── Step 6: Attach socket via cross-platform BIO ─────────────────
        // socket.release() transfers ownership of the native socket handle
        // to the BIO (BIO_CLOSE).  From this point on, SSL_free() will close
        // the socket.  Boost no longer owns it.
        auto released_fd = static_cast<int>(socket.release());
        BIO* bio = BIO_new_socket(released_fd, BIO_CLOSE);
        if (!bio) {
            // BIO creation failed; we still own released_fd, so close it.
            closeNativeSocket(released_fd);
            SSL_free(ssl_obj);
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                        "BIO_new_socket() failed");
            return std::nullopt;
        }
        SSL_set_bio(ssl_obj, bio, bio);  // SSL takes ownership of bio (and socket)
        
        // ── Step 7: TLS handshake ─────────────────────────────────────────
        int ret = SSL_connect(ssl_obj);
        if (ret != 1) {
            char err_buf[256] = {};
            ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
            THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                        "TLS handshake failed to {}:{}: {}", host, port, err_buf);
            SSL_free(ssl_obj);  // frees bio which closes released_fd
            return std::nullopt;
        }
        
        // ── Step 8: Post-handshake peer certificate verification ──────────
        if (config_.verify_peer) {
            long vresult = SSL_get_verify_result(ssl_obj);
            if (vresult != X509_V_OK) {
                THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                            "Peer certificate verification failed to {}:{}: {} ({})",
                            host, port,
                            X509_verify_cert_error_string(vresult),
                            vresult);
                SSL_free(ssl_obj);
                return std::nullopt;
            }
        }
        
        THEMIS_INFO("[MTLSConnectionFactory::createConnection] "
                    "mTLS connection established to {}:{}", host, port);
        
        return std::make_optional(std::unique_ptr<SSL, SSLDeleter>(ssl_obj));
        
    } catch (const std::exception& e) {
        THEMIS_WARN("[MTLSConnectionFactory::createConnection] "
                    "Exception connecting to {}: {}", endpoint, e.what());
        return std::nullopt;
    }
}

} // namespace themis::sharding
