/**
 * @file runtime_license_gate.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.41
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

#pragma once

#include "themis/export.h"
#include "themis/license_info.h"
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace license {

// ============================================================================
// LicenseDenialReason — structured reason code for feature gate denials
// ============================================================================

/**
 * @brief Reason a feature was denied by the runtime license gate.
 *
 * Used by GateResult to provide structured error information that callers
 * can act on programmatically (e.g. display targeted upgrade messages).
 */
enum class LicenseDenialReason {
    NONE,                   ///< Feature is allowed (no denial)
    TIER_TOO_LOW,           ///< Current edition does not include this feature
    LICENSE_EXPIRED,        ///< License has passed its expiry date
    SIGNATURE_MISMATCH,     ///< License signature verification failed or gate uninitialized
    NODE_LIMIT_EXCEEDED,    ///< Number of active nodes exceeds the license cap
    STORAGE_LIMIT_EXCEEDED  ///< Storage usage exceeds the license cap
};

// ============================================================================
// GateResult — result of a RuntimeLicenseGate::checkFeature() call
// ============================================================================

/**
 * @brief Result of a feature-gate check.
 *
 * Extends the simple bool returned by isFeatureAllowed() with a
 * machine-readable denial reason code and a human-readable message.
 *
 * Backward-compatible addition: existing callers that use isFeatureAllowed()
 * are unaffected.
 *
 * Example:
 * @code
 *   auto result = RuntimeLicenseGate::instance().checkFeature("field_encryption");
 *   if (!result) {
 *       LOG_WARN("Denied: {} — {}", result.denial_reason_name(), result.message());
 *   }
 * @endcode
 */
struct THEMIS_BASE_API GateResult {
    /// True if the feature is allowed; false if denied.
    bool allowed = false;

    /// Structured denial reason (NONE when allowed).
    LicenseDenialReason denial_reason = LicenseDenialReason::NONE;

    /**
     * @brief Returns a locale-independent English description of the denial
     *        reason suitable for logging.
     *
     * Returns "Feature is allowed." when @c allowed is true.
     */
    std::string message() const;

    /// Convenience implicit conversion to bool.
    explicit operator bool() const noexcept { return allowed; }
};

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

    /**
     * @brief Check whether a named feature is allowed, returning a GateResult
     *        that includes the structured denial reason for programmatic handling.
     *
     * Prefer this over isFeatureAllowed() when the caller needs to distinguish
     * between different denial reasons (e.g. show an upgrade prompt for
     * TIER_TOO_LOW vs. a "renew your license" prompt for LICENSE_EXPIRED).
     *
     * @param feature_name  Feature name (see edition::IsFeatureEnabled()).
     * @return GateResult with allowed flag and denial_reason set.
     */
    GateResult checkFeature(std::string_view feature_name) const;

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
