/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hsm_provider.cpp                                   ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  ⚫ DRAFT                                        ║
    • Quality Score:   0.0/100                                        ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 29                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f68ad6489  2026-02-21  Implement runtime license system: enforcement, provisioni... ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 📝 Draft / Stub                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Clean minimal stub implementation of HSMProvider.
// Provides deterministic, nicht-kryptographische Operationen fuer Developer-Fallback.
// Wird nur eingebaut, wenn THEMIS_ENABLE_HSM_REAL NICHT definiert ist.

#ifdef THEMIS_ENABLE_HSM_REAL
// Real PKCS#11 Implementierung in hsm_provider_pkcs11.cpp
#else

#include "security/hsm_provider.h"
#include "themis/runtime_license_gate.h"
#include "utils/logger.h"
#include <sstream>
#include <chrono>

namespace themis { namespace security {

class HSMProvider::Impl { };

static std::string to_hex(const std::vector<uint8_t>& data) {
    static const char* d = "0123456789abcdef";
    std::string out; out.reserve(data.size()*2);
    for (auto b : data) { out.push_back(d[(b>>4)&0xF]); out.push_back(d[b&0xF]); }
    return out;
}

static std::string pseudo_b64(const std::vector<uint8_t>& data) {
    return std::string("hex:") + to_hex(data);
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
    const char* force_production = std::getenv("THEMIS_PRODUCTION_MODE");
    
    // Fail-fast: If production mode is explicitly enabled, stub is not allowed
    if (force_production && std::string(force_production) == "1") {
        last_error_ = "HSM stub provider cannot be used in production mode. "
                      "Build with -DTHEMIS_ENABLE_HSM_REAL=ON or disable THEMIS_PRODUCTION_MODE.";
        THEMIS_ERROR("SECURITY ERROR: {}", last_error_);
        return false;
    }
    
    // Fail-fast: Without explicit opt-in, refuse to initialize if production-like environment detected
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
    HSMSignatureResult r;
    if (!initialized_) { r.error_message = "HSM stub not initialized"; return r; }
    THEMIS_WARN("HSMProvider STUB signing - NOT cryptographically secure!");
    r.success = true;
    r.signature_b64 = pseudo_b64(hash);
    r.algorithm = config_.signature_algorithm;
    r.key_id = key_label.empty() ? config_.key_label : key_label;
    r.cert_serial = "STUB-CERT";
    r.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return r;
}

bool HSMProvider::verify(const std::vector<uint8_t>& data, const std::string& signature_b64, const std::string& key_label) {
    auto expected = pseudo_b64(data);
    bool ok = (expected == signature_b64);
    THEMIS_DEBUG("HSMProvider stub verify key='{}' ok={}", key_label.empty()?config_.key_label:key_label, ok);
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

bool HSMProvider::generateKeyPair(const std::string& label, uint32_t key_size, bool extractable) {
    (void)key_size; (void)extractable; // Stub: unused
    THEMIS_WARN("HSMProvider stub generateKeyPair ignored (label='{}')", label);
    return false;
}

bool HSMProvider::importCertificate(const std::string& key_label, const std::string& cert_pem) {
    (void)cert_pem; // Stub: unused
    THEMIS_WARN("HSMProvider stub importCertificate ignored (key='{}')", key_label);
    return false;
}

std::optional<std::string> HSMProvider::getCertificate(const std::string& key_label) {
    (void)key_label; // Stub: unused
    return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");
}

bool HSMProvider::isReady() const { return initialized_; }

std::string HSMProvider::getTokenInfo() const {
    std::ostringstream oss; oss << "HSM STUB label=" << config_.key_label << " ready=" << (initialized_?"true":"false");
    return oss.str();
}

std::string HSMProvider::getLastError() const { return last_error_; }

HSMPerformanceStats HSMProvider::getStats() const {
    // Stub: return empty stats
    return HSMPerformanceStats{};
}

void HSMProvider::resetStats() {
    // Stub: no-op
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
