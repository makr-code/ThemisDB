/**
 * @file mfa_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {
class AuthAuditLogger;
class AuthMetrics;

class MFAAuthenticator {
public:
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
    
    struct EnrollmentData {
        std::string user_id;
        std::string secret_base32;           // Base32-encoded TOTP secret
        std::vector<std::string> recovery_codes;
        std::chrono::system_clock::time_point enrolled_at;
        bool enabled = false;
        
        /**
         * @brief To json.
         * @return Return value.
         */
        nlohmann::json to_json() const;
        /**
         * @brief From json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static EnrollmentData from_json(const nlohmann::json& j);
    };
    
    MFAAuthenticator();
    /**
     * @brief MFAAuthenticator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MFAAuthenticator(const Config& config);
    ~MFAAuthenticator() = default;
    
    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Set Auth Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuthAuditLogger without additional internal calls.
     */
    void setAuthAuditLogger(AuthAuditLogger* logger) { auth_audit_logger_ = logger; }

    /**
     * @brief Set Metrics.
     * @param[in,out] metrics Input/output parameter.
     * @details Implements setMetrics without additional internal calls.
     */
    void setMetrics(AuthMetrics* metrics) { metrics_ = metrics; }
    
    /**
     * @brief Generate Enrollment.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    EnrollmentData generateEnrollment(const std::string& user_id);
    
    /**
     * @brief Generate Provisioning URI.
     * @param[in] enrollment Input parameter.
     * @return Return value.
     */
    std::string generateProvisioningURI(const EnrollmentData& enrollment) const;
    
    bool validateTOTP(
        const std::string& secret_base32,
        const std::string& code,
        std::optional<std::chrono::system_clock::time_point> timestamp = std::nullopt,
        const std::string& subject = ""
    ) const;
    
    /**
     * @brief Validate Recovery Code.
     * @param[in,out] enrollment Input/output parameter.
     * @param[in] recovery_code Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateRecoveryCode(
        EnrollmentData& enrollment,
        const std::string& recovery_code
    );
    
    /**
     * @brief Generate Recovery Codes.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    std::vector<std::string> generateRecoveryCodes(const std::string& user_id);
    
    std::string getCurrentTOTP(
        const std::string& secret_base32,
        std::optional<std::chrono::system_clock::time_point> timestamp = std::nullopt
    ) const;

private:
    Config config_;
    utils::AuditLogger* audit_logger_ = nullptr;  ///< Non-owning, optional.
    AuthAuditLogger* auth_audit_logger_ = nullptr; ///< Non-owning, optional typed logger.
    AuthMetrics* metrics_ = nullptr;               ///< Non-owning, optional metrics.
    
    /**
     * @brief Generate random secret for TOTP (20 bytes = 160 bits)
     * @return Return value.
     */
    std::string generateSecret() const;
    
    /**
     * @brief Generate single recovery code
     * @return Return value.
     */
    std::string generateRecoveryCode() const;
    
    /**
     * @brief Compute TOTP value for given time counter
     * @param[in] secret Input parameter.
     * @param[in] time_counter Input parameter.
     * @return Return value.
     */
    std::string computeTOTP(
        const std::vector<uint8_t>& secret,
        uint64_t time_counter
    ) const;
    
    /**
     * @brief Convert Base32 string to binary
     * @param[in] input Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> base32Decode(const std::string& input) const;
    
    /**
     * @brief Convert binary to Base32 string
     * @param[in] input Input parameter.
     * @return Return value.
     */
    std::string base32Encode(const std::vector<uint8_t>& input) const;
    
    // HMAC-SHA1 implementation
    /**
     * @brief Hmac SHA1.
     * @param[in] key Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> hmacSHA1(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& message
    ) const;
    
    /**
     * @brief Get time counter from timestamp
     * @param[in] timestamp Input parameter.
     * @return Return value.
     */
    uint64_t getTimeCounter(std::chrono::system_clock::time_point timestamp) const;
};

} // namespace auth
} // namespace themis
