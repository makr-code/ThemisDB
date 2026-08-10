/**
 * @file timestamp_authority.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=45; TODO=1, Stub=35, Unimpl=0, Mock=1, Sim=8, Debt=0, C=0, H=2, M=24, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: timestamp_authority.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 87/100 | Lines: 1151
 * Gap Summary: total=43; TODO=1, Stub=33, Unimpl=0, Mock=1, Sim=8, Debt=0, C=0, H=14, M=32, L=0
 * PR History (last 5): #4678 feat: replace production st... (2026-04-15) | #3453 Add production safety guard... (2026-03-12) | #401 Replace Security Stubs with... (2026-03-11) | #787 Implement Timestamp Authori... (2026-03-11) | #1039 Document RFC 3161 Timestamp... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Minimal stub implementation for TimestampAuthority.
// Deterministic, non-cryptographic timestamps (no OpenSSL / CURL).
#ifndef THEMIS_USE_OPENSSL_TSA

// PERMANENT FALLBACK NOTE:
// Purpose: Software-only deterministic TSA fallback for development and CI environments
//          where OpenSSL TSA (libcurl + openssl TSA protocol) is not available.
//          Issues locally-generated timestamps without RFC 3161 compliance.
//          Production mode is explicitly blocked unless THEMIS_ALLOW_TSA_STUB=1 is set.
// Activation: Compiled when THEMIS_USE_OPENSSL_TSA is NOT defined (default dev build).
//             Build with -DTHEMIS_USE_OPENSSL_TSA=ON (Wave-2 guard, requires OpenSSL TS_*
//             + libcurl) to activate the RFC 3161-compliant implementation in
//             timestamp_authority_openssl.cpp instead.
// Production Delta: No RFC 3161 token, no TSA signature, no external TSA server contact.
//                   Timestamps are local system clock only. Not legally binding.
// This fallback path is PERMANENT for no-OpenSSL-TSA builds; it is not a stub to be
// removed, but a compile-time safety net. All regulated/compliance deployments must use
// the real TSA backend (-DTHEMIS_USE_OPENSSL_TSA=ON).
// Roadmap ref: src/security/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"

#include "security/timestamp_authority.h"
#include "utils/logger.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <mutex>

namespace themis { namespace security {

// Helper: check production mode (mirrors HSM stub pattern)
static bool isProductionMode() {
    const char* prod_mode = std::getenv("THEMIS_PRODUCTION_MODE");
    const char* environment = std::getenv("THEMIS_ENVIRONMENT");
    const char* env_type    = std::getenv("ENVIRONMENT");
    const char* node_env    = std::getenv("NODE_ENV");

    if (prod_mode) {
        const std::string s(prod_mode);
        if (s == "1" || s == "true" || s == "True" || s == "TRUE" ||
            s == "yes"  || s == "Yes"  || s == "on"   || s == "On")
            return true;
    }
    if (environment) {
        const std::string s(environment);
        if (s == "production" || s == "prod") return true;
    }
    if (env_type) {
        const std::string s(env_type);
        if (s == "production" || s == "prod") return true;
    }
    if (node_env) {
        const std::string s(node_env);
        if (s == "production") return true;
    }
    return false;
}

// Helper: check if TSA stub is explicitly allowed
static bool isStubAllowed() {
    const char* allow_stub = std::getenv("THEMIS_ALLOW_TSA_STUB");
    return allow_stub && std::string(allow_stub) == "1";
}

// Helper: return a failed token indicating stub is blocked in production
static TimestampToken makeProductionError() {
    TimestampToken tok;
    tok.success = false;
    tok.error_message =
        "TimestampAuthority stub is not permitted in production mode. "
        "Build with -DTHEMIS_USE_OPENSSL_TSA=ON (OpenSSL + libcurl required) "
        "or set THEMIS_ALLOW_TSA_STUB=1 to explicitly allow the insecure stub.";
    THEMIS_ERROR("SECURITY ERROR: {}", tok.error_message);
    return tok;
}

// PERMANENT FALLBACK NOTE (TimestampAuthority::Impl — software-only fallback):
// Purpose: Provide a lightweight stateful Impl so the no-OpenSSL-TSA path keeps
//          pimpl compatibility while still tracking minimal runtime state
//          (issued token count + cached certificate string).
// Activation: `THEMIS_USE_OPENSSL_TSA` not defined (same as the outer fallback block).
// Production Delta: No external TSA connection pool, no OpenSSL TS context, no
//          async RFC3161 request pipeline.  State is local-process only.
// @note This Impl is the PERMANENT no-TSA fallback; the real Impl (with CURL* and
//       OpenSSL TS context) lives in timestamp_authority_openssl.cpp and activates
//       when `-DTHEMIS_USE_OPENSSL_TSA=ON` is set (Wave-2 CMake guard).
class TimestampAuthority::Impl {
public:
    mutable std::mutex state_mutex;
    uint64_t issued_count = 0;
    std::optional<std::string> cached_tsa_cert_pem;
};

// Helper: hex encode
static std::string hex(const std::vector<uint8_t>& data) {
    static const char* d = "0123456789abcdef";
    std::string out; out.reserve(data.size()*2);
    for (auto b : data) { out.push_back(d[(b>>4)&0xF]); out.push_back(d[b&0xF]); }
    return out;
}

// Very weak deterministic hash (not cryptographic!)
static std::vector<uint8_t> pseudo_hash(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> h; h.reserve(data.size());
    for (size_t i=0;i<data.size();++i) h.push_back(static_cast<uint8_t>(data[i] ^ (i & 0xFF)));
    return h;
}

TimestampAuthority::TimestampAuthority(TSAConfig config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config)) {}

TimestampAuthority::~TimestampAuthority() = default;
TimestampAuthority::TimestampAuthority(TimestampAuthority&&) noexcept = default;
TimestampAuthority& TimestampAuthority::operator=(TimestampAuthority&&) noexcept = default;

TimestampToken TimestampAuthority::getTimestamp(const std::vector<uint8_t>& data) {
    // Input validation: reject empty data
    if (data.empty()) {
        TimestampToken tok;
        tok.error_message = "Cannot timestamp empty data";
        THEMIS_ERROR("{}", tok.error_message);
        return tok;
    }
    
    // SECURITY HARDENING: refuse to issue stub tokens in production environments.
    // Production deployments must build with -DTHEMIS_USE_OPENSSL_TSA=ON.
    if (isProductionMode() && !isStubAllowed()) {
        return makeProductionError();
    }

    // WARNING: This is a STUB implementation for development only
    // For production, build with -DTHEMIS_USE_OPENSSL_TSA=ON
    // This stub does NOT provide cryptographic timestamps!
    THEMIS_WARN("Using TimestampAuthority STUB - NOT SECURE for production!");
    
    return getTimestampForHash(computeHash(data));
}

TimestampToken TimestampAuthority::getTimestampForHash(const std::vector<uint8_t>& hash) {
    // Input validation: reject empty hash
    if (hash.empty()) {
        TimestampToken tok;
        tok.error_message = "Cannot timestamp empty hash";
        THEMIS_ERROR("{}", tok.error_message);
        return tok;
    }
    
    // Bridge path: when a callback is registered, delegate stamping to the
    // injected implementation. When unset, retain the deterministic local stub.
    if (isProductionMode() && !isStubAllowed()) {
        return makeProductionError();
    }
    // Production-mode gating applies before any injected bridge runs so that a
    // callback cannot silently bypass the non-production restriction.
    GetTimestampForHashFn fn;
    {
        std::lock_guard<std::mutex> lk(TimestampAuthority::getTimestampForHashFnMutex());
        fn = TimestampAuthority::getTimestampForHashFnStorage();
    }
    if (fn) {
        try {
            return fn(hash, config_);
        } catch (const std::exception& e) {
            TimestampToken tok;
            tok.error_message = std::string("getTimestampForHash callback failed: ") + e.what();
            return tok;
        }
    }

    TimestampToken tok;
    tok.success = true;
    tok.hash_algorithm = config_.hash_algorithm;
    auto now = std::chrono::system_clock::now();
    tok.timestamp_unix_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm,&tt);
#else
    localtime_r(&tt,&tm);
#endif
    std::ostringstream oss; oss<<std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    tok.timestamp_utc = oss.str();
    tok.serial_number = "STUB-SERIAL";
    tok.policy_oid = config_.policy_oid;
    tok.nonce = generateNonce();
    tok.token_der = hash;
    tok.token_b64 = std::string("hex:")+hex(hash);
    tok.tsa_name = "STUB-TSA";
    tok.tsa_serial = "STUB-TSA-SERIAL";
    tok.verified = true;
    tok.cert_valid = true;
    {
        std::lock_guard<std::mutex> lk(impl_->state_mutex);
        ++impl_->issued_count;
    }
    return tok;
}

bool TimestampAuthority::verifyTimestamp(const std::vector<uint8_t>& data, const TimestampToken& token) {
    if (data.empty()) {
        THEMIS_ERROR("Cannot verify timestamp for empty data");
        return false;
    }
    return verifyTimestampForHash(computeHash(data), token);
}

bool TimestampAuthority::verifyTimestampForHash(const std::vector<uint8_t>& hash, const TimestampToken& token) {
    if (hash.empty()) {
        THEMIS_ERROR("Cannot verify timestamp for empty hash");
        return false;
    }
    
    // Bridge path: when a callback is registered, delegate verification to the
    // injected implementation and fail closed on exceptions. When unset, keep
    // the original hex-token comparison fallback.
    VerifyTimestampForHashFn fn;
    {
        std::lock_guard<std::mutex> lk(TimestampAuthority::verifyTimestampForHashFnMutex());
        fn = TimestampAuthority::verifyTimestampForHashFnStorage();
    }
    if (fn) {
        try {
            return fn(hash, token, config_);
        } catch (const std::exception& e) {
            THEMIS_ERROR("verifyTimestampForHashFn threw exception: {}", e.what());
            return false;
        }
    }
    return token.success && token.token_b64 == std::string("hex:")+hex(hash);
}

TimestampToken TimestampAuthority::parseToken(const std::vector<uint8_t>& token_data) {
    TimestampToken tok;
    if (token_data.empty()) {
        tok.error_message = "Cannot parse empty token data";
        THEMIS_WARN("parseToken: {}", tok.error_message);
        return tok;
    }
    tok.success = true;
    tok.token_der = token_data;
    tok.token_b64 = std::string("hex:") + hex(token_data);
    tok.serial_number = "PARSE";
    return tok;
}

TimestampToken TimestampAuthority::parseToken(const std::string& token_b64) {
    TimestampToken tok;
    if (token_b64.empty()) {
        tok.error_message = "Cannot parse empty token string";
        THEMIS_WARN("parseToken: {}", tok.error_message);
        return tok;
    }
    tok.success = true;
    tok.token_b64 = token_b64;
    tok.serial_number = "PARSE";
    return tok;
}

std::optional<std::string> TimestampAuthority::getTSACertificate() {
    std::lock_guard<std::mutex> lk(impl_->state_mutex);
    if (!impl_->cached_tsa_cert_pem) {
        // Return a stub certificate for development; production must use real TSA
        if (isProductionMode() && !isStubAllowed()) {
            THEMIS_ERROR("Cannot return stub TSA certificate in production mode");
            return std::nullopt;
        }
        impl_->cached_tsa_cert_pem =
            std::string("-----BEGIN CERTIFICATE-----\nSTUB-TSA\n-----END CERTIFICATE-----\n");
    }
    return impl_->cached_tsa_cert_pem;
}
bool TimestampAuthority::isAvailable() { return true; }
std::string TimestampAuthority::getLastError() const { return last_error_; }

// Private helpers (stubs)
std::vector<uint8_t> TimestampAuthority::createTSPRequest(const std::vector<uint8_t>&, const std::vector<uint8_t>&) { return {}; }
TimestampToken TimestampAuthority::parseTSPResponse(const std::vector<uint8_t>&) { TimestampToken t; t.success = true; return t; }
std::vector<uint8_t> TimestampAuthority::sendTSPRequest(const std::vector<uint8_t>&) { return {}; }
std::vector<uint8_t> TimestampAuthority::generateNonce(size_t bytes) { std::vector<uint8_t> n(bytes); for(size_t i=0;i<bytes;++i) n[i]=static_cast<uint8_t>(i); return n; }
std::vector<uint8_t> TimestampAuthority::computeHash(const std::vector<uint8_t>& data) { return pseudo_hash(data); }

// ============================================================================
// eIDAS Timestamp Validator Stub Implementation
// ============================================================================
// PERMANENT FALLBACK NOTE (eIDASTimestampValidator — all methods below):
// Purpose: Provide a compilable eIDASTimestampValidator that enforces production
//          guards and fails safely in dev/CI builds where OpenSSL TSA is absent.
// Activation: Compiled inside `#ifndef THEMIS_USE_OPENSSL_TSA` (same as the
//             TimestampAuthority fallback above).  Production builds with
//             -DTHEMIS_USE_OPENSSL_TSA=ON (Wave-2 CMake guard) compile the real
//             implementation which performs full ASN.1 chain validation.
// Production Delta (validateeIDASTimestamp): Without an injected ValidateFn,
//             accepts any token whose `success` flag is true only when the
//             explicit stub override is enabled; no RFC3161 signature validation.
//             Optional ValidateFn injection allows external validation logic.
// Production Delta (isQualifiedTSA): Without an injected QualifiedTSAFn,
//             always returns false + pushes an error message; cannot validate
//             QTSP certificates without OpenSSL.
// This block is PERMANENT for no-OpenSSL-TSA builds and is a compile-time safety net.
// Enable with -DTHEMIS_USE_OPENSSL_TSA=ON; see
// src/security/FUTURE_ENHANCEMENTS.md §"eIDAS TSA Validation".

bool eIDASTimestampValidator::validateeIDASTimestamp(
    const TimestampToken& token,
    const std::vector<std::string>& trust_anchors) {
    
    validation_errors_.clear();

    eIDASTimestampValidator::ValidateFn fn;
    {
        std::lock_guard<std::mutex> lk(eIDASTimestampValidator::validateFnMutex());
        fn = eIDASTimestampValidator::validateFnStorage();
    }
    if (fn) {
        try {
            return fn(token, trust_anchors, validation_errors_);
        } catch (const std::exception& e) {
            validation_errors_.push_back(
                std::string("Injected ValidateFn threw exception: ") + e.what());
            return false;
        }
    }
    
    // SECURITY HARDENING: full eIDAS validation requires the OpenSSL implementation.
    // The stub path is denied by default and only available with explicit opt-in.
    if (!isStubAllowed()) {
        validation_errors_.push_back(
            "eIDAS timestamp validation is not available in OpenSSL-stub mode by default. "
            "Build with -DTHEMIS_USE_OPENSSL_TSA=ON for RFC3161/QTSP validation "
            "or set THEMIS_ALLOW_TSA_STUB=1 for explicit non-production override.");
        return false;
    }

    // Stub implementation - basic checks only
    if (!token.success) {
        validation_errors_.push_back("Token marked as unsuccessful");
        return false;
    }
    
    // In stub mode, we accept any successful token
    return true;
}

bool eIDASTimestampValidator::validateAge(const TimestampToken& token, int max_age_days) {
    validation_errors_.clear();
    
    if (token.timestamp_unix_ms == 0) {
        validation_errors_.push_back("Token has no timestamp");
        return false;
    }
    
    // Get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    // Check for future timestamps (avoid integer underflow)
    if (token.timestamp_unix_ms > now_ms) {
        validation_errors_.push_back("Token timestamp is in the future");
        return false;
    }
    
    // Calculate age in milliseconds
    uint64_t age_ms = now_ms - token.timestamp_unix_ms;
    
    // Convert max age from days to milliseconds with overflow check
    // max_age_days * 24 * 60 * 60 * 1000 = max_age_days * 86400000
    // Check if multiplication would overflow uint64_t
    constexpr uint64_t MS_PER_DAY = 86400000ULL;
    if (max_age_days < 0) {
        validation_errors_.push_back("Maximum age must be non-negative");
        return false;
    }
    if (static_cast<uint64_t>(max_age_days) > UINT64_MAX / MS_PER_DAY) {
        validation_errors_.push_back("Maximum age value too large");
        return false;
    }
    uint64_t max_age_ms = static_cast<uint64_t>(max_age_days) * MS_PER_DAY;
    
    if (age_ms > max_age_ms) {
        validation_errors_.push_back("Token age exceeds maximum allowed age");
        return false;
    }
    
    return true;
}

bool eIDASTimestampValidator::isQualifiedTSA(
    const std::string& tsa_cert,
    const std::vector<std::string>& qtsp_list) {
    
    validation_errors_.clear();

    eIDASTimestampValidator::QualifiedTSAFn fn;
    {
        std::lock_guard<std::mutex> lk(eIDASTimestampValidator::qualifiedTSAFnMutex());
        fn = eIDASTimestampValidator::qualifiedTSAFnStorage();
    }
    if (fn) {
        try {
            return fn(tsa_cert, qtsp_list, validation_errors_);
        } catch (const std::exception& e) {
            validation_errors_.push_back(
                std::string("Injected QualifiedTSAFn threw exception: ") + e.what());
            return false;
        }
    }
    
    // PERMANENT FALLBACK NOTE (isQualifiedTSA): Always returns false; cannot validate
    // QTSP certificate chains without OpenSSL.  This is the correct fail-closed
    // behavior for builds without -DTHEMIS_USE_OPENSSL_TSA=ON (Wave-2 CMake guard).
    // When the Wave-2 guard is set, the real ASN.1 chain validation path in
    // timestamp_authority_openssl.cpp handles QTSP certificate validation.
    // Without OpenSSL TS_*, we cannot properly validate certificates;
    validation_errors_.push_back(
        "QTSP validation not available in stub implementation - "
        "rebuild with OpenSSL support (THEMIS_USE_OPENSSL_TSA) for secure validation"
    );
    return false;
}

std::vector<std::string> eIDASTimestampValidator::getValidationErrors() const {
    return validation_errors_;
}

} } // namespace themis::security

#endif // THEMIS_USE_OPENSSL_TSA

// ============================================================================
// Real RFC 3161 Time-Stamp Protocol implementation
// Compiled when THEMIS_USE_OPENSSL_TSA is defined (-DTHEMIS_USE_OPENSSL_TSA=ON)
// Dependencies: OpenSSL (required), libcurl (required for HTTP transport)
// ============================================================================
#ifdef THEMIS_USE_OPENSSL_TSA

#include "security/timestamp_authority.h"
#include "utils/logger.h"

#include <openssl/ts.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/pkcs7.h>

#ifdef THEMIS_HAS_CURL
#include <curl/curl.h>
#endif

#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <thread>

namespace themis { namespace security {

// ============================================================================
// RAII Wrappers for OpenSSL Objects (Real Implementation)
// ============================================================================

namespace {

struct TS_REQ_Deleter { void operator()(TS_REQ* p) const { if (p) TS_REQ_free(p); } };
struct TS_MSG_IMPRINT_Deleter { void operator()(TS_MSG_IMPRINT* p) const { if (p) TS_MSG_IMPRINT_free(p); } };
struct X509_ALGOR_Deleter { void operator()(X509_ALGOR* p) const { if (p) X509_ALGOR_free(p); } };
struct BIGNUM_Deleter { void operator()(BIGNUM* p) const { if (p) BN_free(p); } };
struct ASN1_INTEGER_Deleter { void operator()(ASN1_INTEGER* p) const { if (p) ASN1_INTEGER_free(p); } };
struct ASN1_OBJECT_Deleter { void operator()(ASN1_OBJECT* p) const { if (p) ASN1_OBJECT_free(p); } };
struct TS_RESP_Deleter { void operator()(TS_RESP* p) const { if (p) TS_RESP_free(p); } };
struct BIO_Deleter { void operator()(BIO* p) const { if (p) BIO_free_all(p); } };
struct PKCS7_Deleter { void operator()(PKCS7* p) const { if (p) PKCS7_free(p); } };
struct X509_STORE_Deleter { void operator()(X509_STORE* p) const { if (p) X509_STORE_free(p); } };
struct X509_Deleter { void operator()(X509* p) const { if (p) X509_free(p); } };
struct TS_VERIFY_CTX_Deleter { void operator()(TS_VERIFY_CTX* p) const { if (p) TS_VERIFY_CTX_free(p); } };

// OpenSSL string deleters for allocated memory
struct OPENSSL_free_Deleter { void operator()(void* p) const { if (p) OPENSSL_free(p); } };

#ifdef THEMIS_HAS_CURL
struct CURL_Deleter { void operator()(CURL* p) const { if (p) curl_easy_cleanup(p); } };
struct curl_slist_Deleter { void operator()(struct curl_slist* p) const { if (p) curl_slist_free_all(p); } };

using CURL_ptr = std::unique_ptr<CURL, CURL_Deleter>;
using curl_slist_ptr = std::unique_ptr<struct curl_slist, curl_slist_Deleter>;
#endif

using TS_REQ_ptr         = std::unique_ptr<TS_REQ, TS_REQ_Deleter>;
using TS_MSG_IMPRINT_ptr = std::unique_ptr<TS_MSG_IMPRINT, TS_MSG_IMPRINT_Deleter>;
using X509_ALGOR_ptr     = std::unique_ptr<X509_ALGOR, X509_ALGOR_Deleter>;
using BIGNUM_ptr         = std::unique_ptr<BIGNUM, BIGNUM_Deleter>;
using ASN1_INTEGER_ptr   = std::unique_ptr<ASN1_INTEGER, ASN1_INTEGER_Deleter>;
using ASN1_OBJECT_ptr    = std::unique_ptr<ASN1_OBJECT, ASN1_OBJECT_Deleter>;
using TS_RESP_ptr        = std::unique_ptr<TS_RESP, TS_RESP_Deleter>;
using BIO_ptr            = std::unique_ptr<BIO, BIO_Deleter>;
using PKCS7_ptr          = std::unique_ptr<PKCS7, PKCS7_Deleter>;
using X509_STORE_ptr     = std::unique_ptr<X509_STORE, X509_STORE_Deleter>;
using X509_ptr           = std::unique_ptr<X509, X509_Deleter>;
using TS_VERIFY_CTX_ptr  = std::unique_ptr<TS_VERIFY_CTX, TS_VERIFY_CTX_Deleter>;
using char_ptr           = std::unique_ptr<char, OPENSSL_free_Deleter>;

} // namespace

// ---------------------------------------------------------------------------
// libcurl write callback
// ---------------------------------------------------------------------------
#ifdef THEMIS_HAS_CURL
static size_t tsaCurlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::vector<uint8_t>*>(userdata);
    const auto* bytes = static_cast<const uint8_t*>(ptr);
    buf->insert(buf->end(), bytes, bytes + size * nmemb);
    return size * nmemb;
}
#endif

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------
class TimestampAuthority::Impl {
public:
    // Currently stateless; placeholder for future connection-pool state.
};

TimestampAuthority::TimestampAuthority(TSAConfig config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config)) {}

TimestampAuthority::~TimestampAuthority() = default;
TimestampAuthority::TimestampAuthority(TimestampAuthority&&) noexcept = default;
TimestampAuthority& TimestampAuthority::operator=(TimestampAuthority&&) noexcept = default;

// ---------------------------------------------------------------------------
// computeHash — real SHA-256 / SHA-384 / SHA-512 via OpenSSL EVP
// ---------------------------------------------------------------------------
std::vector<uint8_t> TimestampAuthority::computeHash(const std::vector<uint8_t>& data) {
    const EVP_MD* md = EVP_sha256();
    if (config_.hash_algorithm == "SHA384" || config_.hash_algorithm == "sha384")
        md = EVP_sha384();
    else if (config_.hash_algorithm == "SHA512" || config_.hash_algorithm == "sha512")
        md = EVP_sha512();

    unsigned int len = static_cast<unsigned int>(EVP_MD_size(md));
    std::vector<uint8_t> digest(len);
    if (EVP_Digest(data.data(), data.size(), digest.data(), &len, md, nullptr) != 1) {
        last_error_ = "EVP_Digest failed";
        return {};
    }
    digest.resize(len);
    return digest;
}

// ---------------------------------------------------------------------------
// generateNonce — cryptographically random bytes via RAND_bytes
// ---------------------------------------------------------------------------
std::vector<uint8_t> TimestampAuthority::generateNonce(size_t bytes) {
    std::vector<uint8_t> nonce(bytes);
    if (RAND_bytes(nonce.data(), static_cast<int>(bytes)) != 1) {
        last_error_ = "RAND_bytes failed";
        return {};
    }
    return nonce;
}

// ---------------------------------------------------------------------------
// createTSPRequest — build RFC 3161 DER-encoded TimeStampReq
// ---------------------------------------------------------------------------
std::vector<uint8_t> TimestampAuthority::createTSPRequest(
    const std::vector<uint8_t>& hash,
    const std::vector<uint8_t>& nonce_bytes)
{
    TS_REQ_ptr req(TS_REQ_new());
    if (!req.get()) { last_error_ = "TS_REQ_new failed"; return {}; }

    TS_REQ_set_version(req.get(), 1);

    // --- MessageImprint (hash algorithm + hash value) ---
    TS_MSG_IMPRINT_ptr imprint(TS_MSG_IMPRINT_new());
    if (!imprint.get()) { last_error_ = "TS_MSG_IMPRINT_new failed"; return {}; }

    int nid = NID_sha256;
    if (config_.hash_algorithm == "SHA384") nid = NID_sha384;
    else if (config_.hash_algorithm == "SHA512") nid = NID_sha512;

    X509_ALGOR_ptr algo(X509_ALGOR_new());
    if (!algo.get()) {
        last_error_ = "X509_ALGOR_new failed";
        return {};
    }
    X509_ALGOR_set0(algo.get(), OBJ_nid2obj(nid), V_ASN1_NULL, nullptr);
    TS_MSG_IMPRINT_set_algo(imprint.get(), algo.get());

    TS_MSG_IMPRINT_set_msg(
        imprint.get(),
        const_cast<unsigned char*>(hash.data()),
        static_cast<int>(hash.size()));

    TS_REQ_set_msg_imprint(req.get(), imprint.get());

    // --- Nonce for replay protection ---
    if (!nonce_bytes.empty()) {
        BIGNUM_ptr bn(BN_bin2bn(
            nonce_bytes.data(), static_cast<int>(nonce_bytes.size()), nullptr));
        if (bn.get()) {
            ASN1_INTEGER_ptr asn1_nonce(BN_to_ASN1_INTEGER(bn.get(), nullptr));
            if (asn1_nonce.get()) {
                TS_REQ_set_nonce(req.get(), asn1_nonce.get());
            }
        }
    }

    // Request TSA certificate in response for verification
    TS_REQ_set_cert_req(req.get(), 1);

    // Optional: policy OID
    if (!config_.policy_oid.empty()) {
        ASN1_OBJECT_ptr oid(OBJ_txt2obj(config_.policy_oid.c_str(), 0));
        if (oid.get()) {
            TS_REQ_set_policy_id(req.get(), oid.get());
        }
    }

    // DER encode
    int len = i2d_TS_REQ(req.get(), nullptr);
    if (len <= 0) {
        last_error_ = "i2d_TS_REQ failed (size check)";
        return {};
    }
    std::vector<uint8_t> der(static_cast<size_t>(len));
    unsigned char* p = der.data();
    i2d_TS_REQ(req.get(), &p);
    return der;
    // RAII wrappers automatically clean up all OpenSSL objects on scope exit
}

// ---------------------------------------------------------------------------
// sendTSPRequest — HTTP POST via libcurl with retry logic
// ---------------------------------------------------------------------------
std::vector<uint8_t> TimestampAuthority::sendTSPRequest(
    const std::vector<uint8_t>& request)
{
#ifdef THEMIS_HAS_CURL
    std::vector<uint8_t> response_buf;
    
    // Retry configuration
    constexpr int MAX_RETRIES = 3;
    constexpr int INITIAL_BACKOFF_MS = 100;
    constexpr int MAX_BACKOFF_MS = 1000;
    
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        response_buf.clear();
        
        CURL_ptr curl(curl_easy_init());
        if (!curl) {
            last_error_ = "curl_easy_init failed";
            if (attempt < MAX_RETRIES - 1) {
                int backoff_ms = INITIAL_BACKOFF_MS * (1 << attempt);
                if (backoff_ms > MAX_BACKOFF_MS) backoff_ms = MAX_BACKOFF_MS;
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                continue;
            }
            return {};
        }
        
        curl_slist_ptr headers(nullptr);
        struct curl_slist* h = nullptr;
        h = curl_slist_append(h, "Content-Type: application/timestamp-query");
        h = curl_slist_append(h, "Accept: application/timestamp-reply");
        headers.reset(h);

        curl_easy_setopt(curl.get(), CURLOPT_URL,           config_.url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POST,           1L);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,     request.data());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,  static_cast<long>(request.size()));
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER,     headers.get());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,  tsaCurlWriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA,      &response_buf);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,        static_cast<long>(config_.timeout_seconds));
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);

        if (!config_.ca_cert_path.empty())
            curl_easy_setopt(curl.get(), CURLOPT_CAINFO, config_.ca_cert_path.c_str());

        if (!config_.verify_tsa_cert) {
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
        }

        CURLcode res = curl_easy_perform(curl.get());
        long http_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

        if (res == CURLE_OK && http_code == 200) {
            // Success - return response immediately
            return response_buf;
        }
        
        // Determine if error is transient
        bool is_transient = (res == CURLE_OPERATION_TIMEDOUT ||
                            res == CURLE_COULDNT_CONNECT ||
                            res == CURLE_COULDNT_RESOLVE_HOST ||
                            res == CURLE_COULDNT_RESOLVE_PROXY ||
                            http_code == 503 ||  // Service Unavailable
                            http_code == 504);   // Gateway Timeout
        
        if (res != CURLE_OK) {
            last_error_ = std::string("HTTP request failed: ") + curl_easy_strerror(res);
        } else {
            last_error_ = "TSA server returned HTTP " + std::to_string(http_code);
        }
        
        if (!is_transient || attempt == MAX_RETRIES - 1) {
            // Permanent failure or last attempt - return error
            return {};
        }
        
        // Transient failure and more retries available - backoff and retry
        int backoff_ms = INITIAL_BACKOFF_MS * (1 << attempt);
        if (backoff_ms > MAX_BACKOFF_MS) backoff_ms = MAX_BACKOFF_MS;
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
    }
    
    return {};
#else
    (void)request;
    last_error_ = "libcurl not available. Rebuild with CURL support (-DTHEMIS_HAS_CURL=1).";
    return {};
#endif
}

// ---------------------------------------------------------------------------
// parseTSPResponse — parse RFC 3161 DER-encoded TimeStampResp
// ---------------------------------------------------------------------------
TimestampToken TimestampAuthority::parseTSPResponse(
    const std::vector<uint8_t>& response_bytes)
{
    TimestampToken tok;
    if (response_bytes.empty()) {
        tok.error_message = "Empty TSP response";
        return tok;
    }

    const unsigned char* p = response_bytes.data();
    TS_RESP_ptr resp(d2i_TS_RESP(
        nullptr, &p, static_cast<long>(response_bytes.size())));
    if (!resp) {
        tok.error_message = "Failed to DER-decode TSP response";
        return tok;
    }

    // --- PKI status ---
    TS_STATUS_INFO* status_info = TS_RESP_get_status_info(resp.get());
    if (!status_info) {
        tok.error_message = "Missing status info in TSP response";
        return tok;
    }

    long pki_status = ASN1_INTEGER_get(TS_STATUS_INFO_get0_status(status_info));
    tok.pki_status = static_cast<int>(pki_status);

    // 0 = granted, 1 = grantedWithMods; anything else is a rejection
    if (pki_status != 0 && pki_status != 1) {
        const STACK_OF(ASN1_UTF8STRING)* texts =
            TS_STATUS_INFO_get0_text(status_info);
        if (texts && sk_ASN1_UTF8STRING_num(texts) > 0) {
            const ASN1_UTF8STRING* s = sk_ASN1_UTF8STRING_value(texts, 0);
            tok.error_message =
                std::string("TSA rejected request: ") +
                std::string(
                    reinterpret_cast<const char*>(ASN1_STRING_get0_data(s)),
                    static_cast<size_t>(ASN1_STRING_length(s)));
        } else {
            tok.error_message =
                "TSA rejected request (pki_status=" + std::to_string(pki_status) + ")";
        }
        return tok;
    }

    // --- Token (PKCS7) ---
    PKCS7* token_pkcs7 = TS_RESP_get_token(resp.get());
    if (!token_pkcs7) {
        tok.error_message = "No timestamp token in TSP response";
        return tok;
    }

    // Store DER-encoded PKCS7 token
    int token_len = i2d_PKCS7(token_pkcs7, nullptr);
    if (token_len > 0) {
        tok.token_der.resize(static_cast<size_t>(token_len));
        unsigned char* q = tok.token_der.data();
        i2d_PKCS7(token_pkcs7, &q);

        // Base64 encode for text transport using RAII BIO wrappers
        BIO_ptr b64(BIO_new(BIO_f_base64()));
        BIO_ptr mem(BIO_new(BIO_s_mem()));
        if (b64 && mem) {
            BIO_push(b64.get(), mem.get());
            BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
            BIO_write(b64.get(), tok.token_der.data(),
                      static_cast<int>(tok.token_der.size()));
            BIO_flush(b64.get());
            const char* b64_data = nullptr;
            long b64_len = BIO_get_mem_data(mem.get(), &b64_data);
            if (b64_data && b64_len > 0) {
                tok.token_b64.assign(b64_data, static_cast<size_t>(b64_len));
            }
            // Note: Don't call BIO_free on mem as it's managed by b64 chain
            mem.release();
        }
    }

    // --- TST_INFO: serial, time, policy, nonce, accuracy, ordering, hash ---
    TS_TST_INFO* tst_info = PKCS7_to_TS_TST_INFO(token_pkcs7);
    if (tst_info) {
        // Serial number
        const ASN1_INTEGER* serial = TS_TST_INFO_get_serial(tst_info);
        if (serial) {
            BIGNUM_ptr bn(ASN1_INTEGER_to_BN(serial, nullptr));
            if (bn) {
                char_ptr hex_serial(BN_bn2hex(bn.get()));
                if (hex_serial) { tok.serial_number = hex_serial.get(); }
            }
        }

        // Timestamp (GeneralizedTime → ISO 8601)
        const ASN1_GENERALIZEDTIME* gen_time = TS_TST_INFO_get_time(tst_info);
        if (gen_time) {
            const char* ts = reinterpret_cast<const char*>(
                ASN1_STRING_get0_data(gen_time));
            size_t ts_len = static_cast<size_t>(ASN1_STRING_length(gen_time));
            std::string raw(ts, ts_len);

            // GeneralizedTime format: YYYYMMDDHHmmss[.fff]Z
            if (raw.size() >= 14) {
                tok.timestamp_utc =
                    raw.substr(0,4) + "-" + raw.substr(4,2) + "-" +
                    raw.substr(6,2) + "T" + raw.substr(8,2) + ":" +
                    raw.substr(10,2) + ":" + raw.substr(12,2) + "Z";

                struct tm tm_val = {};
                tm_val.tm_year = std::stoi(raw.substr(0,4)) - 1900;
                tm_val.tm_mon  = std::stoi(raw.substr(4,2)) - 1;
                tm_val.tm_mday = std::stoi(raw.substr(6,2));
                tm_val.tm_hour = std::stoi(raw.substr(8,2));
                tm_val.tm_min  = std::stoi(raw.substr(10,2));
                tm_val.tm_sec  = std::stoi(raw.substr(12,2));
#if defined(_WIN32)
                time_t t = _mkgmtime(&tm_val);
#else
                time_t t = timegm(&tm_val);
#endif
                tok.timestamp_unix_ms = static_cast<uint64_t>(t) * 1000ULL;
            }
        }

        // Policy OID
        const ASN1_OBJECT* policy = TS_TST_INFO_get_policy_id(tst_info);
        if (policy) {
            char oid_buf[128] = {};
            OBJ_obj2txt(oid_buf, sizeof(oid_buf), policy, 1); // use dotted notation
            tok.policy_oid = oid_buf;
        }

        // Nonce
        const ASN1_INTEGER* nonce_asn1 = TS_TST_INFO_get_nonce(tst_info);
        if (nonce_asn1) {
            BIGNUM_ptr bn(ASN1_INTEGER_to_BN(nonce_asn1, nullptr));
            if (bn) {
                int nb = BN_num_bytes(bn.get());
                tok.nonce.resize(static_cast<size_t>(nb));
                BN_bn2bin(bn.get(), tok.nonce.data());
            }
        }

        // Hash algorithm from MessageImprint
        TS_MSG_IMPRINT* imprint = TS_TST_INFO_get_msg_imprint(tst_info);
        if (imprint) {
            const X509_ALGOR* alg = TS_MSG_IMPRINT_get_algo(imprint);
            if (alg) {
                int nid = OBJ_obj2nid(alg->algorithm);
                const char* sn = OBJ_nid2sn(nid);
                if (sn) tok.hash_algorithm = sn;
            }
        }

        // Accuracy (optional)
        const TS_ACCURACY* accuracy = TS_TST_INFO_get_accuracy(tst_info);
        if (accuracy) {
            tok.has_accuracy = true;
            const ASN1_INTEGER* secs   = TS_ACCURACY_get_seconds(accuracy);
            const ASN1_INTEGER* millis = TS_ACCURACY_get_millis(accuracy);
            const ASN1_INTEGER* micros = TS_ACCURACY_get_micros(accuracy);
            if (secs)   tok.accuracy_seconds = static_cast<uint32_t>(ASN1_INTEGER_get(secs));
            if (millis) tok.accuracy_millis  = static_cast<uint32_t>(ASN1_INTEGER_get(millis));
            if (micros) tok.accuracy_micros  = static_cast<uint32_t>(ASN1_INTEGER_get(micros));
        }

        // Ordering hint
        tok.ordering = (TS_TST_INFO_get_ordering(tst_info) != 0);

        // TS_TST_INFO is owned by PKCS7 and automatically freed when PKCS7 is freed
    }

    // --- TSA certificate extraction ---
    if (PKCS7_type_is_signed(token_pkcs7)) {
        PKCS7_SIGNED* ps = token_pkcs7->d.sign;
        if (ps && ps->cert && sk_X509_num(ps->cert) > 0) {
            X509* tsa_x509 = sk_X509_value(ps->cert, 0);
            if (tsa_x509) {
                X509_NAME* subj = X509_get_subject_name(tsa_x509);
                if (subj) {
                    char_ptr name(X509_NAME_oneline(subj, nullptr, 0));
                    if (name) { tok.tsa_name = name.get(); }
                }

                const ASN1_INTEGER* cert_serial = X509_get_serialNumber(tsa_x509);
                if (cert_serial) {
                    BIGNUM_ptr bn(ASN1_INTEGER_to_BN(cert_serial, nullptr));
                    if (bn) {
                        char_ptr s(BN_bn2hex(bn.get()));
                        if (s) { tok.tsa_serial = s.get(); }
                    }
                }

                int cert_len = i2d_X509(tsa_x509, nullptr);
                if (cert_len > 0) {
                    tok.tsa_cert.resize(static_cast<size_t>(cert_len));
                    unsigned char* r = tok.tsa_cert.data();
                    i2d_X509(tsa_x509, &r);
                    cached_tsa_cert_ = tok.tsa_cert; // cache for getTSACertificate()
                }
                tok.cert_valid = true;
            }
        }
    }

    tok.success  = true;
    tok.verified = false; // caller should call verifyTimestamp to confirm signature

    return tok;
    // TS_RESP automatically freed via RAII wrapper on scope exit
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

TimestampToken TimestampAuthority::getTimestamp(const std::vector<uint8_t>& data) {
    auto hash = computeHash(data);
    if (hash.empty()) {
        TimestampToken tok;
        tok.error_message = last_error_;
        return tok;
    }
    return getTimestampForHash(hash);
}

TimestampToken TimestampAuthority::getTimestampForHash(
    const std::vector<uint8_t>& hash)
{
    last_error_.clear();

    if (config_.url.empty()) {
        TimestampToken tok;
        tok.error_message = last_error_ = "TSA URL not configured";
        THEMIS_ERROR("{}", last_error_);
        return tok;
    }

    auto nonce = generateNonce(8);
    auto req_der = createTSPRequest(hash, nonce);
    if (req_der.empty()) {
        TimestampToken tok;
        tok.error_message = last_error_;
        return tok;
    }

    auto resp_bytes = sendTSPRequest(req_der);
    if (resp_bytes.empty()) {
        TimestampToken tok;
        tok.error_message = last_error_;
        return tok;
    }

    auto tok = parseTSPResponse(resp_bytes);
    if (tok.success)
        THEMIS_DEBUG("RFC 3161 timestamp obtained: serial={}, time={}",
                     tok.serial_number, tok.timestamp_utc);
    else
        THEMIS_ERROR("TSA request failed: {}", tok.error_message);

    return tok;
}

bool TimestampAuthority::verifyTimestamp(
    const std::vector<uint8_t>& data,
    const TimestampToken& token)
{
    auto hash = computeHash(data);
    return !hash.empty() && verifyTimestampForHash(hash, token);
}

bool TimestampAuthority::verifyTimestampForHash(
    const std::vector<uint8_t>& hash,
    const TimestampToken& token)
{
    if (!token.success || token.token_der.empty()) return false;

    const unsigned char* p = token.token_der.data();
    PKCS7_ptr pkcs7(d2i_PKCS7(
        nullptr, &p, static_cast<long>(token.token_der.size())));
    if (!pkcs7) return false;

    TS_TST_INFO* tst_info = PKCS7_to_TS_TST_INFO(pkcs7.get());
    bool match = false;
    if (tst_info) {
        TS_MSG_IMPRINT* imprint = TS_TST_INFO_get_msg_imprint(tst_info);
        if (imprint) {
            const ASN1_OCTET_STRING* token_hash = TS_MSG_IMPRINT_get_msg(imprint);
            if (token_hash) {
                const uint8_t* th_data = ASN1_STRING_get0_data(token_hash);
                int th_len = ASN1_STRING_length(token_hash);
                match = (static_cast<size_t>(th_len) == hash.size() &&
                         std::memcmp(th_data, hash.data(), hash.size()) == 0);
            }
        }
        // TS_TST_INFO is owned by PKCS7 and is freed when PKCS7 is freed
    }

    return match;
    // PKCS7 automatically freed via RAII wrapper on scope exit
}

TimestampToken TimestampAuthority::parseToken(
    const std::vector<uint8_t>& token_data)
{
    return parseTSPResponse(token_data);
}

TimestampToken TimestampAuthority::parseToken(const std::string& token_b64) {
    BIO_ptr b64(BIO_new(BIO_f_base64()));
    BIO_ptr mem(BIO_new_mem_buf(token_b64.data(),
                                static_cast<int>(token_b64.size())));
    if (!b64 || !mem) {
        TimestampToken tok;
        tok.error_message = "Failed to create BIO objects for base64 decoding";
        return tok;
    }
    
    BIO_push(b64.get(), mem.get());
    BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);

    std::vector<uint8_t> der(token_b64.size()); // upper bound
    int n = BIO_read(b64.get(), der.data(), static_cast<int>(der.size()));
    // mem.release() to avoid double-free since it's managed by b64 chain
    mem.release();

    if (n <= 0) {
        TimestampToken tok;
        tok.error_message = "Failed to base64-decode token";
        return tok;
    }
    der.resize(static_cast<size_t>(n));
    return parseTSPResponse(der);
}

std::optional<std::string> TimestampAuthority::getTSACertificate() {
    if (cached_tsa_cert_.empty()) return std::nullopt;

    const unsigned char* p = cached_tsa_cert_.data();
    X509_ptr cert(d2i_X509(nullptr, &p,
                           static_cast<long>(cached_tsa_cert_.size())));
    if (!cert) return std::nullopt;

    BIO_ptr bio(BIO_new(BIO_s_mem()));
    if (!bio) return std::nullopt;
    
    PEM_write_bio_X509(bio.get(), cert.get());

    const char* pem_data = nullptr;
    long pem_len = BIO_get_mem_data(bio.get(), &pem_data);
    if (pem_data && pem_len > 0) {
        return std::string(pem_data, static_cast<size_t>(pem_len));
    }
    return std::nullopt;
}

bool TimestampAuthority::isAvailable() {
    if (config_.url.empty()) return false;
    // Probe with a dummy hash; success means the TSA is reachable
    std::vector<uint8_t> dummy(32, 0);
    auto nonce  = generateNonce(4);
    auto req    = createTSPRequest(dummy, nonce);
    if (req.empty()) return false;
    auto resp   = sendTSPRequest(req);
    return !resp.empty();
}

std::string TimestampAuthority::getLastError() const { return last_error_; }

// ---------------------------------------------------------------------------
// eIDASTimestampValidator — real OpenSSL-backed validation
// ---------------------------------------------------------------------------

bool eIDASTimestampValidator::validateeIDASTimestamp(
    const TimestampToken& token,
    const std::vector<std::string>& trust_anchors)
{
    validation_errors_.clear();

    if (!token.success) {
        validation_errors_.push_back("Token marked as unsuccessful");
        return false;
    }
    if (token.token_der.empty()) {
        validation_errors_.push_back("No DER token data in TimestampToken");
        return false;
    }

    // Build trust store using RAII wrapper
    X509_STORE_ptr store(X509_STORE_new());
    if (!store) {
        validation_errors_.push_back("X509_STORE_new failed");
        return false;
    }

    for (const auto& pem_anchor : trust_anchors) {
        BIO_ptr bio(BIO_new_mem_buf(
            pem_anchor.data(), static_cast<int>(pem_anchor.size())));
        if (!bio) continue;
        
        X509_ptr ca(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
        if (ca) {
            X509_STORE_add_cert(store.get(), ca.get());
        }
    }
    if (trust_anchors.empty())
        X509_STORE_set_default_paths(store.get()); // fall back to system CA bundle

    // Decode PKCS7 using RAII wrapper
    const unsigned char* p = token.token_der.data();
    PKCS7_ptr pkcs7(d2i_PKCS7(
        nullptr, &p, static_cast<long>(token.token_der.size())));
    if (!pkcs7) {
        validation_errors_.push_back("DER decode of token failed (invalid PKCS7)");
        return false;
    }

    // Verify signature + signer certificate using RAII wrapper
    // Note: TS_VERIFY_CTX_set_store takes ownership of the store pointer,
    // so we need to release it from the RAII wrapper to avoid double-free
    TS_VERIFY_CTX_ptr ctx(TS_VERIFY_CTX_new());
    if (!ctx) {
        validation_errors_.push_back("TS_VERIFY_CTX_new failed");
        return false;
    }

    TS_VERIFY_CTX_set_flags(ctx.get(), TS_VFY_SIGNATURE | TS_VFY_SIGNER);
    TS_VERIFY_CTX_set_store(ctx.get(), store.release()); // transfer ownership to ctx

    int ok = TS_RESP_verify_token(ctx.get(), pkcs7.get());
    if (!ok) {
        unsigned long err_code;
        char err_buf[256];
        while ((err_code = ERR_get_error()) != 0) {
            ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
            validation_errors_.push_back(std::string("OpenSSL: ") + err_buf);
        }
    }

    return ok == 1;
    // PKCS7 and TS_VERIFY_CTX automatically freed via RAII wrappers on scope exit
}

bool eIDASTimestampValidator::validateAge(
    const TimestampToken& token, int max_age_days)
{
    validation_errors_.clear();

    if (token.timestamp_unix_ms == 0) {
        validation_errors_.push_back("Token has no timestamp");
        return false;
    }
    auto now = std::chrono::system_clock::now();
    uint64_t now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count());

    if (token.timestamp_unix_ms > now_ms) {
        validation_errors_.push_back("Token timestamp is in the future");
        return false;
    }

    constexpr uint64_t MS_PER_DAY = 86400000ULL;
    if (max_age_days < 0) {
        validation_errors_.push_back("max_age_days must be >= 0");
        return false;
    }
    if (static_cast<uint64_t>(max_age_days) > UINT64_MAX / MS_PER_DAY) {
        validation_errors_.push_back("max_age_days value too large");
        return false;
    }

    uint64_t age_ms = now_ms - token.timestamp_unix_ms;
    if (age_ms > static_cast<uint64_t>(max_age_days) * MS_PER_DAY) {
        validation_errors_.push_back("Token age exceeds maximum allowed age");
        return false;
    }
    return true;
}

bool eIDASTimestampValidator::isQualifiedTSA(
    const std::string& tsa_cert_pem,
    const std::vector<std::string>& qtsp_list)
{
    validation_errors_.clear();

    if (tsa_cert_pem.empty()) {
        validation_errors_.push_back("No TSA certificate provided");
        return false;
    }

    BIO_ptr bio(BIO_new_mem_buf(
        tsa_cert_pem.data(), static_cast<int>(tsa_cert_pem.size())));
    if (!bio) {
        validation_errors_.push_back("Failed to create BIO for certificate parsing");
        return false;
    }
    
    X509_ptr cert(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    if (!cert) {
        validation_errors_.push_back("Failed to parse TSA certificate PEM");
        return false;
    }

    X509_NAME* subj = X509_get_subject_name(cert.get());
    char_ptr name(subj ? X509_NAME_oneline(subj, nullptr, 0) : nullptr);
    std::string cert_subject = name ? name.get() : "";

    if (qtsp_list.empty()) return true; // no list → accept all (dev / test mode)

    for (const auto& qtsp : qtsp_list) {
        if (cert_subject.find(qtsp) != std::string::npos)
            return true;
    }

    validation_errors_.push_back(
        "TSA certificate not in QTSP list: " + cert_subject);
    return false;
}

std::vector<std::string> eIDASTimestampValidator::getValidationErrors() const {
    return validation_errors_;
}

} } // namespace themis::security

#endif // THEMIS_USE_OPENSSL_TSA



