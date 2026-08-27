/**
 * @file passkey_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace auth {

// Forward declaration — avoid pulling the full header into every TU.
class AuthAuditLogger;

// ---------------------------------------------------------------------------
// PasskeyCredential — stored credential record after registration
// ---------------------------------------------------------------------------

/**
 * @brief Persisted credential record for a registered passkey.
 *
 * `public_key_cbor` is the COSE-encoded public key from the authenticator's
 * attestation statement.  `sign_count` must be persisted and updated on every
 * successful authentication to enable clone detection.
 */
struct PasskeyCredential {
    std::string credential_id;       ///< Base64url-encoded credential ID.
    std::string user_id;
    std::string public_key_cbor;     ///< CBOR-encoded COSE public key.
    uint32_t    sign_count = 0;      ///< Monotonic counter from authenticator.
    std::string aaguid;              ///< Authenticator AAGUID (UUID string).
    bool        resident_key = true; ///< Whether stored as discoverable credential.
    bool        user_verification_required = true;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_used_at;
};

// ---------------------------------------------------------------------------
// PasskeyChallenge — server-generated challenge for registration/authentication
// ---------------------------------------------------------------------------

/**
 * @brief Server-generated challenge issued at the start of a WebAuthn ceremony.
 *
 * The challenge is single-use and expires at `expires_at`.  The relying party
 * must securely store the challenge until `complete*()` is called.
 */
struct PasskeyChallenge {
    std::string challenge_id;
    std::string challenge_bytes_b64;  ///< Base64url-encoded random challenge (≥ 16 bytes).
    std::chrono::system_clock::time_point expires_at;
    std::string user_id;              ///< Empty for discoverable (usernameless) flow.
};

// ---------------------------------------------------------------------------
// PasskeyAssertionResponse — authenticator assertion from the client
// ---------------------------------------------------------------------------

/**
 * @brief Authenticator assertion returned by the client during authentication.
 */
struct PasskeyAssertionResponse {
    std::string credential_id;            ///< Base64url-encoded credential ID.
    std::string authenticator_data_b64;   ///< Base64url-encoded authenticatorData.
    std::string client_data_json_b64;     ///< Base64url-encoded clientDataJSON.
    std::string signature_b64;            ///< Base64url-encoded assertion signature.
    std::string user_handle_b64;          ///< Base64url-encoded userHandle (may be empty).
};

// ---------------------------------------------------------------------------
// PasskeyVerifyResult — outcome of a completeAuthentication() call
// ---------------------------------------------------------------------------

/**
 * @brief Outcome of a WebAuthn authentication ceremony.
 */
enum class PasskeyVerifyResult {
    SUCCESS,
    INVALID_SIGNATURE,          ///< Cryptographic verification failed.
    INVALID_CHALLENGE,          ///< Challenge expired, not found, or already consumed.
    CREDENTIAL_NOT_FOUND,       ///< No registered credential matches the assertion.
    REPLAY_ATTACK,              ///< sign_count regression detected (cloned authenticator).
    USER_VERIFICATION_FAILED,   ///< UV flag not set but user verification was required.
};

// ---------------------------------------------------------------------------
// IPasskeyAuthenticator — FIDO2/WebAuthn resident-key authenticator interface
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for FIDO2 Passkey / WebAuthn authentication.
 *
 * Provides the two-phase registration and authentication ceremonies as defined
 * in the WebAuthn Level 2 specification.
 *
 * ### Thread safety
 * All methods must be safe to call concurrently from multiple threads.
 */
class IPasskeyAuthenticator {
public:
    virtual ~IPasskeyAuthenticator() = default;

    // -----------------------------------------------------------------------
    // Registration ceremony
    // -----------------------------------------------------------------------

    /**
     * @brief Begin a passkey registration ceremony.
     *
     * Generates a challenge and stores it for verification.  The returned
     * PasskeyChallenge is sent to the client as publicKeyCredentialCreationOptions.
     */
    [[nodiscard]] virtual PasskeyChallenge beginRegistration(const std::string& user_id) = 0;

