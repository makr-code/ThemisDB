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

/*
 * ThemisDB | File: signature_verifier.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:49:01
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 169
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #527 Implement RSA-SHA256 signat... (2026-03-11) | #518 LLM/LoRA System Analysis: C... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
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
 */
class SignatureVerifierBuilder {
public:
    SignatureVerifierBuilder() = default;
    
    SignatureVerifierBuilder& withRSA_SHA256();
    SignatureVerifierBuilder& withCertificateChainValidation(
        const std::string& ca_bundle_path
    );
    SignatureVerifierBuilder& withCRLCheck(const std::string& crl_url);
    
    std::shared_ptr<ISignatureVerifier> build();

private:
    std::shared_ptr<ISignatureVerifier> head_;
    std::shared_ptr<ISignatureVerifier> tail_;
};

} // namespace security
} // namespace llm
} // namespace themis
