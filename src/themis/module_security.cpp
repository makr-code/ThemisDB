/**
 * @file module_security.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ModuleSecurityVerifier implementation – bridges the plugin security
// infrastructure (PluginSecurityVerifier / PluginSecurityPolicy) to the
// module loading subsystem.
//
// Migrated from src/base/module_loader.cpp to src/themis/ as part of the
// v1.7.0 modular build architecture.

#include "themis/base/module_loader.h"
#include "acceleration/plugin_security.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace modules {

using namespace themis::acceleration;

// ============================================================================
// ModuleSecurityVerifier::Impl – Bridge to existing plugin security
// ============================================================================

class ModuleSecurityVerifier::Impl {
public:
    Impl() : policy_(), verifier_(policy_) {
#ifdef NDEBUG
        // Production: Require signature verification
        policy_.requireSignature = true;
        policy_.allowUnsigned = false;
        policy_.verifyFileHash = true;
        policy_.checkRevocation = true;
        policy_.minTrustLevel = PluginTrustLevel::TRUSTED;
#else
        // Development: Allow unsigned modules for testing
        policy_.requireSignature = false;
        policy_.allowUnsigned = true;
        policy_.verifyFileHash = true;
        policy_.checkRevocation = false;
        policy_.minTrustLevel = PluginTrustLevel::UNTRUSTED;
#endif

        spdlog::info("ModuleSecurityVerifier initialized (Production mode: {})",
                     policy_.requireSignature);
    }

    bool verifyModule(const std::string& modulePath, std::string& errorMessage) {
        spdlog::debug("Verifying module: {}", modulePath);

        bool result = verifier_.verifyPlugin(modulePath, errorMessage);

        if (result) {
            spdlog::info("Module verification PASSED: {}", modulePath);
        } else {
            spdlog::error("Module verification FAILED: {} - {}", modulePath, errorMessage);
        }

        return result;
    }

    std::string calculateFileHash(const std::string& modulePath) {
        return verifier_.calculateFileHash(modulePath);
    }

    void setRequireSignature([[maybe_unused]] bool require) {
        policy_.requireSignature = require;
        verifier_.updatePolicy(policy_);
    }

    void setAllowUnsigned([[maybe_unused]] bool allow) {
        policy_.allowUnsigned = allow;
        verifier_.updatePolicy(policy_);
    }

    void addWhitelistedHash(const std::string& hash) {
        policy_.whitelistedHashes.push_back(hash);
        verifier_.updatePolicy(policy_);
    }

    void addBlacklistedHash(const std::string& hash) {
        policy_.blacklistedHashes.push_back(hash);
        verifier_.updatePolicy(policy_);
    }

private:
    PluginSecurityPolicy   policy_;
    PluginSecurityVerifier verifier_;
};

// ============================================================================
// ModuleSecurityVerifier public interface
// ============================================================================

ModuleSecurityVerifier::ModuleSecurityVerifier()
    : impl_(std::make_unique<Impl>()) {
}

ModuleSecurityVerifier::~ModuleSecurityVerifier() = default;

bool ModuleSecurityVerifier::verifyModule(const std::string& modulePath,
                                          std::string& errorMessage) {
    return impl_->verifyModule(modulePath, errorMessage);
}

std::string ModuleSecurityVerifier::calculateFileHash(const std::string& modulePath) {
    return impl_->calculateFileHash(modulePath);
}

void ModuleSecurityVerifier::setRequireSignature([[maybe_unused]] bool require) {
    impl_->setRequireSignature(require);
}

void ModuleSecurityVerifier::setAllowUnsigned([[maybe_unused]] bool allow) {
    impl_->setAllowUnsigned(allow);
}

void ModuleSecurityVerifier::addWhitelistedHash(const std::string& hash) {
    impl_->addWhitelistedHash(hash);
}

void ModuleSecurityVerifier::addBlacklistedHash(const std::string& hash) {
    impl_->addBlacklistedHash(hash);
}

} // namespace modules
} // namespace themis
