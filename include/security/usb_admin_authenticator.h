/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            usb_admin_authenticator.h                          ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:58:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     164                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3a19581dc  2026-01-07  Address code review findings - improve security warnings ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <optional>
#include <memory>
#include <chrono>
#include <vector>
#include <mutex>

namespace themis {
namespace security {

/// USB Admin License Structure stored on encrypted USB device
struct USBAdminLicense {
    std::string license_key;           // Unique license key (e.g., THEMIS-ENT-XXXXX-ADMIN)
    std::string organization;          // Organization name
    std::string hardware_id;           // Hardware ID this license is bound to (CPU serial, MAC, etc.)
    std::chrono::system_clock::time_point issued_date;
    std::chrono::system_clock::time_point expiry_date;
    std::vector<std::string> admin_scopes; // Allowed admin scopes
    std::string signature;             // RSA signature to prevent tampering
    
    bool isValid() const;
    bool isExpired() const;
    bool matchesHardware(const std::string& current_hw_id) const;
};

/// Configuration for USB Admin Authentication
struct USBAdminConfig {
    std::string mount_path = "/mnt/themis-admin";  // Where encrypted USB should be mounted
    std::string license_file = "themis_admin.lic"; // License file name on USB
    std::string challenge_file = "challenge.dat";   // Challenge file for replay protection
    bool require_usb_for_admin = true;              // Enforce USB requirement
    bool silent_failure = true;                     // Silent failure mode (no error messages)
    std::string audit_log_path = "/var/log/themis/admin_auth_audit.log";
    std::chrono::seconds challenge_ttl{300};        // Challenge valid for 5 minutes
    uint32_t max_validation_attempts = 3;           // Max attempts before lockout
    std::chrono::seconds lockout_duration{600};     // 10 minute lockout
    
    // Configurable list of scopes that require USB authentication
    // Default: admin scopes that require USB validation
    std::vector<std::string> usb_protected_scopes = {
        "admin",
        "config:write",
        "cdc:admin",
        "admin:backup",
        "admin:restore",
        "admin:topology",
        "admin:rebalance"
    };
};

/// USB Admin Authenticator
/// Validates that an encrypted USB with proper license is present before allowing admin operations
class USBAdminAuthenticator {
public:
    explicit USBAdminAuthenticator(const USBAdminConfig& config);
    ~USBAdminAuthenticator();
    
    // Non-copyable, movable
    USBAdminAuthenticator(const USBAdminAuthenticator&) = delete;
    USBAdminAuthenticator& operator=(const USBAdminAuthenticator&) = delete;
    USBAdminAuthenticator(USBAdminAuthenticator&&) noexcept;
    USBAdminAuthenticator& operator=(USBAdminAuthenticator&&) noexcept;
    
    /// Initialize and perform initial USB detection
    bool initialize();
    
    /// Check if USB with valid admin license is present
    /// Returns true if admin operations are allowed
    bool isAdminUSBPresent() const;
    
    /// Validate admin operation is allowed
    /// @param scope Admin scope being requested (e.g., "admin", "config:write")
    /// @param user_id User attempting operation (for audit)
    /// @return true if operation is allowed
    bool validateAdminOperation(const std::string& scope, const std::string& user_id);
    
    /// Get current USB license (if present)
    std::optional<USBAdminLicense> getCurrentLicense() const;
    
    /// Manually refresh USB status (automatically done periodically)
    bool refreshUSBStatus();
    
    /// Check if currently in lockout mode due to failed attempts
    bool isLockedOut() const;
    
    /// Get metrics for monitoring
    struct Metrics {
        uint64_t admin_ops_allowed = 0;
        uint64_t admin_ops_denied_no_usb = 0;
        uint64_t admin_ops_denied_invalid_license = 0;
        uint64_t admin_ops_denied_expired = 0;
        uint64_t admin_ops_denied_lockout = 0;
        uint64_t usb_mount_checks = 0;
        uint64_t usb_mount_detected = 0;
        std::chrono::system_clock::time_point last_valid_check;
    };
    
    Metrics getMetrics() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    USBAdminConfig config_;
    mutable std::mutex mutex_;
    mutable Metrics metrics_;
    
    // Validation state
    uint32_t failed_attempts_ = 0;
    std::chrono::system_clock::time_point lockout_until_;
    std::optional<USBAdminLicense> current_license_;
    std::chrono::system_clock::time_point last_usb_check_;
    
    /// Internal: Check if USB device is mounted at expected path
    bool checkUSBMounted() const;
    
    /// Internal: Load and parse license file from USB
    std::optional<USBAdminLicense> loadLicenseFromUSB() const;
    
    /// Internal: Validate license signature
    bool validateLicenseSignature(const USBAdminLicense& license) const;
    
    /// Internal: Get hardware ID for this system
    std::string getSystemHardwareID() const;
    
    /// Internal: Create challenge for replay protection
    std::string createChallenge() const;
    
    /// Internal: Validate challenge response
    bool validateChallengeResponse(const std::string& challenge, const std::string& response) const;
    
    /// Internal: Audit log entry
    void auditLog(const std::string& event, const std::string& details, const std::string& user_id) const;
};

} // namespace security
} // namespace themis
