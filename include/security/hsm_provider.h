/**
 * @file hsm_provider.h
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
#include <functional>
#include <mutex>

namespace themis {
namespace security {

/**
 * HSM (Hardware Security Module) Provider Interface
 * 
 * Provides secure cryptographic operations using hardware-backed keys.
 * Supports PKCS#11 interface for HSM device communication.
 * 
 * Features:
 * - Hardware-backed key storage
 * - Secure signing operations (never exposes private key)
 * - Certificate management
 * - PIN/password protection
 * - Multi-slot support
 * 
 * Supported HSMs:
 * - Thales/SafeNet Luna HSM
 * - Utimaco CryptoServer
 * - AWS CloudHSM
 * - SoftHSM2 (software emulation for testing)
 * 
 * Example Usage:
 * ```cpp
 * HSMConfig config;
 * config.library_path = "/usr/lib/softhsm/libsofthsm2.so";
 * config.slot_id = 0;
 * config.pin = "1234";
 * 
 * auto hsm = std::make_unique<HSMProvider>(config);
 * if (hsm->initialize()) {
 *     auto signature = hsm->sign(data, "my-key-label");
 * }
 * ```
 */

struct HSMConfig {
    // PKCS#11 library path (e.g., /usr/lib/softhsm/libsofthsm2.so)
    std::string library_path;
    
    // HSM slot ID (default: 0)
    uint32_t slot_id = 0;
    
    // User PIN for authentication
    std::string pin;
    
    // Optional: Token label for filtering
    std::string token_label;
    
    // Signature algorithm (default: RSA-SHA256)
    std::string signature_algorithm = "RSA-SHA256";
    
    // Key label for operations (default: "themis-signing-key")
    std::string key_label = "themis-signing-key";
    
    // Enable verbose logging
    bool verbose = false;

    // Anzahl paralleler PKCS#11 Sessions (nur bei realem Provider genutzt)
    uint32_t session_pool_size = 1; // 1 = bisheriges Verhalten
};

struct HSMSignatureResult {
    bool success = false;
    std::string signature_b64;      // Base64-encoded signature
    std::string algorithm;           // Signature algorithm used
    std::string key_id;              // HSM key identifier
    std::string cert_serial;         // Certificate serial number (if available)
    std::string error_message;       // Error details on failure
    uint64_t timestamp_ms = 0;       // Unix timestamp in milliseconds
};

struct HSMPerformanceStats {
    uint64_t sign_count = 0;         // Total sign operations
    uint64_t verify_count = 0;       // Total verify operations
    uint64_t sign_errors = 0;        // Failed sign operations
    uint64_t verify_errors = 0;      // Failed verify operations
    uint64_t total_sign_time_us = 0; // Cumulative sign time (microseconds)
    uint64_t total_verify_time_us = 0; // Cumulative verify time (microseconds)
    uint32_t pool_size = 0;          // Configured pool size
    uint64_t pool_round_robin_hits = 0; // Successful round-robin selections
};

struct HSMKeyInfo {
    std::string label;               // Key label
    std::string id;                  // Key ID (hex)
    std::string algorithm;           // Algorithm (e.g., RSA-2048)
    bool can_sign = false;           // Key can be used for signing
    bool can_verify = false;         // Key can be used for verification
    bool extractable = false;        // Key can be extracted (should be false)
    uint32_t key_size = 0;          // Key size in bits
};

/**
 * HSM Provider Implementation
 * 
 * Wraps PKCS#11 API for HSM operations.
 * Handles session management, login, and cryptographic operations.
 */
class HSMProvider {
public:
    /**
     * @brief TBD: Describe HSMProvider.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HSMProvider(HSMConfig config);
    ~HSMProvider();

    // Disable copy (HSM sessions are not copyable)
    HSMProvider(const HSMProvider&) = delete;
    HSMProvider& operator=(const HSMProvider&) = delete;

    // Enable move
    HSMProvider(HSMProvider&&) noexcept;
    HSMProvider& operator=(HSMProvider&&) noexcept;

    /**
     * Initialize HSM connection and authenticate
     * @return true on success, false otherwise
     * @brief TBD: Describe initialize.
     */
    bool initialize();

    /**
     * Finalize HSM session and cleanup
     * @brief TBD: Describe finalize.
     */
    void finalize();

    /**
     * Sign data using HSM-backed private key
     * @param data: Data to sign (will be hashed internally)
     * @param key_label: Key label in HSM (optional, uses config default if empty)
     * @return Signature result with base64-encoded signature
     */
    HSMSignatureResult sign(const std::vector<uint8_t>& data, 
                            const std::string& key_label = "");

