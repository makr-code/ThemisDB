/*
 * ThemisDB | File: fips_crypto_mode.cpp | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 247
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=39 | delta=36 | status=divergent
 * External Severity (v3): C=6, H=31, M=2
 * PR: #3389 feat(security): implement FIPS 140-2/3 validated cryptography mode (2026-03-12T07:08:28Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file fips_crypto_mode.cpp
 * @brief Implementation of FipsCryptoMode — FIPS 140-2/3 validated cryptography mode.
 *
 * Uses the OpenSSL 3.x provider API:
 *   - OSSL_PROVIDER_load()                to load the "fips" provider module
 *   - EVP_default_properties_enable_fips() to restrict all EVP operations to
 *                                          FIPS-approved implementations
 *   - EVP_default_properties_is_fips_enabled() to query active state
 *   - OSSL_PROVIDER_self_test()           to trigger provider self-tests
 *   - OPENSSL_cleanse()                   for secure zeroization
 *
 * Graceful degradation: if the FIPS provider shared object is not installed
 * (OSSL_PROVIDER_load returns nullptr), enable() returns false and all
 * subsequent operations continue in non-FIPS mode.  The system FIPS policy
 * is not violated because the validation gate sits in FipsCryptoMode itself.
 */

#include "security/fips_crypto_mode.h"
#include "utils/logger.h"

#include <openssl/evp.h>
#include <openssl/provider.h>
#include <openssl/crypto.h>

#include <algorithm>
#include <cctype>
#include <mutex>

namespace themis {

// ---------------------------------------------------------------------------
// Approved algorithm set (NIST SP 800-175B rev.1 / FIPS 140-3)
// ---------------------------------------------------------------------------

static const std::unordered_set<std::string> kFipsApprovedAlgorithms = {
    // Symmetric — AES block cipher modes
    "AES-128-CBC", "AES-192-CBC", "AES-256-CBC",
    "AES-128-CTR", "AES-192-CTR", "AES-256-CTR",
    "AES-128-GCM", "AES-192-GCM", "AES-256-GCM",
    "AES-128-CCM", "AES-256-CCM",
    "AES-128-XTS", "AES-256-XTS",
    "AES-128-KW",  "AES-256-KW",   // AES Key Wrap (RFC 3394)
    "AES-128-ECB", "AES-192-ECB",  "AES-256-ECB",  // ECB permitted for internal key transport only

    // Hash functions
    "SHA-224", "SHA-256", "SHA-384", "SHA-512",
    "SHA-512/224", "SHA-512/256",
    "SHA3-224", "SHA3-256", "SHA3-384", "SHA3-512",

    // MAC
    "HMAC-SHA-224", "HMAC-SHA-256", "HMAC-SHA-384", "HMAC-SHA-512",
    "CMAC-AES-128", "CMAC-AES-256",

    // Asymmetric / signatures
    "RSA-2048", "RSA-3072", "RSA-4096",
    "ECDSA-P256", "ECDSA-P384", "ECDSA-P521",
    "ECDH-P256",  "ECDH-P384",  "ECDH-P521",
    "DH-2048",    "DH-3072",    "DH-4096",
    "ED25519",    "ED448",      // FIPS 186-5 (approved in FIPS 140-3 modules)

    // Key derivation functions
    "PBKDF2-SHA-256", "PBKDF2-SHA-384", "PBKDF2-SHA-512",
    "HKDF-SHA-256",   "HKDF-SHA-384",   "HKDF-SHA-512",
    "SP800-108-CTR", "SP800-108-FEEDBACK", "SP800-108-PIPELINE",
    "TLS1-PRF-SHA-256", "TLS1-PRF-SHA-384", "TLS1-PRF-SHA-512",

    // Deterministic RNGs
    "CTR_DRBG",  "HASH_DRBG",  "HMAC_DRBG",
};

// ---------------------------------------------------------------------------
// Pimpl body
// ---------------------------------------------------------------------------

struct FipsCryptoMode::Impl {
    mutable std::mutex mtx;
    OSSL_PROVIDER* fips_provider = nullptr;
    bool fips_active              = false;

    Impl() = default;

