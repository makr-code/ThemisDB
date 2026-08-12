/**
 * @file openssl_deleter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB OpenSSL RAII Wrappers
 * ================================
 * Provides RAII (Resource Acquisition Is Initialization) wrappers for OpenSSL objects
 * using std::unique_ptr with custom deleters. This eliminates manual memory management
 * and prevents memory leaks in error paths.
 * 
 * Usage:
 *   auto pkey = utils::make_evp_key();
 *   if (!pkey) return error;
 *   // ... use pkey ...
 *   // Automatic cleanup when pkey goes out of scope
 */

#pragma once

#include <memory>
#include <optional>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

namespace themis {
namespace utils {

// ============================================================================
// Custom Deleters for OpenSSL Objects
// ============================================================================

/// Deleter for EVP_PKEY
struct EVPKeyDeleter {
    void operator()(EVP_PKEY* pkey) const noexcept {
        if (pkey) EVP_PKEY_free(pkey);
    }
};

/// Deleter for EVP_MD_CTX
struct EVPMDCtxDeleter {
    void operator()(EVP_MD_CTX* ctx) const noexcept {
        if (ctx) EVP_MD_CTX_free(ctx);
    }
};

/// Deleter for X509
struct X509Deleter {
    void operator()(X509* cert) const noexcept {
        if (cert) X509_free(cert);
    }
};

/// Deleter for X509_CRL
struct X509CRLDeleter {
    void operator()(X509_CRL* crl) const noexcept {
        if (crl) X509_CRL_free(crl);
    }
};

/// Deleter for X509_REQ (PKCS#10 Certificate Signing Request)
struct X509REQDeleter {
    void operator()(X509_REQ* req) const noexcept {
        if (req) X509_REQ_free(req);
    }
};

/// Deleter for X509_STORE
struct X509StoreDeleter {
    void operator()(X509_STORE* store) const noexcept {
        if (store) X509_STORE_free(store);
    }
};

/// Deleter for X509_STORE_CTX
struct X509StoreCtxDeleter {
    void operator()(X509_STORE_CTX* ctx) const noexcept {
        if (ctx) X509_STORE_CTX_free(ctx);
    }
};

/// Deleter for BIO
struct BIODeleter {
    void operator()(BIO* bio) const noexcept {
        if (bio) BIO_free_all(bio);
    }
};

/// Deleter for RSA
struct RSADeleter {
    void operator()(RSA* rsa) const noexcept {
        if (rsa) RSA_free(rsa);
    }
};

/// Deleter for BIGNUM
struct BIGNUMDeleter {
    void operator()(BIGNUM* bn) const noexcept {
        if (bn) BN_free(bn);
    }
};

// ============================================================================
// RAII Wrappers using unique_ptr
// ============================================================================

/// RAII wrapper for EVP_PKEY
using EVPKeyPtr = std::unique_ptr<EVP_PKEY, EVPKeyDeleter>;

/// RAII wrapper for EVP_MD_CTX
using EVPMDCtxPtr = std::unique_ptr<EVP_MD_CTX, EVPMDCtxDeleter>;

/// RAII wrapper for X509
using X509Ptr = std::unique_ptr<X509, X509Deleter>;

/// RAII wrapper for X509_CRL
using X509CRLPtr = std::unique_ptr<X509_CRL, X509CRLDeleter>;

/// RAII wrapper for X509_REQ (PKCS#10 Certificate Signing Request)
using X509REQPtr = std::unique_ptr<X509_REQ, X509REQDeleter>;

/// RAII wrapper for X509_STORE
using X509StorePtr = std::unique_ptr<X509_STORE, X509StoreDeleter>;

/// RAII wrapper for X509_STORE_CTX
using X509StoreCtxPtr = std::unique_ptr<X509_STORE_CTX, X509StoreCtxDeleter>;

/// RAII wrapper for BIO
using BIOPtr = std::unique_ptr<BIO, BIODeleter>;

/// RAII wrapper for RSA
using RSAPtr = std::unique_ptr<RSA, RSADeleter>;

/// RAII wrapper for BIGNUM
using BIGNUMPtr = std::unique_ptr<BIGNUM, BIGNUMDeleter>;

// ============================================================================
// Helper Functions
// ============================================================================

/// Create EVP_PKEY with RAII
inline EVPKeyPtr make_evp_key() noexcept {
    return EVPKeyPtr(EVP_PKEY_new());
}

/// Create EVP_MD_CTX with RAII
inline EVPMDCtxPtr make_evp_md_ctx() noexcept {
    return EVPMDCtxPtr(EVP_MD_CTX_new());
}

/// Create RSA with RAII
inline RSAPtr make_rsa() noexcept {
    return RSAPtr(RSA_new());
}

/// Create BIGNUM with RAII
inline BIGNUMPtr make_bignum() noexcept {
    return BIGNUMPtr(BN_new());
}

/// Create BIO_mem_buf with RAII
inline BIOPtr make_bio_mem_buf(const void* data, int size) noexcept {
    return BIOPtr(BIO_new_mem_buf(data, size));
}

/// Create BIO_file with RAII
inline BIOPtr make_bio_file(const char* filename, const char* mode) noexcept {
    return BIOPtr(BIO_new_file(filename, mode));
}

/// Load X509 from BIO with RAII
inline X509Ptr read_x509_from_bio(BIO* bio) noexcept {
    if (!bio) return X509Ptr(nullptr);
    return X509Ptr(PEM_read_bio_X509(bio, nullptr, nullptr, nullptr));
}

/// Load X509_CRL from BIO with RAII
inline X509CRLPtr read_x509_crl_from_bio(BIO* bio) noexcept {
    if (!bio) return X509CRLPtr(nullptr);
    return X509CRLPtr(PEM_read_bio_X509_CRL(bio, nullptr, nullptr, nullptr));
}

} // namespace utils
} // namespace themis
