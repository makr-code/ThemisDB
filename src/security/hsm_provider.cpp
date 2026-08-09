/**
 * @file hsm_provider.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=58; TODO=1, Stub=47, Unimpl=0, Mock=1, Sim=7, Debt=2, C=0, H=1, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: hsm_provider.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 83/100 | Lines: 525
 * Gap Summary: total=58; TODO=1, Stub=47, Unimpl=0, Mock=1, Sim=7, Debt=2, C=0, H=10, M=7, L=0
 * PR History (last 5): #3462 [HSM] Production failsafe: ... (2026-03-12) | #3454 fix: Wire PKCS#11 HSM produ... (2026-03-12) | #2585 feat(security): HSM PKCS#11... (2026-03-12) | #2564 feat(security): HSM direct ... (2026-03-12) | #401 Replace Security Stubs with... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Clean minimal stub implementation of HSMProvider.
// Provides deterministic, nicht-kryptographische Operationen fuer Developer-Fallback.
// Wird nur eingebaut, wenn THEMIS_ENABLE_HSM_REAL NICHT definiert ist.

#ifdef THEMIS_ENABLE_HSM_REAL
// Real PKCS#11 Implementierung in hsm_provider_pkcs11.cpp
#else

// PERMANENT FALLBACK NOTE:
// Purpose: Software-only AES-256-GCM fallback for HSMProvider when no real HSM hardware
//          is present. Provides deterministic key-wrap/unwrap for developer and CI use.
//          Production mode is explicitly blocked unless THEMIS_ALLOW_HSM_STUB=1 env var
//          is set, or the --allow-stub-hsm server flag is passed.
// Activation: Compiled when THEMIS_ENABLE_HSM_REAL is NOT defined (default in dev builds).
//             Build with -DTHEMIS_ENABLE_HSM_REAL=ON (Wave-2 CMake guard) to activate the
//             real PKCS#11 implementation in hsm_provider_pkcs11.cpp instead.
// Production Delta: Uses a randomly-generated in-memory KEK (not persisted across restarts,
//                   not protected by HSM hardware). All crypto is software-only OpenSSL.
//                   Not suitable for production key management.
// This fallback path is PERMANENT for no-HSM builds; it is a compile-time safety net.
// No v1.x production deployment ships without a real HSM backend.
// Roadmap ref: src/security/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"

#include "security/hsm_provider.h"
#include <stdexcept>
#include "core/production_mode.h"
#include "themis/runtime_license_gate.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <memory>
#include <sstream>
#include <chrono>
#include <atomic>

namespace themis { namespace security {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct EVP_CIPHER_CTX_Deleter {
    void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
};

using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_Deleter>;

class HSMProvider::Impl {
public:
    std::vector<uint8_t> stub_kek; // 32-byte AES-256 KEK for stub wrap/unwrap

    // Performance stats (atomic for thread safety)
    std::atomic<uint64_t> sign_count{0};
    std::atomic<uint64_t> verify_count{0};
    std::atomic<uint64_t> sign_errors{0};
    std::atomic<uint64_t> verify_errors{0};
    std::atomic<uint64_t> total_sign_time_us{0};
    std::atomic<uint64_t> total_verify_time_us{0};
};

static std::string to_hex(const std::vector<uint8_t>& data) {
    static const char* d = "0123456789abcdef";
    std::string out; out.reserve(data.size()*2);
    for (auto b : data) { out.push_back(d[(b>>4)&0xF]); out.push_back(d[b&0xF]); }
    return out;
}

static std::string pseudo_b64(const std::vector<uint8_t>& data) {
    return std::string("hex:") + to_hex(data);
}

// Get OpenSSL error string for diagnostics
static std::string ossl_error() {
    unsigned long code = ERR_peek_last_error();
    if (!code) return "Unknown OpenSSL error";
    char buf[256] = {0};
    ERR_error_string_n(code, buf, sizeof(buf));
    ERR_clear_error();
    return std::string(buf);
}

// AES-256-GCM encrypt: returns iv(12) || ciphertext || tag(16)
static std::vector<uint8_t> stub_aes_encrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data) {
    if (key.size() != 32) {
        throw std::runtime_error("AES-256-GCM encryption: invalid key size (expected 32 bytes, got " + 
                                std::to_string(key.size()) + ")");
    }
    std::vector<uint8_t> iv(12);
    if (RAND_bytes(iv.data(), 12) != 1) {
        throw std::runtime_error("AES-256-GCM encryption: RAND_bytes failed: " + ossl_error());
    }
    EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw std::runtime_error("AES-256-GCM encryption: EVP_CIPHER_CTX_new failed: " + ossl_error());
    }
    std::vector<uint8_t> ciphertext(data.size() + 16);
    std::vector<uint8_t> tag(16);
    int len = 0, ct_len = 0;
    bool ok =
        EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) == 1 &&
        EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) == 1 &&
        EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len, data.data(), (int)data.size()) == 1;
    ct_len = len;
    if (ok) ok = EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + len, &len) == 1;
    ct_len += len;
    if (ok) ok = EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag.data()) == 1;
    if (!ok) {
        throw std::runtime_error("AES-256-GCM encryption failed: " + ossl_error());
    }
    ciphertext.resize(ct_len);
    std::vector<uint8_t> result;
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    result.insert(result.end(), tag.begin(), tag.end());
    return result;
}

// AES-256-GCM decrypt: expects iv(12) || ciphertext || tag(16)
static std::vector<uint8_t> stub_aes_decrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& encrypted) {
    if (key.size() != 32) {
        throw std::runtime_error("AES-256-GCM decryption: invalid key size (expected 32 bytes, got " +
                                std::to_string(key.size()) + ")");
    }
    if (encrypted.size() < 12 + 16) {
        throw std::runtime_error("AES-256-GCM decryption: encrypted data too short (expected at least " +
                                std::to_string(12 + 16) + " bytes, got " + 
                                std::to_string(encrypted.size()) + ")");
    }
    const uint8_t* iv  = encrypted.data();
    size_t ct_len      = encrypted.size() - 12 - 16;
    const uint8_t* ct  = encrypted.data() + 12;
    const uint8_t* tag = encrypted.data() + 12 + ct_len;
    EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw std::runtime_error("AES-256-GCM decryption: EVP_CIPHER_CTX_new failed: " + ossl_error());
    }
    std::vector<uint8_t> plaintext(ct_len);
    int len = 0, pt_len = 0;
    bool ok =
        EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) == 1 &&
        EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv) == 1 &&
        EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, ct, (int)ct_len) == 1;
    pt_len = len;
    if (ok) ok = EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, (void*)tag) == 1;
    if (ok) ok = EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &len) > 0;
    if (!ok) {
        throw std::runtime_error("AES-256-GCM decryption failed (possible tag mismatch): " + ossl_error());
    }
    plaintext.resize(pt_len);
    return plaintext;
}

HSMProvider::HSMProvider(HSMConfig config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config)) {}

HSMProvider::~HSMProvider() = default;
HSMProvider::HSMProvider(HSMProvider&&) noexcept = default;
HSMProvider& HSMProvider::operator=(HSMProvider&&) noexcept = default;

bool HSMProvider::initialize() {
    if (initialized_) return true;

    // Runtime license gate: HSM is an Enterprise/Hyperscaler feature.
    std::string license_error;
    if (!license::RuntimeLicenseGate::instance().isFeatureAllowed("hsm", license_error)) {
        last_error_ = "HSM unavailable: " + license_error;
        THEMIS_ERROR("{}", last_error_);
        return false;
    }
    
    // SECURITY HARDENING: Check for explicit opt-in to use stub provider
    // This prevents accidental production deployment with insecure stub
    const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
    
    // Hard fail-fast: reject stub in any Themis production environment.
    // Covers THEMIS_PRODUCTION_MODE (all truthy values) and THEMIS_ENVIRONMENT=production|prod.
    // This cannot be overridden by THEMIS_ALLOW_HSM_STUB.
    if (core::ProductionMode::isEnabled()) {
        last_error_ = "HSM stub provider cannot be used in production mode. "
                      "Build with -DTHEMIS_ENABLE_HSM_REAL=ON, or disable production mode "
                      "(THEMIS_PRODUCTION_MODE / THEMIS_ENVIRONMENT).";
        THEMIS_ERROR("SECURITY ERROR: {}", last_error_);
        return false;
    }
    
    // Soft fail: Without explicit opt-in, refuse to initialize if other
    // production-like environment indicators are detected (ENVIRONMENT, NODE_ENV).
    if (!allow_stub || std::string(allow_stub) != "1") {
        // Check for production-like indicators
        const char* env_type = std::getenv("ENVIRONMENT");
        const char* node_env = std::getenv("NODE_ENV");
        
        bool production_indicators = 
            (env_type && (std::string(env_type) == "production" || std::string(env_type) == "prod")) ||
            (node_env && std::string(node_env) == "production");
        
        if (production_indicators) {
            last_error_ = "HSM stub provider detected production environment but THEMIS_ALLOW_HSM_STUB is not set. "
                          "Set THEMIS_ALLOW_HSM_STUB=1 to explicitly allow insecure stub, or use real HSM.";
            THEMIS_ERROR("SECURITY ERROR: {}", last_error_);
            return false;
        }
    }
    
    initialized_ = true;
    
    // Recreate Impl if finalize() was called previously (impl_ would be nullptr)
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }
    
    // Generate stub KEK for consistent wrap/unwrap operations
    impl_->stub_kek.resize(32);
    if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {
        last_error_ = "Failed to generate stub KEK";
        THEMIS_ERROR("HSMProvider stub: {}", last_error_);
        initialized_ = false;
        return false;
    }
    
    // CRITICAL SECURITY WARNING: Stub provider active
    THEMIS_WARN("╔═══════════════════════════════════════════════════════════════╗");
    THEMIS_WARN("║  ⚠️  INSECURE CONFIGURATION: HSM STUB PROVIDER ACTIVE!  ⚠️   ║");
    THEMIS_WARN("╠═══════════════════════════════════════════════════════════════╣");
    THEMIS_WARN("║  Master keys are NOT protected by hardware security.         ║");
    THEMIS_WARN("║  This configuration is for DEVELOPMENT ONLY.                 ║");
    THEMIS_WARN("║                                                               ║");
    THEMIS_WARN("║  Production deployment REQUIRES real HSM configuration:      ║");
    THEMIS_WARN("║  - Build with -DTHEMIS_ENABLE_HSM_REAL=ON                    ║");
    THEMIS_WARN("║  - Configure PKCS#11 HSM (Luna, CloudHSM, etc.)             ║");
    THEMIS_WARN("║  - Or use cloud KMS (AWS KMS, Azure Key Vault, GCP KMS)     ║");
    THEMIS_WARN("║                                                               ║");
    THEMIS_WARN("║  To explicitly allow stub (dev only):                        ║");
    THEMIS_WARN("║  - Set THEMIS_ALLOW_HSM_STUB=1 environment variable          ║");
    THEMIS_WARN("║                                                               ║");
    THEMIS_WARN("║  See: docs/security/HSM_PRODUCTION_SETUP.md                  ║");
    THEMIS_WARN("╚═══════════════════════════════════════════════════════════════╝");
    
    return true;
}

void HSMProvider::finalize() {
    initialized_ = false;
    impl_.reset();
    THEMIS_INFO("HSMProvider stub finalized");
}

HSMSignatureResult HSMProvider::sign(const std::vector<uint8_t>& data, const std::string& key_label) {
    return signHash(data, key_label); // treat data as pre-hash
}

HSMSignatureResult HSMProvider::signHash(const std::vector<uint8_t>& hash, const std::string& key_label) {
    auto startTime = std::chrono::high_resolution_clock::now();
    HSMSignatureResult r;
    if (!initialized_ || !impl_) {
        r.error_message = "HSM stub not initialized";
        if (impl_) impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
        return r;
    }

    // initialize() already enforces the stub production-mode restrictions before
    // `initialized_` becomes true, so an injected bridge cannot bypass them.
    SignHashFn fn;
    {
        std::lock_guard<std::mutex> lk(HSMProvider::signHashFnMutex());
        fn = HSMProvider::signHashFnStorage();
    }
    if (fn) {
        try {
            auto result = fn(hash, key_label.empty() ? config_.key_label : key_label);
            if (result.success) {
                impl_->sign_count.fetch_add(1, std::memory_order_relaxed);
            } else {
                impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - startTime).count();
            impl_->total_sign_time_us.fetch_add(static_cast<uint64_t>(elapsed), std::memory_order_relaxed);
            return result;
        } catch (const std::exception& e) {
            r.error_message = std::string("signHash callback failed: ") + e.what();
            impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
            return r;
        } catch (...) {
            r.error_message = "signHash callback failed: unknown exception";
            impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
            return r;
        }
    }

    THEMIS_WARN("HSMProvider STUB signing - NOT cryptographically secure!");
    r.success = true;
    r.signature_b64 = pseudo_b64(hash);
    r.algorithm = config_.signature_algorithm;
    r.key_id = key_label.empty() ? config_.key_label : key_label;
    r.cert_serial = "STUB-CERT";
    r.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    impl_->sign_count.fetch_add(1, std::memory_order_relaxed);
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - startTime).count();
    impl_->total_sign_time_us.fetch_add(static_cast<uint64_t>(elapsed), std::memory_order_relaxed);
    return r;
}

bool HSMProvider::verify(const std::vector<uint8_t>& data, const std::string& signature_b64, const std::string& key_label) {
    auto startTime = std::chrono::high_resolution_clock::now();
    VerifyFn fn;
    {
        std::lock_guard<std::mutex> lk(HSMProvider::verifyFnMutex());
        fn = HSMProvider::verifyFnStorage();
    }
    bool ok = false;
    if (fn) {
        try {
            ok = fn(data, signature_b64, key_label.empty() ? config_.key_label : key_label);
        } catch (...) {
            ok = false;
        }
    } else {
        auto expected = pseudo_b64(data);
        ok = (expected == signature_b64);
    }
    THEMIS_DEBUG("HSMProvider stub verify key='{}' ok={}", key_label.empty()?config_.key_label:key_label, ok);
    if (impl_) {
        if (ok) impl_->verify_count.fetch_add(1, std::memory_order_relaxed);
        else impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        impl_->total_verify_time_us.fetch_add(static_cast<uint64_t>(elapsed), std::memory_order_relaxed);
    }
    return ok;
}

std::vector<HSMKeyInfo> HSMProvider::listKeys() {
    HSMKeyInfo info;
    info.label = config_.key_label;
    info.id = "stub-id";
    info.algorithm = config_.signature_algorithm;
    info.can_sign = true;
    info.can_verify = true;
    info.extractable = false;
    info.key_size = 0;
    return {info};
}

std::vector<uint8_t> HSMProvider::encryptData(const std::vector<uint8_t>& data, [[maybe_unused]] const std::string& key_label) {
    if (!initialized_) { last_error_ = "HSM stub not initialized"; return {}; }
    if (data.empty()) { last_error_ = "Cannot encrypt empty data"; return {}; }
    EncryptDataFn fn;
    {
        std::lock_guard<std::mutex> lk(HSMProvider::encryptDataFnMutex());
        fn = HSMProvider::encryptDataFnStorage();
    }
    if (fn) {
        try {
            return fn(data, key_label.empty() ? config_.key_label : key_label);
        } catch (const std::exception& e) {
            last_error_ = std::string("encryptData callback failed: ") + e.what();
            return {};
        } catch (...) {
            last_error_ = "encryptData callback failed: unknown exception";
            return {};
        }
    }
    THEMIS_WARN("HSMProvider STUB encryptData - NOT hardware-protected, for development only!");
    try {
        return stub_aes_encrypt(impl_->stub_kek, data);
    } catch (const std::exception& e) {
        last_error_ = std::string("Stub AES encrypt failed: ") + e.what();
        return {};
    }
}

std::vector<uint8_t> HSMProvider::decryptData(const std::vector<uint8_t>& encrypted, [[maybe_unused]] const std::string& key_label) {
    if (!initialized_) { last_error_ = "HSM stub not initialized"; return {}; }
    if (encrypted.empty()) { last_error_ = "Cannot decrypt empty data"; return {}; }
    DecryptDataFn fn;
    {
        std::lock_guard<std::mutex> lk(HSMProvider::decryptDataFnMutex());
        fn = HSMProvider::decryptDataFnStorage();
    }
    if (fn) {
        try {
            return fn(encrypted, key_label.empty() ? config_.key_label : key_label);
        } catch (const std::exception& e) {
            last_error_ = std::string("decryptData callback failed: ") + e.what();
            return {};
        } catch (...) {
            last_error_ = "decryptData callback failed: unknown exception";
            return {};
        }
    }
    THEMIS_WARN("HSMProvider STUB decryptData - NOT hardware-protected, for development only!");
    try {
        return stub_aes_decrypt(impl_->stub_kek, encrypted);
    } catch (const std::exception& e) {
        last_error_ = std::string("Stub AES decrypt failed: ") + e.what();
        return {};
    }
}

// STUB/SIMULATION NOTE (generateKeyPair / importCertificate / getCertificate):
// Purpose: Satisfy the HSMProvider public interface in the software-only stub
//          build so callers do not need conditional compilation.
// Activation: Inside `#ifndef THEMIS_ENABLE_HSM_REAL` (same activation as the
//             overall HSM stub class documented at the top of this file).
// Production Delta: generateKeyPair always returns false and does NOT generate
//             any key material — the real PKCS#11 path creates an asymmetric
//             key pair on the HSM token.  importCertificate returns false without
//             storing anything.  getCertificate returns a hardcoded dummy PEM
//             string instead of a real device certificate.
// Removal Plan: Replaced by hsm_provider_pkcs11.cpp when -DTHEMIS_ENABLE_HSM_REAL
//             is set.  See src/security/FUTURE_ENHANCEMENTS.md §"HSM Key Management".
bool HSMProvider::generateKeyPair(const std::string& label, [[maybe_unused]] uint32_t key_size, [[maybe_unused]] bool extractable) {
    GenerateKeyPairFn fn;
    {
        std::lock_guard<std::mutex> lk(HSMProvider::generateKeyPairFnMutex());
        fn = HSMProvider::generateKeyPairFnStorage();
    }
    if (fn) {
        try {
            return fn(label, key_size, extractable);
        } catch (const std::exception& e) {
            last_error_ = std::string("generateKeyPair callback failed: ") + e.what();
            THEMIS_ERROR("{}", last_error_);
            return false;
        } catch (...) {
            last_error_ = "generateKeyPair callback failed: unknown exception";
            THEMIS_ERROR("{}", last_error_);
            return false;
        }
    }
    // Stub: unused
    THEMIS_WARN("HSMProvider stub generateKeyPair ignored (label='{}')", label);
    return false;
}

bool HSMProvider::importCertificate(const std::string& key_label, [[maybe_unused]] const std::string& cert_pem) {
    ImportCertificateFn fn;
    {
        std::lock_guard<std::mutex> lk(HSMProvider::importCertificateFnMutex());
        fn = HSMProvider::importCertificateFnStorage();
    }
    if (fn) {
        try {
            return fn(key_label, cert_pem);
        } catch (const std::exception& e) {
            last_error_ = std::string("importCertificate callback failed: ") + e.what();
            THEMIS_ERROR("{}", last_error_);
            return false;
        } catch (...) {
            last_error_ = "importCertificate callback failed: unknown exception";
            THEMIS_ERROR("{}", last_error_);
            return false;
        }
    }
    // Stub: unused
    THEMIS_WARN("HSMProvider stub importCertificate ignored (key='{}')", key_label);
    return false;
}

std::optional<std::string> HSMProvider::getCertificate([[maybe_unused]] const std::string& key_label) {
    GetCertificateFn fn;
    {
        std::lock_guard<std::mutex> lk(HSMProvider::getCertificateFnMutex());
        fn = HSMProvider::getCertificateFnStorage();
    }
    if (fn) {
        try {
            return fn(key_label);
        } catch (const std::exception& e) {
            last_error_ = std::string("getCertificate callback failed: ") + e.what();
            THEMIS_ERROR("{}", last_error_);
            return std::nullopt;
        } catch (...) {
            last_error_ = "getCertificate callback failed: unknown exception";
            THEMIS_ERROR("{}", last_error_);
            return std::nullopt;
        }
    }
    // Fail-closed by default: returning a dummy PEM to an unsuspecting caller is dangerous.
    // A caller that trusts the returned certificate for authentication or TLS would use
    // a meaningless stub cert, opening the door to certificate-validation bypass.
    // Require explicit opt-in to the insecure stub path.
    const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
    if (!allow_stub || std::string(allow_stub) != "1") {
        THEMIS_ERROR(
            "HSMProvider stub getCertificate('{}') refused: returning a dummy PEM "
            "is insecure. Set THEMIS_ALLOW_HSM_STUB=1 for explicit development override, "
            "or build with -DTHEMIS_ENABLE_HSM_REAL=ON.",
            key_label);
        return std::nullopt;
    }
    THEMIS_WARN(
        "HSMProvider stub getCertificate('{}') returning hardcoded dummy PEM "
        "(THEMIS_ALLOW_HSM_STUB=1). Not suitable for production.",
        key_label);
    return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");
}

bool HSMProvider::isReady() const { return initialized_; }

std::string HSMProvider::getTokenInfo() const {
    std::ostringstream oss; oss << "HSM stub/fallback label=" << config_.key_label << " ready=" << (initialized_?"true":"false");
    return oss.str();
}

std::string HSMProvider::getLastError() const { return last_error_; }

HSMPerformanceStats HSMProvider::getStats() const {
    HSMPerformanceStats stats;
    stats.sign_count = impl_ ? impl_->sign_count.load(std::memory_order_relaxed) : 0;
    stats.verify_count = impl_ ? impl_->verify_count.load(std::memory_order_relaxed) : 0;
    stats.sign_errors = impl_ ? impl_->sign_errors.load(std::memory_order_relaxed) : 0;
    stats.verify_errors = impl_ ? impl_->verify_errors.load(std::memory_order_relaxed) : 0;
    stats.total_sign_time_us = impl_ ? impl_->total_sign_time_us.load(std::memory_order_relaxed) : 0;
    stats.total_verify_time_us = impl_ ? impl_->total_verify_time_us.load(std::memory_order_relaxed) : 0;
    stats.pool_size = 0; // No real pool in stub
    stats.pool_round_robin_hits = 0;
    return stats;
}

void HSMProvider::resetStats() {
    if (!impl_) return;
    impl_->sign_count.store(0, std::memory_order_relaxed);
    impl_->verify_count.store(0, std::memory_order_relaxed);
    impl_->sign_errors.store(0, std::memory_order_relaxed);
    impl_->verify_errors.store(0, std::memory_order_relaxed);
    impl_->total_sign_time_us.store(0, std::memory_order_relaxed);
    impl_->total_verify_time_us.store(0, std::memory_order_relaxed);
}

bool HSMProvider::isStubProvider() const {
    // Always true for stub implementation
    return true;
}

void HSMProvider::periodicSecurityCheck() {
    if (!initialized_) return;
    
    // Log ERROR-level warning for production monitoring
    THEMIS_ERROR("⚠️  HSM SECURITY WARNING: Using stub provider in production!");
    THEMIS_ERROR("Master keys are NOT hardware-protected. Configure proper HSM immediately.");
    THEMIS_ERROR("Compliance impact: NIST SP 800-53 SC-12, PCI DSS 3.6, GDPR Art. 32");
    THEMIS_ERROR("See: docs/security/HSM_PRODUCTION_SETUP.md");
}

// HSMPKIClient
HSMPKIClient::HSMPKIClient(HSMConfig config) : hsm_(std::make_unique<HSMProvider>(std::move(config))) { hsm_->initialize(); }
HSMPKIClient::~HSMPKIClient() { if (hsm_) hsm_->finalize(); }
HSMSignatureResult HSMPKIClient::sign(const std::vector<uint8_t>& data) { return hsm_->sign(data); }
bool HSMPKIClient::verify(const std::vector<uint8_t>& data, const std::string& signature_b64) { return hsm_->verify(data, signature_b64); }
std::optional<std::string> HSMPKIClient::getCertSerial() { return std::string("STUB-SERIAL"); }
bool HSMPKIClient::isReady() const { return hsm_->isReady(); }

} } // namespace themis::security

#endif // !THEMIS_ENABLE_HSM_REAL


