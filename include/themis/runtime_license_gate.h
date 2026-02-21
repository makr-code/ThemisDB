/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            runtime_license_gate.h                             ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 18:43:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     185                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB Runtime License Gate
 * ==============================
 * Bridges the gap between compile-time edition feature flags and the runtime
 * license validity returned by LicenseClient.
 *
 * Usage (in any Enterprise/Hyperscaler code-path):
 *
 *   #include "themis/runtime_license_gate.h"
 *
 *   // At server startup (once, after license validation):
 *   themis::license::RuntimeLicenseGate::instance().initialize(
 *       activationResult, licenseData);
 *
 *   // At any feature call-site:
 *   if (!themis::license::RuntimeLicenseGate::instance()
 *           .isFeatureAllowed("field_encryption")) {
 *       return {Error::LicenseRequired, "Field-level encryption requires..."};
 *   }
 */

#ifndef THEMIS_RUNTIME_LICENSE_GATE_H
#define THEMIS_RUNTIME_LICENSE_GATE_H

#include "themis/license_info.h"
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace license {

// ============================================================================
// RuntimeLicenseGate
// ============================================================================

/**
 * @brief Singleton that enforces runtime license validity for Enterprise /
 *        Hyperscaler features.
 *
 * The gate evaluates **two independent conditions** before allowing a feature:
 *
 *   1. **Compile-time gate** – `edition::IsFeatureEnabled(feature_name)` must
 *      return true.  If the binary was built as Community edition, the feature
 *      is never allowed regardless of any license.
 *
 *   2. **Runtime gate** – The current license (set by `initialize()`) must be
 *      valid.  An expired or invalid license blocks all Enterprise/Hyperscaler
 *      features, even in an ENTERPRISE-edition binary.
 *
 * Grace-period handling: when the license server is temporarily unreachable,
 * `LicenseClient` sets `status = "grace"` and `grace_days_remaining > 0`.
 * The gate treats grace-period activations as valid and logs a warning.
 *
 * Community edition note: Community-edition features are always allowed
 * regardless of license state — Community users should never be blocked.
 */
class RuntimeLicenseGate {
public:
    /// Retrieve the process-wide singleton.
    static RuntimeLicenseGate& instance();

    // Non-copyable / non-movable (singleton)
    RuntimeLicenseGate(const RuntimeLicenseGate&)            = delete;
    RuntimeLicenseGate& operator=(const RuntimeLicenseGate&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Initialise the gate with the result of a LicenseClient::activate()
     *        or validate() call.
     *
     * Must be called once at server startup (after license validation) before
     * any feature checks are performed.  Thread-safe.
     *
     * @param activation  Result from LicenseClient::activate() / validate().
     * @param license     Optional authoritative license data (may come from
     *                    activation.refreshed_license or getEmbeddedLicense()).
     */
    void initialize(const LicenseActivationResult& activation,
                    const std::optional<LicenseData>& license = std::nullopt);

    /**
     * @brief Re-initialise the gate (used after a periodic license refresh).
     *        Equivalent to `initialize()`.
     */
    void update(const LicenseActivationResult& activation,
                const std::optional<LicenseData>& license = std::nullopt);

    // -------------------------------------------------------------------------
    // Feature checking
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true if the named feature is allowed at runtime.
     *
     * The following features are gated:
     *   - "enterprise_plugins"
     *   - "multi_master"
     *   - "field_encryption"
     *   - "rbac"
     *   - "hsm"
     *
     * Features not on the list (e.g. Community-edition features) are always
     * allowed — the gate only restricts Enterprise/Hyperscaler capabilities.
     *
     * @param feature_name  Feature name (see edition::IsFeatureEnabled()).
     * @return true if the feature may be used; false if blocked.
     */
    bool isFeatureAllowed(std::string_view feature_name) const;

    /**
     * @brief Like isFeatureAllowed() but also sets @p error_out on failure with
     *        a human-readable message (license key, expiry info, contact email).
     *
     * @param feature_name  Feature to check.
     * @param error_out     Populated with an error string when returning false.
     * @return true if allowed; false with @p error_out filled in if blocked.
     */
    bool isFeatureAllowed(std::string_view feature_name,
                          std::string& error_out) const;

    // -------------------------------------------------------------------------
    // Diagnostics
    // -------------------------------------------------------------------------

    /// Returns true if the gate has been initialized.
    bool isInitialized() const;

    /// Returns the current license status string ("active", "grace", "expired",
    /// "invalid", "offline", …).
    std::string licenseStatus() const;

    /// Returns the number of grace days remaining (0 if not in grace period).
    int graceDaysRemaining() const;

    /// Returns the active license data (if available).
    std::optional<LicenseData> currentLicense() const;

private:
    RuntimeLicenseGate() = default;
    ~RuntimeLicenseGate() = default;

    /// Build a user-facing denial message for a blocked feature.
    std::string buildDenialMessage(std::string_view feature_name) const;

    mutable std::mutex           mutex_;
    bool                         initialized_  = false;
    LicenseActivationResult      activation_;
    std::optional<LicenseData>   license_;
};

} // namespace license
} // namespace themis

#endif // THEMIS_RUNTIME_LICENSE_GATE_H
