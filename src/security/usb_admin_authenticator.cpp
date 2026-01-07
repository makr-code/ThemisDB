#include "security/usb_admin_authenticator.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#if defined(__linux__)
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#elif defined(_WIN32)
#include <Windows.h>
#endif

namespace themis {
namespace security {

// Implementation class
class USBAdminAuthenticator::Impl {
public:
    // Placeholder for any platform-specific state
};

// USBAdminLicense methods
bool USBAdminLicense::isValid() const {
    return !license_key.empty() && !signature.empty();
}

bool USBAdminLicense::isExpired() const {
    return std::chrono::system_clock::now() > expiry_date;
}

bool USBAdminLicense::matchesHardware(const std::string& current_hw_id) const {
    return hardware_id == current_hw_id;
}

// USBAdminAuthenticator implementation
USBAdminAuthenticator::USBAdminAuthenticator(const USBAdminConfig& config)
    : impl_(std::make_unique<Impl>())
    , config_(config)
    , last_usb_check_(std::chrono::system_clock::time_point::min())
{
}

USBAdminAuthenticator::~USBAdminAuthenticator() = default;
USBAdminAuthenticator::USBAdminAuthenticator(USBAdminAuthenticator&&) noexcept = default;
USBAdminAuthenticator& USBAdminAuthenticator::operator=(USBAdminAuthenticator&&) noexcept = default;

bool USBAdminAuthenticator::initialize() {
    THEMIS_INFO("USBAdminAuthenticator initializing with mount_path='{}'", config_.mount_path);
    
    // Initial USB check
    bool status = refreshUSBStatus();
    
    if (config_.require_usb_for_admin && !status) {
        THEMIS_WARN("USB Admin Authentication enabled but no valid USB detected at startup");
    } else if (status) {
        THEMIS_INFO("USB Admin Authentication: valid admin USB detected");
    }
    
    return true; // Always return true for initialization - USB can be inserted later
}

bool USBAdminAuthenticator::isAdminUSBPresent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we need to refresh (cache for 5 seconds to avoid constant filesystem checks)
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_usb_check_);
    
    if (elapsed.count() > 5) {
        // Need to refresh - but we're const, so just return cached value
        // Caller should use validateAdminOperation which will refresh
        return current_license_.has_value();
    }
    
    return current_license_.has_value();
}

bool USBAdminAuthenticator::validateAdminOperation(const std::string& scope, const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // If USB requirement is disabled, always allow
    if (!config_.require_usb_for_admin) {
        metrics_.admin_ops_allowed++;
        return true;
    }
    
    // Check lockout first
    auto now = std::chrono::system_clock::now();
    if (now < lockout_until_) {
        metrics_.admin_ops_denied_lockout++;
        auditLog("ADMIN_DENIED_LOCKOUT", "Admin operation denied: system in lockout", user_id);
        
        if (config_.silent_failure) {
            return false; // Silent failure
        }
        return false;
    }
    
    // Refresh USB status if needed
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_usb_check_);
    if (elapsed.count() > 5) {
        const_cast<USBAdminAuthenticator*>(this)->refreshUSBStatus();
    }
    
    // Check if USB is present
    if (!current_license_.has_value()) {
        failed_attempts_++;
        metrics_.admin_ops_denied_no_usb++;
        auditLog("ADMIN_DENIED_NO_USB", "Admin operation denied: no USB device present for scope=" + scope, user_id);
        
        // Check if we should enter lockout
        if (failed_attempts_ >= config_.max_validation_attempts) {
            lockout_until_ = now + config_.lockout_duration;
            THEMIS_WARN("USBAdminAuthenticator: entering lockout mode for {} seconds", config_.lockout_duration.count());
            auditLog("ADMIN_LOCKOUT_ENGAGED", "Too many failed admin attempts - entering lockout", user_id);
        }
        
        return false;
    }
    
    // Check if license is expired
    if (current_license_->isExpired()) {
        failed_attempts_++;
        metrics_.admin_ops_denied_expired++;
        auditLog("ADMIN_DENIED_EXPIRED", "Admin operation denied: license expired", user_id);
        return false;
    }
    
    // Check if scope is allowed
    bool scope_allowed = false;
    for (const auto& allowed_scope : current_license_->admin_scopes) {
        if (allowed_scope == "*" || allowed_scope == scope) {
            scope_allowed = true;
            break;
        }
    }
    
    if (!scope_allowed) {
        failed_attempts_++;
        metrics_.admin_ops_denied_invalid_license++;
        auditLog("ADMIN_DENIED_SCOPE", "Admin operation denied: scope '" + scope + "' not in license", user_id);
        return false;
    }
    
    // Success - reset failed attempts and allow
    failed_attempts_ = 0;
    metrics_.admin_ops_allowed++;
    metrics_.last_valid_check = now;
    auditLog("ADMIN_ALLOWED", "Admin operation allowed: scope=" + scope, user_id);
    
    return true;
}

