/**
 * @file timestamp_authority.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <chrono>
#include <functional>
#include <mutex>

namespace themis {
namespace security {

/**
 * Timestamp Authority (TSA) Client - RFC 3161 Implementation
 * 
 * Provides Time-Stamp Protocol (TSP) client for eIDAS-compliant timestamping.
 * Timestamps are cryptographic proof that data existed at a specific time.
 * 
 * Features:
 * - RFC 3161 Time-Stamp Protocol
 * - TSP request/response handling
 * - Timestamp token verification
 * - Certificate chain validation
 * - Nonce generation for replay protection
 * - HTTP/HTTPS transport
 * 
 * Use Cases:
 * - eIDAS qualified signatures (Art. 32 - Long-term validation)
 * - Audit log timestamping
 * - Document timestamping
 * - SAGA transaction timestamping
 * 
 * Example Usage:
 * ```cpp
 * TSAConfig config;
 * config.url = "https://freetsa.org/tsr";
 * config.hash_algorithm = "SHA256";
 * 
 * TimestampAuthority tsa(config);
 * 
 * std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
 * auto token = tsa.getTimestamp(data);
 * 
 * if (token.success) {
 *     std::cout << "Timestamp: " << token.timestamp_utc << std::endl;
 *     std::cout << "Serial: " << token.serial_number << std::endl;
 * }
 * 
 * bool valid = tsa.verifyTimestamp(data, token);
 * ```
 */

struct TSAConfig {
    // TSA server URL (e.g., https://freetsa.org/tsr)
    std::string url;
    
    // Hash algorithm for message imprint (SHA256, SHA384, SHA512)
    std::string hash_algorithm = "SHA256";
    
    // Request certificate in response
    bool cert_req = true;
    
    // HTTP timeout in seconds
    int timeout_seconds = 30;
    
    // Optional: TSA authentication (if required)
    std::string username;
    std::string password;
    
    // Optional: Client certificate for mTLS
    std::string client_cert_path;
    std::string client_key_path;
    
    // Optional: CA certificate for TSA validation
    std::string ca_cert_path;
    
    // Verify TSA certificate
    bool verify_tsa_cert = true;
    
    // Policy OID (optional, TSA-specific)
    std::string policy_oid;
};

struct TimestampToken {
    bool success = false;
    
    // Timestamp information
    std::string timestamp_utc;       // ISO 8601 format (e.g., 2025-11-17T14:30:00Z)
    uint64_t timestamp_unix_ms = 0;  // Unix timestamp in milliseconds
    
    // Token metadata
    std::string serial_number;       // Timestamp serial number (hex)
    std::string policy_oid;          // TSA policy OID
    std::string hash_algorithm;      // Hash algorithm used
    std::vector<uint8_t> nonce;      // Nonce (if requested)
    
    // RFC 3161 Accuracy metadata (optional per RFC)
    // Indicates the time deviation accuracy of the TSA's time source
    bool has_accuracy = false;       // Whether accuracy info is present
    uint32_t accuracy_seconds = 0;   // Accuracy in seconds
    uint32_t accuracy_millis = 0;    // Accuracy in milliseconds (0-999)
    uint32_t accuracy_micros = 0;    // Accuracy in microseconds (0-999)
    
    // RFC 3161 Ordering hint (optional per RFC)
    // If true, TSA guarantees tokens are issued in chronological order
    bool ordering = false;           // Ordering guarantee from TSA
    
    // Token data
    std::vector<uint8_t> token_der;  // DER-encoded timestamp token
    std::string token_b64;           // Base64-encoded token (for transport)
    
    // TSA information
    std::string tsa_name;            // TSA certificate subject name
    std::string tsa_serial;          // TSA certificate serial
    std::vector<uint8_t> tsa_cert;   // TSA certificate (DER)
    
    // Validation
    bool verified = false;           // Token signature verified
    bool cert_valid = false;         // TSA certificate valid
    
    // Error information
    std::string error_message;
    int status_code = 0;             // HTTP status code
    int pki_status = 0;              // PKI status from TSP response
};

/**
 * Timestamp Authority Client
 * 
 * Implements RFC 3161 Time-Stamp Protocol for obtaining cryptographic timestamps.
 */
class TimestampAuthority {
public:
    using GetTimestampForHashFn =
        std::function<TimestampToken(const std::vector<uint8_t>& hash,
                                     const TSAConfig& config)>;
    using VerifyTimestampForHashFn =
        std::function<bool(const std::vector<uint8_t>& hash,
                           const TimestampToken& token,
                           const TSAConfig& config)>;