    /**
     * Sign a pre-computed hash using HSM-backed private key
     * @param hash: Pre-computed hash (e.g., SHA-256)
     * @param key_label: Key label in HSM
     * @return Signature result
     */
    HSMSignatureResult signHash(const std::vector<uint8_t>& hash,
                                const std::string& key_label = "");

    /**
     * Verify signature using HSM-backed public key
     * @param data: Original data
     * @param signature_b64: Base64-encoded signature
     * @param key_label: Key label in HSM
     * @return true if signature is valid, false otherwise
     */
    bool verify(const std::vector<uint8_t>& data,
                const std::string& signature_b64,
                const std::string& key_label = "");

    using SignHashFn =
        std::function<HSMSignatureResult(const std::vector<uint8_t>& hash,
                                         const std::string& key_label)>;
    using VerifyFn =
        std::function<bool(const std::vector<uint8_t>& data,
                           const std::string& signature_b64,
                           const std::string& key_label)>;
    using EncryptDataFn =
        std::function<std::vector<uint8_t>(const std::vector<uint8_t>& data,
                                           const std::string& key_label)>;
    using DecryptDataFn =
        std::function<std::vector<uint8_t>(const std::vector<uint8_t>& encrypted,
                                           const std::string& key_label)>;

    /**
     * @brief Register a signing bridge for stub builds without a real HSM.
     * @param[in] fn Input parameter.
     * @details Thread-safe; pass an empty function to restore the built-in stub path. Calls: lk(), signHashFnMutex(), signHashFnStorage(), std::move().
     */
    static void setSignHashFn(SignHashFn fn) {
        std::lock_guard<std::mutex> lk(signHashFnMutex());
        signHashFnStorage() = std::move(fn);
    }
    /**
     * @brief Register a verification bridge for stub builds without a real HSM.
     * @param[in] fn Input parameter.
     * @details Thread-safe; pass an empty function to restore the built-in stub path. Calls: lk(), verifyFnMutex(), verifyFnStorage(), std::move().
     */
    static void setVerifyFn(VerifyFn fn) {
        std::lock_guard<std::mutex> lk(verifyFnMutex());
        verifyFnStorage() = std::move(fn);
    }
    /**
     * @brief Register a wrap/encrypt bridge for stub builds without a real HSM.
     * @param[in] fn Input parameter.
     * @details Thread-safe; pass an empty function to restore the built-in stub path. Calls: lk(), encryptDataFnMutex(), encryptDataFnStorage(), std::move().
     */
    static void setEncryptDataFn(EncryptDataFn fn) {
        std::lock_guard<std::mutex> lk(encryptDataFnMutex());
        encryptDataFnStorage() = std::move(fn);
    }
    /**
     * @brief Register an unwrap/decrypt bridge for stub builds without a real HSM.
     * @param[in] fn Input parameter.
     * @details Thread-safe; pass an empty function to restore the built-in stub path. Calls: lk(), decryptDataFnMutex(), decryptDataFnStorage(), std::move().
     */
    static void setDecryptDataFn(DecryptDataFn fn) {
        std::lock_guard<std::mutex> lk(decryptDataFnMutex());
        decryptDataFnStorage() = std::move(fn);
    }

    /**
     * List available keys in HSM
     * @return Vector of key information
     * @brief TBD: Describe listKeys.
     */
    std::vector<HSMKeyInfo> listKeys();

    /**
     * Generate new RSA key pair in HSM
     * @param label: Key label
     * @param key_size: Key size in bits (2048, 3072, 4096)
     * @param extractable: Whether key can be extracted (should be false)
     * @return true on success, false otherwise
     */
    bool generateKeyPair(const std::string& label, 
                         uint32_t key_size = 2048,
                         bool extractable = false);

    /**
     * Import certificate for existing key
     * @param key_label: Associated key label
     * @param cert_pem: Certificate in PEM format
     * @return true on success, false otherwise
     * @brief TBD: Describe importCertificate.
     */
    bool importCertificate(const std::string& key_label,
                           const std::string& cert_pem);

    /**
     * Get certificate for key
     * @param key_label: Key label
     * @return Certificate in PEM format, or empty optional if not found
     * @brief TBD: Describe getCertificate.
     */
    std::optional<std::string> getCertificate(const std::string& key_label);

