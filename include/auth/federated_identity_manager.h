/**
 * @file federated_identity_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/oidc_provider.h"
#include "auth/jwt_validator.h"
#include "auth/auth_error.h"
#include "auth/auth_audit_logger.h"

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <functional>
#include <optional>
#include <utility>
#include <chrono>

namespace themis {
namespace auth {

struct FederatedValidationResult {
    JWTClaims    claims;      ///< Validated JWT claims
    std::string  realm;       ///< Issuer URL of the realm that validated the token
};

struct CachedValidation {
    FederatedValidationResult result;                   ///< The cached validation result
    std::chrono::system_clock::time_point expires_at;  ///< Wall-clock expiry from JWT exp
};

struct TokenExchangeResult {
    std::string  access_token;        ///< Exchanged access token (raw JWT)
    std::string  issued_token_type;   ///< Token type URI returned by the IdP
    std::string  token_type;          ///< Bearer token type (usually "Bearer")
    int          expires_in{0};       ///< Lifetime in seconds (0 = not provided)
    std::string  scope;               ///< Granted scopes (space-separated, may be empty)
    JWTClaims    claims;              ///< Validated claims from the exchanged token
    std::string  realm;               ///< Issuer URL of the realm that issued the token
};

class FederatedIdentityManager {
public:
    FederatedIdentityManager() = default;

    // Non-copyable (owns OIDCProvider instances)
    FederatedIdentityManager(const FederatedIdentityManager&) = delete;
    FederatedIdentityManager& operator=(const FederatedIdentityManager&) = delete;

    // Movable
    FederatedIdentityManager(FederatedIdentityManager&&) noexcept = default;
    FederatedIdentityManager& operator=(FederatedIdentityManager&&) noexcept = default;

    // -----------------------------------------------------------------------
    // Realm registration
    // -----------------------------------------------------------------------

    /**
     * @brief Add Realm.
     * @param[in] config Input parameter.
     */
    void addRealm(const OIDCProviderConfig& config);

    /**
     * @brief Remove Realm.
     * @param[in] issuer_url Input parameter.
     * @return True when the operation succeeds.
     */
    bool removeRealm(const std::string& issuer_url);

    /**
     * @brief Has Realm.
     * @param[in] issuer_url Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasRealm(const std::string& issuer_url) const;

    /**
     * @brief Realm Issuers.
     * @return Return value.
     */
    std::vector<std::string> realmIssuers() const;

    /**
     * @brief Realm Count.
     * @return Return value.
     */
    size_t realmCount() const;

    // -----------------------------------------------------------------------
    // Token validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate Token.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    FederatedValidationResult validateToken(const std::string& token);

    // -----------------------------------------------------------------------
    // RFC 8693 Token Exchange
    // -----------------------------------------------------------------------

    TokenExchangeResult exchangeToken(
        const std::string& subject_token,
        const std::string& subject_token_type,
        const std::string& requested_token_type,
        const std::vector<std::string>& target_scopes = {});

    /**
     * @brief Realm Provider.
     * @param[in] issuer_url Input parameter.
     * @return Return value.
     */
    OIDCProvider& realmProvider(const std::string& issuer_url);

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(AuthAuditLogger* logger) { audit_logger_ = logger; }

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    void setHttpGetForTesting(
        std::function<std::string(const std::string& url)> fn);

    void setHttpPostForTesting(
        std::function<std::string(const std::string& url,
                                  const std::string& body)> fn);

    /**
     * @brief ----------------------------------------------------------------------- Cross-provider trust registry Records which issuers are trusted by which realms.
     * @param[in] subject_issuer Input parameter.
     * @param[in] trusting_issuer Input parameter.
     * @details Used internally by exchangeToken() to guard cross-realm token exchange. All methods are thread-safe. -----------------------------------------------------------------------
     */

    void addCrossProviderTrust(const std::string& subject_issuer,
                                const std::string& trusting_issuer);

    /**
     * @brief Remove Cross Provider Trust.
     * @param[in] subject_issuer Input parameter.
     * @param[in] trusting_issuer Input parameter.
     * @return True when the operation succeeds.
     */
    bool removeCrossProviderTrust(const std::string& subject_issuer,
                                  const std::string& trusting_issuer);

    /**
     * @brief Is Trusted By.
     * @param[in] subject_issuer Input parameter.
     * @param[in] trusting_issuer Input parameter.
     * @return True when the operation succeeds.
     */
    bool isTrustedBy(const std::string& subject_issuer,
                     const std::string& trusting_issuer) const;

    /**
     * @brief Get Cross Provider Trusts.
     * @param[in] trusting_issuer Input parameter.
     * @return Return value.
     */
    std::vector<std::string> getCrossProviderTrusts(
        const std::string& trusting_issuer) const;

    /**
     * @brief ----------------------------------------------------------------------- Multi-realm distributed trust-state synchronization (ROADMAP §3c) Propagates the local trust registry to a peer node via a simple TCP JSON payload using the existing TBLK/v1 retry pattern.
     * @param[in] peer_node_id Identifier of the peer node.
     * @param[in] peer_rpc_endpoint Input parameter.
     * @details The peer node must expose a JSON-over-TCP listener on @p peer_rpc_endpoint. Wire format: {"op":"sync_trust","entries":[{"subject":"<issuer>","trusting":"<issuer>"},…]} This push throws AuthException(AUTH_INTERNAL_ERROR) if the connect or send fails after all retry attempts; individual attempt failures within the retry budget are logged but swallowed. -----------------------------------------------------------------------
     */

    void syncTrustState(const std::string& peer_node_id,
                        const std::string& peer_rpc_endpoint);

    /**
     * @brief ----------------------------------------------------------------------- In-memory token validation cache validateToken() populates the cache automatically after each successful validation.
     * @param[in] token Input parameter.
     * @param[in] result Input parameter.
     * @details Callers may also query and manage the cache directly. All entries are keyed by the raw bearer token string. -----------------------------------------------------------------------
     */

    void cacheValidationResult(const std::string& token,
                               const FederatedValidationResult& result);

    /**
     * @brief Get Cached Result.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    std::optional<FederatedValidationResult> getCachedResult(
        const std::string& token) const;

    /**
     * @brief Evict Expired Cache Entries.
     * @return Return value.
     */
    size_t evictExpiredCacheEntries();

    /**
     * @brief Clear Token Cache.
     */
    void clearTokenCache();

    /**
     * @brief Token Cache Size.
     * @return Return value.
     */
    size_t tokenCacheSize() const;

