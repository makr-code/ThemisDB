/**
 * @file license_info.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: license_info.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB License Information
 * ============================
 * Compile-time embedded license and company data.
 * This header provides runtime access to license information that was
 * embedded into the binary during the build process.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include "themis/export.h"

namespace themis {
namespace license {

// ============================================================================
// LICENSE DATA STRUCTURE
// ============================================================================

struct LicenseData {
    // Organization information
    std::string organization_name;
    std::string organization_id;
    std::string contact_email;
    
    // License key and edition
    std::string license_key;
    std::string edition;  // MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER
    
    // Validity period
    std::string issued_date;      // ISO 8601 format: YYYY-MM-DD
    std::string expiry_date;      // ISO 8601 format: YYYY-MM-DD
    
    // License limits
    int max_nodes       = -1;     // ✓ SECURITY FIX #2: Initialized
    int max_cores       = -1;     // ✓ SECURITY FIX #3: Initialized
    int max_storage_tb  = -1;     // ✓ SECURITY FIX #4: Initialized
    
    // Build information
    std::string build_id;
    std::string build_timestamp;
    
    // Optional signature for verification
    std::string signature;
};

// ============================================================================
// LICENSE INFORMATION ACCESS
// ============================================================================

/**
 * Get the embedded license data from this build
 * Returns empty optional if no license data was embedded
 */
THEMIS_BASE_API std::optional<LicenseData> getEmbeddedLicense();

/**
 * Check if this build has embedded license data
 */
THEMIS_BASE_API bool hasEmbeddedLicense();

/**
 * Get a human-readable summary of the license information
 * suitable for logging at server startup
 */
THEMIS_BASE_API std::string formatLicenseInfo(const LicenseData& license);

/**
 * Verify license validity (expiry date check)
 * Returns true if license is currently valid
 */
THEMIS_BASE_API bool isLicenseValid(const LicenseData& license);

/**
 * Get number of days until license expires
 * Returns negative value if already expired
 */
THEMIS_BASE_API int getDaysUntilExpiry(const LicenseData& license);

/**
 * Verify license signature (if present)
 * Returns true if signature is valid or no signature present
 */
THEMIS_BASE_API bool verifyLicenseSignature(const LicenseData& license);

// ============================================================================
// LICENSE CLIENT – Online / Offline Activation (Phase 6 – v1.7.0)
// ============================================================================

/**
 * @brief Result of a license activation or validation request.
 */
struct LicenseActivationResult {
    bool        success       = false;
    std::string status;         ///< "active", "expired", "invalid", "grace", "offline"
    std::string error_message;
    int         grace_days_remaining = 0; ///< Days left in the grace period (0 = no grace)
    std::optional<LicenseData> refreshed_license; ///< Updated license if the server returned one
};

/**
 * @brief Configuration for the license server client.
 */
struct LicenseClientConfig {
    std::string server_url;            ///< e.g. "https://license.themisdb.io/v1"
    std::string api_key;               ///< Organisation API key for online checks
    std::chrono::seconds timeout{10};  ///< HTTP request timeout
    bool        allow_offline = true;  ///< Fall back to embedded license when server unreachable
    int         grace_period_days = 7; ///< Days to allow after online check fails (offline grace)
};

/**
 * @brief Thin client that activates and periodically validates a ThemisDB license
 *        against the ThemisDB license server.
 *
 * The client operates in three modes:
 *
 *   1. **Online mode** – Contacts the license server via HTTPS to activate or
 *      refresh the license.  Returns the authoritative LicenseData from the server.
 *
 *   2. **Grace mode** – If the online check fails (network unavailable), the
 *      previously cached / embedded license is used for up to
 *      `config.grace_period_days` days.
 *
 *   3. **Offline mode** – If `allow_offline = true`, permanently falls back to
 *      the embedded license without ever contacting the server.
 */
class THEMIS_BASE_API LicenseClient {
public:
    explicit LicenseClient(const LicenseClientConfig& config);
    ~LicenseClient();

    // Non-copyable
    LicenseClient(const LicenseClient&)            = delete;
    LicenseClient& operator=(const LicenseClient&) = delete;

    /**
     * @brief Activate the license with the server.
     *
     * Sends the embedded license key + machine fingerprint to the license
     * server.  On success the server returns an authoritative LicenseData
     * (possibly with an updated expiry date).
     *
     * @return Activation result with status and optional refreshed license.
     */
    LicenseActivationResult activate();

    /**
     * @brief Validate the currently active license (online or offline).
     *
     * If an online validation was previously successful and the cached result
     * is still fresh, this returns immediately.  Otherwise it re-contacts the
     * server.  Falls back to grace/offline mode as configured.
     *
     * @return Validation result.
     */
    LicenseActivationResult validate();

    /**
     * @brief Return the currently cached license, if any.
     */
    std::optional<LicenseData> getCachedLicense() const;

    /**
     * @brief Force-refresh the cached license from the server.
     */
    LicenseActivationResult refresh();

    /**
     * @brief Machine fingerprint used to bind licenses.
     *
     * Computed from stable hardware identifiers (MAC address, CPU ID, etc.).
     * This value is sent to the license server during activation.
     */
    static std::string getMachineFingerprint();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// LICENSE INFO HELPER VIEW (v1.7.1)
// ============================================================================

/**
 * @brief Helper view over LicenseData providing computed runtime properties.
 *
 * Wraps a LicenseData reference to expose properties that are computed
 * from the raw data, such as the remaining grace period after expiry.
 *
 * Example:
 * @code
 *   auto lic = getEmbeddedLicense();
 *   if (lic) {
 *       LicenseInfo info(*lic);
 *       int days = info.remaining_grace_days();
 *   }
 * @endcode
 */
class THEMIS_BASE_API LicenseInfo {
public:
    /// Default grace period used when none is specified.
    static constexpr int kDefaultGracePeriodDays = 7;

    /**
     * @brief Construct a LicenseInfo view over @p data.
     *
     * @param data              The license data to inspect.
     * @param grace_period_days The configured grace period (days).
     *                          Defaults to kDefaultGracePeriodDays.
     */
    explicit LicenseInfo(const LicenseData& data,
                         int grace_period_days = kDefaultGracePeriodDays);

    /**
     * @brief Returns the number of days remaining in the grace window.
     *
     * Interpretation:
     *   - If the license has **not yet expired**, returns @c grace_period_days
     *     (the full grace window is available after expiry).
     *   - If the license **has expired** and the expiry occurred within the
     *     grace window, returns the remaining days in that window.
     *   - If the license has **fully expired** (beyond the grace window),
     *     returns 0.
     *   - If @c expiry_date is empty or unparseable, returns 0.
     *
     * @return Non-negative integer days remaining in grace period.
     */
    int remaining_grace_days() const;

    /// Accessor for the underlying data.
    const LicenseData& data() const noexcept { return data_; }

private:
    const LicenseData& data_;
    int grace_period_days_;
};

} // namespace license
} // namespace themis
