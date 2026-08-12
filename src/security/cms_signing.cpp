/**
 * @file cms_signing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/cms_signing.h"
#include <openssl/cms.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <memory>
#include <stdexcept>

namespace themis {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct CMS_BIO_Deleter {
    void operator()(BIO* p) const { if (p) BIO_free(p); }
};
struct CMS_ContentInfo_Deleter {
    void operator()(CMS_ContentInfo* p) const { if (p) CMS_ContentInfo_free(p); }
};
struct CMS_X509_STORE_Deleter {
    void operator()(X509_STORE* p) const { if (p) X509_STORE_free(p); }
};

using CMS_BIO_ptr = std::unique_ptr<BIO, CMS_BIO_Deleter>;
using CMS_ContentInfo_ptr = std::unique_ptr<CMS_ContentInfo, CMS_ContentInfo_Deleter>;
using CMS_X509_STORE_ptr = std::unique_ptr<X509_STORE, CMS_X509_STORE_Deleter>;

} // anonymous namespace

CMSSigningService::CMSSigningService(std::shared_ptr<X509> cert, std::shared_ptr<EVP_PKEY> pkey)
    : cert_(std::move(cert)), pkey_(std::move(pkey)) {}

CMSSigningService::CMSSigningService(X509* cert, EVP_PKEY* pkey)
        : cert_(cert ? std::shared_ptr<X509>(cert, X509_free) : nullptr),
            pkey_(pkey ? std::shared_ptr<EVP_PKEY>(pkey, EVP_PKEY_free) : nullptr) {}

CMSSigningService::~CMSSigningService() = default;

SigningResult CMSSigningService::sign(const std::vector<uint8_t>& data, const std::string& /*key_id*/) {
    SigningResult res;
    res.algorithm = "CMS/DETACHED+SHA256";

    CMS_BIO_ptr in(BIO_new_mem_buf(data.data(), static_cast<int>(data.size())));
    if (!in) throw std::runtime_error("BIO_new_mem_buf failed");

    CMS_ContentInfo_ptr cms(CMS_sign(cert_.get(), pkey_.get(), nullptr, in.get(), CMS_DETACHED | CMS_BINARY));
    if (!cms) {
        throw std::runtime_error("CMS_sign failed");
    }

    CMS_BIO_ptr out(BIO_new(BIO_s_mem()));
    if (!out) {
        throw std::runtime_error("BIO_new failed");
    }

    if (i2d_CMS_bio(out.get(), cms.get()) <= 0) {
        throw std::runtime_error("i2d_CMS_bio failed");
    }

    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(out.get(), &bptr);
    if (bptr && bptr->length > 0) {
        res.signature.assign(reinterpret_cast<uint8_t*>(bptr->data), reinterpret_cast<uint8_t*>(bptr->data) + bptr->length);
    }

    return res;
}

bool CMSSigningService::verify(const std::vector<uint8_t>& data,
                                const std::vector<uint8_t>& signature,
                                const std::string& /*key_id*/) {
    CMS_BIO_ptr sig_bio(BIO_new_mem_buf(signature.data(), static_cast<int>(signature.size())));
    if (!sig_bio) return false;

    CMS_ContentInfo_ptr cms(d2i_CMS_bio(sig_bio.get(), nullptr));
    if (!cms) return false;

    CMS_BIO_ptr in(BIO_new_mem_buf(data.data(), static_cast<int>(data.size())));
    if (!in) return false;

    CMS_X509_STORE_ptr store(X509_STORE_new());
    if (!store) return false;

    // Add our cert as trusted for verification (self-signed test use-case)
    if (X509_STORE_add_cert(store.get(), cert_.get()) != 1) {
        return false;
    }

    int flags = CMS_BINARY;
    int ok = CMS_verify(cms.get(), nullptr, store.get(), in.get(), nullptr, flags);

    return ok == 1;
}

} // namespace themis

