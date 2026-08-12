/**
 * @file webauthn_authenticator.h
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
#include <optional>
#include <functional>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

/**
 * @brief WebAuthn/FIDO2 authenticator for hardware keys and biometrics
 *
 * Implements W3C WebAuthn Level 2 specification for phishing-resistant
 * authentication using hardware security keys (YubiKey, Titan Key, etc.),
 * platform authenticators (Touch ID, Face ID, Windows Hello), or passkeys.
 *
 * Supported credential algorithms:
 *   - ES256  (ECDSA-P256-SHA256)   – preferred; used by all modern hardware keys
 *   - RS256  (RSA-PKCS1v1.5-SHA256) – fallback for legacy TPM-based tokens
 *
 * Typical usage:
 * @code
 *   WebAuthnAuthenticator wa({"example.com", "My App"});
 *   wa.setExpectedOrigin("https://example.com");
 *
 *   // Registration
 *   auto opts = wa.startRegistration({user_id, email, name});
 *   // send opts.to_json() to client → get credential_response
 *   auto reg = wa.completeRegistration(credential_response);
 *   // store reg.credential_id, reg.public_key, reg.sign_count in DB
 *
 *   // Authentication
 *   auto req = wa.startAuthentication();
 *   // send req.to_json() to client → get assertion_response
 *   auto asr = wa.completeAuthentication(assertion_response,
 *                                        stored_public_key, stored_sign_count);
 *   // update stored sign_count to asr.sign_count
 * @endcode
 *
 * Security considerations:
 *   - Challenges are single-use (replay prevention) with configurable TTL.
 *   - Signature counter is verified; a rollback indicates a cloned token.
 *   - RP ID hash prevents cross-origin credential reuse.
 *   - User Presence (UP) flag is mandatory for all operations.
 *
 * Compliance: W3C WebAuthn Level 2, FIDO2 CTAP2, RFC 8152 (COSE keys)
 */
class WebAuthnAuthenticator {
public:
    // -----------------------------------------------------------------------
    // Configuration / data structures
    // -----------------------------------------------------------------------

    /**
     * @brief Relying Party (server) identification
     */
    struct RelyingParty {
        std::string id;    ///< Effective domain, e.g. "example.com"
        std::string name;  ///< Human-readable name, e.g. "ThemisDB"
    };

    /**
     * @brief End-user information attached to a registered credential
     */
    struct User {
        std::string id;            ///< Opaque user ID (stored in the authenticator)
        std::string name;          ///< Username or email
        std::string display_name;  ///< Full display name
    };

    /**
     * @brief Authenticator selection preferences for registration
     */
    struct AuthenticatorSelection {
        /// "platform" (e.g. TPM, Secure Enclave) or "cross-platform" (USB/NFC keys)
        std::optional<std::string> authenticator_attachment;
        bool require_resident_key{false};
        /// "required" | "preferred" | "discouraged"
        std::string user_verification{"preferred"};
    };

    /**
     * @brief Options sent to the client for navigator.credentials.create()
     *
     * Serialise with to_json() and deliver to the browser / native app.
     */
    struct CredentialCreationOptions {
        std::string challenge;  ///< Base64url-encoded 32-byte random challenge
        RelyingParty rp;
        User user;
        std::vector<std::string> pub_key_cred_params;  ///< ["ES256", "RS256"]
        std::optional<int> timeout_ms;
        std::string attestation{"none"};  ///< "none" | "indirect" | "direct"
        AuthenticatorSelection authenticator_selection;
        std::vector<std::string> exclude_credentials;  ///< Prevent duplicate registration

        nlohmann::json to_json() const;
    };

    /**
     * @brief Options sent to the client for navigator.credentials.get()
     *
     * Serialise with to_json() and deliver to the browser / native app.
     */
    struct CredentialRequestOptions {
        std::string challenge;  ///< Base64url-encoded 32-byte random challenge
        std::string rp_id;
        std::optional<int> timeout_ms;
        /// "required" | "preferred" | "discouraged"
        std::string user_verification{"preferred"};
        /// Credential IDs to allow (empty = discoverable credential / passkey flow)
        std::vector<std::string> allow_credentials;

        nlohmann::json to_json() const;
    };