    /**
     * @brief Complete a passkey registration ceremony.
     *
     * Validates the authenticator's attestation response against the stored
     * challenge and persists the credential on success.
     *
     * @return `true` if registration succeeded; `false` on validation failure.
     */
    [[nodiscard]] virtual bool completeRegistration(
        const std::string& challenge_id,
        const PasskeyCredential& credential
    ) = 0;

    // -----------------------------------------------------------------------
    // Authentication ceremony
    // -----------------------------------------------------------------------

    /**
     * @brief Begin a passkey authentication ceremony.
     *
     * @param user_id  User to authenticate.  Pass an empty string for the
     *                 discoverable (usernameless) resident-key flow.
     */
    [[nodiscard]] virtual PasskeyChallenge beginAuthentication(
        const std::string& user_id = ""
    ) = 0;

    /**
     * @brief Complete a passkey authentication ceremony.
     *
     * @param challenge_id  Challenge issued by beginAuthentication().
     * @param response      Authenticator assertion from the client.
     * @param out_user_id   Populated with the authenticated user ID on SUCCESS.
     * @return Verification result code.
     */
    [[nodiscard]] virtual PasskeyVerifyResult completeAuthentication(
        const std::string& challenge_id,
        const PasskeyAssertionResponse& response,
        std::string& out_user_id
    ) = 0;

    // -----------------------------------------------------------------------
    // Credential management
    // -----------------------------------------------------------------------

    /// Return all registered credentials for @p user_id.
    [[nodiscard]] virtual std::vector<PasskeyCredential> listCredentials(
        const std::string& user_id
    ) const = 0;

    /**
     * @brief Revoke a registered credential.
     *
     * @return `false` if the credential was not found.
     */
    [[nodiscard]] virtual bool revokeCredential(const std::string& credential_id) = 0;
};

// ---------------------------------------------------------------------------
// PasskeyAuthenticator — in-process concrete implementation
// ---------------------------------------------------------------------------

/**
 * @brief In-process PasskeyAuthenticator implementing IPasskeyAuthenticator.
 *
 * Thread-safe.  Stores credentials and pending challenges in memory using
 * std::mutex-guarded unordered_maps.
 *
 * For production deployments replace the in-memory stores with a persistent
 * backend (database, key-value store, etc.).
 */
class PasskeyAuthenticator : public IPasskeyAuthenticator {
public:
    /**
     * @brief Construct a PasskeyAuthenticator.
     *
     * @param relying_party_id   The RP ID (domain, e.g. "example.com") used to
     *                           compute and validate the rpIdHash in authenticator data.
     * @param expected_origin    The full origin expected in clientDataJSON
     *                           (e.g. "https://example.com").
     */
    explicit PasskeyAuthenticator(std::string relying_party_id, std::string expected_origin);

    // -----------------------------------------------------------------------
    // IPasskeyAuthenticator
    // -----------------------------------------------------------------------

    /**
     * @brief Begin a passkey registration ceremony.
     *
     * Generates a cryptographically secure challenge, stores it as a pending
     * challenge, and returns the challenge for forwarding to the client.
     *
     * @param user_id  Identifier of the user attempting registration.
     * @return         A PasskeyChallenge with a 10-minute expiry.
     */
    [[nodiscard]] PasskeyChallenge beginRegistration(const std::string& user_id) override;

    /**
     * @brief Complete a passkey registration ceremony.
     *
     * Validates the pending challenge (expiry check), then stores the supplied
     * pre-verified PasskeyCredential in the in-memory credential store.
     *
     * @param challenge_id  ID returned by beginRegistration().
     * @param credential    Verified credential to persist.
     * @return `true` on success; `false` if the challenge is unknown/expired.
     */
    [[nodiscard]] bool completeRegistration(const std::string& challenge_id,
                                            const PasskeyCredential& credential) override;

    /**
     * @brief Begin a passkey authentication ceremony.
     *
     * @param user_id  User to authenticate, or empty for the usernameless
     *                 (discoverable resident-key) flow.
     * @return         A PasskeyChallenge with a 5-minute expiry.
     */
    [[nodiscard]] PasskeyChallenge beginAuthentication(const std::string& user_id = "") override;

