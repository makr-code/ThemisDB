/**
 * @file api_key_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

/**
 * @brief Claims returned after successful API key authentication
 */
struct ApiKeyClaims {
    std::string key_id;       ///< The API key identifier that was authenticated
    std::string principal;    ///< Subject/user principal associated with this key
    std::string tenant_id;    ///< Optional tenant identifier
    std::vector<std::string> scopes; ///< Authorised scopes for this key
    std::vector<std::string> roles;  ///< Optional roles associated with this key
    std::chrono::system_clock::time_point expires_at; ///< Zero means no expiry

    /**
     * @brief Return true if the key has an explicit expiry and it has passed.
     */
    bool isExpired() const {
        static const std::chrono::system_clock::time_point epoch{};
        if (expires_at == epoch) {
            return false;
        }
        return std::chrono::system_clock::now() > expires_at;
    }

    /**
     * @brief Return true if the given scope is authorised.
     */
    bool hasScope(const std::string& scope) const {
        for (const auto& s : scopes) {
            if (s == scope) return true;
        }
        return false;
    }
};

/**
 * @brief A stored API key credential (server-side representation)
 *
 * Secrets are stored as a SHA-256 hex digest so that the raw secret
 * is never retained after initial key provisioning.  Use
 * ApiKeyAuthenticator::hashSecret() to derive secret_hash when
 * creating credentials.
 */
struct ApiKeyCredential {
    std::string key_id;          ///< Public key identifier (e.g., "sk_live_abc123")
    std::string secret_hash;     ///< SHA-256(secret) as lowercase hex
    std::string principal;       ///< Subject/user principal
    std::string tenant_id;       ///< Optional tenant identifier
    std::vector<std::string> scopes; ///< Authorised scopes
    std::vector<std::string> roles;  ///< Optional roles
    std::chrono::system_clock::time_point expires_at; ///< Zero = no expiry
    bool active{true};           ///< False means the key has been revoked
};

/**
 * @brief API key authenticator (static key + secret)
 *
 * Validates API key credentials presented by clients via the header
 * `X-API-Key: <key_id>.<raw_secret>` or as separate parameters.
 *
 * Security properties:
 *   - Raw secrets are never stored; only their SHA-256 hash is retained.
 *   - Secret comparison is performed with CRYPTO_memcmp (constant-time)
 *     to prevent timing-based oracle attacks.
 *   - All credential mutations are protected by a mutex for thread safety.
 *
 * Typical usage:
 * @code
 *   ApiKeyAuthenticator auth;
 *
 *   // Provision a key during setup
 *   auto cred = ApiKeyAuthenticator::createCredential(
 *       "sk_live_abc123", "super-secret",
 *       "alice@example.com", {"data:read", "data:write"});
 *   auth.addCredential(cred);
 *
 *   // Authenticate an incoming request
 *   auto claims = auth.authenticate("sk_live_abc123", "super-secret");
 * @endcode
 *
 * Compliance: OWASP API Security Top 10 – API2:2023 Broken Authentication
 */
class ApiKeyAuthenticator {
public:
    /**
     * @brief Configuration for the authenticator
     */
    struct Config {
        bool check_expiry{true};        ///< Reject keys whose expiry has passed
        size_t max_key_id_length{128};  ///< Maximum allowed key_id length
        size_t max_secret_length{512};  ///< Maximum allowed secret length
        static Config defaults() { return {}; }
    };

    /**
     * @brief Construct with optional configuration
     */
    explicit ApiKeyAuthenticator(const Config& config = Config::defaults());

    /**
     * @brief Attach an AuditLogger to receive LOGIN_SUCCESS / LOGIN_FAILED events.
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    // -------------------------------------------------------------------------
    // Credential management
    // -------------------------------------------------------------------------

    /**
     * @brief Add or replace a credential in the store.
     *
     * Thread-safe.  If a credential with the same key_id already exists
     * it is silently replaced.
     *
     * @param credential  Credential to add (secret_hash must already be hashed)
     * @throws AuthException (AUTH_CONFIG_INVALID) if key_id is empty or
     *         secret_hash has unexpected length
     */
    void addCredential(const ApiKeyCredential& credential);

    /**
     * @brief Remove a credential from the store.
     *
     * Thread-safe.  A no-op if the key_id is not found.
     *
     * @param key_id  Key to remove
     */
    void removeCredential(const std::string& key_id);

    /**
     * @brief Return the number of stored credentials.
     */
    size_t credentialCount() const;

    // -------------------------------------------------------------------------
    // Authentication
    // -------------------------------------------------------------------------

    /**
     * @brief Authenticate a key_id + raw secret pair.
     *
     * Steps:
     *   1. Validate input lengths.
     *   2. Look up key_id in the credential store.
     *   3. Reject inactive or expired keys.
     *   4. Hash the presented secret and compare with the stored hash
     *      using CRYPTO_memcmp (constant-time).
     *   5. Return ApiKeyClaims on success.
     *
     * @param key_id  Public key identifier from the client request
     * @param secret  Raw secret presented by the client
     * @return ApiKeyClaims on successful authentication
     * @throws AuthException on any authentication or validation failure
     */
    ApiKeyClaims authenticate(const std::string& key_id,
                               const std::string& secret);

    /**
     * @brief Parse and authenticate a combined "key_id.secret" string.
     *
     * Splits on the first '.' character and delegates to authenticate().
     *
     * @param combined  String of the form "<key_id>.<secret>"
     * @return ApiKeyClaims on success
     * @throws AuthException if the format is invalid or authentication fails
     */
    ApiKeyClaims authenticateCombined(const std::string& combined);

    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Hash a raw secret using SHA-256.
     *
     * @param secret  Raw secret string
     * @return Lowercase hex-encoded SHA-256 digest (64 characters)
     * @throws AuthException (AUTH_INTERNAL_ERROR) on OpenSSL failure
     */
    static std::string hashSecret(const std::string& secret);

    /**
     * @brief Create a fully initialised ApiKeyCredential.
     *
     * Hashes the raw secret automatically.
     *
     * @param key_id     Public key identifier
     * @param secret     Raw secret (hashed internally; not stored)
     * @param principal  Subject/user principal
     * @param scopes     Authorised scopes
     * @param roles      Optional roles
     * @param tenant_id  Optional tenant identifier
     * @param expires_at Optional expiry (default = no expiry)
     * @return Populated ApiKeyCredential with secret_hash set
     */
    static ApiKeyCredential createCredential(
        const std::string& key_id,
        const std::string& secret,
        const std::string& principal,
        const std::vector<std::string>& scopes = {},
        const std::vector<std::string>& roles = {},
        const std::string& tenant_id = "",
        std::chrono::system_clock::time_point expires_at =
            std::chrono::system_clock::time_point{}
    );

private:
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ApiKeyCredential> credentials_;
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.

    static bool constantTimeEqual(const std::string& a, const std::string& b);
    static std::string hexEncode(const unsigned char* data, size_t len);

    ApiKeyClaims claimsFromCredential(const ApiKeyCredential& cred) const;
};

} // namespace auth
} // namespace themis