std::optional<USBAdminLicense> USBAdminAuthenticator::getCurrentLicense() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_license_;
}

bool USBAdminAuthenticator::refreshUSBStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    last_usb_check_ = now;
    metrics_.usb_mount_checks++;
    
    // Check if USB is mounted
    if (!checkUSBMounted()) {
        current_license_.reset();
        return false;
    }
    
    // Try to load license from USB
    auto license = loadLicenseFromUSB();
    if (!license.has_value()) {
        current_license_.reset();
        return false;
    }
    
    // Validate license
    if (!license->isValid()) {
        THEMIS_WARN("USBAdminAuthenticator: license file invalid");
        current_license_.reset();
        return false;
    }
    
    // Check hardware binding
    std::string hw_id = getSystemHardwareID();
    if (!license->matchesHardware(hw_id)) {
        THEMIS_WARN("USBAdminAuthenticator: license hardware mismatch (expected={}, current={})", 
                    license->hardware_id, hw_id);
        current_license_.reset();
        return false;
    }
    
    // Validate signature
    if (!validateLicenseSignature(*license)) {
        THEMIS_WARN("USBAdminAuthenticator: license signature validation failed");
        current_license_.reset();
        return false;
    }
    
    // All checks passed
    current_license_ = license;
    metrics_.usb_mount_detected++;
    THEMIS_INFO("USBAdminAuthenticator: valid admin license loaded from USB (org={})", license->organization);
    
    return true;
}

bool USBAdminAuthenticator::isLockedOut() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    return now < lockout_until_;
}

USBAdminAuthenticator::Metrics USBAdminAuthenticator::getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

// Private helper methods

