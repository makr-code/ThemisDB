/*
 * ThemisDB License Information
 * ============================
 * Compile-time embedded license and company data.
 * This header provides runtime access to license information that was
 * embedded into the binary during the build process.
 */

#ifndef THEMIS_LICENSE_INFO_H
#define THEMIS_LICENSE_INFO_H

#include <string>
#include <optional>
#include <chrono>

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

} // namespace license
} // namespace themis

#endif // THEMIS_LICENSE_INFO_H
