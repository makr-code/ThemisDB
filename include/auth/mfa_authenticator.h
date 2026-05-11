/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mfa_authenticator.h                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:44:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     220                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 94bb63df8d  2026-03-12  feat(auth): TOTP/MFA configurable window enforcement and ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include &lt;optional&gt;
#include <nlohmann/json.hpp>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {
class AuthAuditLogger;
class AuthMetrics;

/**
 * @brief TOTP-based Multi-Factor Authentication
 * 
 * Implements Time-based One-Time Password (TOTP) according to RFC 6238
 * for multi-factor authentication. Provides:
 * - TOTP secret generation and storage
 * - TOTP code validation with time window
 * - Recovery codes for account recovery
 * - QR code generation for mobile app setup
 * 
 * Security considerations:
 * - Secrets stored encrypted in database
 * - Time window prevents replay attacks
 * - Rate limiting prevents brute force
 * - Recovery codes single-use only
 * 
 * Compliance: SOC 2 CC6.1, NIST SP 800-63B Level 2
 */
class MFAAuthenticator {
public:
    /**
     * @brief MFA configuration
     */
    struct Config {
        // TOTP time step in seconds (default: 30s per RFC 6238)
        int time_step_seconds = 30;
        
        // Number of digits in TOTP code (6 or 8)
        int code_length = 6;
        
        // Time window tolerance (accept codes from N steps before/after)
        int time_window = 1;
        
        // Maximum allowed time window steps; the constructor enforces that
        // time_window may not exceed this value, capped at an absolute hard
        // limit of 2 to prevent wide windows from weakening replay resistance.
        uint8_t max_window_steps = 1;
        
        // Number of recovery codes to generate
        int recovery_codes_count = 8;
        
        // Issuer name for TOTP URI (e.g., "ThemisDB")
        std::string issuer = "ThemisDB";
    };
    
    /**
     * @brief MFA enrollment data for a user
     */
    struct EnrollmentData {
        std::string user_id;
        std::string secret_base32;           // Base32-encoded TOTP secret
        std::vector<std::string> recovery_codes;
        std::chrono::system_clock::time_point enrolled_at;
        bool enabled = false;
        
        nlohmann::json to_json() const;
        static EnrollmentData from_json(const nlohmann::json& j);
    };
    
    MFAAuthenticator();
    explicit MFAAuthenticator(const Config& config);
    ~MFAAuthenticator() = default;
    
    /**
     * @brief Attach an AuditLogger to receive MFA events (enroll, TOTP, recovery).
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Attach an AuthAuditLogger for typed MFA audit events including drift.
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setAuthAuditLogger(AuthAuditLogger* logger) { auth_audit_logger_ = logger; }

    /**
     * @brief Attach an AuthMetrics instance for TOTP drift observability.
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setMetrics(AuthMetrics* metrics) { metrics_ = metrics; }
    
    /**
     * @brief Generate new TOTP secret and recovery codes for user enrollment
     * 
     * @param user_id User identifier
     * @return Enrollment data with secret and recovery codes
     */
    EnrollmentData generateEnrollment(const std::string& user_id);
    
    /**
     * @brief Generate TOTP provisioning URI for QR code
     * 
     * Format: otpauth://totp/{issuer}:{user}?secret={secret}&issuer={issuer}
     * 
     * @param enrollment Enrollment data
     * @return URI string for QR code generation
     */
    std::string generateProvisioningURI(const EnrollmentData& enrollment) const;
    
    /**
     * @brief Validate TOTP code for user
     * 
     * @param secret_base32 User's TOTP secret (base32 encoded)
     * @param code TOTP code to validate
     * @param timestamp Optional timestamp (defaults to current time)
     * @param subject Optional user identifier recorded in drift audit entries
     * @return true if code is valid within time window
     */
    bool validateTOTP(
        const std::string& secret_base32,
        const std::string& code,
        std::optional<std::chrono::system_clock::time_point> timestamp = std::nullopt,
        const std::string& subject = ""
    ) const;
    
    /**
     * @brief Validate recovery code for user
     * 
     * @param enrollment User's enrollment data (will be modified to mark code as used)
     * @param recovery_code Recovery code to validate
     * @return true if code is valid and unused
     */
    bool validateRecoveryCode(
        EnrollmentData& enrollment,
        const std::string& recovery_code
    );
    
    /**
     * @brief Generate new recovery codes (e.g., after user uses all codes)
     * 
     * @param user_id User identifier
     * @return New recovery codes
     */
    std::vector<std::string> generateRecoveryCodes(const std::string& user_id);
    
    /**
     * @brief Get current TOTP code for secret (for testing/validation)
     * 
     * @param secret_base32 TOTP secret (base32 encoded)
     * @param timestamp Optional timestamp (defaults to current time)
     * @return Current TOTP code
     */
    std::string getCurrentTOTP(
        const std::string& secret_base32,
        std::optional<std::chrono::system_clock::time_point> timestamp = std::nullopt
    ) const;

private:
    Config config_;
    utils::AuditLogger* audit_logger_ = nullptr;  ///< Non-owning, optional.
    AuthAuditLogger* auth_audit_logger_ = nullptr; ///< Non-owning, optional typed logger.
    AuthMetrics* metrics_ = nullptr;               ///< Non-owning, optional metrics.
    
    // Generate random secret for TOTP (20 bytes = 160 bits)
    std::string generateSecret() const;
    
    // Generate single recovery code
    std::string generateRecoveryCode() const;
    
    // Compute TOTP value for given time counter
    std::string computeTOTP(
        const std::vector<uint8_t>& secret,
        uint64_t time_counter
    ) const;
    
    // Convert Base32 string to binary
    std::vector<uint8_t> base32Decode(const std::string& input) const;
    
    // Convert binary to Base32 string
    std::string base32Encode(const std::vector<uint8_t>& input) const;
    
    // HMAC-SHA1 implementation
    std::vector<uint8_t> hmacSHA1(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& message
    ) const;
    
    // Get time counter from timestamp
    uint64_t getTimeCounter(std::chrono::system_clock::time_point timestamp) const;
};

} // namespace auth
} // namespace themis
