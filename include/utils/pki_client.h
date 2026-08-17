/**
 * @file pki_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <functional>

namespace themis {
namespace utils {

struct PKIConfig {
    std::string service_id;
    std::string endpoint;           // e.g. https://localhost:8443/api/v1
    std::string cert_path;          // optional: certificate path (PEM)
    std::string key_path;           // optional: private key path (PEM)
    std::string key_passphrase;     // optional: private key passphrase
    std::string signature_algorithm = "RSA-SHA256";
    
    // TLS Hardening: Certificate Pinning
    bool enable_cert_pinning = false;                   // Enable certificate pinning
    // Allowed pins for the TLS peer public key. Accepted formats:
    // - hex SHA-256 fingerprint (with or without ':' separators)
    // - libcurl "sha256//<base64>" pin format
    // Requests fail closed when pinning is enabled and no valid pin can be built.
    std::vector<std::string> pinned_cert_fingerprints;
    bool pin_leaf_only = false;                         // If true, pin only leaf cert; if false, pin any cert in chain

    // X.509 Chain Verification: path to PEM trust store (CA bundle or CA cert).
    // When set, verifyHash() validates the full certificate chain via X509_verify_cert()
    // before accepting any signature.  Must be configured for production traffic.
    std::string trust_store_path;

    // Internal CA / ACME URL for automatic PKCS#10 CSR-based certificate provisioning.
    // When set, signHash() generates a PKCS#10 CSR via X509_REQ_* API and submits it to
    // {ca_url}/sign-csr to obtain a real X.509 certificate; only falls back to the stub
    // when this is empty AND no key_path/cert_path is configured.
    std::string ca_url;
};

struct SignatureResult {
    bool ok = false;
    std::string signature_id;       // opaque id from PKI
    std::string algorithm;          // e.g. RSA-SHA256
    std::string signature_b64;      // signature over provided hash (base64)
    std::string cert_serial;        // certificate serial used
};

// PKI client: sign/verify data hashes using OpenSSL.
// Production paths: local RSA key/cert, REST PKI endpoint, or PKCS#10 CSR provisioning via ca_url.
// Stub fallback paths are only compiled when THEMIS_TEST_MODE is defined.
/** @brief Stub fallback paths are only compiled when THEMIS_TEST_MODE is defined. */
class VCCPKIClient {
public:
    using SignHashFn = std::function<SignatureResult(const std::vector<uint8_t>& hash_bytes)>;
    using VerifyHashFn = std::function<bool(const std::vector<uint8_t>& hash_bytes, const SignatureResult& sig)>;

    explicit VCCPKIClient(PKIConfig cfg);

    // Sign a precomputed hash (e.g. SHA-256 over ciphertext batch)
    //
    // Graceful degradation for PKI service failure:
    // - If PKI endpoint is unreachable: falls back to local key if available
    // - If certificate is expired: signHash still succeeds but increments warning counter
    // - If key material is invalid: returns empty/invalid SignatureResult
    // - Caller should validate result.ok before using signature
    //
    // @error_contract
    // | Condition | Behavior | Recovery |
    // |-----------|----------|----------|
    // | PKI endpoint timeout | Falls back to local key | Escalate and retry later |
    // | Certificate expired | Warning logged, signs with expired cert | Renew certificate |
    // | No key available | Returns ok=false | Configure key_path or ca_url |
    // | Local key corruption | Returns ok=false | Rotate to new key material |
    [[nodiscard]] SignatureResult signHash(const std::vector<uint8_t>& hash_bytes) const;

    // Verify a signature against a precomputed hash
    //
    // Graceful degradation for PKI service failure:
    // - If PKI endpoint is unreachable: falls back to cached cert (if available)
    // - If certificate chain cannot be verified: returns false (fail-secure)
    // - If certificate is expired: verifyHash still validates signature but warns
    // - Caller should always check return value before trusting verification
    //
    // @error_contract
    // | Condition | Behavior | Recovery |
    // |-----------|----------|----------|
    // | Certificate not found | Returns false | Ensure cert_path or trust_store configured |
    // | Chain verification fails | Returns false | Check certificate chain integrity |
    // | Signature invalid | Returns false | Check data integrity and signature format |
    // | Trust store unavailable | Skips chain validation, verifies sig only | Configure trust_store_path |
    [[nodiscard]] bool verifyHash(const std::vector<uint8_t>& hash_bytes, const SignatureResult& sig) const;

    const PKIConfig& config() const { return cfg_; }

    // Return certificate serial (hex) if a certificate path is configured and readable.
    // Returns empty optional when no cert is available or parsing fails.
    std::optional<std::string> getCertSerial() const;

    // Generate a PKCS#10 CSR (PEM-encoded) using the configured key and service_id.
    // Returns the PEM string on success, or empty string on failure.
    // Requires key_path to be set in the configuration.
    std::string generateCSR() const;

    static void setSignHashFn(SignHashFn fn) {
        std::lock_guard<std::mutex> lock(signHashFnMutex());
        signHashFnStorage() = std::move(fn);
    }
    static void setVerifyHashFn(VerifyHashFn fn) {
        std::lock_guard<std::mutex> lock(verifyHashFnMutex());
        verifyHashFnStorage() = std::move(fn);
    }

private:
    static std::mutex& signHashFnMutex() {
        static std::mutex m;
        return m;
    }
    static SignHashFn& signHashFnStorage() {
        static SignHashFn fn;
        return fn;
    }
    static std::mutex& verifyHashFnMutex() {
        static std::mutex m;
        return m;
    }
    static VerifyHashFn& verifyHashFnStorage() {
        static VerifyHashFn fn;
        return fn;
    }

    PKIConfig cfg_;

    // In-memory certificate cache populated via CSR provisioning (ca_url path).
    // Protected by cert_cache_mutex_ for thread-safe lazy initialisation.
    mutable std::mutex    cert_cache_mutex_;
    mutable std::string   cached_cert_pem_;
    mutable std::string   cached_cert_serial_;
};

} // namespace utils
} // namespace themis