    /**
     * @brief Complete a passkey authentication ceremony.
     *
     * Looks up the pending challenge and the registered credential, then
     * cryptographically verifies the assertion.  On success the credential's
     * sign_count and last_used_at fields are updated.
     *
     * @param challenge_id  ID returned by beginAuthentication().
     * @param response      Assertion fields from the client.
     * @param out_user_id   Populated with the authenticated user ID on SUCCESS.
     * @return              PasskeyVerifyResult status code.
     */
    [[nodiscard]] PasskeyVerifyResult completeAuthentication(
        const std::string& challenge_id,
        const PasskeyAssertionResponse& response,
        std::string& out_user_id) override;

    /**
     * @brief Return all registered credentials for a given user.
     *
     * @param user_id  The user whose credentials should be listed.
     * @return         Vector of matching PasskeyCredential records (may be empty).
     */
    [[nodiscard]] std::vector<PasskeyCredential> listCredentials(
        const std::string& user_id) const override;

    /**
     * @brief Revoke (delete) a registered credential by its ID.
     *
     * @param credential_id  Base64url-encoded credential ID to remove.
     * @return `true` if the credential was found and removed; `false` otherwise.
     */
    [[nodiscard]] bool revokeCredential(const std::string& credential_id) override;

    // -----------------------------------------------------------------------
    // Audit logger injection
    // -----------------------------------------------------------------------

    /**
     * @brief Attach an AuthAuditLogger that receives passkey success/failure events.
     * @param logger Non-owning pointer; may be nullptr (disables audit logging).
     */
    void setAuditLogger(AuthAuditLogger* logger) { audit_logger_ = logger; }

    // -----------------------------------------------------------------------
    // Low-level cryptographic helpers (used internally; exposed for testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Verify an attestation response against a registration challenge.
     *
     * Decodes the base64url attestation CBOR, parses authData, validates the
     * rpIdHash, and stores the extracted credential.
     *
     * @param challenge               The original challenge to validate against.
     * @param attestation_response_b64 Base64url-encoded attestation CBOR object.
     * @return `true` on successful verification; `false` on any error.
     */
    [[nodiscard]] bool verifyRegistration(const PasskeyChallenge& challenge,
                                          const std::string& attestation_response_b64);

    /**
     * @brief Verify an assertion response against a stored credential.
     *
     * Validates the rpIdHash, UP flag, signature, and sign_count.
     *
     * @param challenge               The original challenge to validate against.
     * @param credential              The stored credential whose public key is used.
     * @param assertion_response_b64  Base64url-encoded JSON assertion response.
     * @return `true` on successful verification; `false` on any error.
     */
    [[nodiscard]] bool verifyAuthentication(const PasskeyChallenge& challenge,
                                            const PasskeyCredential& credential,
                                            const std::string& assertion_response_b64);

    /**
     * @brief Detect a potentially cloned authenticator via sign count regression.
     *
     * Per WebAuthn §7.2 step 17: if either counter is non-zero the new counter
     * MUST be strictly greater than the stored counter.
     *
     * @param stored_sign_count    Counter value stored from the last authentication.
     * @param assertion_sign_count Counter value received in the current assertion.
     * @return `true` if a clone is detected (the assertion should be rejected).
     */
    [[nodiscard]] static bool cloneDetectionFailed(uint32_t stored_sign_count,
                                                   uint32_t assertion_sign_count) noexcept;

private:
    std::string relying_party_id_; ///< RP ID used for rpIdHash computation.
    std::string expected_origin_;  ///< Expected origin in clientDataJSON.

    mutable std::mutex cred_mutex_;
    /// credential_id → PasskeyCredential
    std::unordered_map<std::string, PasskeyCredential> credentials_;

    mutable std::mutex challenge_mutex_;
    /// challenge_id → PasskeyChallenge
    std::unordered_map<std::string, PasskeyChallenge> pending_challenges_;

    AuthAuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.

    /**
     * @brief Generate a cryptographically secure base64url challenge string.
     *
     * @param bytes Number of random bytes to use (minimum 16; default 32).
     * @return Base64url-encoded random string.
     */
    [[nodiscard]] std::string generateSecureChallenge(size_t bytes = 32) const;
};

} // namespace auth
} // namespace themis
