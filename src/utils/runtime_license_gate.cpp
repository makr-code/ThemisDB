/**
 * @file runtime_license_gate.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.41
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Runtime License Gate – Implementation
 * ================================================
 * See include/themis/runtime_license_gate.h for full API documentation.
 */

#include "themis/runtime_license_gate.h"
#include "themis/edition.h"

#include <sstream>

namespace themis {
namespace license {

// ============================================================================
// Singleton
// ============================================================================

RuntimeLicenseGate& RuntimeLicenseGate::instance() {
    static RuntimeLicenseGate gate;
    return gate;
}

// ============================================================================
// Lifecycle
// ============================================================================

void RuntimeLicenseGate::initialize(const LicenseActivationResult& activation,
                                     const std::optional<LicenseData>& license) {
    std::lock_guard<std::mutex> lock(mutex_);
    activation_   = activation;
    license_      = license.has_value() ? license
                  : activation.refreshed_license;
    initialized_  = true;
}

void RuntimeLicenseGate::update(const LicenseActivationResult& activation,
                                 const std::optional<LicenseData>& license) {
    initialize(activation, license);
}

// ============================================================================
// Feature checking helpers
// ============================================================================

namespace {

/// Returns true if `feature_name` is a known Enterprise/Hyperscaler-only gate.
/// Community-only or universal features are NOT in this list and are always
/// allowed.
bool isEnterpriseFeature(std::string_view feature_name) {
    return feature_name == "enterprise_plugins"
        || feature_name == "multi_master"
        || feature_name == "field_encryption"
        || feature_name == "rbac"
        || feature_name == "hsm";
}

/// Returns true if the activation status string represents a usable (allowed)
/// state for feature access.
bool isStatusAllowed(std::string_view status) {
    // "active" and "grace" are allowed; everything else is blocked.
    return status == "active" || status == "grace";
}

} // anonymous namespace

// ============================================================================
// isFeatureAllowed
// ============================================================================

bool RuntimeLicenseGate::isFeatureAllowed(std::string_view feature_name) const {
    std::string unused = {};
    return isFeatureAllowed(feature_name, unused);
}

bool RuntimeLicenseGate::isFeatureAllowed(std::string_view feature_name,
                                           std::string& error_out) const {
    // Step 1: If the feature is not a known Enterprise/Hyperscaler-gated feature,
    // allow it unconditionally.  This covers Community-edition features and any
    // unknown feature name — the gate only restricts the known Enterprise capabilities.
    if (!isEnterpriseFeature(feature_name)) {
        return true;
    }

    // Step 2: Compile-time gate for known Enterprise features.
    // If the binary was not compiled with this feature (e.g. Community-edition
    // binary), deny regardless of runtime license state.
    if (!edition::IsFeatureEnabled(feature_name)) {
        std::ostringstream msg = {};
        msg << "Feature '" << feature_name << "' is not available in the "
            << edition::EDITION_STRING << " edition.";
        if (edition::GetEditionType() == edition::EditionType::COMMUNITY) {
            msg << " Please upgrade to Enterprise or Hyperscaler Edition.";
        }
        error_out = msg.str();
        return false;
    }

    // Step 3: Runtime license check — acquire the lock once for all member access.
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        error_out = std::string("Feature '") + std::string(feature_name)
            + "' is unavailable: license has not been validated yet. "
              "Ensure the server completes startup before using this feature.";
        return false;
    }

    if (!activation_.success || !isStatusAllowed(activation_.status)) {
        error_out = buildDenialMessage(feature_name);
        return false;
    }

    return true;
}

// ============================================================================
// Diagnostics
// ============================================================================

bool RuntimeLicenseGate::isInitialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

std::string RuntimeLicenseGate::licenseStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activation_.status;
}

int RuntimeLicenseGate::graceDaysRemaining() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activation_.grace_days_remaining;
}