bool USBAdminAuthenticator::checkUSBMounted() const {
#if defined(__linux__)
    // Check if mount point exists and is accessible
    struct stat st;
    if (stat(config_.mount_path.c_str(), &st) != 0) {
        return false;
    }
    
    // Check if it's a directory
    if (!S_ISDIR(st.st_mode)) {
        return false;
    }
    
    // Check if license file exists
    std::string license_path = config_.mount_path + "/" + config_.license_file;
    if (stat(license_path.c_str(), &st) != 0) {
        return false;
    }
    
    return true;
#elif defined(_WIN32)
    // Check if mount path exists on Windows
    DWORD attrs = GetFileAttributesA(config_.mount_path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    
    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }
    
    // Check if license file exists
    std::string license_path = config_.mount_path + "\\" + config_.license_file;
    attrs = GetFileAttributesA(license_path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    
    return true;
#else
    THEMIS_WARN("USBAdminAuthenticator: USB checking not implemented for this platform");
    return false;
#endif
}

std::optional<USBAdminLicense> USBAdminAuthenticator::loadLicenseFromUSB() const {
    std::string license_path = config_.mount_path;
#if defined(_WIN32)
    license_path += "\\" + config_.license_file;
#else
    license_path += "/" + config_.license_file;
#endif
    
    try {
        std::ifstream file(license_path);
        if (!file.is_open()) {
            THEMIS_WARN("USBAdminAuthenticator: failed to open license file at {}", license_path);
            return std::nullopt;
        }
        
        nlohmann::json j;
        file >> j;
        
        USBAdminLicense license;
        license.license_key = j.value("license_key", "");
        license.organization = j.value("organization", "");
        license.hardware_id = j.value("hardware_id", "");
        license.signature = j.value("signature", "");
        
        // Parse dates
        std::string issued_str = j.value("issued_date", "");
        std::string expiry_str = j.value("expiry_date", "");
        
        // Simple date parsing (ISO format: YYYY-MM-DD)
        // For production, use proper date parsing library
        if (!issued_str.empty()) {
            std::tm tm = {};
            std::istringstream ss(issued_str);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            license.issued_date = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }
        
        if (!expiry_str.empty()) {
            std::tm tm = {};
            std::istringstream ss(expiry_str);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            license.expiry_date = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }
        
        // Parse admin scopes
        if (j.contains("admin_scopes") && j["admin_scopes"].is_array()) {
            for (const auto& scope : j["admin_scopes"]) {
                license.admin_scopes.push_back(scope.get<std::string>());
            }
        }
        
        return license;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("USBAdminAuthenticator: exception loading license: {}", e.what());
        return std::nullopt;
    }
}

bool USBAdminAuthenticator::validateLicenseSignature(const USBAdminLicense& license) const {
    // For now, simplified signature validation
    // In production, this should use RSA signature verification with a public key
    // embedded in the ThemisDB binary
    
    if (license.signature.empty()) {
        return false;
    }
    
    // TODO: Implement proper RSA signature verification
    // This is a placeholder that accepts any non-empty signature
    // Real implementation should:
    // 1. Extract signature from license.signature
    // 2. Compute hash of license data (license_key, organization, hardware_id, dates, scopes)
    // 3. Verify signature against hash using public key
    
    THEMIS_DEBUG("USBAdminAuthenticator: signature validation (simplified) for license {}", license.license_key);
    
    return true; // Placeholder - always valid if signature is present
}

std::string USBAdminAuthenticator::getSystemHardwareID() const {
    // Generate a hardware ID based on system characteristics
    // This should be stable across reboots but unique per machine
    
#if defined(__linux__)
    // Try to read machine-id (most reliable on Linux)
    std::ifstream machine_id_file("/etc/machine-id");
    if (machine_id_file.is_open()) {
        std::string machine_id;
        std::getline(machine_id_file, machine_id);
        if (!machine_id.empty()) {
            return machine_id;
        }
    }
    
    // Fallback: try to read product UUID
    std::ifstream product_uuid("/sys/class/dmi/id/product_uuid");
    if (product_uuid.is_open()) {
        std::string uuid;
        std::getline(product_uuid, uuid);
        if (!uuid.empty()) {
            return uuid;
        }
    }
#elif defined(_WIN32)
    // On Windows, use MachineGuid from registry
    // For now, return a placeholder
    return "WINDOWS-PLACEHOLDER-HW-ID";
#endif
    
    // Ultimate fallback: generate a "hardware id" that will work for development
    return "DEV-HARDWARE-ID-00000000";
}

std::string USBAdminAuthenticator::createChallenge() const {
    // Create a random challenge for replay protection
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::vector<uint8_t> challenge_bytes(32);
    for (auto& byte : challenge_bytes) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    // Convert to hex string
    std::ostringstream oss;
    for (auto byte : challenge_bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    
    return oss.str();
}

bool USBAdminAuthenticator::validateChallengeResponse(const std::string& challenge, const std::string& response) const {
    // Validate challenge-response for replay protection
    // This is a simplified implementation
    // Real implementation should use proper cryptographic challenge-response
    
    (void)challenge;
    (void)response;
    
    // TODO: Implement proper challenge-response validation
    return true;
}

void USBAdminAuthenticator::auditLog(const std::string& event, const std::string& details, const std::string& user_id) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ofstream log_file(config_.audit_log_path, std::ios::app);
    if (log_file.is_open()) {
        log_file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                 << " | " << event
                 << " | user=" << user_id
                 << " | " << details
                 << std::endl;
    }
    
    // Also log via ThemisDB logger
    if (event.find("DENIED") != std::string::npos || event.find("LOCKOUT") != std::string::npos) {
        THEMIS_WARN("USBAdminAuth: {} | user={} | {}", event, user_id, details);
    } else {
        THEMIS_INFO("USBAdminAuth: {} | user={} | {}", event, user_id, details);
    }
}

} // namespace security
} // namespace themis
