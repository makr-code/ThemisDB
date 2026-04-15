/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            signature_verifier.h                               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:35:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     183                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • afc6b2738b  2026-03-26  fix: Resolve BSI/RAG production blockers – JWT, mTLS, CRL... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
