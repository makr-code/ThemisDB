/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            license_info.h                                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:38:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     214                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b6c51ef3e  2026-02-20  Themis Core Framework – Production Readiness (All 7 Phase... ║
    • 3a69428e4  2026-01-09  Add license data embedding infrastructure and documentation ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB License Information
 * ============================
 * Compile-time embedded license and company data.
 * This header provides runtime access to license information that was
 * embedded into the binary during the build process.
 */

#ifndef THEMIS_LICENSE_INFO_H
#define THEMIS_LICENSE_INFO_H

#include <chrono>
#include <functional>
#include <optional>
#include <string>

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
    int max_nodes;
    int max_cores;
    int max_storage_tb;
    
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
std::optional<LicenseData> getEmbeddedLicense();

/**
 * Check if this build has embedded license data
 */
bool hasEmbeddedLicense();

/**
 * Get a human-readable summary of the license information
 * suitable for logging at server startup
 */
std::string formatLicenseInfo(const LicenseData& license);

/**
 * Verify license validity (expiry date check)
 * Returns true if license is currently valid
 */
bool isLicenseValid(const LicenseData& license);

/**
 * Get number of days until license expires
 * Returns negative value if already expired
 */
int getDaysUntilExpiry(const LicenseData& license);

/**
 * Verify license signature (if present)
 * Returns true if signature is valid or no signature present
 */
bool verifyLicenseSignature(const LicenseData& license);

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
class LicenseClient {
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

} // namespace license
} // namespace themis

#endif // THEMIS_LICENSE_INFO_H
