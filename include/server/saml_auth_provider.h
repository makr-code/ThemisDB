/**
 * @file saml_auth_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/saml_authenticator.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>

namespace themis {
namespace server {

/**
 * @brief SAML 2.0 Service Provider HTTP handler
 *
 * Bridges the auth-layer SAMLAuthenticator with the HTTP server routing.
 * Exposes the following endpoints:
 *
 *   GET  /api/v1/auth/saml/login
 *     Redirect the browser to the IdP SSO URL (SP-initiated flow).
 *     Query param: relay_state (optional, max 80 chars, URL-safe)
 *     Response: 302 redirect to IdP
 *
 *   POST /api/v1/auth/saml/acs
 *     Assertion Consumer Service.  Receives the SAMLResponse POST from the IdP,
 *     validates it and returns an internal session token.
 *     Form body: SAMLResponse=<base64>&RelayState=<optional>
 *     On success: { "token": "...", "user_id": "...", "email": "...", "relay_state": "..." }
 *     On failure: 401 / 403 with JSON error
 *
 *   POST /api/v1/auth/saml/slo
 *     Initiates Single Logout.  Accepts an authenticated request carrying the
 *     bearer token issued after ACS and triggers the IdP SLO flow if configured.
 *     Body (optional): { "session_index": "..." }
 *     Response: { "success": true } or redirect to IdP SLO URL
 *
 *   GET  /api/v1/auth/saml/metadata
 *     Returns the SP SAML metadata XML document.
 *     Response: Content-Type: application/samlmetadata+xml
 *
 * Thread-safe after construction.
 */
class SamlAuthProvider {
public:
    /**
     * @brief Configuration for the server-layer SAML provider
     */
    struct Config {
        /// Underlying SAML SP configuration (IdP cert, entity IDs, ACS URL, etc.)
        auth::SAMLConfig saml;

        /// IdP Single Logout URL (HTTP-Redirect binding).  Optional.
        /// When set, handleSlo() issues a redirect to this URL.
        std::string idp_slo_url;

        /// SP Single Logout URL exposed in metadata.  Optional.
        std::string sp_slo_url;

        /// Optional: organisation name for SP metadata
        std::string org_name;
        std::string org_display_name;
        std::string org_url;

        /// Optional: technical contact email for SP metadata
        std::string contact_email;

        /// Token factory: given SAMLClaims, produce an internal session token.
        /// Default implementation returns a UUID prefixed with "saml_".
        std::function<std::string(const auth::SAMLClaims&)> token_factory;
    };

    /**
     * @brief Construct from configuration.
     * @throws std::invalid_argument if required config fields are empty.
     * @throws std::runtime_error   if the IdP certificate cannot be parsed.
     */
    explicit SamlAuthProvider(const Config& config);

    ~SamlAuthProvider() = default;

    // Non-copyable, movable
    SamlAuthProvider(const SamlAuthProvider&) = delete;
    SamlAuthProvider& operator=(const SamlAuthProvider&) = delete;
    SamlAuthProvider(SamlAuthProvider&&) noexcept = default;
    SamlAuthProvider& operator=(SamlAuthProvider&&) noexcept = default;

    // -----------------------------------------------------------------------
    // HTTP handlers – each returns a JSON result with an embedded status_code
    // field on error (mirroring the SessionApiHandler convention).
    // -----------------------------------------------------------------------

    /**
     * @brief Handle GET /api/v1/auth/saml/login
     *
     * Builds and returns the IdP redirect URL so the caller can issue a 302.
     * Also stores the generated request_id in an in-process pending-requests
     * map for InResponseTo validation in handleAcs().
     *
     * @param relay_state Optional opaque value forwarded via the IdP round-trip.
     * @return JSON: { "redirect_url": "...", "request_id": "..." }
     */
    nlohmann::json handleLogin(const std::string& relay_state = "");

    /**
     * @brief Handle POST /api/v1/auth/saml/acs
     *
     * Validates the SAMLResponse, issues an internal token, and returns claims.
     * Consumes the stored request_id for the InResponseTo check if present.
     *
     * @param saml_response_b64  Base64-encoded SAMLResponse form field.
     * @param relay_state        Optional RelayState form field.
     * @param in_response_to     Optional request_id from a prior handleLogin().
     * @return JSON: { "token": "...", "user_id": "...", "email": "...",
     *                 "issuer": "...", "relay_state": "...", "attributes": {...} }
     *         or error JSON with "status_code" key.
     */
    nlohmann::json handleAcs(
        const std::string& saml_response_b64,
        const std::string& relay_state = "",
        const std::string& in_response_to = "");

    /**
     * @brief Handle POST /api/v1/auth/saml/slo
     *
     * When idp_slo_url is configured, returns a redirect to the IdP SLO
     * endpoint.  Otherwise acknowledges logout with { "success": true }.
     *
     * @note This implementation builds a simplified redirect URL without a
     *       fully signed SAMLLogoutRequest element.  SAML 2.0 Bindings §3.4
     *       requires a DEFLATE-compressed and Base64-encoded LogoutRequest;
     *       a standards-compliant, signed LogoutRequest can be added in a
     *       future enhancement once SP private-key support is available.
     *       The current implementation is suitable for IdPs that accept a basic
     *       redirect carrying the issuer parameter.
     *
     * @param session_index  Optional SAML SessionIndex to include in SLO request.
     * @return JSON: { "success": true } or { "redirect_url": "...", "success": true }
     */
    nlohmann::json handleSlo(const std::string& session_index = "");

    /**
     * @brief Handle GET /api/v1/auth/saml/metadata
     *
     * Generates the SP SAML 2.0 metadata XML document.
     *
     * @return XML string (Content-Type: application/samlmetadata+xml)
     */
    std::string buildMetadataXml() const;

    /**
     * @brief Override the clock used inside SAMLAuthenticator (for testing).
     */
    void setClockForTesting(
        std::function<std::chrono::system_clock::time_point()> clock);

private:
    Config config_;
    std::unique_ptr<auth::SAMLAuthenticator> authenticator_;

    /// In-flight SP-initiated request IDs → expiry (to prevent replay of stale IDs).
    mutable std::mutex pending_mutex_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> pending_requests_;

    /// Evict expired pending request IDs (called lazily under lock).
    void evictExpiredPendingRequests();

    /// Build a standardised error JSON object.
    static nlohmann::json makeError(int status_code, const std::string& message);

    /// Default token factory: returns "saml_<uuid>".
    static std::string defaultTokenFactory(const auth::SAMLClaims& claims);

    /// URL-encode a query parameter value.
    static std::string urlEncode(const std::string& input);
};

} // namespace server
} // namespace themis