    ~Impl() {
        if (fips_provider) {
            // Best-effort — errors at shutdown are not actionable.
            (void)EVP_default_properties_enable_fips(nullptr, 0);
            OSSL_PROVIDER_unload(fips_provider);
            fips_provider = nullptr;
            fips_active   = false;
        }
    }
};

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

FipsCryptoMode& FipsCryptoMode::instance() {
    static FipsCryptoMode inst;
    return inst;
}

FipsCryptoMode::FipsCryptoMode() : impl_(new Impl()) {}

FipsCryptoMode::~FipsCryptoMode() {
    delete impl_;
}

// ---------------------------------------------------------------------------
// enable / disable
// ---------------------------------------------------------------------------

bool FipsCryptoMode::enable() {
    std::lock_guard<std::mutex> lock(impl_->mtx);

    if (impl_->fips_active) {
        THEMIS_INFO("FipsCryptoMode::enable: FIPS mode already active");
        return true;
    }

    // Attempt to load the FIPS provider.
    OSSL_PROVIDER* prov = OSSL_PROVIDER_load(nullptr, "fips");
    if (!prov) {
        THEMIS_WARN("FipsCryptoMode::enable: FIPS provider not available "
                    "(install a FIPS-validated OpenSSL build). "
                    "Running in non-FIPS mode.");
        return false;
    }

    // Restrict all EVP algorithm lookups to FIPS-approved implementations.
    if (EVP_default_properties_enable_fips(nullptr, 1) != 1) {
        OSSL_PROVIDER_unload(prov);
        throw std::runtime_error(
            "FipsCryptoMode::enable: EVP_default_properties_enable_fips failed");
    }

    impl_->fips_provider = prov;
    impl_->fips_active   = true;

    THEMIS_INFO("FipsCryptoMode::enable: FIPS 140-2/3 validated cryptography mode ACTIVE");
    return true;
}

void FipsCryptoMode::disable() {
    std::lock_guard<std::mutex> lock(impl_->mtx);

    if (!impl_->fips_active) {
        return;
    }

    (void)EVP_default_properties_enable_fips(nullptr, 0);

    if (impl_->fips_provider) {
        OSSL_PROVIDER_unload(impl_->fips_provider);
        impl_->fips_provider = nullptr;
    }

    impl_->fips_active = false;
    THEMIS_INFO("FipsCryptoMode::disable: FIPS mode deactivated");
}

// ---------------------------------------------------------------------------
// isEnabled / isAvailable
// ---------------------------------------------------------------------------

bool FipsCryptoMode::isEnabled() const {
    // Cross-check our internal flag against the OpenSSL property setting so
    // that external code that directly calls EVP_default_properties_enable_fips
    // is also reflected here.
    return impl_->fips_active &&
           EVP_default_properties_is_fips_enabled(nullptr) == 1;
}

bool FipsCryptoMode::isAvailable() const {
    // Probe without permanently loading the provider.
    OSSL_PROVIDER* prov = OSSL_PROVIDER_try_load(nullptr, "fips", 1 /*retain_fallbacks*/);
    if (!prov) {
        return false;
    }
    OSSL_PROVIDER_unload(prov);
    return true;
}

// ---------------------------------------------------------------------------
// Algorithm validation
// ---------------------------------------------------------------------------

static std::string toUpper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

void FipsCryptoMode::validateAlgorithm(const std::string& algorithm) const {
    const std::string upper = toUpper(algorithm);
    if (kFipsApprovedAlgorithms.find(upper) == kFipsApprovedAlgorithms.end()) {
        throw FipsPolicyViolation(
            "algorithm '" + algorithm + "' is not on the FIPS 140-2/3 approved list");
    }
}

const std::unordered_set<std::string>& FipsCryptoMode::approvedAlgorithms() const {
    return kFipsApprovedAlgorithms;
}

// ---------------------------------------------------------------------------
// Self-tests
// ---------------------------------------------------------------------------

bool FipsCryptoMode::runSelfTests() const {
    std::lock_guard<std::mutex> lock(impl_->mtx);

    if (!impl_->fips_active || !impl_->fips_provider) {
        THEMIS_WARN("FipsCryptoMode::runSelfTests: FIPS mode not active — skipping");
        return false;
    }

    int rc = OSSL_PROVIDER_self_test(impl_->fips_provider);
    if (rc != 1) {
        THEMIS_ERROR("FipsCryptoMode::runSelfTests: FIPS provider self-tests FAILED");
        return false;
    }

    THEMIS_INFO("FipsCryptoMode::runSelfTests: FIPS provider self-tests passed");
    return true;
}

// ---------------------------------------------------------------------------
// Zeroization
// ---------------------------------------------------------------------------

void FipsCryptoMode::zeroize(void* ptr, std::size_t len) noexcept {
    if (ptr && len > 0) {
        OPENSSL_cleanse(ptr, len);
    }
}

}  // namespace themis
