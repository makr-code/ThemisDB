/**
 * @file keyprovider_signing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/signing.h"
#include <stdexcept>
#include "security/signing_provider.h"
#include "security/cms_signing.h"
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <memory>
#include <sstream>

namespace themis {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct BIO_Deleter {
    void operator()(BIO* p) const { if (p) BIO_free(p); }
};
struct EVP_PKEY_Deleter {
    void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); }
};
struct X509_Deleter {
    void operator()(X509* p) const { if (p) X509_free(p); }
};

using BIO_ptr = std::unique_ptr<BIO, BIO_Deleter>;
using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEY_Deleter>;
using X509_ptr = std::unique_ptr<X509, X509_Deleter>;

} // anonymous namespace

class KeyProviderSigningService : public SigningService {
public:
    explicit KeyProviderSigningService(std::shared_ptr<KeyProvider> kp) : kp_(std::move(kp)) {}

    SigningResult sign(const std::vector<uint8_t>& data, const std::string& key_id) override {
        // If KeyProvider also implements SigningProvider (HSM/KMS), use its sign API
        if (auto sp = dynamic_cast<SigningProvider*>(kp_.get())) {
            return sp->sign(key_id, data);
        }

        // Fallback: retrieve raw private key bytes and perform local CMS signing
        auto key_bytes = kp_->getKey(key_id);
        BIO_ptr bio(BIO_new_mem_buf(key_bytes.data(), static_cast<int>(key_bytes.size())));
        if (!bio) throw std::runtime_error("BIO_new_mem_buf failed");

        EVP_PKEY_ptr pkey(PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr));
        if (!pkey) throw std::runtime_error("Failed to parse private key from KeyProvider");

        // Optionally load cert (transfer ownership to CMSSigningService)
        X509* cert_ptr = nullptr;
        try {
            auto cert_bytes = kp_->getKey(key_id + ":cert");
            if (!cert_bytes.empty()) {
                BIO_ptr cbio(BIO_new_mem_buf(cert_bytes.data(), static_cast<int>(cert_bytes.size())));
                X509_ptr x(PEM_read_bio_X509(cbio.get(), nullptr, nullptr, nullptr));
                if (x) cert_ptr = x.release(); // transfer ownership to CMSSigningService below
            }
        } catch (...) {
            // missing cert is acceptable
        }

        // pkey is owned by the CMSSigningService after construction
        CMSSigningService cms(cert_ptr, pkey.release());
        return cms.sign(data, key_id);
    }

    bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature, const std::string& key_id) override {
        // Try to obtain cert from KeyProvider
        try {
            auto cert_bytes = kp_->getKey(key_id + ":cert");
            if (!cert_bytes.empty()) {
                BIO_ptr cbio(BIO_new_mem_buf(cert_bytes.data(), static_cast<int>(cert_bytes.size())));
                X509_ptr x(PEM_read_bio_X509(cbio.get(), nullptr, nullptr, nullptr));
                if (x) {
                    // transfer ownership of 'x' to CMSSigningService
                    CMSSigningService cms(x.release(), nullptr);
                    return cms.verify(data, signature, key_id);
                }
            }
        } catch (...) {
            // fallthrough
        }
        // No cert available -> verification not possible here
        return false;
    }

private:
    std::shared_ptr<KeyProvider> kp_;
};

std::shared_ptr<SigningService> createKeyProviderSigningService(std::shared_ptr<KeyProvider> kp) {
    return std::make_shared<KeyProviderSigningService>(kp);
}

} // namespace themis


