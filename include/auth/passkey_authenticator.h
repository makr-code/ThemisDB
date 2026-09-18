/**
 * @file passkey_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

struct PasskeyChallenge {
    std::string challenge_id;
    std::string challenge_bytes_b64;  ///< Base64url-encoded random challenge (≥ 16 bytes).
    std::chrono::system_clock::time_point expires_at;
    std::string user_id;              ///< Empty for discoverable (usernameless) flow.
};

// ---------------------------------------------------------------------------
// PasskeyAssertionResponse — authenticator assertion from the client
// ---------------------------------------------------------------------------

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

class IPasskeyAuthenticator {
public:
    /**
     * @brief IPasskey Authenticator.
     * @return Return value.
     */
    virtual ~IPasskeyAuthenticator() = default;

    // -----------------------------------------------------------------------
    // Registration ceremony
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual PasskeyChallenge beginRegistration(const std::string& user_id) = 0;

    [[nodiscard]] virtual bool completeRegistration(
        const std::string& challenge_id,
        const PasskeyCredential& credential
    ) = 0;

    // -----------------------------------------------------------------------
    // Authentication ceremony
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual PasskeyChallenge beginAuthentication(
        const std::string& user_id = ""
    ) = 0;

    [[nodiscard]] virtual PasskeyVerifyResult completeAuthentication(
        const std::string& challenge_id,
        const PasskeyAssertionResponse& response,
        std::string& out_user_id
    ) = 0;

    // -----------------------------------------------------------------------
    // Credential management
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::vector<PasskeyCredential> listCredentials(
        const std::string& user_id
    ) const = 0;

    [[nodiscard]] virtual bool revokeCredential(const std::string& credential_id) = 0;
};

// ---------------------------------------------------------------------------
// PasskeyAuthenticator — in-process concrete implementation
// ---------------------------------------------------------------------------

class PasskeyAuthenticator : public IPasskeyAuthenticator {
public:
    /**
     * @brief Passkey Authenticator.
     * @param[in] relying_party_id Identifier of the relying party.
     * @param[in] expected_origin Input parameter.
     * @return Return value.
     */
    explicit PasskeyAuthenticator(std::string relying_party_id, std::string expected_origin);

    // -----------------------------------------------------------------------
    // IPasskeyAuthenticator
    // -----------------------------------------------------------------------

    [[nodiscard]] PasskeyChallenge beginRegistration(const std::string& user_id) override;

    [[nodiscard]] bool completeRegistration(const std::string& challenge_id,
                                            const PasskeyCredential& credential) override;

    [[nodiscard]] PasskeyChallenge beginAuthentication(const std::string& user_id = "") override;

    [[nodiscard]] PasskeyVerifyResult completeAuthentication(
        const std::string& challenge_id,
        const PasskeyAssertionResponse& response,
        std::string& out_user_id) override;

    [[nodiscard]] std::vector<PasskeyCredential> listCredentials(
        const std::string& user_id) const override;

    [[nodiscard]] bool revokeCredential(const std::string& credential_id) override;

    // -----------------------------------------------------------------------
    // Audit logger injection
    // -----------------------------------------------------------------------

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(AuthAuditLogger* logger) { audit_logger_ = logger; }

    // -----------------------------------------------------------------------
    // Low-level cryptographic helpers (used internally; exposed for testing)
    // -----------------------------------------------------------------------

    [[nodiscard]] bool verifyRegistration(const PasskeyChallenge& challenge,
                                          const std::string& attestation_response_b64);

    [[nodiscard]] bool verifyAuthentication(const PasskeyChallenge& challenge,
                                            const PasskeyCredential& credential,
                                            const std::string& assertion_response_b64);

    [[nodiscard]] static bool cloneDetectionFailed(uint32_t stored_sign_count,
                                                   uint32_t assertion_sign_count) noexcept;

private:
    std::string relying_party_id_; ///< RP ID used for rpIdHash computation.
    std::string expected_origin_;  ///< Expected origin in clientDataJSON.

    mutable std::mutex cred_mutex_;
    std::unordered_map<std::string, PasskeyCredential> credentials_;

    mutable std::mutex challenge_mutex_;
    std::unordered_map<std::string, PasskeyChallenge> pending_challenges_;

    AuthAuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.

    [[nodiscard]] std::string generateSecureChallenge(size_t bytes = 32) const;
};

} // namespace auth
} // namespace themis
