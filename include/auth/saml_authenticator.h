/**
 * @file saml_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <mutex>
#include <functional>

// Forward-declare pugi::xml_node to avoid a full pugixml include in the public header.
namespace pugi { class xml_node; }

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

/**
 * @brief SAML 2.0 identity provider integration
 *
 * Implements Service Provider (SP) functionality for SAML 2.0 SSO:
 *  - SP-initiated AuthnRequest generation (HTTP-Redirect binding)
 *  - SAMLResponse validation (HTTP-POST binding)
 *  - XML signature verification (RSA-SHA256 and RSA-SHA1)
 *  - Assertion condition enforcement (NotBefore, NotOnOrAfter, Audience)
 *  - Replay-attack prevention via TTL-based AssertionID cache
 *
 * Standards references:
 *  - SAML 2.0 Core: https://docs.oasis-open.org/security/saml/v2.0/saml-core-2.0-os.pdf
 *  - SAML 2.0 Bindings: https://docs.oasis-open.org/security/saml/v2.0/saml-bindings-2.0-os.pdf
 *
 * Known limitations:
 *  - XML signature verification uses raw XML serialization instead of Exclusive
 *    Canonicalization (C14N, https://www.w3.org/TR/xml-exc-c14n/). This means
 *    assertions whose canonical form differs from their serialized form due to
 *    namespace prefixes or whitespace normalization may fail verification when
 *    interoperating with strict IdPs. Production deployments requiring strict
 *    C14N compliance should integrate a C14N library (e.g., libxml2 c14n support).
 *  - Encrypted assertions (EncryptedAssertion element) are supported when
 *    SAMLConfig::sp_private_key_loader is provided.  AES-128-CBC and AES-256-CBC
 *    data encryption with RSA-OAEP or RSA-PKCS1-v1.5 key transport are supported.
 *    When sp_private_key_loader is not configured and an EncryptedAssertion is
 *    received, SAML_DECRYPTION_FAILED is thrown with a clear diagnostic message.
 *  - The in-process replay cache does not survive process restarts; high-availability
 *    deployments should use a shared TTL store (e.g., Redis).
 *
 * Compliance: SOC 2 CC6.1, NIST SP 800-63C Federation Assurance Level 2
 */

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief SAML 2.0 Service Provider configuration
 */
struct SAMLConfig {
    // SP identity
    std::string sp_entity_id;       ///< SP EntityID URI (e.g. "https://myapp.example.com/saml/metadata")
    std::string sp_acs_url;         ///< Assertion Consumer Service URL (receives POST from IdP)

    // IdP metadata
    std::string idp_sso_url;        ///< IdP Single Sign-On URL (HTTP-Redirect binding)
    std::string idp_entity_id;      ///< IdP EntityID (validated in response)
    std::string idp_certificate_pem;///< IdP X.509 certificate in PEM format (used to verify signatures)

    // Validation options
    std::chrono::seconds clock_skew{60};          ///< Allowed clock skew for NotBefore/NotOnOrAfter
    bool require_signed_response{true};            ///< Whether SAMLResponse element must be signed
    bool require_signed_assertion{true};           ///< Whether Assertion element must be signed
    bool require_encrypted_assertion{false};       ///< When true, plain (unencrypted) Assertions are rejected with SAML_INVALID_RESPONSE. Use together with sp_private_key_loader to enforce encrypted-only assertion delivery.
    size_t max_replay_cache_size{100000};          ///< Maximum number of assertion IDs to keep in the in-memory replay cache

    // SP private key loader for assertion decryption (EncryptedAssertion support).
    // Called lazily when a SAMLResponse with an EncryptedAssertion element is received.
    // The callback MUST return an *unencrypted* (passphrase-free) PEM-encoded PKCS#8 or
    // PKCS#1 RSA private key string.  Passphrase-protected PEM keys are not supported
    // by the decryption path; decrypt the key before returning it from the loader.
    // Return an empty string to signal that the key is unavailable.
    //
    // Security note: NEVER store the private key as a hardcoded string. Load it
    // from a hardware security module (HSM), key management service (KMS), or
    // a secrets manager (e.g. HashiCorp Vault, AWS Secrets Manager).
    // Example (environment variable – minimum acceptable for non-production):
    //   cfg.sp_private_key_loader = []() {
    //       const char* p = std::getenv("SP_PRIVATE_KEY_PEM");
    //       return p ? std::string(p) : std::string{};
    //   };
    std::function<std::string()> sp_private_key_loader;

    // Attribute mapping (IdP attribute name → local claim name)
    std::string attr_email{"email"};               ///< Attribute name carrying the user's email
    std::string attr_name_id_format{              ///< Expected NameID Format
        "urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress"};