    /**
     * @brief Result of a successful registration ceremony
     *
     * Persist all fields in your user/credential store.
     */
    struct AttestationResult {
        std::string credential_id;        ///< Base64url-encoded credential identifier
        std::vector<uint8_t> public_key;  ///< DER-encoded SubjectPublicKeyInfo (SPKI)
        std::string algorithm;            ///< "ES256" or "RS256"
        uint32_t sign_count{0};           ///< Initial signature counter value
        std::vector<uint8_t> aaguid;      ///< 16-byte authenticator model GUID
    };

    /**
     * @brief Result of a successful authentication ceremony
     *
     * Update the stored sign_count to the new value to detect cloned tokens.
     */
    struct AssertionResult {
        std::string credential_id;          ///< Identifies which credential was used
        uint32_t sign_count{0};             ///< New counter value (store this)
        std::optional<std::string> user_handle;  ///< Set for discoverable credentials
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Construct with Relying Party configuration
     *
     * @param rp  RP domain and display name
     * @throws AuthException(AUTH_CONFIG_INVALID) if rp.id is empty
     */
    explicit WebAuthnAuthenticator(const RelyingParty& rp);
    ~WebAuthnAuthenticator() = default;

    /**
     * @brief Attach an AuditLogger for security event recording
     *
     * Pass nullptr to detach. Does NOT take ownership of the pointer.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    // -----------------------------------------------------------------------
    // Registration ceremony
    // -----------------------------------------------------------------------

    /**
     * @brief Begin registration – generate credential creation options
     *
     * Generates a cryptographically secure challenge that is stored
     * internally for verification in completeRegistration().
     *
     * @param user         User information to embed in the credential
     * @param resident_key If true, requests a discoverable credential (passkey)
     * @return Options to serialise and send to the client
     */
    CredentialCreationOptions startRegistration(
        const User& user,
        bool resident_key = false
    );

    /**
     * @brief Complete registration – verify attestation response
     *
     * Verifies the challenge, origin, RP ID, and authenticator data from
     * the JSON object returned by navigator.credentials.create().
     *
     * @param credential_response  Parsed JSON from the client
     * @return AttestationResult containing the public key and credential ID
     *
     * @throws AuthException(AUTH_TOKEN_INVALID)      – bad/expired challenge, wrong origin/RP
     * @throws AuthException(AUTH_INVALID_CREDENTIALS) – UP flag missing
     * @throws AuthException(AUTH_NOT_IMPLEMENTED)    – unsupported key algorithm
     * @throws AuthException(AUTH_INTERNAL_ERROR)     – CBOR/crypto failure
     */
    AttestationResult completeRegistration(const nlohmann::json& credential_response);

    // -----------------------------------------------------------------------
    // Authentication ceremony
    // -----------------------------------------------------------------------

    /**
     * @brief Begin authentication – generate credential request options
     *
     * @param user_id  Optional; when provided the caller is responsible for
     *                 populating allow_credentials with that user's credential IDs
     *                 before sending options to the client.
     * @return Options to serialise and send to the client
     */
    CredentialRequestOptions startAuthentication(
        const std::optional<std::string>& user_id = std::nullopt
    );

    /**
     * @brief Complete authentication – verify assertion response
     *
     * Verifies the challenge, origin, RP ID, signature counter, and ECDSA/RSA
     * signature from the JSON object returned by navigator.credentials.get().
     *
     * @param credential_response  Parsed JSON from the client
     * @param stored_public_key    DER-encoded SPKI from AttestationResult::public_key
     * @param stored_sign_count    Last known counter value (0 if unknown)
     * @return AssertionResult with updated sign counter and optional user handle
     *
     * @throws AuthException(AUTH_TOKEN_INVALID)      – bad/expired challenge, wrong origin/RP,
     *                                                   counter rollback (cloned token)
     * @throws AuthException(AUTH_INVALID_CREDENTIALS) – UP flag missing or bad signature
     * @throws AuthException(AUTH_INTERNAL_ERROR)     – crypto failure
     */
    AssertionResult completeAuthentication(
        const nlohmann::json& credential_response,
        const std::vector<uint8_t>& stored_public_key,
        uint32_t stored_sign_count
    );

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Override the CSPRNG source for deterministic tests
     *
     * The function must fill exactly @p len bytes into @p buf.
     * If not set, OpenSSL RAND_bytes is used.
     */
    void setRandBytesForTesting(
        std::function<void(unsigned char* buf, std::size_t len)> fn
    );

