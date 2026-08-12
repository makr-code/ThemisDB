/**
 * @file signature_verifier.h
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
#include <vector>
#include <memory>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <mutex>
#include <chrono>

namespace themis {
namespace llm {
namespace security {

/**
 * @brief Result of Signature Verification
 */
struct SignatureVerificationResult {
    bool is_valid = false;
    std::string algorithm;
    std::string signer_identity;
    std::string error_message;
    
    // Certificate chain info
    std::vector<std::string> chain_fingerprints;
    bool chain_valid = false;
};

/**
 * @brief Abstract Base Class for Signature Verification
 * 
 * Design Pattern: Chain of Responsibility
 * Each verifier checks one aspect and passes to next
 */
class ISignatureVerifier {
public:
    virtual ~ISignatureVerifier() = default;
    
    virtual SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) = 0;
    
    void setNext(std::shared_ptr<ISignatureVerifier> next) {
        next_ = next;
    }

protected:
    std::shared_ptr<ISignatureVerifier> next_;
    
    SignatureVerificationResult passToNext(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    );
};

/**
 * @brief RSA-SHA256 Signature Verifier
 */
class RSA_SHA256_Verifier : public ISignatureVerifier {
public:
    RSA_SHA256_Verifier() = default;
    ~RSA_SHA256_Verifier() override = default;
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::unique_ptr<X509, decltype(&X509_free)> 
        loadCertificate(const std::string& cert_pem);
    
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>
        extractPublicKey(X509* cert);
};

/**
 * @brief ECDSA-SHA256 Signature Verifier
 * 
 * Supports P-256 (prime256v1) and P-384 (secp384r1) curves.
 * Accepts DER-encoded ECDSA signatures (standard OpenSSL output format).
 * Raw concatenated (r||s) input (64 bytes for P-256, 96 bytes for P-384)
 * is automatically converted to DER before verification.
 */
class ECDSA_SHA256_Verifier : public ISignatureVerifier {
public:
    ECDSA_SHA256_Verifier() = default;
    ~ECDSA_SHA256_Verifier() override = default;
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::unique_ptr<X509, decltype(&X509_free)> 
        loadCertificate(const std::string& cert_pem);
    
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>
        extractPublicKey(X509* cert);
    
    /**
     * @brief Validate EC curve OID to prevent algorithm downgrade
     * @param cert X.509 certificate containing the EC key
     * @return true if curve is P-256 or P-384, false otherwise
     */
    bool validateECCurve(X509* cert);
    
    /**
     * @brief Convert concatenated (r||s) format to DER format if needed
     * @param signature_input Input signature (may be concatenated or DER)
     * @return Signature in DER format for OpenSSL verification
     */
    std::vector<uint8_t> convertSignatureToDER(const std::vector<uint8_t>& signature_input);
};

/**
 * @brief ECDSA-SHA384 Signature Verifier
 * 
 * Supports P-256 (prime256v1) and P-384 (secp384r1) curves.
 * Provides stronger cryptographic guarantees than SHA256.
 * Accepts DER-encoded ECDSA signatures (standard OpenSSL output format).
 * Raw concatenated (r||s) input (64 bytes for P-256, 96 bytes for P-384)
 * is automatically converted to DER before verification.
 */
class ECDSA_SHA384_Verifier : public ISignatureVerifier {
public:
    ECDSA_SHA384_Verifier() = default;
    ~ECDSA_SHA384_Verifier() override = default;
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::unique_ptr<X509, decltype(&X509_free)> 
        loadCertificate(const std::string& cert_pem);
    
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>
        extractPublicKey(X509* cert);
    
    /**
     * @brief Validate EC curve OID to prevent algorithm downgrade
     * @param cert X.509 certificate containing the EC key
     * @return true if curve is P-256 or P-384, false otherwise
     */
    bool validateECCurve(X509* cert);
    
    /**
     * @brief Convert concatenated (r||s) format to DER format if needed
     * @param signature_input Input signature (may be concatenated or DER)
     * @return Signature in DER format for OpenSSL verification
     */
    std::vector<uint8_t> convertSignatureToDER(const std::vector<uint8_t>& signature_input);
};

/**
 * @brief Certificate Chain Verifier
 */
class CertificateChainVerifier : public ISignatureVerifier {
public:
    explicit CertificateChainVerifier(const std::string& ca_bundle_path);
    ~CertificateChainVerifier() override = default;
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::string ca_bundle_path_;
    
    bool verifyCertificateChain(X509* cert, X509_STORE* store);
};

/**
 * @brief Certificate Revocation List (CRL) Checker
 */
class CRLChecker : public ISignatureVerifier {
public:
    explicit CRLChecker(const std::string& crl_url);
    ~CRLChecker() override = default;
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::string crl_url_;
    
    // In-memory CRL cache: parsed CRL + expiry time point
    struct CRLCache {
        X509_CRL* crl = nullptr;
        std::chrono::steady_clock::time_point expires_at;
    };
    mutable std::mutex cache_mutex_;
    mutable CRLCache crl_cache_;

    /** Download and parse the CRL from crl_url_; returns nullptr on failure. */
    X509_CRL* downloadAndParseCRL() const;

    /** Return the cached CRL (re-fetching if expired), or nullptr. */
    X509_CRL* getOrRefreshCRL() const;

    bool isCertificateRevoked(X509* cert);
};

/**
 * @brief Signature Verifier Builder (Fluent Interface)
 * 
 * Design Pattern: Builder Pattern
 * 
 * Example usage:
 * @code
 * auto verifier = SignatureVerifierBuilder()
 *     .withECDSA_SHA256()
 *     .withCertificateChainValidation("/etc/ssl/certs/ca-certificates.crt")
 *     .build();
 * @endcode
 */
class SignatureVerifierBuilder {
public:
    SignatureVerifierBuilder() = default;
    
    /// Add RSA-SHA256 signature verification to the chain
    SignatureVerifierBuilder& withRSA_SHA256();
    
    /// Add ECDSA-SHA256 signature verification to the chain (supports P-256, P-384)
    SignatureVerifierBuilder& withECDSA_SHA256();
    
    /// Add ECDSA-SHA384 signature verification to the chain (supports P-256, P-384)
    SignatureVerifierBuilder& withECDSA_SHA384();
    
    /// Add certificate chain validation using the provided CA bundle
    SignatureVerifierBuilder& withCertificateChainValidation(
        const std::string& ca_bundle_path
    );
    
    /// Add CRL (Certificate Revocation List) checking
    SignatureVerifierBuilder& withCRLCheck(const std::string& crl_url);
    
    /// Build and return the verification chain
    std::shared_ptr<ISignatureVerifier> build();

private:
    std::shared_ptr<ISignatureVerifier> head_;
    std::shared_ptr<ISignatureVerifier> tail_;
};

} // namespace security
} // namespace llm
} // namespace themis