    /**
     * @brief TBD: Describe TimestampAuthority.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TimestampAuthority(TSAConfig config);
    ~TimestampAuthority();

    // Disable copy (HTTP client state is not copyable)
    TimestampAuthority(const TimestampAuthority&) = delete;
    TimestampAuthority& operator=(const TimestampAuthority&) = delete;

    // Enable move
    TimestampAuthority(TimestampAuthority&&) noexcept;
    TimestampAuthority& operator=(TimestampAuthority&&) noexcept;

    /**
     * Get timestamp for data
     * @param data: Data to timestamp (will be hashed internally)
     * @return Timestamp token
     * @brief TBD: Describe getTimestamp.
     */
    TimestampToken getTimestamp(const std::vector<uint8_t>& data);

    /**
     * Get timestamp for pre-computed hash
     * @param hash: Pre-computed hash (e.g., SHA-256)
     * @return Timestamp token
     * @brief TBD: Describe getTimestampForHash.
     */
    TimestampToken getTimestampForHash(const std::vector<uint8_t>& hash);

    /**
     * Verify timestamp token against data
     * @param data: Original data
     * @param token: Timestamp token to verify
     * @return true if valid, false otherwise
     * @brief TBD: Describe verifyTimestamp.
     */
    bool verifyTimestamp(const std::vector<uint8_t>& data, 
                         const TimestampToken& token);

    /**
     * Verify timestamp token against hash
     * @param hash: Pre-computed hash
     * @param token: Timestamp token to verify
     * @return true if valid, false otherwise
     * @brief TBD: Describe verifyTimestampForHash.
     */
    bool verifyTimestampForHash(const std::vector<uint8_t>& hash,
                                const TimestampToken& token);

    /**
     * Parse timestamp token from DER or Base64
     * @param token_data: Token in DER format or Base64 string
     * @return Parsed timestamp token
     * @brief TBD: Describe parseToken.
     */
    TimestampToken parseToken(const std::vector<uint8_t>& token_data);
    /**
     * @brief TBD: Describe parseToken.
     * @param[in] token_b64 Input parameter.
     * @return Return value.
     */
    TimestampToken parseToken(const std::string& token_b64);

    /**
     * Get TSA certificate
     * @return TSA certificate in PEM format, or empty optional if not available
     * @brief TBD: Describe getTSACertificate.
     */
    std::optional<std::string> getTSACertificate();

    /**
     * Check if TSA is reachable
     * @return true if TSA responds, false otherwise
     * @brief TBD: Describe isAvailable.
     */
    bool isAvailable();

    /**
     * Get last error message
     * @brief TBD: Describe getLastError.
     * @return Return value.
     */
    std::string getLastError() const;

