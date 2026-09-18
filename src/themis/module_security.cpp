/**
 * @file module_security.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
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

    /**
     * @brief Verify Module.
     * @param[in] modulePath Input parameter.
     * @param[in,out] errorMessage Input/output parameter.
     * @return True when the operation succeeds.
     * @details Calls: spdlog::debug(), verifyPlugin(), spdlog::info(), spdlog::error().
     */
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

    /**
     * @brief Calculate File Hash.
     * @param[in] modulePath Input parameter.
     * @return Return value.
     * @details Implements calculateFileHash without additional internal calls.
     */
    std::string calculateFileHash(const std::string& modulePath) {
        return verifier_.calculateFileHash(modulePath);
    }

    /**
     * @brief Set Require Signature.
     * @param[in] require Input parameter.
     * @details Calls: updatePolicy().
     */
    void setRequireSignature(bool require) {
        policy_.requireSignature = require;
        verifier_.updatePolicy(policy_);
    }

    /**
     * @brief Set Allow Unsigned.
     * @param[in] allow Input parameter.
     * @details Calls: updatePolicy().
     */
    void setAllowUnsigned(bool allow) {
        policy_.allowUnsigned = allow;
        verifier_.updatePolicy(policy_);
    }

    /**
     * @brief Add Whitelisted Hash.
     * @param[in] hash Input parameter.
     * @details Calls: push_back(), updatePolicy().
     */
    void addWhitelistedHash(const std::string& hash) {
        policy_.whitelistedHashes.push_back(hash);
        verifier_.updatePolicy(policy_);
    }

    /**
     * @brief Add Blacklisted Hash.
     * @param[in] hash Input parameter.
     * @details Calls: push_back(), updatePolicy().
     */
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

/**
 * @brief Verify Module.
 * @param[in] modulePath Input parameter.
 * @param[in,out] errorMessage Input/output parameter.
 * @return True when the operation succeeds.
 * @details Implements verifyModule without additional internal calls.
 */
bool ModuleSecurityVerifier::verifyModule(const std::string& modulePath,
                                          std::string& errorMessage) {
    return impl_->verifyModule(modulePath, errorMessage);
}

/**
 * @brief Calculate File Hash.
 * @param[in] modulePath Input parameter.
 * @return Return value.
 * @details Implements calculateFileHash without additional internal calls.
 */
std::string ModuleSecurityVerifier::calculateFileHash(const std::string& modulePath) {
    return impl_->calculateFileHash(modulePath);
}

/**
 * @brief Set Require Signature.
 * @param[in] require Input parameter.
 * @details Implements setRequireSignature without additional internal calls.
 */
void ModuleSecurityVerifier::setRequireSignature(bool require) {
    impl_->setRequireSignature(require);
}

/**
 * @brief Set Allow Unsigned.
 * @param[in] allow Input parameter.
 * @details Implements setAllowUnsigned without additional internal calls.
 */
void ModuleSecurityVerifier::setAllowUnsigned(bool allow) {
    impl_->setAllowUnsigned(allow);
}

/**
 * @brief Add Whitelisted Hash.
 * @param[in] hash Input parameter.
 * @details Implements addWhitelistedHash without additional internal calls.
 */
void ModuleSecurityVerifier::addWhitelistedHash(const std::string& hash) {
    impl_->addWhitelistedHash(hash);
}

/**
 * @brief Add Blacklisted Hash.
 * @param[in] hash Input parameter.
 * @details Implements addBlacklistedHash without additional internal calls.
 */
void ModuleSecurityVerifier::addBlacklistedHash(const std::string& hash) {
    impl_->addBlacklistedHash(hash);
}

} // namespace modules
} // namespace themis