    // Optional: NameID policy to request in AuthnRequest
    std::string requested_authn_context{
        "urn:oasis:names:tc:SAML:2.0:ac:classes:PasswordProtectedTransport"};

    // Algorithm security policy.
    // SHA-1 based digest and signature algorithms (e.g. http://www.w3.org/2000/09/xmldsig#sha1,
    // http://www.w3.org/2000/09/xmldsig#rsa-sha1) are cryptographically broken and
    // MUST NOT be accepted in new deployments (CWE-327, NIST SP 800-131A rev. 2).
    // Set to true ONLY for temporary backward compatibility with legacy IdPs that
    // cannot yet be migrated to SHA-256.  Log a security warning each time SHA-1
    // is encountered regardless of this setting.
    bool allow_sha1_deprecated{false};
};

// ============================================================================
// Claims extracted from SAML Assertion
// ============================================================================

/**
 * @brief Claims extracted from a validated SAML 2.0 Assertion
 */
struct SAMLClaims {
    std::string subject_name_id;                  ///< NameID value (typically email or unique opaque ID)
    std::string name_id_format;                   ///< NameID Format URI
    std::string email;                            ///< Email extracted from configured attribute (may equal subject_name_id)
    std::string issuer;                           ///< Issuer (IdP EntityID)
    std::string session_index;                    ///< AuthnStatement SessionIndex (for SLO)
    std::string assertion_id;                     ///< Assertion ID (used for replay detection)

    std::chrono::system_clock::time_point issued_at;       ///< IssueInstant of the Assertion
    std::chrono::system_clock::time_point not_before;      ///< SubjectConfirmationData NotBefore
    std::chrono::system_clock::time_point not_on_or_after; ///< SubjectConfirmationData NotOnOrAfter

    std::vector<std::string> attributes_groups;   ///< Values of any "groups" / "memberOf" attribute
    std::vector<std::string> attributes_roles;    ///< Values of any "roles" attribute
    std::vector<std::string> audience;            ///< AudienceRestriction values

    // Raw attributes for extensibility
    std::vector<std::pair<std::string, std::string>> raw_attributes; ///< All IdP attributes (name, value)
};

// ============================================================================
// SP-initiated AuthnRequest result
// ============================================================================

/**
 * @brief Result of building an SP-initiated AuthnRequest
 *
 * Carries both the redirect URL and the generated request ID so that the
 * caller can store the request ID and pass it to processResponse() as
 * @c in_response_to for SP-initiated flow InResponseTo validation.
 */
struct AuthnRequestParams {
    std::string url;        ///< Full redirect URL (SAMLRequest + optional RelayState)
    std::string request_id; ///< NCName-safe AuthnRequest ID for InResponseTo validation
};

// ============================================================================
// Main class
// ============================================================================

/**
 * @brief SAML 2.0 Service Provider authenticator
 *
 * Thread-safe after construction (all public methods are thread-safe).
 */
class SAMLAuthenticator {
public:
    /**
     * @brief Construct with Service Provider configuration
     * @throws std::invalid_argument if required config fields are empty
     * @throws std::runtime_error if the IdP certificate cannot be parsed
     */
    explicit SAMLAuthenticator(const SAMLConfig& config);

    ~SAMLAuthenticator();

    // ----------------------------------------------------------------
    // SP-initiated flow
    // ----------------------------------------------------------------

    /**
     * @brief Build an SP-initiated AuthnRequest for HTTP-Redirect binding
     *
     * Returns an AuthnRequestParams carrying both the redirect URL and the
     * generated request ID.  Store @c request_id and pass it as
     * @c in_response_to to processResponse() to enable InResponseTo
     * validation of the IdP response (SP-initiated flow).
     *
     * @param relay_state Optional opaque string passed through the IdP round-trip
     *                    (e.g. original requested URL).  URL-safe; max 80 chars.
     * @return AuthnRequestParams with redirect URL and request ID
     * @throws std::runtime_error on encoding error
     */
    AuthnRequestParams buildAuthnRequest(const std::string& relay_state = "") const;

    /**
     * @brief Convenience wrapper: build an SP-initiated AuthnRequest URL only
     *
     * Equivalent to @c buildAuthnRequest(relay_state).url.  Use
     * @c buildAuthnRequest() instead when InResponseTo validation is needed.
     *
     * @param relay_state Optional opaque string passed through the IdP round-trip
     * @return Full redirect URL including SAMLRequest (and RelayState if provided)
     * @throws std::runtime_error on encoding error
     */
    std::string buildAuthnRequestUrl(const std::string& relay_state = "") const;

    // ----------------------------------------------------------------
    // IdP-response processing
    // ----------------------------------------------------------------

