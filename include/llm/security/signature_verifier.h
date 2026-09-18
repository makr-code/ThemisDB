/**
 * @file signature_verifier.h
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

struct SignatureVerificationResult {
    bool is_valid = false;
    std::string algorithm;
    std::string signer_identity;
    std::string error_message;
    
    // Certificate chain info
    std::vector<std::string> chain_fingerprints;
    bool chain_valid = false;
};

class ISignatureVerifier {
public:
    /**
     * @brief ISignature Verifier.
     * @return Return value.
     */
    virtual ~ISignatureVerifier() = default;
    
    /**
     * @brief Verify identity and enforce network policies for a request.
     * @param[in] data Input parameter.
     * @param[in] signature Input parameter.
     * @param[in] cert_pem Input parameter.
     * @return Verification result.
     */
    virtual SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) = 0;
    
    /**
     * @brief Set Next.
     * @param[in] next Input parameter.
     * @details Implements setNext without additional internal calls.
     */
    void setNext(std::shared_ptr<ISignatureVerifier> next) {
        next_ = next;
    }

protected:
    std::shared_ptr<ISignatureVerifier> next_;
    
    /**
     * @brief Pass To Next.
     * @param[in] data Input parameter.
     * @param[in] signature Input parameter.
     * @param[in] cert_pem Input parameter.
     * @return Return value.
     */
    SignatureVerificationResult passToNext(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    );
};

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
     * @brief Validate ECCurve.
     * @param[in,out] cert Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateECCurve(X509* cert);
    
    /**
     * @brief Convert Signature To DER.
     * @param[in] signature_input Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> convertSignatureToDER(const std::vector<uint8_t>& signature_input);
};

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
     * @brief Validate ECCurve.
     * @param[in,out] cert Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateECCurve(X509* cert);
    
    /**
     * @brief Convert Signature To DER.
     * @param[in] signature_input Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> convertSignatureToDER(const std::vector<uint8_t>& signature_input);
};

class CertificateChainVerifier : public ISignatureVerifier {
public:
    /**
     * @brief Certificate Chain Verifier.
     * @param[in] ca_bundle_path Path to the ca bundle.
     * @return Return value.
     */
    explicit CertificateChainVerifier(const std::string& ca_bundle_path);
    ~CertificateChainVerifier() override = default;
    
    SignatureVerificationResult verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_pem
    ) override;

private:
    std::string ca_bundle_path_;
    
    /**
     * @brief Verify Certificate Chain.
     * @param[in,out] cert Input/output parameter.
     * @param[in,out] store Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyCertificateChain(X509* cert, X509_STORE* store);
};

class CRLChecker : public ISignatureVerifier {
public:
    /**
     * @brief CRLChecker.
     * @param[in] crl_url Input parameter.
     * @return Return value.
     */
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

    /**
     * @brief Download And Parse CRL.
     * @return Pointer to the result.
     */
    X509_CRL* downloadAndParseCRL() const;

    /**
     * @brief Get Or Refresh CRL.
     * @return Pointer to the result.
     */
    X509_CRL* getOrRefreshCRL() const;

    /**
     * @brief Is Certificate Revoked.
     * @param[in,out] cert Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool isCertificateRevoked(X509* cert);
};

class SignatureVerifierBuilder {
public:
    SignatureVerifierBuilder() = default;
    
    /**
     * @brief With RSA SHA256.
     * @return Return value.
     */
    SignatureVerifierBuilder& withRSA_SHA256();
    
    /**
     * @brief With ECDSA SHA256.
     * @return Return value.
     */
    SignatureVerifierBuilder& withECDSA_SHA256();
    
    /**
     * @brief With ECDSA SHA384.
     * @return Return value.
     */
    SignatureVerifierBuilder& withECDSA_SHA384();
    
    /**
     * @brief With Certificate Chain Validation.
     * @param[in] ca_bundle_path Path to the ca bundle.
     * @return Return value.
     */
    SignatureVerifierBuilder& withCertificateChainValidation(
        const std::string& ca_bundle_path
    );
    
    /**
     * @brief With CRLCheck.
     * @param[in] crl_url Input parameter.
     * @return Return value.
     */
    SignatureVerifierBuilder& withCRLCheck(const std::string& crl_url);
    
    /**
     * @brief Build.
     * @return Return value.
     */
    std::shared_ptr<ISignatureVerifier> build();

private:
    std::shared_ptr<ISignatureVerifier> head_;
    std::shared_ptr<ISignatureVerifier> tail_;
};

} // namespace security
} // namespace llm
} // namespace themis
