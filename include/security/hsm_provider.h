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

class HSMProvider {
public:
    /**
     * @brief HSMProvider.
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
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();

    /**
     * @brief Finalize.
     */
    void finalize();

    HSMSignatureResult sign(const std::vector<uint8_t>& data, 
                            const std::string& key_label = "");

    HSMSignatureResult signHash(const std::vector<uint8_t>& hash,
                                const std::string& key_label = "");

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
     * @brief Set Sign Hash Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), signHashFnMutex(), signHashFnStorage(), std::move().
     */
    static void setSignHashFn(SignHashFn fn) {
        std::lock_guard<std::mutex> lk(signHashFnMutex());
        signHashFnStorage() = std::move(fn);
    }
    /**
     * @brief Set Verify Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), verifyFnMutex(), verifyFnStorage(), std::move().
     */
    static void setVerifyFn(VerifyFn fn) {
        std::lock_guard<std::mutex> lk(verifyFnMutex());
        verifyFnStorage() = std::move(fn);
    }
    /**
     * @brief Set Encrypt Data Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), encryptDataFnMutex(), encryptDataFnStorage(), std::move().
     */
    static void setEncryptDataFn(EncryptDataFn fn) {
        std::lock_guard<std::mutex> lk(encryptDataFnMutex());
        encryptDataFnStorage() = std::move(fn);
    }
    /**
     * @brief Set Decrypt Data Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), decryptDataFnMutex(), decryptDataFnStorage(), std::move().
     */
    static void setDecryptDataFn(DecryptDataFn fn) {
        std::lock_guard<std::mutex> lk(decryptDataFnMutex());
        decryptDataFnStorage() = std::move(fn);
    }

    /**
     * @brief List Keys.
     * @return Return value.
     */
    std::vector<HSMKeyInfo> listKeys();

    bool generateKeyPair(const std::string& label, 
                         uint32_t key_size = 2048,
                         bool extractable = false);

    /**
     * @brief Import Certificate.
     * @param[in] key_label Input parameter.
     * @param[in] cert_pem Input parameter.
     * @return True when the operation succeeds.
     */
    bool importCertificate(const std::string& key_label,
                           const std::string& cert_pem);

    /**
     * @brief Get Certificate.
     * @param[in] key_label Input parameter.
     * @return Return value.
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
     * @brief Set Generate Key Pair Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), generateKeyPairFnMutex(), generateKeyPairFnStorage(), std::move().
     */
    static void setGenerateKeyPairFn(GenerateKeyPairFn fn) {
        std::lock_guard<std::mutex> lk(generateKeyPairFnMutex());
        generateKeyPairFnStorage() = std::move(fn);
    }
    /**
     * @brief Set Import Certificate Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), importCertificateFnMutex(), importCertificateFnStorage(), std::move().
     */
    static void setImportCertificateFn(ImportCertificateFn fn) {
        std::lock_guard<std::mutex> lk(importCertificateFnMutex());
        importCertificateFnStorage() = std::move(fn);
    }
    /**
     * @brief Set Get Certificate Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), getCertificateFnMutex(), getCertificateFnStorage(), std::move().
     */
    static void setGetCertificateFn(GetCertificateFn fn) {
        std::lock_guard<std::mutex> lk(getCertificateFnMutex());
        getCertificateFnStorage() = std::move(fn);
    }

    std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data,
                                     const std::string& key_label = "");

    std::vector<uint8_t> decryptData(const std::vector<uint8_t>& encrypted,
                                     const std::string& key_label = "");

    /**
     * @brief Is Ready.
     * @return True when the operation succeeds.
     */
    bool isReady() const;

    /**
     * @brief Get Token Info.
     * @return Return value.
     */
    std::string getTokenInfo() const;

    /**
     * @brief Get Last Error.
     * @return Return value.
     */
    std::string getLastError() const;

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    HSMPerformanceStats getStats() const;

    /**
     * @brief Reset Stats.
     */
    void resetStats();

    /**
     * @brief Is Stub Provider.
     * @return True when the operation succeeds.
     */
    bool isStubProvider() const;

    /**
     * @brief Periodic Security Check.
     */
    void periodicSecurityCheck();

private:
    /**
     * @brief Sign Hash Fn Mutex.
     * @return Return value.
     * @details Implements signHashFnMutex without additional internal calls.
     */
    static std::mutex& signHashFnMutex();
    static SignHashFn& signHashFnStorage();
    static std::mutex& verifyFnMutex();
    static VerifyFn& verifyFnStorage();
    static std::mutex& encryptDataFnMutex();
    static EncryptDataFn& encryptDataFnStorage();
    static std::mutex& decryptDataFnMutex();
    static DecryptDataFn& decryptDataFnStorage();
    static std::mutex& generateKeyPairFnMutex();
    static GenerateKeyPairFn& generateKeyPairFnStorage();
    static std::mutex& importCertificateFnMutex();
    static ImportCertificateFn& importCertificateFnStorage();
    static std::mutex& getCertificateFnMutex();
    static GetCertificateFn& getCertificateFnStorage();

    class Impl;
    std::unique_ptr<Impl> impl_;
    HSMConfig config_;
    bool initialized_ = false;
    std::string last_error_;

    // PKCS#11 helper discovery functions (only active when THEMIS_ENABLE_HSM_REAL)
    struct SessionEntry; // forward
    /**
     * @brief Discover Keys Session.
     * @param[in,out] s Input/output parameter.
     */
    void discoverKeysSession(SessionEntry& s);
    /**
     * @brief Discover Certificate Session.
     * @param[in,out] s Input/output parameter.
     */
    void discoverCertificateSession(SessionEntry& s);
    /**
     * @brief Acquire Session.
     * @return Pointer to the result.
     */
    SessionEntry* acquireSession();
    /**
     * @brief Release Session.
     * @param[in,out] s Input/output parameter.
     */
    void releaseSession(SessionEntry* s);
};

class HSMPKIClient {
public:
    /**
     * @brief HSMPKIClient.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HSMPKIClient(HSMConfig config);
    ~HSMPKIClient();

    /**
     * @brief Sign.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    HSMSignatureResult sign(const std::vector<uint8_t>& data);

    /**
     * @brief Verify identity and enforce network policies for a request.
     * @param[in] data Input parameter.
     * @param[in] signature_b64 Input parameter.
     * @return Verification result.
     */
    bool verify(const std::vector<uint8_t>& data, const std::string& signature_b64);

    /**
     * @brief Get Cert Serial.
     * @return Return value.
     */
    std::optional<std::string> getCertSerial();

    /**
     * @brief Is Ready.
     * @return True when the operation succeeds.
     */
    bool isReady() const;

private:
    std::unique_ptr<HSMProvider> hsm_;
};

} // namespace security
} // namespace themis
