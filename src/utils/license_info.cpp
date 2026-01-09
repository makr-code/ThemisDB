/*
 * ThemisDB License Information Implementation
 * ============================================
 * Provides access to compile-time embedded license data.
 */

#include "themis/license_info.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace themis {
namespace license {

// ============================================================================
// CONSTANTS
// ============================================================================

// Magic values for license expiry calculation
constexpr int PERPETUAL_LICENSE_DAYS = 999999;
constexpr int INVALID_LICENSE_DAYS = -999999;

// ============================================================================
// COMPILE-TIME LICENSE DATA (Injected by CMake)
// ============================================================================
// These macros are defined by CMake during the build process.
// If no license file is provided, they default to empty strings.

#ifndef THEMIS_LICENSE_ORG_NAME
#define THEMIS_LICENSE_ORG_NAME ""
#endif

#ifndef THEMIS_LICENSE_ORG_ID
#define THEMIS_LICENSE_ORG_ID ""
#endif

#ifndef THEMIS_LICENSE_CONTACT_EMAIL
#define THEMIS_LICENSE_CONTACT_EMAIL ""
#endif

#ifndef THEMIS_LICENSE_KEY
#define THEMIS_LICENSE_KEY ""
#endif

#ifndef THEMIS_LICENSE_EDITION
#define THEMIS_LICENSE_EDITION ""
#endif

#ifndef THEMIS_LICENSE_ISSUED_DATE
#define THEMIS_LICENSE_ISSUED_DATE ""
#endif

#ifndef THEMIS_LICENSE_EXPIRY_DATE
#define THEMIS_LICENSE_EXPIRY_DATE ""
#endif

#ifndef THEMIS_LICENSE_MAX_NODES
#define THEMIS_LICENSE_MAX_NODES -1
#endif

#ifndef THEMIS_LICENSE_MAX_CORES
#define THEMIS_LICENSE_MAX_CORES -1
#endif

#ifndef THEMIS_LICENSE_MAX_STORAGE_TB
#define THEMIS_LICENSE_MAX_STORAGE_TB -1
#endif

#ifndef THEMIS_LICENSE_BUILD_ID
#define THEMIS_LICENSE_BUILD_ID ""
#endif

#ifndef THEMIS_LICENSE_BUILD_TIMESTAMP
#define THEMIS_LICENSE_BUILD_TIMESTAMP ""
#endif

#ifndef THEMIS_LICENSE_SIGNATURE
#define THEMIS_LICENSE_SIGNATURE ""
#endif

// ============================================================================
// LICENSE DATA ACCESS IMPLEMENTATION
// ============================================================================

std::optional<LicenseData> getEmbeddedLicense() {
    // Check if license key is present
    std::string license_key = THEMIS_LICENSE_KEY;
    if (license_key.empty()) {
        return std::nullopt;
    }
    
    LicenseData data;
    data.organization_name = THEMIS_LICENSE_ORG_NAME;
    data.organization_id = THEMIS_LICENSE_ORG_ID;
    data.contact_email = THEMIS_LICENSE_CONTACT_EMAIL;
    data.license_key = license_key;
    data.edition = THEMIS_LICENSE_EDITION;
    data.issued_date = THEMIS_LICENSE_ISSUED_DATE;
    data.expiry_date = THEMIS_LICENSE_EXPIRY_DATE;
    data.max_nodes = THEMIS_LICENSE_MAX_NODES;
    data.max_cores = THEMIS_LICENSE_MAX_CORES;
    data.max_storage_tb = THEMIS_LICENSE_MAX_STORAGE_TB;
    data.build_id = THEMIS_LICENSE_BUILD_ID;
    data.build_timestamp = THEMIS_LICENSE_BUILD_TIMESTAMP;
    data.signature = THEMIS_LICENSE_SIGNATURE;
    
    return data;
}

bool hasEmbeddedLicense() {
    std::string license_key = THEMIS_LICENSE_KEY;
    return !license_key.empty();
}

std::string formatLicenseInfo(const LicenseData& license) {
    std::ostringstream oss;
    
    oss << "\n";
    oss << "===============================================================================\n";
    oss << "                      THEMIS DATABASE LICENSE INFORMATION                       \n";
    oss << "===============================================================================\n";
    oss << "\n";
    
    // Organization Information
    oss << "ORGANIZATION:\n";
    oss << "  Name:               " << license.organization_name << "\n";
    if (!license.organization_id.empty()) {
        oss << "  Organization ID:    " << license.organization_id << "\n";
    }
    if (!license.contact_email.empty()) {
        oss << "  Contact Email:      " << license.contact_email << "\n";
    }
    oss << "\n";
    
    // License Details
    oss << "LICENSE:\n";
    oss << "  License Key:        " << license.license_key << "\n";
    oss << "  Edition:            " << license.edition << "\n";
    oss << "  Issued Date:        " << license.issued_date << "\n";
    oss << "  Expiry Date:        " << license.expiry_date << "\n";
    
    // Calculate days until expiry
    int days = getDaysUntilExpiry(license);
    if (days >= 0) {
        oss << "  Days Until Expiry:  " << days << " days\n";
    } else {
        oss << "  Status:             EXPIRED (" << (-days) << " days ago)\n";
    }
    oss << "\n";
    
    // License Limits
    oss << "LICENSE LIMITS:\n";
    if (license.max_nodes == -1) {
        oss << "  Max Nodes:          Unlimited\n";
    } else {
        oss << "  Max Nodes:          " << license.max_nodes << "\n";
    }
    
    if (license.max_cores == -1) {
        oss << "  Max Cores:          Unlimited\n";
    } else {
        oss << "  Max Cores:          " << license.max_cores << "\n";
    }
    
    if (license.max_storage_tb == -1) {
        oss << "  Max Storage:        Unlimited\n";
    } else {
        oss << "  Max Storage:        " << license.max_storage_tb << " TB\n";
    }
    oss << "\n";
    
    // Build Information
    if (!license.build_id.empty() || !license.build_timestamp.empty()) {
        oss << "BUILD INFORMATION:\n";
        if (!license.build_id.empty()) {
            oss << "  Build ID:           " << license.build_id << "\n";
        }
        if (!license.build_timestamp.empty()) {
            oss << "  Build Timestamp:    " << license.build_timestamp << "\n";
        }
        oss << "\n";
    }
    
    // Signature verification
    if (!license.signature.empty()) {
        bool valid = verifyLicenseSignature(license);
        oss << "SIGNATURE:\n";
        oss << "  Status:             " << (valid ? "VALID" : "INVALID") << "\n";
        oss << "  Signature:          " << license.signature.substr(0, 32) << "...\n";
        oss << "\n";
    }
    
    oss << "===============================================================================\n";
    
    return oss.str();
}

bool isLicenseValid(const LicenseData& license) {
    return getDaysUntilExpiry(license) >= 0;
}

int getDaysUntilExpiry(const LicenseData& license) {
    // Parse expiry date (ISO 8601 format: YYYY-MM-DD)
    if (license.expiry_date.empty() || license.expiry_date == "9999-12-31") {
        // No expiry or perpetual license
        return PERPETUAL_LICENSE_DAYS;
    }
    
    try {
        std::tm expiry_tm = {};
        std::istringstream ss(license.expiry_date);
        ss >> std::get_time(&expiry_tm, "%Y-%m-%d");
        
        if (ss.fail()) {
            // Invalid date format, assume expired
            return INVALID_LICENSE_DAYS;
        }
        
        // Get current time (thread-safe)
        auto now = std::chrono::system_clock::now();
        std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        
        // Use gmtime for thread-safe conversion (UTC)
        std::tm now_tm = {};
#ifdef _WIN32
        gmtime_s(&now_tm, &now_t);
#else
        gmtime_r(&now_t, &now_tm);
#endif
        
        // Convert to time_t for comparison
        std::time_t expiry_t = std::mktime(&expiry_tm);
        std::time_t now_time = std::mktime(&now_tm);
        
        // Calculate difference in days
        double diff_seconds = std::difftime(expiry_t, now_time);
        int diff_days = static_cast<int>(diff_seconds / (60 * 60 * 24));
        
        return diff_days;
    } catch (...) {
        // Error parsing date, assume expired
        return INVALID_LICENSE_DAYS;
    }
}

bool verifyLicenseSignature(const LicenseData& license) {
    // If no signature present, consider it valid
    if (license.signature.empty()) {
        return true;
    }
    
    // TODO: Implement actual signature verification using RSA/SHA-256
    // SECURITY WARNING: This is a placeholder implementation!
    // For production use, implement proper RSA signature verification:
    // 1. Construct the data to sign (license_key + org + dates + limits)
    // 2. Verify RSA signature using public key embedded in binary
    // 3. Return true if signature matches
    //
    // Example implementation:
    // std::string data_to_verify = license.license_key + license.organization_name +
    //                               license.issued_date + license.expiry_date;
    // return verify_rsa_signature(data_to_verify, license.signature, PUBLIC_KEY);
    
    // PLACEHOLDER: For now, just check if signature is present
    // This does NOT provide real security - implement proper verification!
    return !license.signature.empty();
}

} // namespace license
} // namespace themis
