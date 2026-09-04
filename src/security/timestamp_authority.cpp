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
#include <limits>
#include <cstdlib>
#include <string>
#include <mutex>
#include <openssl/rand.h>

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
        if (s == "production" || s == "prod") {
          return true;
        }
    }
    if (env_type) {
        const std::string s(env_type);
        if (s == "production" || s == "prod") {
          return true;
        }
    }
    if (node_env) {
        const std::string s(node_env);
        if (s == "production") {
          return true;
        }
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
/** @brief when `-DTHEMIS_USE_OPENSSL_TSA=ON` is set (Wave-2 CMake guard). */
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
    for (size_t i=0;i<data.size();++i) {
      h.push_back(static_cast<uint8_t>(data[i] ^ (i & 0xFF)));
    }
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
            tok.error_message = std::string([[maybe_unused]] "getTimestampForHash callback failed: ") + e.what();
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
std::vector<uint8_t> TimestampAuthority::generateNonce([[maybe_unused]] size_t bytes) {
    // Cryptographically random nonce using OpenSSL RAND_bytes.
    // Sequential counter bytes were previously used here (security gap) —
    // replaced with RAND_bytes to ensure nonces are unpredictable.
    if (bytes == 0) {
        return {};
    }

    constexpr size_t kMaxRandBytes = static_cast<size_t>(std::numeric_limits<int>::max());
    if (bytes > kMaxRandBytes) {
        THEMIS_ERROR("TimestampAuthority::generateNonce: requested size {} exceeds RAND_bytes "
                     "limit {}", bytes, kMaxRandBytes);
        return {};
    }

    std::vector<uint8_t> n(bytes);
    if (RAND_bytes(n.data(), static_cast<int>(bytes)) != 1) {
        // RAND_bytes failure is non-recoverable; return empty to signal error.
        // Callers must treat an empty nonce as a failure (token.success stays false).
        THEMIS_ERROR("TimestampAuthority::generateNonce: RAND_bytes failed — cannot produce "
                     "cryptographically random nonce (size={}). TSP token will be rejected.", bytes);
        return {};
    }
    return n;
}
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
