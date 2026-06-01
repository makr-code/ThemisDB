/*
 * ThemisDB | File: passkey_authenticator.h | Version: 0.1.0 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 194
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file passkey_authenticator.h
 * @brief FIDO2 Passkey / WebAuthn resident-key authentication interface.
 *
 * IPasskeyAuthenticator provides the two-phase registration and authentication
 * flows required by WebAuthn Level 2 (W3C Recommendation) and FIDO2 CTAP2.
 *
 * Security considerations:
 * - Challenges are single-use; completeAuthentication() invalidates the challenge.
 * - sign_count regression detection prevents replay attacks (PasskeyVerifyResult::REPLAY_ATTACK).
 * - Resident-key (discoverable credential) flow is supported via empty user_id in beginAuthentication().
 *
 * Compliance: FIDO2, WebAuthn L2, NIST SP 800-63B AAL3, eIDAS LoA HIGH.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace auth {

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

} // namespace auth
} // namespace themis
