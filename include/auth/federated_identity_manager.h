/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            federated_identity_manager.h                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:52:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     203                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a0247640c  2026-02-24  feat(auth): implement federated identity across multiple ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "auth/oidc_provider.h"
#include "auth/jwt_validator.h"
#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <optional>

namespace themis {
namespace auth {

/**
 * @brief Result of a federated token validation
 *
 * Carries the validated claims together with the realm (issuer URL) that
 * successfully validated the token.
 */
struct FederatedValidationResult {
    JWTClaims    claims;      ///< Validated JWT claims
    std::string  realm;       ///< Issuer URL of the realm that validated the token
};

/**
 * @brief Manages federated identity across multiple OIDC realms
 *
 * Allows ThemisDB to accept tokens issued by any of a set of registered OIDC
 * identity providers (realms).  When a token arrives the manager inspects its
 * @c iss claim, locates the matching realm, and delegates full JWT validation
 * to the corresponding OIDCProvider.
 *
 * Thread safety: all public methods are safe to call concurrently.
 *
 * Typical usage:
 * @code
 *   FederatedIdentityManager fed;
 *
 *   OIDCProviderConfig prod_cfg;
 *   prod_cfg.issuer_url = "https://keycloak.example.com/realms/production";
 *   prod_cfg.client_id  = "themisdb";
 *   fed.addRealm(prod_cfg);
 *
 *   OIDCProviderConfig dev_cfg;
 *   dev_cfg.issuer_url = "https://keycloak.example.com/realms/development";
 *   dev_cfg.client_id  = "themisdb";
 *   fed.addRealm(dev_cfg);
 *
 *   // Validate a bearer token without knowing which realm issued it
 *   FederatedValidationResult result = fed.validateToken(bearer_token);
 *   std::cout << "Authenticated via realm: " << result.realm << "\n";
 *   std::cout << "Subject: " << result.claims.sub << "\n";
 * @endcode
 */
class FederatedIdentityManager {
public:
    FederatedIdentityManager() = default;

    // Non-copyable (owns OIDCProvider instances)
    FederatedIdentityManager(const FederatedIdentityManager&) = delete;
    FederatedIdentityManager& operator=(const FederatedIdentityManager&) = delete;

    // Movable
    FederatedIdentityManager(FederatedIdentityManager&&) = default;
    FederatedIdentityManager& operator=(FederatedIdentityManager&&) = default;

    // -----------------------------------------------------------------------
    // Realm registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register a new OIDC realm.
     *
     * The trailing slash of @p config.issuer_url (if any) is stripped before
     * registration so that "https://idp.example.com/realms/x" and
     * "https://idp.example.com/realms/x/" are treated as the same realm.
     *
     * @param config  Provider configuration for the realm.
     * @throws AuthException(AUTH_CONFIG_INVALID) if @p config.issuer_url is
     *         empty or if a realm with the same normalized issuer URL is
     *         already registered.
     */
    void addRealm(const OIDCProviderConfig& config);

    /**
     * @brief Remove a previously registered realm.
     *
     * @param issuer_url  Issuer URL of the realm to remove (trailing slash
     *                    normalized automatically).
     * @return true if the realm was found and removed, false otherwise.
     */
    bool removeRealm(const std::string& issuer_url);

    /**
     * @brief Check whether a realm is registered.
     *
     * @param issuer_url  Issuer URL (trailing slash normalized automatically).
     */
    bool hasRealm(const std::string& issuer_url) const;

    /**
     * @brief Return the normalized issuer URLs of all registered realms.
     */
    std::vector<std::string> realmIssuers() const;

    /**
     * @brief Return the number of registered realms.
     */
    size_t realmCount() const;

    // -----------------------------------------------------------------------
    // Token validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate a bearer token against the realm that issued it.
     *
     * The method peeks at the token's @c iss claim (without full validation)
     * to identify the responsible realm, then delegates to that realm's
     * OIDCProvider::validateToken().
     *
     * @param token  JWT bearer token (with or without "Bearer " prefix).
     * @return FederatedValidationResult containing the validated claims and
     *         the matched realm's issuer URL.
     * @throws AuthException(JWT_ISSUER_MISMATCH) if the token's issuer does
     *         not match any registered realm.
     * @throws AuthException on any other validation failure (expired,
     *         invalid signature, audience mismatch, …).
     */
    FederatedValidationResult validateToken(const std::string& token);

    /**
     * @brief Access a specific realm's OIDCProvider.
     *
     * Calls discover() lazily if the provider has not yet fetched its
     * discovery document.
     *
     * @param issuer_url  Issuer URL (trailing slash normalized automatically).
     * @return Reference to the OIDCProvider for that realm.
     * @throws AuthException(AUTH_CONFIG_INVALID) if no realm with the given
     *         issuer URL is registered.
     */
    OIDCProvider& realmProvider(const std::string& issuer_url);

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Override the HTTP GET function injected into every realm's
     *        OIDCProvider (for unit tests only).
     *
     * Must be called *before* addRealm() for the hook to apply, or use
     * realmProvider() to inject per-realm after registration.
     */
    void setHttpGetForTesting(
        std::function<std::string(const std::string& url)> fn);

private:
    /// Normalize an issuer URL by stripping trailing slashes.
    static std::string normalize(const std::string& url);

    /// Peek at the JWT payload and extract the "iss" claim without
    /// performing any cryptographic verification.
    static std::string extractIssuer(const std::string& token);

    mutable std::mutex mutex_;

    /// issuer_url (normalized) -> OIDCProvider
    std::unordered_map<std::string, std::shared_ptr<OIDCProvider>> realms_;

    /// Optional HTTP mock injected for testing; applied to all new realms
    std::function<std::string(const std::string& url)> http_get_fn_;
};

} // namespace auth
} // namespace themis
