/**
 * @file webauthn_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class WebAuthnAuthenticator {
public:
    // -----------------------------------------------------------------------
    // Configuration / data structures
    // -----------------------------------------------------------------------

    struct RelyingParty {
        std::string id;    ///< Effective domain, e.g. "example.com"
        std::string name;  ///< Human-readable name, e.g. "ThemisDB"
    };

    struct User {
        std::string id;            ///< Opaque user ID (stored in the authenticator)
        std::string name;          ///< Username or email
        std::string display_name;  ///< Full display name
    };

    struct AuthenticatorSelection {
        std::optional<std::string> authenticator_attachment;
        bool require_resident_key{false};
        std::string user_verification{"preferred"};
    };

    struct CredentialCreationOptions {
        std::string challenge;  ///< Base64url-encoded 32-byte random challenge
        RelyingParty rp;
        User user;
        std::vector<std::string> pub_key_cred_params;  ///< ["ES256", "RS256"]
        std::optional<int> timeout_ms;
        std::string attestation{"none"};  ///< "none" | "indirect" | "direct"
        AuthenticatorSelection authenticator_selection;
        std::vector<std::string> exclude_credentials;  ///< Prevent duplicate registration

        /**
         * @brief To json.
         * @return Return value.
         */
        nlohmann::json to_json() const;
    };

    struct CredentialRequestOptions {
        std::string challenge;  ///< Base64url-encoded 32-byte random challenge
        std::string rp_id;
        std::optional<int> timeout_ms;
        std::string user_verification{"preferred"};
        std::vector<std::string> allow_credentials;

        /**
         * @brief To json.
         * @return Return value.
         */
        nlohmann::json to_json() const;
    };

    struct AttestationResult {
        std::string credential_id;        ///< Base64url-encoded credential identifier
        std::vector<uint8_t> public_key;  ///< DER-encoded SubjectPublicKeyInfo (SPKI)
        std::string algorithm;            ///< "ES256" or "RS256"
        uint32_t sign_count{0};           ///< Initial signature counter value
        std::vector<uint8_t> aaguid;      ///< 16-byte authenticator model GUID
    };

    struct AssertionResult {
        std::string credential_id;          ///< Identifies which credential was used
        uint32_t sign_count{0};             ///< New counter value (store this)
        std::optional<std::string> user_handle;  ///< Set for discoverable credentials
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Web Authn Authenticator.
     * @param[in] rp Input parameter.
     * @return Return value.
     */
    explicit WebAuthnAuthenticator(const RelyingParty& rp);
    ~WebAuthnAuthenticator() = default;

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    // -----------------------------------------------------------------------
    // Registration ceremony
    // -----------------------------------------------------------------------

    CredentialCreationOptions startRegistration(
        const User& user,
        bool resident_key = false
    );

    /**
     * @brief Complete Registration.
     * @param[in] credential_response Input parameter.
     * @return Return value.
     */
    AttestationResult completeRegistration(const nlohmann::json& credential_response);

    // -----------------------------------------------------------------------
    // Authentication ceremony
    // -----------------------------------------------------------------------

    CredentialRequestOptions startAuthentication(
        const std::optional<std::string>& user_id = std::nullopt
    );

    /**
     * @brief Complete Authentication.
     * @param[in] credential_response Input parameter.
     * @param[in] stored_public_key Input parameter.
     * @param[in] stored_sign_count Input parameter.
     * @return Return value.
     */
    AssertionResult completeAuthentication(
        const nlohmann::json& credential_response,
        const std::vector<uint8_t>& stored_public_key,
        uint32_t stored_sign_count
    );

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    void setRandBytesForTesting(
        std::function<void(unsigned char* buf, std::size_t len)> fn
    );

    /**
     * @brief Set Expected Origin.
     * @param[in] origin Input parameter.
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

    /**
     * @brief Generate Challenge.
     * @return Return value.
     */
    std::string generateChallenge();

    /**
     * @brief Verify And Consume Challenge.
     * @param[in] challenge_b64url Input parameter.
     */
    void verifyAndConsumeChallenge(const std::string& challenge_b64url);

    /**
     * @brief Purge Expired Challenges.
     */
    void purgeExpiredChallenges();

    /**
     * @brief Fill Random Bytes.
     * @param[in,out] buf Input/output parameter.
     * @param[in] len Input parameter.
     */
    void fillRandomBytes(unsigned char* buf, std::size_t len);

    // Cryptographic primitives
    /**
     * @brief Sha256.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
    /**
     * @brief Sha256.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> sha256(const std::string& data);

    /**
     * @brief Base64URL codec (RFC 4648 §5, no padding)
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::string         base64UrlEncode(const std::vector<uint8_t>& data);
    /**
     * @brief Base64 Url Decode.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> base64UrlDecode(const std::string& input);

    // Parsed fields from the clientDataJSON byte sequence
    struct ClientData {
        std::string type;
        std::string challenge;  ///< base64url
        std::string origin;
    };
    /**
     * @brief Parse Client Data JSON.
     * @param[in] client_data_json Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Parse Auth Data.
     * @param[in] auth_data_bytes Input parameter.
     * @return Return value.
     */
    static AuthData parseAuthData(const std::vector<uint8_t>& auth_data_bytes);

    /**
     * @brief Parse Attestation Object.
     * @param[in] cbor_bytes Input parameter.
     * @param[in,out] fmt Input/output parameter.
     * @param[in,out] auth_data Input/output parameter.
     */
    static void parseAttestationObject(
        const std::vector<uint8_t>& cbor_bytes,
        std::string& fmt,
        std::vector<uint8_t>& auth_data
    );

    static std::pair<std::vector<uint8_t>, std::string> coseKeyToSpki(
        const std::vector<uint8_t>& cose_key_bytes
    );

    /**
     * @brief Verify Signature.
     * @param[in] auth_data_bytes Input parameter.
     * @param[in] client_data_hash Input parameter.
     * @param[in] signature_bytes Input parameter.
     * @param[in] spki_bytes Input parameter.
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