    /**
     * @brief Register a stamping bridge for non-OpenSSL TSA builds.
     * @param[in] fn Input parameter.
     * @details Thread-safe; pass an empty function to restore the deterministic fallback. Calls: lk(), getTimestampForHashFnMutex(), getTimestampForHashFnStorage(), std::move().
     */
    static void setGetTimestampForHashFn(GetTimestampForHashFn fn) {
        std::lock_guard<std::mutex> lk(getTimestampForHashFnMutex());
        getTimestampForHashFnStorage() = std::move(fn);
    }
    /**
     * @brief Register a verification bridge for non-OpenSSL TSA builds.
     * @param[in] fn Input parameter.
     * @details Thread-safe; pass an empty function to restore the built-in fallback. Calls: lk(), verifyTimestampForHashFnMutex(), verifyTimestampForHashFnStorage(), std::move().
     */
    static void setVerifyTimestampForHashFn(VerifyTimestampForHashFn fn) {
        std::lock_guard<std::mutex> lk(verifyTimestampForHashFnMutex());
        verifyTimestampForHashFnStorage() = std::move(fn);
    }

private:
    /**
     * @brief TBD: Describe getTimestampForHashFnMutex.
     * @return Return value.
     * @details Implements getTimestampForHashFnMutex without additional internal calls.
     */
    static std::mutex& getTimestampForHashFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe getTimestampForHashFnStorage.
     * @return Return value.
     * @details Implements getTimestampForHashFnStorage without additional internal calls.
     */
    static GetTimestampForHashFn& getTimestampForHashFnStorage() {
        static GetTimestampForHashFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe verifyTimestampForHashFnMutex.
     * @return Return value.
     * @details Implements verifyTimestampForHashFnMutex without additional internal calls.
     */
    static std::mutex& verifyTimestampForHashFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe verifyTimestampForHashFnStorage.
     * @return Return value.
     * @details Implements verifyTimestampForHashFnStorage without additional internal calls.
     */
    static VerifyTimestampForHashFn& verifyTimestampForHashFnStorage() {
        static VerifyTimestampForHashFn fn;
        return fn;
    }
    class Impl;
    std::unique_ptr<Impl> impl_;
    TSAConfig config_;
    std::string last_error_;
    std::vector<uint8_t> cached_tsa_cert_;  // Most recently received TSA certificate
    
    /**
     * @brief Helper: Create TSP request (RFC 3161)
     * @param[in] hash Input parameter.
     * @param[in] nonce Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> createTSPRequest(const std::vector<uint8_t>& hash,
                                          const std::vector<uint8_t>& nonce);
    
    /**
     * @brief Helper: Parse TSP response
     * @param[in] response Input parameter.
     * @return Return value.
     */
    TimestampToken parseTSPResponse(const std::vector<uint8_t>& response);
    
    /**
     * @brief Helper: Send HTTP request to TSA
     * @param[in] request Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> sendTSPRequest(const std::vector<uint8_t>& request);
    
    // Helper: Generate nonce for replay protection
    std::vector<uint8_t> generateNonce(size_t bytes = 8);
    
    /**
     * @brief Helper: Compute hash of data
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> computeHash(const std::vector<uint8_t>& data);
};

/**
 * eIDAS Timestamp Validator
 * 
 * Validates timestamps according to eIDAS requirements.
 * Implements long-term validation (LTV) for archived timestamps.
 */
class eIDASTimestampValidator {
public:
    eIDASTimestampValidator() = default;

    // -----------------------------------------------------------------------
    // Injectable validation bridge (STUB #213 / #214)
    // -----------------------------------------------------------------------
    using ValidateFn = std::function<bool(const TimestampToken& token,
                                          const std::vector<std::string>& trust_anchors,
                                          std::vector<std::string>& validation_errors)>;
    using QualifiedTSAFn = std::function<bool(const std::string& tsa_cert,
                                              const std::vector<std::string>& qtsp_list,
                                              std::vector<std::string>& validation_errors)>;

    /**
     * @brief Register callback used by validateeIDASTimestamp() in non-OpenSSL builds.
     * @param[in] fn Input parameter.
     * @details Pass empty fn to restore default stub behavior. Calls: lk(), validateFnMutex(), validateFnStorage(), std::move().
     */
    static void setValidateFn(ValidateFn fn) {
        std::lock_guard<std::mutex> lk(validateFnMutex());
        validateFnStorage() = std::move(fn);
    }
    /**
     * @brief Register callback used by isQualifiedTSA() in non-OpenSSL builds.
     * @param[in] fn Input parameter.
     * @details Pass empty fn to restore default stub behavior. Calls: lk(), qualifiedTSAFnMutex(), qualifiedTSAFnStorage(), std::move().
     */
    static void setQualifiedTSAFn(QualifiedTSAFn fn) {
        std::lock_guard<std::mutex> lk(qualifiedTSAFnMutex());
        qualifiedTSAFnStorage() = std::move(fn);
    }
    
    /**
     * Validate timestamp for eIDAS compliance
     * @param token: Timestamp token
     * @param trust_anchors: Trusted CA certificates (PEM)
     * @return true if eIDAS-compliant, false otherwise
     * @brief TBD: Describe validateeIDASTimestamp.
     */
    bool validateeIDASTimestamp(const TimestampToken& token,
                                const std::vector<std::string>& trust_anchors);
    
    /**
     * Validate timestamp age (for long-term validation)
     * @param token: Timestamp token
     * @param max_age_days: Maximum age in days (eIDAS: 30 years = 10950 days)
     * @return true if within acceptable age, false otherwise
     */
    bool validateAge(const TimestampToken& token, int max_age_days = 10950);
    
    /**
     * Check if TSA is qualified (eIDAS QTSP)
     * @param tsa_cert: TSA certificate (PEM)
     * @param qtsp_list: List of qualified TSPs
     * @return true if qualified, false otherwise
     * @brief TBD: Describe isQualifiedTSA.
     */
    bool isQualifiedTSA(const std::string& tsa_cert,
                        const std::vector<std::string>& qtsp_list);
    
    /**
     * Get validation errors
     * @brief TBD: Describe getValidationErrors.
     * @return Return value.
     */
    std::vector<std::string> getValidationErrors() const;

private:
    /**
     * @brief TBD: Describe validateFnMutex.
     * @return Return value.
     * @details Implements validateFnMutex without additional internal calls.
     */
    static std::mutex& validateFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe validateFnStorage.
     * @return Return value.
     * @details Implements validateFnStorage without additional internal calls.
     */
    static ValidateFn& validateFnStorage() {
        static ValidateFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe qualifiedTSAFnMutex.
     * @return Return value.
     * @details Implements qualifiedTSAFnMutex without additional internal calls.
     */
    static std::mutex& qualifiedTSAFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe qualifiedTSAFnStorage.
     * @return Return value.
     * @details Implements qualifiedTSAFnStorage without additional internal calls.
     */
    static QualifiedTSAFn& qualifiedTSAFnStorage() {
        static QualifiedTSAFn fn;
        return fn;
    }

private:
    std::vector<std::string> validation_errors_;
};

} // namespace security
} // namespace themis