private:
    /**
     * @brief Normalize.
     * @param[in] url Input parameter.
     * @return Return value.
     */
    static std::string normalize(const std::string& url);

    /**
     * @brief Extract Issuer.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    static std::string extractIssuer(const std::string& token);

    static std::string buildFormBody(
        const std::vector<std::pair<std::string, std::string>>& params);

    /**
     * @brief Http Post.
     * @param[in] url Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    std::string httpPost(const std::string& url, const std::string& body) const;

    mutable std::mutex mutex_;

    std::unordered_map<std::string, std::shared_ptr<OIDCProvider>> realms_;

    std::function<std::string(const std::string& url)> http_get_fn_;

    std::function<std::string(const std::string& url,
                               const std::string& body)> http_post_fn_;

    AuthAuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.

    // -----------------------------------------------------------------------
    // In-memory token validation cache (cross-provider state sync)
    // Protected by cache_mutex_ (separate from mutex_ to avoid lock inversion
    // when validateToken() holds mutex_ and stores to cache).
    //
    // [W8-16] Cache keys are SHA-256(token) hex strings (64 chars) rather than
    // raw JWT strings, preventing unbounded key growth from large bearer tokens.
    // The LRU order list (cache_lru_order_) enforces kTokenCacheMaxSize cap.
    // -----------------------------------------------------------------------
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, CachedValidation> token_cache_;
    std::list<std::string> cache_lru_order_;

    // -----------------------------------------------------------------------
    // Cross-provider trust registry: trusting_issuer -> {trusted subject issuers}
    // Protected by trust_mutex_.
    // -----------------------------------------------------------------------
    mutable std::mutex trust_mutex_;
    std::unordered_map<std::string, std::unordered_set<std::string>> trust_map_;
};

} // namespace auth
} // namespace themis