std::optional<LicenseData> RuntimeLicenseGate::currentLicense() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return license_;
}

// ============================================================================
// Private helpers
// ============================================================================

std::string RuntimeLicenseGate::buildDenialMessage(std::string_view feature_name) const {
    // PRECONDITION: mutex_ is held by the caller (isFeatureAllowed, runtime path).
    // This method must never be called without holding mutex_.
    // It accesses activation_ and license_ which are protected by the lock.
    // The compile-time denial path is handled inline in isFeatureAllowed().

    std::ostringstream msg = {};
    msg << "Feature '" << feature_name << "' is not available: ";

    // initialized_ is always true when this method is called (the caller
    // already checked the uninitialized case before calling us).
    const std::string& status = activation_.status;

    if (status == "expired") {
        msg << "license has expired.";
    } else if (status == "suspended" || status == "cancelled") {
        msg << "license has been " << status << ".";
    } else if (status == "invalid") {
        msg << "license is invalid.";
    } else if (status == "offline") {
        msg << "unable to reach the license server and offline fallback is disabled.";
    } else {
        msg << "license status is '" << status << "'.";
    }

    if (!activation_.error_message.empty()) {
        msg << " (" << activation_.error_message << ")";
    }

    if (license_.has_value() && !license_->contact_email.empty()) {
        msg << " Please contact " << license_->contact_email << " to renew.";
    } else {
        msg << " Please renew your license or contact your license provider.";
    }

    return msg.str();
}

// ============================================================================
// GateResult::message()
// ============================================================================

std::string GateResult::message() const {
    switch (denial_reason) {
    case LicenseDenialReason::NONE:
        return "Feature is allowed.";
    case LicenseDenialReason::TIER_TOO_LOW:
        return "Current edition tier does not include this feature. "
               "Upgrade to Enterprise or Hyperscaler Edition.";
    case LicenseDenialReason::LICENSE_EXPIRED:
        return "License has expired. Please renew your license.";
    case LicenseDenialReason::SIGNATURE_MISMATCH:
        return "License signature verification failed or license has not been validated. "
               "Ensure the server completes license validation at startup.";
    case LicenseDenialReason::NODE_LIMIT_EXCEEDED:
        return "Number of active nodes exceeds the limit in your license. "
               "Upgrade your license or reduce the number of nodes.";
    case LicenseDenialReason::STORAGE_LIMIT_EXCEEDED:
        return "Storage usage exceeds the limit in your license. "
               "Upgrade your license or reduce storage usage.";
    }
    return "Unknown denial reason.";
}

// ============================================================================
// RuntimeLicenseGate::checkFeature()
// ============================================================================

GateResult RuntimeLicenseGate::checkFeature(std::string_view feature_name) const {
    GateResult result;

    // Step 1: Not a gated feature → always allowed.
    if (!isEnterpriseFeature(feature_name)) {
        result.allowed       = true;
        result.denial_reason = LicenseDenialReason::NONE;
        return result;
    }

    // Step 2: Compile-time gate (binary edition check).
    if (!edition::IsFeatureEnabled(feature_name)) {
        result.allowed       = false;
        result.denial_reason = LicenseDenialReason::TIER_TOO_LOW;
        return result;
    }

    // Step 3: Runtime license state.
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        result.allowed       = false;
        result.denial_reason = LicenseDenialReason::SIGNATURE_MISMATCH;
        return result;
    }

    if (!activation_.success || !isStatusAllowed(activation_.status)) {
        result.allowed = false;
        const auto& status = activation_.status;
        if (status == "expired") {
            result.denial_reason = LicenseDenialReason::LICENSE_EXPIRED;
        } else {
            result.denial_reason = LicenseDenialReason::SIGNATURE_MISMATCH;
        }
        return result;
    }

    result.allowed       = true;
    result.denial_reason = LicenseDenialReason::NONE;
    return result;
}

} // namespace license
} // namespace themis