    /**
     * @brief Override the expected client origin (default: "https://{rp.id}")
     *
     * Useful for test environments where the origin is "http://localhost:3000"
     * or similar non-HTTPS origins.
     */
    void setExpectedOrigin(const std::string& origin);

private:
    RelyingParty rp_;
    std::string  expected_origin_;

    utils::AuditLogger* audit_logger_{nullptr};
    std::function<void(unsigned char*, std::size_t)> rand_bytes_fn_;

    // Pending challenges: base64url challenge → expiry timestamp
    struct PendingEntry {
        std::chrono::system_clock::time_point expires_at;
    };
    std::unordered_map<std::string, PendingEntry> pending_challenges_;
    std::mutex challenges_mutex_;
    static constexpr std::chrono::seconds kChallengeTTL{300};  // 5 minutes

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /// Generate 32 random bytes encoded as base64url; store in pending_challenges_
    std::string generateChallenge();

    /// Verify @p challenge_b64url is in the pending set and not expired; remove it
    void verifyAndConsumeChallenge(const std::string& challenge_b64url);

    /// Remove all entries whose expiry is in the past
    void purgeExpiredChallenges();

    /// Fill @p buf with @p len cryptographically random bytes
    void fillRandomBytes(unsigned char* buf, std::size_t len);

    // Cryptographic primitives
    static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> sha256(const std::string& data);

    // Base64URL codec (RFC 4648 §5, no padding)
    static std::string         base64UrlEncode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> base64UrlDecode(const std::string& input);

    // Parsed fields from the clientDataJSON byte sequence
    struct ClientData {
        std::string type;
        std::string challenge;  ///< base64url
        std::string origin;
    };
    static ClientData parseClientDataJSON(const std::vector<uint8_t>& client_data_json);

    // Parsed fields from the binary authenticatorData structure
    struct AuthData {
        std::array<uint8_t, 32> rp_id_hash{};
        uint8_t  flags{0};
        uint32_t sign_count{0};

        // Only present when the AT flag (bit 6) is set
        bool has_attested_credential{false};
        std::vector<uint8_t> aaguid;         ///< 16 bytes
        std::string          credential_id;  ///< base64url
        std::vector<uint8_t> cose_key_bytes; ///< raw CBOR of the credential public key
    };
    static AuthData parseAuthData(const std::vector<uint8_t>& auth_data_bytes);

    /**
     * @brief Decode a CBOR-encoded attestation object
     *
     * @param cbor_bytes   Raw bytes of the attestationObject
     * @param fmt          [out] attestation format ("none", "packed", …)
     * @param auth_data    [out] raw authenticatorData bytes
     */
    static void parseAttestationObject(
        const std::vector<uint8_t>& cbor_bytes,
        std::string& fmt,
        std::vector<uint8_t>& auth_data
    );

    /**
     * @brief Parse a CBOR COSE key and return DER-encoded SPKI + algorithm name
     *
     * Supports ES256 (ECDSA-P256) and RS256 (RSA PKCS#1 v1.5).
     *
     * @return {der_spki_bytes, "ES256" or "RS256"}
     * @throws AuthException(AUTH_NOT_IMPLEMENTED) for unsupported key types
     */
    static std::pair<std::vector<uint8_t>, std::string> coseKeyToSpki(
        const std::vector<uint8_t>& cose_key_bytes
    );

    /**
     * @brief Verify an ES256 or RS256 WebAuthn signature
     *
     * The signed message is: authenticatorData || SHA256(clientDataJSON).
     *
     * @param auth_data_bytes   Raw authenticatorData bytes
     * @param client_data_hash  SHA-256 of the raw clientDataJSON bytes
     * @param signature_bytes   Signature from the assertion response
     * @param spki_bytes        DER-encoded SubjectPublicKeyInfo
     * @throws AuthException(AUTH_INVALID_CREDENTIALS) on verification failure
     */
    static void verifySignature(
        const std::vector<uint8_t>& auth_data_bytes,
        const std::vector<uint8_t>& client_data_hash,
        const std::vector<uint8_t>& signature_bytes,
        const std::vector<uint8_t>& spki_bytes
    );
};

} // namespace auth
} // namespace themis

