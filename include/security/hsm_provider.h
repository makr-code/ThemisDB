/**
 * @file hsm_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=21; TODO=1, Stub=19, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
     */
    bool initialize();

    /**
     * Finalize HSM session and cleanup
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

    /// Register a signing bridge for stub builds without a real HSM.
    /// Thread-safe; pass an empty function to restore the built-in stub path.
    static void setSignHashFn(SignHashFn fn) {
        std::lock_guard<std::mutex> lk(signHashFnMutex());
        signHashFnStorage() = std::move(fn);
    }
    /// Register a verification bridge for stub builds without a real HSM.
    /// Thread-safe; pass an empty function to restore the built-in stub path.
    static void setVerifyFn(VerifyFn fn) {
        std::lock_guard<std::mutex> lk(verifyFnMutex());
        verifyFnStorage() = std::move(fn);
    }
    /// Register a wrap/encrypt bridge for stub builds without a real HSM.
    /// Thread-safe; pass an empty function to restore the built-in stub path.
    static void setEncryptDataFn(EncryptDataFn fn) {
        std::lock_guard<std::mutex> lk(encryptDataFnMutex());
        encryptDataFnStorage() = std::move(fn);
    }
    /// Register an unwrap/decrypt bridge for stub builds without a real HSM.
    /// Thread-safe; pass an empty function to restore the built-in stub path.
    static void setDecryptDataFn(DecryptDataFn fn) {
        std::lock_guard<std::mutex> lk(decryptDataFnMutex());
        decryptDataFnStorage() = std::move(fn);
    }

    /**
     * List available keys in HSM
     * @return Vector of key information
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
     */
    bool importCertificate(const std::string& key_label,
                           const std::string& cert_pem);

    /**
     * Get certificate for key
     * @param key_label: Key label
     * @return Certificate in PEM format, or empty optional if not found
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

    /// Register callback used by generateKeyPair() in stub builds.
    /// Pass empty fn to restore default stub behavior.
    static void setGenerateKeyPairFn(GenerateKeyPairFn fn) {
        std::lock_guard<std::mutex> lk(generateKeyPairFnMutex());
        generateKeyPairFnStorage() = std::move(fn);
    }
    /// Register callback used by importCertificate() in stub builds.
    /// Pass empty fn to restore default stub behavior.
    static void setImportCertificateFn(ImportCertificateFn fn) {
        std::lock_guard<std::mutex> lk(importCertificateFnMutex());
        importCertificateFnStorage() = std::move(fn);
    }
    /// Register callback used by getCertificate() in stub builds.
    /// Pass empty fn to restore default stub behavior.
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
     */
    bool isReady() const;

    /**
     * Get HSM token information
     * @return Token label, serial number, firmware version
     */
    std::string getTokenInfo() const;

    /**
     * Get last error message
     */
    std::string getLastError() const;

    /**
     * Get performance statistics
     * @return Performance metrics (sign/verify counts, timings, pool stats)
     */
    HSMPerformanceStats getStats() const;

    /**
     * Reset performance statistics
     */
    void resetStats();

    /**
     * Check if using stub provider (insecure development mode)
     * @return true if stub provider is active, false if real HSM
     */
    bool isStubProvider() const;

    /**
     * Perform periodic security check and log warnings if stub is active
     * Should be called periodically (e.g., every 5 minutes) from server
     */
    void periodicSecurityCheck();

private:
    static std::mutex& signHashFnMutex() {
        static std::mutex m;
        return m;
    }
    static SignHashFn& signHashFnStorage() {
        static SignHashFn fn;
        return fn;
    }
    static std::mutex& verifyFnMutex() {
        static std::mutex m;
        return m;
    }
    static VerifyFn& verifyFnStorage() {
        static VerifyFn fn;
        return fn;
    }
    static std::mutex& encryptDataFnMutex() {
        static std::mutex m;
        return m;
    }
    static EncryptDataFn& encryptDataFnStorage() {
        static EncryptDataFn fn;
        return fn;
    }
    static std::mutex& decryptDataFnMutex() {
        static std::mutex m;
        return m;
    }
    static DecryptDataFn& decryptDataFnStorage() {
        static DecryptDataFn fn;
        return fn;
    }
    static std::mutex& generateKeyPairFnMutex() {
        static std::mutex m;
        return m;
    }
    static GenerateKeyPairFn& generateKeyPairFnStorage() {
        static GenerateKeyPairFn fn;
        return fn;
    }
    static std::mutex& importCertificateFnMutex() {
        static std::mutex m;
        return m;
    }
    static ImportCertificateFn& importCertificateFnStorage() {
        static ImportCertificateFn fn;
        return fn;
    }
    static std::mutex& getCertificateFnMutex() {
        static std::mutex m;
        return m;
    }
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
    void discoverKeysSession(SessionEntry& s);
    void discoverCertificateSession(SessionEntry& s);
    // Pool-Hilfen (nur real)
    SessionEntry* acquireSession();
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
    explicit HSMPKIClient(HSMConfig config);
    ~HSMPKIClient();

    /**
     * Sign data with HSM and return PKI-compatible result
     */
    HSMSignatureResult sign(const std::vector<uint8_t>& data);

    /**
     * Verify signature
     */
    bool verify(const std::vector<uint8_t>& data, const std::string& signature_b64);

    /**
     * Get certificate serial number
     */
    std::optional<std::string> getCertSerial();

    /**
     * Check if HSM is ready
     */
    bool isReady() const;

private:
    std::unique_ptr<HSMProvider> hsm_;
};

} // namespace security
} // namespace themis
