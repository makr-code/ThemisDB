/*
 * ThemisDB | File: redis_tls_config.h | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file redis_tls_config.h
 * @brief Redis TLS enforcement configuration and enforcer interface.
 *
 * RedisTLSConfig carries all parameters required to configure TLS on
 * a Redis connection, including CA verification, optional mTLS client
 * certificates, cipher suite constraints, and minimum protocol version.
 *
 * IRedisTLSEnforcer applies a config to a live connection manager and
 * exposes observability accessors for the negotiated TLS session.
 *
 * Compliance: PCI DSS 4.2.1, FIPS 140-3 (with approved cipher list).
 */

#pragma once

#include <string>

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// RedisTLSConfig — configuration for TLS-enforced Redis connections
// ---------------------------------------------------------------------------

/**
 * @brief TLS configuration for Redis connections.
 *
 * When `require_tls` is true, any attempt to open a plaintext Redis
 * connection must be rejected by the connection manager.
 *
 * `min_tls_version` follows OpenSSL wire constants:
 *   TLS 1.2 = 0x0303, TLS 1.3 = 0x0304.
 *
 * `allowed_cipher_suites` is a colon-separated OpenSSL cipher string
 * (e.g. "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256").
 * An empty string means "use OpenSSL defaults".
 */
struct RedisTLSConfig {
    bool        require_tls           = true;    ///< Reject plaintext connections.
    std::string ca_cert_path;                    ///< CA certificate for server verification.
    std::string client_cert_path;                ///< Client certificate for mTLS (optional).
    std::string client_key_path;                 ///< Client private key for mTLS (optional).
    bool        verify_hostname       = true;    ///< Verify server hostname against cert CN/SAN.
    std::string allowed_cipher_suites;           ///< OpenSSL cipher string; empty = defaults.
    int         min_tls_version       = 0x0303;  ///< Minimum TLS version (TLS 1.2 default).

    /**
     * @brief Validate that the config is internally consistent.
     *
     * @return `true` if TLS is disabled OR a CA cert path has been provided.
     */
    bool isValid() const {
        return !require_tls || !ca_cert_path.empty();
    }
};

// ---------------------------------------------------------------------------
// IRedisTLSEnforcer — applies TLS config to a Redis connection manager
// ---------------------------------------------------------------------------

/**
 * @brief Interface for applying TLS enforcement to a Redis connection manager.
 *
 * Implementations wrap hiredis-tls, redis-plus-plus, or Boost.Redis and
 * expose the negotiated TLS session parameters for observability.
 *
 * ### Thread safety
 * `applyTLSConfig()` is not required to be thread-safe; call it during
 * initialisation before any concurrent Redis operations begin.
 * `isTLSActive()`, `getTLSVersion()`, and `getActiveCipherSuite()` must
 * be thread-safe.
 */
class IRedisTLSEnforcer {
public:
    virtual ~IRedisTLSEnforcer() = default;

    /**
     * @brief Apply a TLS configuration to the underlying connection manager.
     *
     * @return `true` on success; `false` if the configuration is invalid or
     *         the TLS handshake with the Redis server failed.
     */
    virtual bool applyTLSConfig(const RedisTLSConfig& config) = 0;

    /// Return `true` if the current connection is TLS-protected.
    virtual bool isTLSActive() const = 0;

    /// Return the negotiated TLS protocol version string (e.g., "TLSv1.3").
    virtual std::string getTLSVersion() const = 0;

    /// Return the negotiated cipher suite (e.g., "TLS_AES_256_GCM_SHA384").
    virtual std::string getActiveCipherSuite() const = 0;
};

} // namespace cache
} // namespace themis
