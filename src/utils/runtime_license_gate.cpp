/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            runtime_license_gate.cpp                           ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 19:20:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     210                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    std::string unused;
    return isFeatureAllowed(feature_name, unused);
}

bool RuntimeLicenseGate::isFeatureAllowed(std::string_view feature_name,
                                           std::string& error_out) const {
    // Step 1: Compile-time gate.
    // If the binary was not compiled with this feature, it is never allowed.
    // Build the denial message inline here — no member state access needed.
    if (!edition::IsFeatureEnabled(feature_name)) {
        std::ostringstream msg;
        msg << "Feature '" << feature_name << "' is not available in the "
            << edition::EDITION_STRING << " edition.";
        if (edition::GetEditionType() == edition::EditionType::COMMUNITY) {
            msg << " Please upgrade to Enterprise or Hyperscaler Edition.";
        }
        error_out = msg.str();
        return false;
    }

    // Step 2: If the feature is not an Enterprise-gated one, it's always OK.
    // (The compile-time gate already handled it for Enterprise features.)
    if (!isEnterpriseFeature(feature_name)) {
        return true;
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

    std::ostringstream msg;
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

} // namespace license
} // namespace themis
