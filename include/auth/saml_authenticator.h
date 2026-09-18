/**
 * @file saml_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
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


// ============================================================================
// Configuration
// ============================================================================

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

struct AuthnRequestParams {
    std::string url;        ///< Full redirect URL (SAMLRequest + optional RelayState)
    std::string request_id; ///< NCName-safe AuthnRequest ID for InResponseTo validation
};

// ============================================================================
// Main class
// ============================================================================

class SAMLAuthenticator {
public:
    /**
     * @brief SAMLAuthenticator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SAMLAuthenticator(const SAMLConfig& config);

    ~SAMLAuthenticator();

    // ----------------------------------------------------------------
    // SP-initiated flow
    // ----------------------------------------------------------------

    AuthnRequestParams buildAuthnRequest(const std::string& relay_state = "") const;

    std::string buildAuthnRequestUrl(const std::string& relay_state = "") const;

    // ----------------------------------------------------------------
    // IdP-response processing
    // ----------------------------------------------------------------

    SAMLClaims processResponse(
        const std::string& saml_response_b64,
        const std::string& in_response_to = "") const;

    // ----------------------------------------------------------------
    // Testing support
    // ----------------------------------------------------------------

    void setClockForTesting(std::function<std::chrono::system_clock::time_point()> clock);

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
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

    /**
     * @brief --- private helpers ---
     */

    void loadIdPCertificate();

    /**
     * @brief Build Authn Request Xml.
     * @param[in] request_id Identifier of the request.
     * @param[in] issue_instant Input parameter.
     * @return Return value.
     */
    std::string buildAuthnRequestXml(const std::string& request_id,
                                     const std::string& issue_instant) const;

    /**
     * @brief Deflate And Base64 Encode.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::string deflateAndBase64Encode(const std::string& input);

    /**
     * @brief Base64 Decode.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> base64Decode(const std::string& input);

    /**
     * @brief Verify Xml Signature.
     * @param[in] reference_xml Input parameter.
     * @param[in] signature_value_b64 Input parameter.
     * @param[in] signed_info_c14n Input parameter.
     * @param[in] digest_value_b64 Input parameter.
     * @param[in] digest_algorithm_uri Input parameter.
     * @param[in] sig_algorithm_uri Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyXmlSignature(const std::string& reference_xml,
                            const std::string& signature_value_b64,
                            const std::string& signed_info_c14n,
                            const std::string& digest_value_b64,
                            const std::string& digest_algorithm_uri,
                            const std::string& sig_algorithm_uri) const;

    /**
     * @brief Parse Date Time.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::chrono::system_clock::time_point parseDateTime(const std::string& s);

    /**
     * @brief Generate Request Id.
     * @return Return value.
     */
    static std::string generateRequestId();

    /**
     * @brief Url Encode.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::string urlEncode(const std::string& input);

    /**
     * @brief Process Response Impl.
     * @param[in] saml_response_b64 Input parameter.
     * @param[in] in_response_to Input parameter.
     * @return Return value.
     */
    SAMLClaims processResponseImpl(const std::string& saml_response_b64,
                                   const std::string& in_response_to) const;

    /**
     * @brief Decrypt Assertion.
     * @param[in] encrypted_assertion_node Input parameter.
     * @return Return value.
     */
    std::string decryptAssertion(const pugi::xml_node& encrypted_assertion_node) const;
};

} // namespace auth
} // namespace themis
