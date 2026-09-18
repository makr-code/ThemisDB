/**
 * @file redis_tls_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// RedisTLSConfig — configuration for TLS-enforced Redis connections
// ---------------------------------------------------------------------------

struct RedisTLSConfig {
    bool        require_tls           = true;    ///< Reject plaintext connections.
    std::string ca_cert_path;                    ///< CA certificate for server verification.
    std::string client_cert_path;                ///< Client certificate for mTLS (optional).
    std::string client_key_path;                 ///< Client private key for mTLS (optional).
    bool        verify_hostname       = true;    ///< Verify server hostname against cert CN/SAN.
    std::string allowed_cipher_suites;           ///< OpenSSL cipher string; empty = defaults.
    int         min_tls_version       = 0x0303;  ///< Minimum TLS version (TLS 1.2 default).

    bool isValid() const {
        return !require_tls || !ca_cert_path.empty();
    }
};

// ---------------------------------------------------------------------------
// IRedisTLSEnforcer — applies TLS config to a Redis connection manager
// ---------------------------------------------------------------------------

class IRedisTLSEnforcer {
public:
    /**
     * @brief IRedis TLSEnforcer.
     * @return Return value.
     */
    virtual ~IRedisTLSEnforcer() = default;

    /**
     * @brief Apply TLSConfig.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool applyTLSConfig(const RedisTLSConfig& config) = 0;

    /**
     * @brief Is TLSActive.
     * @return True when the operation succeeds.
     */
    virtual bool isTLSActive() const = 0;

    /**
     * @brief Get TLSVersion.
     * @return Return value.
     */
    virtual std::string getTLSVersion() const = 0;

    /**
     * @brief Get Active Cipher Suite.
     * @return Return value.
     */
    virtual std::string getActiveCipherSuite() const = 0;
};

} // namespace cache
} // namespace themis