    // -----------------------------------------------------------------------
    // Injectable key-management bridge (STUB #215)
    // -----------------------------------------------------------------------
    using GenerateKeyPairFn =
        std::function<bool(const std::string& label, uint32_t key_size, bool extractable)>;
    using ImportCertificateFn =
        std::function<bool(const std::string& key_label, const std::string& cert_pem)>;
    using GetCertificateFn =
        std::function<std::optional<std::string>(const std::string& key_label)>;

    /**
     * @brief Register callback used by generateKeyPair() in stub builds.
     * @param[in] fn Input parameter.
     * @details Pass empty fn to restore default stub behavior. Calls: lk(), generateKeyPairFnMutex(), generateKeyPairFnStorage(), std::move().
     */
    static void setGenerateKeyPairFn(GenerateKeyPairFn fn) {
        std::lock_guard<std::mutex> lk(generateKeyPairFnMutex());
        generateKeyPairFnStorage() = std::move(fn);
    }
    /**
     * @brief Register callback used by importCertificate() in stub builds.
     * @param[in] fn Input parameter.
     * @details Pass empty fn to restore default stub behavior. Calls: lk(), importCertificateFnMutex(), importCertificateFnStorage(), std::move().
     */
    static void setImportCertificateFn(ImportCertificateFn fn) {
        std::lock_guard<std::mutex> lk(importCertificateFnMutex());
        importCertificateFnStorage() = std::move(fn);
    }
    /**
     * @brief Register callback used by getCertificate() in stub builds.
     * @param[in] fn Input parameter.
     * @details Pass empty fn to restore default stub behavior. Calls: lk(), getCertificateFnMutex(), getCertificateFnStorage(), std::move().
     */
    static void setGetCertificateFn(GetCertificateFn fn) {
        std::lock_guard<std::mutex> lk(getCertificateFnMutex());
        getCertificateFnStorage() = std::move(fn);
    }

    /**
     * Encrypt data using HSM-backed public key (RSA-PKCS#1 v1.5 or OAEP)
     * Intended for DEK wrapping in the key management hierarchy.
     * @param data: Plaintext to encrypt (max ~245 bytes for RSA-2048)
     * @param key_label: Key label in HSM (optional, uses config default if empty)
     * @return Encrypted bytes, empty on failure (check getLastError())
     */
    std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data,
                                     const std::string& key_label = "");

    /**
     * Decrypt data using HSM-backed private key (RSA-PKCS#1 v1.5 or OAEP)
     * Intended for DEK unwrapping in the key management hierarchy.
     * @param encrypted: Ciphertext produced by encryptData()
     * @param key_label: Key label in HSM (optional, uses config default if empty)
     * @return Decrypted plaintext bytes, empty on failure (check getLastError())
     */
    std::vector<uint8_t> decryptData(const std::vector<uint8_t>& encrypted,
                                     const std::string& key_label = "");

    /**
     * Check if HSM is initialized and ready
     * @brief TBD: Describe isReady.
     * @return True on success.
     */
    bool isReady() const;

    /**
     * Get HSM token information
     * @return Token label, serial number, firmware version
     * @brief TBD: Describe getTokenInfo.
     */
    std::string getTokenInfo() const;

    /**
     * Get last error message
     * @brief TBD: Describe getLastError.
     * @return Return value.
     */
    std::string getLastError() const;

    /**
     * Get performance statistics
     * @return Performance metrics (sign/verify counts, timings, pool stats)
     * @brief TBD: Describe getStats.
     */
    HSMPerformanceStats getStats() const;

    /**
     * Reset performance statistics
     * @brief TBD: Describe resetStats.
     */
    void resetStats();

    /**
     * Check if using stub provider (insecure development mode)
     * @return true if stub provider is active, false if real HSM
     * @brief TBD: Describe isStubProvider.
     */
    bool isStubProvider() const;

    /**
     * Perform periodic security check and log warnings if stub is active
     * Should be called periodically (e.g., every 5 minutes) from server
     * @brief TBD: Describe periodicSecurityCheck.
     */
    void periodicSecurityCheck();