    /**
     * @brief Process and validate a Base64-encoded SAMLResponse from the IdP
     *
     * Performs these checks in order:
     *  1. Base64-decode the response
     *  2. Parse XML with pugixml
     *  3. Verify top-level Status is Success
     *  3b. Validate Response-level InResponseTo against in_response_to (SP-initiated)
     *  4. Verify XML signature on Response (if require_signed_response)
     *  5. Verify XML signature on Assertion (if require_signed_assertion)
     *  6. Validate Issuer against configured idp_entity_id
     *  7. Validate Conditions: NotBefore / NotOnOrAfter / AudienceRestriction
     *  8. Validate SubjectConfirmationData InResponseTo / Recipient / NotOnOrAfter
     *  9. Replay detection on AssertionID
     * 10. Extract SAMLClaims from Assertion
     *
     * @param saml_response_b64 Raw Base64-encoded SAMLResponse POST body value
     * @param in_response_to    If non-empty, the ID of the original AuthnRequest
     *                          (from AuthnRequestParams::request_id); enables
     *                          SP-initiated InResponseTo validation
     * @return Validated SAMLClaims
     * @throws AuthException (SAML_*) on any validation failure
     */
    SAMLClaims processResponse(
        const std::string& saml_response_b64,
        const std::string& in_response_to = "") const;

    // ----------------------------------------------------------------
    // Testing support
    // ----------------------------------------------------------------

    /**
     * @brief Override the current time for testing time-sensitive conditions
     */
    void setClockForTesting(std::function<std::chrono::system_clock::time_point()> clock);

    /**
     * @brief Attach an AuditLogger to receive LOGIN_SUCCESS / LOGIN_FAILED events.
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

private:
    SAMLConfig config_;
    void* idp_public_key_{nullptr}; ///< EVP_PKEY* for IdP certificate (opaque to avoid OpenSSL headers)
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning, optional.

    // Replay-attack prevention: maps assertion ID -> expiry time (NotOnOrAfter + clock_skew).
    // Expired entries are evicted lazily on each processResponse() call.
    mutable std::mutex replay_cache_mutex_;
    mutable std::unordered_map<std::string, std::chrono::system_clock::time_point> seen_assertion_ids_;

    // Pluggable clock (default: system clock)
    std::function<std::chrono::system_clock::time_point()> clock_;

    // --- private helpers ---

    /// Load IdP public key from PEM certificate string
    void loadIdPCertificate();

    /// Build the SAMLRequest XML string
    std::string buildAuthnRequestXml(const std::string& request_id,
                                     const std::string& issue_instant) const;

    /// DEFLATE + Base64 encode (for HTTP-Redirect binding)
    static std::string deflateAndBase64Encode(const std::string& input);

    /// Standard Base64 decode (for SAMLResponse body)
    static std::vector<uint8_t> base64Decode(const std::string& input);

    /// Verify an enveloped XML signature using the IdP public key
    /// @param reference_xml  Raw XML of the signed element
    /// @param signature_value_b64  Base64-encoded signature bytes
    /// @param signed_info_c14n  Canonicalized SignedInfo XML for digest verification
    /// @param digest_value_b64 Base64-encoded digest value from Reference
    /// @param digest_algorithm_uri URI identifying digest algorithm
    /// @param sig_algorithm_uri    URI identifying signature algorithm
    bool verifyXmlSignature(const std::string& reference_xml,
                            const std::string& signature_value_b64,
                            const std::string& signed_info_c14n,
                            const std::string& digest_value_b64,
                            const std::string& digest_algorithm_uri,
                            const std::string& sig_algorithm_uri) const;

    /// Parse RFC 3339 / ISO 8601 datetime strings used in SAML
    static std::chrono::system_clock::time_point parseDateTime(const std::string& s);

    /// Generate a unique ID for AuthnRequest (NCName-safe)
    static std::string generateRequestId();

    /// URL-encode a string (for SAMLRequest query parameter)
    static std::string urlEncode(const std::string& input);

    /// Internal implementation of processResponse (called by the public method)
    SAMLClaims processResponseImpl(const std::string& saml_response_b64,
                                   const std::string& in_response_to) const;

    /// Decrypt an EncryptedAssertion element using the SP private key loaded via
    /// SAMLConfig::sp_private_key_loader.  Supports AES-128-CBC and AES-256-CBC
    /// data encryption with RSA-OAEP (default) or RSA-PKCS1-v1.5 key transport.
    ///
    /// @param encrypted_assertion_node  The <EncryptedAssertion>lt;EncryptedAssertion<EncryptedAssertion>gt; pugixml node
    /// @return Decrypted assertion XML as a UTF-8 string
    /// @throws AuthException (SAML_DECRYPTION_FAILED) on any failure
    std::string decryptAssertion(const pugi::xml_node& encrypted_assertion_node) const;
};

} // namespace auth
} // namespace themis
