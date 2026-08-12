/**
 * @file usb_admin_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <optional>
#include <memory>
#include <chrono>
#include <vector>
#include <mutex>
#include <functional>
#include <unordered_map>

#include "security/usb_volume_hardening.h"

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

    // ── USB Volume Hardening (defence against FAT manipulation) ───────────────

    /// When true, refreshUSBStatus() rejects the USB unless the filesystem is
    /// mounted read-only.  A read-only mount prevents any process running on the
    /// host from writing to the stick while it is in use.
    bool require_readonly_mount = false;

    /// If non-empty, refreshUSBStatus() rejects the USB unless the SHA-256 hash
    /// of the license file matches this value (lowercase hex, 64 chars).
    /// Provision this value from a secure, server-side configuration store.
    /// Any FAT-level modification of the license file will be detected.
    std::string expected_volume_hash;

    /// If non-empty, refreshUSBStatus() rejects the USB unless the device serial
    /// number matches this value.  This prevents a `dd` clone of the stick from
    /// being accepted on the same or a different host.
    std::string expected_usb_serial;
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
        uint64_t usb_denied_not_readonly = 0;      ///< Rejected: filesystem not mounted read-only
        uint64_t usb_denied_volume_hash_mismatch = 0; ///< Rejected: FAT-level file tampering detected
        uint64_t usb_denied_serial_mismatch = 0;   ///< Rejected: cloned USB device detected
        std::chrono::system_clock::time_point last_valid_check;
    };
    
    Metrics getMetrics() const;

    /**
     * @brief License verifier callback type.
     *
     * When injected via setLicenseVerifierFn(), this function completely
     * replaces the hardware-binding check (matchesHardware) and the
     * RSA signature verification (validateLicenseSignature) inside
     * refreshUSBStatus().  It receives the loaded license and the
     * current system hardware ID and must return true iff the license
     * is considered valid for this host.
     *
     * Injection is the primary mechanism for tests and alternative
     * production integrations (e.g. HMAC-based license server) to bypass
     * the embedded placeholder RSA public key.
     *
     * Passing nullptr clears the override and restores the built-in RSA
     * verification path.
     */
    using LicenseVerifierFn = std::function<bool(const USBAdminLicense&,
                                                  const std::string& hw_id)>;

    /**
     * @brief Inject a custom license verifier (replaces hardware + RSA checks).
     *
     * @param fn  Verifier callback; pass nullptr to restore built-in behaviour.
     */
    void setLicenseVerifierFn(LicenseVerifierFn fn);

    /**
     * @brief Generate a one-time cryptographic challenge for replay-protected auth.
     *
     * The challenge is a CSPRNG-generated 32-byte value encoded as a 64-character
     * lowercase hex string.  The challenge is registered internally with a
     * timestamp; `validateChallengeResponse()` enforces the TTL and one-time-use
     * invariant.
     *
     * @return 64-character lowercase hex challenge string.
     */
    std::string createChallenge() const;

    /**
     * @brief Validate a challenge-response round-trip.
     *
     * The expected response is HMAC-SHA256(key=license_key, message=challenge),
     * hex-encoded.  Only the holder of the USB license (and therefore its
     * `license_key`) can produce a matching response.
     *
     * Security properties:
     *   - **Replay prevention**: each challenge is one-time-use.
     *   - **TTL enforcement**: challenges expire after `challenge_ttl` seconds.
     *   - **Constant-time comparison**: prevents timing side-channels.
     *
     * @param challenge  Challenge string previously returned by `createChallenge()`.
     * @param response   Hex-encoded HMAC-SHA256 response.
     * @return true if response is valid; false if unknown challenge, expired,
     *         no license present, or HMAC mismatch.
     */
    bool validateChallengeResponse(const std::string& challenge,
                                    const std::string& response) const;

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

    // Challenge tracking: maps challenge hex string → issued timestamp.
    // Challenges are one-time-use; used ones are erased after validation.
    // Expired entries (older than challenge_ttl) are purged lazily.
    mutable std::unordered_map<std::string, std::chrono::system_clock::time_point> issued_challenges_;
    
    /// Internal: Check if USB device is mounted at expected path
    bool checkUSBMounted() const;
    
    /// Internal: Load and parse license file from USB
    std::optional<USBAdminLicense> loadLicenseFromUSB() const;
    
    /// Internal: Validate license signature
    bool validateLicenseSignature(const USBAdminLicense& license) const;
    
    /// Internal: Get hardware ID for this system
    std::string getSystemHardwareID() const;
    
    /// Internal: Audit log entry
    void auditLog(const std::string& event, const std::string& details, const std::string& user_id) const;
};

} // namespace security
} // namespace themis