private:
    /**
     * @brief TBD: Describe signHashFnMutex.
     * @return Return value.
     * @details Implements signHashFnMutex without additional internal calls.
     */
    static std::mutex& signHashFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe signHashFnStorage.
     * @return Return value.
     * @details Implements signHashFnStorage without additional internal calls.
     */
    static SignHashFn& signHashFnStorage() {
        static SignHashFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe verifyFnMutex.
     * @return Return value.
     * @details Implements verifyFnMutex without additional internal calls.
     */
    static std::mutex& verifyFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe verifyFnStorage.
     * @return Return value.
     * @details Implements verifyFnStorage without additional internal calls.
     */
    static VerifyFn& verifyFnStorage() {
        static VerifyFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe encryptDataFnMutex.
     * @return Return value.
     * @details Implements encryptDataFnMutex without additional internal calls.
     */
    static std::mutex& encryptDataFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe encryptDataFnStorage.
     * @return Return value.
     * @details Implements encryptDataFnStorage without additional internal calls.
     */
    static EncryptDataFn& encryptDataFnStorage() {
        static EncryptDataFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe decryptDataFnMutex.
     * @return Return value.
     * @details Implements decryptDataFnMutex without additional internal calls.
     */
    static std::mutex& decryptDataFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe decryptDataFnStorage.
     * @return Return value.
     * @details Implements decryptDataFnStorage without additional internal calls.
     */
    static DecryptDataFn& decryptDataFnStorage() {
        static DecryptDataFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe generateKeyPairFnMutex.
     * @return Return value.
     * @details Implements generateKeyPairFnMutex without additional internal calls.
     */
    static std::mutex& generateKeyPairFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe generateKeyPairFnStorage.
     * @return Return value.
     * @details Implements generateKeyPairFnStorage without additional internal calls.
     */
    static GenerateKeyPairFn& generateKeyPairFnStorage() {
        static GenerateKeyPairFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe importCertificateFnMutex.
     * @return Return value.
     * @details Implements importCertificateFnMutex without additional internal calls.
     */
    static std::mutex& importCertificateFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe importCertificateFnStorage.
     * @return Return value.
     * @details Implements importCertificateFnStorage without additional internal calls.
     */
    static ImportCertificateFn& importCertificateFnStorage() {
        static ImportCertificateFn fn;
        return fn;
    }
    /**
     * @brief TBD: Describe getCertificateFnMutex.
     * @return Return value.
     * @details Implements getCertificateFnMutex without additional internal calls.
     */
    static std::mutex& getCertificateFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief TBD: Describe getCertificateFnStorage.
     * @return Return value.
     * @details Implements getCertificateFnStorage without additional internal calls.
     */
    static GetCertificateFn& getCertificateFnStorage() {
        static GetCertificateFn fn;
        return fn;
    }

    class Impl;
    std::unique_ptr<Impl> impl_;
    HSMConfig config_;
    bool initialized_ = false;
    std::string last_error_;

    // PKCS#11 helper discovery functions (only active when THEMIS_ENABLE_HSM_REAL)
    struct SessionEntry; // forward
    /**
     * @brief TBD: Describe discoverKeysSession.
     * @param[in,out] s Input/output parameter.
     */
    void discoverKeysSession(SessionEntry& s);
    /**
     * @brief TBD: Describe discoverCertificateSession.
     * @param[in,out] s Input/output parameter.
     */
    void discoverCertificateSession(SessionEntry& s);
    /**
     * @brief Pool-Hilfen (nur real)
     * @return Pointer to the result.
     */
    SessionEntry* acquireSession();
    /**
     * @brief TBD: Describe releaseSession.
     * @param[in,out] s Input/output parameter.
     */
    void releaseSession(SessionEntry* s);
};

/**
 * HSM-Backed PKI Client
 * 
 * High-level wrapper that combines HSM operations with PKI workflows.
 * Compatible with existing VCCPKIClient interface.
 */
class HSMPKIClient {
public:
    /**
     * @brief TBD: Describe HSMPKIClient.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HSMPKIClient(HSMConfig config);
    ~HSMPKIClient();

    /**
     * Sign data with HSM and return PKI-compatible result
     * @brief TBD: Describe sign.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    HSMSignatureResult sign(const std::vector<uint8_t>& data);

    /**
     * Verify signature
     * @brief TBD: Describe verify.
     * @param[in] data Input parameter.
     * @param[in] signature_b64 Input parameter.
     * @return True on success.
     */
    bool verify(const std::vector<uint8_t>& data, const std::string& signature_b64);

    /**
     * Get certificate serial number
     * @brief TBD: Describe getCertSerial.
     * @return Return value.
     */
    std::optional<std::string> getCertSerial();

    /**
     * Check if HSM is ready
     * @brief TBD: Describe isReady.
     * @return True on success.
     */
    bool isReady() const;

private:
    std::unique_ptr<HSMProvider> hsm_;
};

} // namespace security
} // namespace themis
