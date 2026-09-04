/**
 * @file mfa_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/mfa_authenticator.h"
#include "auth/auth_audit_logger.h"
#include "auth/auth_metrics.h"
#include "utils/audit_logger.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>

namespace themis {
namespace auth {

namespace {
    // Base32 alphabet (RFC 4648)
    constexpr char BASE32_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    
    // Random device for cryptographic randomness
    std::random_device rd = {};
    std::mt19937_64 gen(rd());
}

// ============================================================================
// EnrollmentData JSON serialization
// ============================================================================

nlohmann::json MFAAuthenticator::EnrollmentData::to_json() const {
    nlohmann::json j;
    j["user_id"] = user_id;
    j["secret_base32"] = secret_base32;
    j["recovery_codes"] = recovery_codes;
    j["enrolled_at"] = std::chrono::system_clock::to_time_t(enrolled_at);
    j["enabled"] = enabled;
    return j;
}

MFAAuthenticator::EnrollmentData MFAAuthenticator::EnrollmentData::from_json(const nlohmann::json& j) {
    EnrollmentData data;
    data.user_id = j.at("user_id").get<std::string>();
    data.secret_base32 = j.at("secret_base32").get<std::string>();
    data.recovery_codes = j.at("recovery_codes").get<std::vector<std::string>>();
    data.enrolled_at = std::chrono::system_clock::from_time_t(j.at("enrolled_at").get<time_t>());
    data.enabled = j.at("enabled").get<bool>();
    return data;
}

// ============================================================================
// MFAAuthenticator Implementation
// ============================================================================

MFAAuthenticator::MFAAuthenticator()
    : MFAAuthenticator(Config{}) {
}

MFAAuthenticator::MFAAuthenticator(const Config& config)
    : config_(config) {
    
    if (config_.code_length != 6 && config_.code_length != 8) {
        spdlog::warn("MFA code length must be 6 or 8, using 6");
        config_.code_length = 6;
    }
    
    // Log a soft warning only when time_window is above the recommended soft limit
    // (max_window_steps) but still within the absolute hard limit of 2.
    // Values above 2 are rejected below, so warn only in the range (max_window_steps, 2].
    if (config_.time_window > config_.max_window_steps && config_.time_window <= 2) {
        spdlog::warn("TOTP time_window ({}) exceeds max_window_steps ({}); "
                     "consider reducing to limit replay exposure",
                     config_.time_window, config_.max_window_steps);
    }
    // Absolute hard limit: a window wider than ±2 steps (e.g., ±60 s when
    // time_step_seconds=30) substantially weakens replay resistance beyond
    // totp_replay_cache mitigations.
    if (config_.time_window > 2) {
        throw std::invalid_argument(
            "TOTP time_window must not exceed 2 steps; "
            "larger windows substantially weaken replay resistance");
    }
    
    spdlog::info("MFA Authenticator initialized:");
    spdlog::info("  Time step: {}s", config_.time_step_seconds);
    spdlog::info("  Code length: {}", config_.code_length);
    spdlog::info("  Time window: ±{} steps", config_.time_window);
}

MFAAuthenticator::EnrollmentData MFAAuthenticator::generateEnrollment(const std::string& user_id) {
    EnrollmentData data;
    data.user_id = user_id;
    data.secret_base32 = generateSecret();
    data.recovery_codes = generateRecoveryCodes(user_id);
    data.enrolled_at = std::chrono::system_clock::now();
    data.enabled = false;  // Require explicit activation
    
    spdlog::info("Generated MFA enrollment for user: {}", user_id);
    if (audit_logger_) {
        audit_logger_->logSecurityEvent(utils::SecurityEventType::MFA_ENROLLED,
            user_id, "mfa/enrollment", {});
    }
    return data;
}

std::string MFAAuthenticator::generateProvisioningURI(const EnrollmentData& enrollment) const {
    std::ostringstream uri = {};
    uri << "otpauth://totp/"
        << config_.issuer << ":" << enrollment.user_id
        << "?secret=" << enrollment.secret_base32
        << "&issuer=" << config_.issuer
        << "&digits=" << config_.code_length
        << "&period=" << config_.time_step_seconds;
    return uri.str();
}

bool MFAAuthenticator::validateTOTP(
    const std::string& secret_base32,
    const std::string& code,
    std::optional<std::chrono::system_clock::time_point> timestamp,
    const std::string& subject
) const {
    if (code.length() != static_cast<size_t>(config_.code_length)) {
        return false;
    }
    
    // Decode secret from base32
    std::vector<uint8_t> secret;
    try {
        secret = base32Decode(secret_base32);
    } catch (const std::exception& e) {
        spdlog::error("Failed to decode TOTP secret: {}", e.what()); // NOPII: e.what() is an error description, not the secret value
        return false;
    }
    
    // Use current time if not provided
    auto ts = timestamp.value_or(std::chrono::system_clock::now());
    uint64_t current_counter = getTimeCounter(ts);
    
    // Check time window (allow codes from past/future steps)
    for (int offset = -config_.time_window; offset <= config_.time_window; ++offset) {
        uint64_t counter = current_counter + offset;
        std::string expected = computeTOTP(secret, counter);
        
        if (expected == code) {
            spdlog::debug("TOTP validated successfully (offset: {})", offset);
            if (audit_logger_) {
                audit_logger_->logSecurityEvent(utils::SecurityEventType::MFA_TOTP_SUCCESS,
                    "", "mfa/totp", {});
            }
            if (offset != 0) {
                spdlog::warn("TOTP validated with non-zero time drift (offset: {} steps)", offset);
                if (auth_audit_logger_) {
                    auth_audit_logger_->logTOTPDrift(subject, offset, ts);
                }
                if (metrics_) {
                    metrics_->recordTOTPDrift(offset);
                }
            }
            return true;
        }
    }
    
    spdlog::debug("TOTP validation failed");
    if (audit_logger_) {
        audit_logger_->logSecurityEvent(utils::SecurityEventType::MFA_TOTP_FAILED,
            "", "mfa/totp", {});
    }
    return false;
}

bool MFAAuthenticator::validateRecoveryCode(
    EnrollmentData& enrollment,
    const std::string& recovery_code
) {
    // Constant-time linear scan: always traverse every code regardless of match
    // to prevent an attacker from inferring the match position through timing
    // differences (early-exit prevention). CRYPTO_memcmp() is used for the
    // per-code comparison to avoid compiler-optimised short-circuit evaluation.
    bool found = false;
    size_t found_idx = 0;
    const size_t incoming_len = recovery_code.size();

    for (size_t i = 0; i <static_cast<int>(enrollment.recovery_codes.size()); ++i) {
        const auto& stored = enrollment.recovery_codes[i];
        // Length mismatch cannot be a match; the branch does not leak the
        // match position because it depends only on the (fixed) stored length,
        // not on which index matched.
        bool len_match = (static_cast<int>(stored.size()) == incoming_len);
        int diff = len_match
            ? CRYPTO_memcmp(stored.data(), recovery_code.data(), incoming_len)
            : 1;
        if (diff == 0 && !found) {
            found = true;
            found_idx = i;
        }
    }

    if (found) {
        // Remove the used recovery code so it cannot be replayed.
        enrollment.recovery_codes.erase(
            enrollment.recovery_codes.begin() + static_cast<ptrdiff_t>(found_idx));
        spdlog::info("Recovery code validated for user: {}", enrollment.user_id);
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::MFA_RECOVERY_CODE_USED,
                enrollment.user_id, "mfa/recovery_code", {});
        }
        return true;
    }

    spdlog::debug("Recovery code validation failed");
    return false;
}

std::vector<std::string> MFAAuthenticator::generateRecoveryCodes(const std::string& user_id) {
    std::vector<std::string> codes;
    codes.reserve(config_.recovery_codes_count);
    
    for (int i = 0; i < config_.recovery_codes_count; ++i) {
        codes.push_back(generateRecoveryCode());
    }
    
    spdlog::debug("Generated {} recovery codes for user: {}", 
                 config_.recovery_codes_count, user_id);
    return codes;
}

std::string MFAAuthenticator::getCurrentTOTP(
    const std::string& secret_base32,
    std::optional<std::chrono::system_clock::time_point> timestamp
) const {
    std::vector<uint8_t> secret = base32Decode(secret_base32);
    auto ts = timestamp.value_or(std::chrono::system_clock::now());
    uint64_t counter = getTimeCounter(ts);
    return computeTOTP(secret, counter);
}

// ============================================================================
// Private helper methods
// ============================================================================

std::string MFAAuthenticator::generateSecret() const {
    // Generate 20 random bytes (160 bits) for TOTP secret
    std::vector<uint8_t> secret(20);
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : secret) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    return base32Encode(secret);
}

std::string MFAAuthenticator::generateRecoveryCode() const {
    // Generate 8-character alphanumeric recovery code
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
    
    std::string code = {};
    code.reserve(8);
    for (int i = 0; i < 8; ++i) {
        code += charset[dis(gen)];
    }
    
    return code;
}

std::string MFAAuthenticator::computeTOTP(
    const std::vector<uint8_t>& secret,
    uint64_t time_counter
) const {
    // Convert counter to 8-byte big-endian
    std::vector<uint8_t> counter_bytes(8);
    for (int i = 7; i >= 0; --i) {
        counter_bytes[i] = time_counter & 0xFF;
        time_counter >>= 8;
    }
    
    // Compute HMAC-SHA1
    std::vector<uint8_t> hash = hmacSHA1(secret, counter_bytes);
    
    // Dynamic truncation (RFC 4226)
    int offset = hash[hash.size() - 1] & 0x0F;
    uint32_t binary = 
        ((hash[offset] & 0x7F) << 24) |
        ((hash[offset + 1] & 0xFF) << 16) |
        ((hash[offset + 2] & 0xFF) << 8) |
        (hash[offset + 3] & 0xFF);
    
    // Generate N-digit code
    uint32_t modulus = 1;
    for (int i = 0; i < config_.code_length; ++i) {
        modulus *= 10;
    }
    uint32_t code_value = binary % modulus;
    
    // Format with leading zeros
    std::ostringstream oss = {};
    oss << std::setw(config_.code_length) << std::setfill('0') << code_value;
    return oss.str();
}

std::vector<uint8_t> MFAAuthenticator::base32Decode(const std::string& input) const {
    std::vector<uint8_t> output;
    int buffer = 0;
    int bits_left = 0;
    
    for (char c : input) {
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            continue;  // Skip whitespace
        }
        
        // Find character in alphabet
        const char* pos = std::strchr(BASE32_ALPHABET, std::toupper(c));
        if (!pos) {
            throw std::invalid_argument("Invalid base32 character");
        }
        
        int value = static_cast<int>(pos - BASE32_ALPHABET);
        buffer = (buffer << 5) | value;
        bits_left += 5;
        
        if (bits_left >= 8) {
            output.push_back((buffer >> (bits_left - 8)) & 0xFF);
            bits_left -= 8;
        }
    }
    
    return output;
}

std::string MFAAuthenticator::base32Encode(const std::vector<uint8_t>& input) const {
    std::string output = {};
    int buffer = 0;
    int bits_left = 0;
    
    for (uint8_t byte : input) {
        buffer = (buffer << 8) | byte;
        bits_left += 8;
        
        while (bits_left >= 5) {
            int index = (buffer >> (bits_left - 5)) & 0x1F;
            output += BASE32_ALPHABET[index];
            bits_left -= 5;
        }
    }
    
    if (bits_left > 0) {
        int index = (buffer << (5 - bits_left)) & 0x1F;
        output += BASE32_ALPHABET[index];
    }
    
    return output;
}

std::vector<uint8_t> MFAAuthenticator::hmacSHA1(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& message
) const {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    
    HMAC(EVP_sha1(), 
         key.data(), static_cast<int>(key.size()),
         message.data(), static_cast<int>(message.size()),
         hash, &hash_len);
    
    return std::vector<uint8_t>(hash, hash + hash_len);
}

uint64_t MFAAuthenticator::getTimeCounter(std::chrono::system_clock::time_point timestamp) const {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        timestamp.time_since_epoch()
    ).count();
    return seconds / config_.time_step_seconds;
}

} // namespace auth
} // namespace themis

