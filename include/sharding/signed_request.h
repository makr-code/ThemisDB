/**
 * @file signed_request.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * Signed Request Protocol
 * 
 * Provides defense-in-depth security layer on top of mTLS.
 * Requests are signed with the sender's private key and include:
 * - Timestamp for freshness validation
 * - Nonce for replay protection
 * - Certificate serial for identity verification
 * 
 * This protects against:
 * - Replay attacks
 * - Man-in-the-middle (additional layer beyond mTLS)
 * - Request tampering
 */

/**
 * @brief Signed shard-to-shard request payload.
 */
struct SignedRequest {
    /// Versioned signature format identifier for canonicalization/verification.
    /// Current required value: "themis-shard-sig-v1".
    static constexpr const char* kSignatureFormatV1 = "themis-shard-sig-v1";

    std::string shard_id;       // Sender shard ID
    std::string operation;      // HTTP method (GET, POST, PUT, DELETE)
    std::string path;           // Request path (e.g., URN or API endpoint)
    nlohmann::json body;        // Request body (empty for GET/DELETE)
    uint64_t timestamp_ms;      // Unix timestamp in milliseconds
    uint64_t nonce;             // Random nonce for uniqueness
    
    std::string signature_format = kSignatureFormatV1; // Signature/canonicalization format version
    std::string key_id;         // Trust-store key identifier (supports key rotation)
    std::string signature_b64;  // Signature (base64 encoded; algorithm implied by key type/format)
    std::string cert_serial;    // Certificate serial number (hex)
    
    /**
     * @brief Serialize request to transport JSON.
     * @return JSON object containing all signing and payload fields.
     */
    nlohmann::json toJSON() const;
    
    /**
     * @brief Parse request from transport JSON.
     * @param j Input JSON object.
     * @return Parsed request, or std::nullopt when required fields are missing/invalid.
     */
    static std::optional<SignedRequest> fromJSON(const nlohmann::json& j);
    
    /**
     * Get canonical string representation for signing
     *
     * Format (v1, strict):
     * signature_format=<format>\n
     * shard_id=<shard_id>\n
     * operation=<operation>\n
     * path=<path>\n
     * body=<normalized_json>\n
     * timestamp_ms=<timestamp_ms>\n
     * nonce=<nonce>\n
     * key_id=<key_id>\n
     * cert_serial=<cert_serial>\n
     */
    std::string getCanonicalString() const;
};

/**
 * @brief Creates signatures for outbound shard requests.
 */
class SignedRequestSigner {
public:
    /** @brief Configuration for signing context and key material. */
    struct Config {
        std::string shard_id;       // This shard's ID
        std::string cert_path;      // Certificate path (for serial extraction)
        std::string key_path;       // Private key path
        std::string key_passphrase; // Optional: key passphrase
    };
    
    /**
     * @brief Construct signer with certificate/private-key configuration.
     * @param config Signer configuration.
     */
    explicit SignedRequestSigner(const Config& config);
    
    /**
     * @brief Populate metadata and sign request in place.
     * @param request Request to sign; modified with timestamp/nonce/signature fields.
     * @return true when signing succeeds; false when key material or signing fails.
     */
    bool sign(SignedRequest& request);
    
    /**
     * @brief Build and sign a request in one call.
     * @param operation HTTP verb.
     * @param path Request path.
     * @param body JSON payload.
     * @return Signed request object (signature fields may be empty if signing failed).
     */
    SignedRequest createSignedRequest(const std::string& operation,
                                     const std::string& path,
                                     const nlohmann::json& body = nlohmann::json{});

private:
    Config config_;
    std::string cert_serial_;
    
    /** @brief Generate nonce value used for replay protection. */
    uint64_t generateNonce() const;
    
    /** @brief Return current wall-clock time in milliseconds since epoch. */
    uint64_t getCurrentTimestampMs() const;
    
    /**
     * @brief Sign canonical request string.
     * @param data Canonical string to sign.
     * @return Base64-encoded signature on success, std::nullopt on cryptographic failure.
     */
    std::optional<std::string> signData(const std::string& data);
};

/**
 * @brief Verifies request authenticity and replay safety.
 */
class SignedRequestVerifier {
public:
    /** @brief Verification and replay-window settings. */
    struct Config {
        std::string ca_cert_path;       ///< Root CA certificate path (used to validate peer certs)
        std::string trusted_certs_dir;  ///< Directory of trusted shard certificates (PEM files)
                                        ///<   named `<key_id>.pem`. Required for signature
                                        ///<   verification; `verifySignature()` returns false when empty.
        std::string crl_path;           ///< Optional: path to CRL file (PEM) for revocation checks
        uint64_t max_time_skew_ms = 60000;  // Max timestamp deviation (60s default)
        size_t max_nonce_cache = 10000;     // Max nonces to track
        uint64_t nonce_expiry_ms = 300000;  // Nonce expiry time (5 min default)
    };
    
    /**
     * @brief Construct verifier.
     * @param config Verification configuration.
     */
    explicit SignedRequestVerifier(const Config& config);
    
    /**
     * @brief Verify freshness, replay state, identity and signature.
     * @param request Signed request to verify.
     * @param expected_shard_id Optional expected sender shard identifier.
     * @return true when request passes all checks; false otherwise.
     */
    bool verify(const SignedRequest& request,
                const std::string& expected_shard_id = "");
    
    /** @brief Remove expired nonces from replay cache. */
    void cleanupExpiredNonces();

private:
    Config config_;
    
    // Nonce tracking for replay protection
    struct NonceEntry {
        uint64_t nonce = 0;
        uint64_t timestamp_ms;
    };
    std::unordered_map<uint64_t, uint64_t> seen_nonces_;
    std::deque<NonceEntry> nonce_fifo_;
    mutable std::mutex nonce_mutex_;
    
    /** @brief Check request timestamp skew against configured limit. */
    bool verifyTimestamp(uint64_t timestamp_ms) const;
    
    /** @brief Validate nonce uniqueness within replay window. */
    bool verifyNonce(uint64_t nonce, uint64_t timestamp_ms);
    
    /** @brief Verify signature using trust-store certificate material. */
    bool verifySignature(const SignedRequest& request);

    /**
     * @brief Purge replay-cache entries older than configured expiry.
     * @param now_ms Current timestamp in milliseconds.
     * @note Caller must hold nonce_mutex_.
     */
    void purgeExpiredNoncesLocked(uint64_t now_ms);
    
    /** @brief Return current wall-clock time in milliseconds since epoch. */
    uint64_t getCurrentTimestampMs() const;
};

} // namespace themis::sharding
