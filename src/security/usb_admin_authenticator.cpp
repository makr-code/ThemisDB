/**
 * @file usb_admin_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=10; TODO=1, Stub=5, Unimpl=1, Mock=1, Sim=2, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/usb_admin_authenticator.h"
#include "security/usb_volume_hardening.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <memory>

#if defined(__linux__)
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#elif defined(_WIN32)
#include <Windows.h>
#endif

namespace themis {
namespace security {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct BIO_Deleter {
    void operator()(BIO* p) const { if (p) BIO_free_all(p); }
};
struct USBAdmin_EVP_PKEY_Deleter {
    void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); }
};
struct USBAdmin_EVP_MD_CTX_Deleter {
    void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
};

using USBAdmin_BIO_ptr        = std::unique_ptr<BIO, BIO_Deleter>;
using USBAdmin_EVP_PKEY_ptr   = std::unique_ptr<EVP_PKEY, USBAdmin_EVP_PKEY_Deleter>;
using USBAdmin_EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, USBAdmin_EVP_MD_CTX_Deleter>;

} // anonymous namespace

// Implementation class
/** @brief Implementation class. */
class USBAdminAuthenticator::Impl {
public:
    // PERMANENT FALLBACK NOTE (USBAdminAuthenticator — placeholder RSA key):
    // Purpose: Provide an Impl class shell so USBAdminAuthenticator compiles on
    //          all platforms while the platform-specific USB enumeration and
    //          license-key cryptography are not yet implemented.
    // Activation: Built-in RSA path always active when license_verifier_fn is
    //             null (no LicenseVerifierFn injected).  The embedded RSA public
    //             key is a PLACEHOLDER — it will reject all real signatures.
    // Production Delta: Without a real LicenseVerifierFn injected:
    //   (a) matchesHardware() is called with the system hardware ID, and
    //   (b) validateLicenseSignature() uses the fake embedded RSA public key
    //       (returns false for all real signatures).
    //   When a LicenseVerifierFn is injected it replaces both (a) and (b).
    // Note: Inject a production-grade LicenseVerifierFn that performs
    //   real RSA/HMAC verification against a provisioned public key, OR replace
    //   the embedded placeholder key with a real one when the key-management
    //   workflow is established.
    LicenseVerifierFn license_verifier_fn;
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

// Custom move constructor: do not move std::mutex (non-movable)
USBAdminAuthenticator::USBAdminAuthenticator(USBAdminAuthenticator&& other) noexcept
    : impl_(std::move(other.impl_))
    , config_(std::move(other.config_))
    , metrics_(other.metrics_)
    , failed_attempts_(other.failed_attempts_)
    , lockout_until_(other.lockout_until_)
    , current_license_(std::move(other.current_license_))
    , last_usb_check_(other.last_usb_check_)
{
    // mutex_ is default-constructed; intentionally not moved
}

// Custom move assignment: do not assign std::mutex
USBAdminAuthenticator& USBAdminAuthenticator::operator=(USBAdminAuthenticator&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock(mutex_);
        impl_ = std::move(other.impl_);
        config_ = std::move(other.config_);
        metrics_ = other.metrics_;
        failed_attempts_ = other.failed_attempts_;
        lockout_until_ = other.lockout_until_;
        current_license_ = std::move(other.current_license_);
        last_usb_check_ = other.last_usb_check_;
        // mutex_ remains default-constructed for this instance
    }
    return *this;
}

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
    
    // Note: Cannot refresh in const method - caller should call refreshUSBStatus() explicitly
    // or use validateAdminOperation() which refreshes automatically
    if (elapsed.count() > 5) {
        // Cache may be stale - return cached value but log warning
        THEMIS_DEBUG("USBAdminAuthenticator: USB status cache may be stale ({}s old)", elapsed.count());
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
    
    // Refresh USB status if needed (removed const_cast - this method is non-const)
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_usb_check_);
    if (elapsed.count() > 5) {
        refreshUSBStatus();
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
    
    // Validate hardware binding and license signature.
    // When a LicenseVerifierFn has been injected it replaces both checks
    // (hardware ID matching and RSA signature verification).
    if (impl_->license_verifier_fn) {
        std::string hw_id = getSystemHardwareID();
        if (!impl_->license_verifier_fn(*license, hw_id)) {
            THEMIS_WARN("USBAdminAuthenticator: injected license verifier rejected the license");
            current_license_.reset();
            return false;
        }
    } else {
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
    }
    
    // ── USB Volume Hardening checks ───────────────────────────────────────────
    // These checks run after the license is loaded and signature is validated.
    // They defend against FAT-level manipulation, cloned USB sticks, and live
    // writes to the stick during authentication.

    // 1. Read-only mount enforcement
    if (config_.require_readonly_mount) {
        if (!USBVolumeHardening::isMountedReadOnly(config_.mount_path)) {
            THEMIS_WARN("USBAdminAuthenticator: USB filesystem is not mounted read-only — rejecting");
            metrics_.usb_denied_not_readonly++;
            auditLog("USB_DENIED_NOT_READONLY",
                     "USB filesystem is not mounted read-only at " + config_.mount_path,
                     "");
            current_license_.reset();
            return false;
        }
    }

    // 2. Volume integrity hash (FAT-manipulation detection)
    if (!config_.expected_volume_hash.empty()) {
        if (!USBVolumeHardening::verifyVolumeHash(
                config_.mount_path, config_.license_file, config_.expected_volume_hash)) {
            THEMIS_WARN("USBAdminAuthenticator: volume hash mismatch — FAT manipulation suspected");
            metrics_.usb_denied_volume_hash_mismatch++;
            auditLog("USB_DENIED_VOLUME_HASH_MISMATCH",
                     "License file hash does not match pinned value — FAT manipulation suspected",
                     "");
            current_license_.reset();
            return false;
        }
    }

    // 3. USB device serial binding (anti-cloning)
    if (!config_.expected_usb_serial.empty()) {
        if (!USBVolumeHardening::verifyUSBSerial(
                config_.mount_path, config_.expected_usb_serial)) {
            THEMIS_WARN("USBAdminAuthenticator: USB serial mismatch — possible cloned device");
            metrics_.usb_denied_serial_mismatch++;
            auditLog("USB_DENIED_SERIAL_MISMATCH",
                     "USB device serial does not match provisioned value — cloned device suspected",
                     "");
            current_license_.reset();
            return false;
        }
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

void USBAdminAuthenticator::setLicenseVerifierFn(LicenseVerifierFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    impl_->license_verifier_fn = std::move(fn);
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
            license.admin_scopes.reserve(j["admin_scopes"].size());
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

// Helper: Base64 decode
static std::vector<uint8_t> base64Decode(const std::string& encoded) {
    BIO_ptr b64(BIO_new(BIO_f_base64()));
    BIO_ptr bmem(BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size())));
    BIO* result = BIO_push(b64.release(), bmem.release());
    BIO_set_flags(result, BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<uint8_t> output(encoded.size());
    int decoded_size = BIO_read(result, output.data(), static_cast<int>(output.size()));
    BIO_free_all(result);
    
    if (decoded_size < 0) {
        return {};
    }
    output.resize(decoded_size);
    return output;
}

// Embedded RSA public key for USB admin license verification
static const char* USB_ADMIN_PUBLIC_KEY_PEM = 
"-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0Z3VS\n"
"QscQaIyIKDiREBnYUmDZXEsCg5HmYgLzGEcNdHd/IxA5vp3Qr\n"
"H5jGxW5qxFmFrEfNdEJ8ZNFxQqI9p5m0KqR3yqEhWBYyBvO6\n"
"oEGHxH2QzJKqZqAjF0YhLfNzM4pWYjJ3MxDGqKFxYjH5NxRq\n"
"J3pYxGhLqMzJhKqZxFjH3QxDhJqZhFjH3QxLqMzJhKqZxFjH\n"
"3QxDhJqZhFjH3QxLqMzJhKqZxFjH3QxDhJqZhFjH3QxLqMzJ\n"
"hKqZxFjH3QxDhJqZhFjH3QxLqMzJhKqZxFjH3QxDhJqZhFjH\n"
"3QxLqMzJhKqZxFjH3QxDhJqZhFjH3QxLqMzJhKqZxFjH3QxD\n"
"hJqZhFjH3QxLqMzJhKqZxFwIDAQAB\n"
"-----END PUBLIC KEY-----\n";

bool USBAdminAuthenticator::validateLicenseSignature(const USBAdminLicense& license) const {
    if (license.signature.empty()) {
        THEMIS_ERROR("USBAdminAuthenticator: license signature is empty");
        return false;
    }
    
    // Construct the canonical data that was signed
    std::ostringstream data_stream;
    data_stream << license.license_key
                << "|" << license.organization
                << "|" << license.hardware_id
                << "|" << std::chrono::system_clock::to_time_t(license.issued_date)
                << "|" << std::chrono::system_clock::to_time_t(license.expiry_date);
    
    // Add admin scopes to signed data
    for (const auto& scope : license.admin_scopes) {
        data_stream << "|" << scope;
    }
    
    std::string data_to_verify = data_stream.str();
    
    // Load the embedded public key
    BIO_ptr key_bio(BIO_new_mem_buf(USB_ADMIN_PUBLIC_KEY_PEM, -1));
    if (!key_bio) {
        THEMIS_ERROR("USBAdminAuthenticator: failed to create BIO for public key");
        return false;
    }
    
    USBAdmin_EVP_PKEY_ptr public_key(PEM_read_bio_PUBKEY(key_bio.get(), nullptr, nullptr, nullptr));
    
    if (!public_key) {
        THEMIS_ERROR("USBAdminAuthenticator: failed to load public key");
        return false;
    }
    
    // Decode the base64 signature
    std::vector<uint8_t> signature_bytes = base64Decode(license.signature);
    if (signature_bytes.empty()) {
        THEMIS_ERROR("USBAdminAuthenticator: failed to decode signature");
        return false;
    }
    
    // Verify the signature using SHA-256
    USBAdmin_EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new());
    if (!ctx) {
        THEMIS_ERROR("USBAdminAuthenticator: failed to create EVP context");
        return false;
    }
    
    bool valid = false;
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, public_key.get()) == 1) {
        if (EVP_DigestVerifyUpdate(ctx.get(), data_to_verify.data(), data_to_verify.size()) == 1) {
            int verify_result = EVP_DigestVerifyFinal(ctx.get(), signature_bytes.data(), signature_bytes.size());
            valid = (verify_result == 1);
            
            if (!valid) {
                THEMIS_ERROR("USBAdminAuthenticator: signature verification failed for license {}", license.license_key);
            }
        } else {
            THEMIS_ERROR("USBAdminAuthenticator: EVP_DigestVerifyUpdate failed");
        }
    } else {
        THEMIS_ERROR("USBAdminAuthenticator: EVP_DigestVerifyInit failed");
    }
    
    return valid;
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
    // Read MachineGuid from the Windows registry — stable across reboots, unique per machine.
    // Key: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Cryptography\MachineGuid
    HKEY hKey = nullptr;
    LONG rc = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography",
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &hKey
    );
    if (rc == ERROR_SUCCESS) {
        char guid_buf[64] = {};
        DWORD buf_size = static_cast<DWORD>(sizeof(guid_buf));
        DWORD value_type = REG_SZ;
        rc = RegQueryValueExA(hKey, "MachineGuid", nullptr, &value_type,
                              reinterpret_cast<LPBYTE>(guid_buf), &buf_size);
        RegCloseKey(hKey);
        if (rc == ERROR_SUCCESS && buf_size > 0) {
            return std::string(guid_buf);
        }
        THEMIS_WARN("USBAdminAuthenticator: RegQueryValueEx(MachineGuid) failed (rc={})", rc);
    } else {
        THEMIS_WARN("USBAdminAuthenticator: failed to open Cryptography registry key (rc={})", rc);
    }
#endif
    
    // Ultimate fallback: CRITICAL - This defeats hardware binding!
    // If this is reached, it means hardware ID detection failed
    // Log error and return empty string to fail license validation
    THEMIS_ERROR("USBAdminAuthenticator: CRITICAL - Failed to detect hardware ID on this system!");
    THEMIS_ERROR("USBAdminAuthenticator: Hardware binding will fail - admin operations will be denied");
    
    // Return empty string to fail hardware binding check
    // This is more secure than using a fallback that multiple systems might share
    return "";
}

std::string USBAdminAuthenticator::createChallenge() const {
    // Generate 32 cryptographically random bytes using OpenSSL CSPRNG.
    std::vector<uint8_t> challenge_bytes(32);
    if (RAND_bytes(challenge_bytes.data(), 32) != 1) {
        // RAND_bytes failure is a hard error: never fall back to a weaker RNG.
        // A predictable challenge defeats the replay-protection guarantee.
        THEMIS_ERROR("USBAdminAuthenticator: RAND_bytes failed — cannot generate a secure challenge: {}",
                     ERR_error_string(ERR_get_error(), nullptr));
        throw std::runtime_error(
            "USBAdminAuthenticator: cryptographically secure RNG unavailable");
    }

    // Convert to hex string
    std::ostringstream oss;
    for (auto byte : challenge_bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::string challenge = oss.str();

    // Register challenge with issue timestamp for TTL and one-time-use enforcement.
    // Lazily purge expired entries while holding the mutex.
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    // Evict expired challenges to cap memory growth
    for (auto it = issued_challenges_.begin(); it != issued_challenges_.end(); ) {
        if ((now - it->second) >= config_.challenge_ttl) {
            it = issued_challenges_.erase(it);
        } else {
            ++it;
        }
    }
    issued_challenges_[challenge] = now;
    return challenge;
}

bool USBAdminAuthenticator::validateChallengeResponse(const std::string& challenge,
                                                       const std::string& response) const {
    // ── 1. Verify the challenge was actually issued by this instance ──────────
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = issued_challenges_.find(challenge);
    if (it == issued_challenges_.end()) {
        THEMIS_WARN("USBAdminAuthenticator: challenge-response rejected — unknown challenge");
        return false;
    }

    // ── 2. Enforce TTL (replay protection: old challenges are invalid) ─────────
    auto now = std::chrono::system_clock::now();
    if ((now - it->second) >= config_.challenge_ttl) {
        issued_challenges_.erase(it);
        THEMIS_WARN("USBAdminAuthenticator: challenge-response rejected — challenge expired");
        return false;
    }

    // ── 3. One-time-use: erase challenge BEFORE computing HMAC ────────────────
    //    This prevents a race where two concurrent calls with the same challenge
    //    both see it as valid before either deletes it.
    issued_challenges_.erase(it);

    // ── 4. Require a valid license to use as the HMAC key ─────────────────────
    if (!current_license_.has_value() || current_license_->license_key.empty()) {
        THEMIS_WARN("USBAdminAuthenticator: challenge-response rejected — no current license");
        return false;
    }
    const std::string& license_key = current_license_->license_key;

    // ── 5. Compute expected response: HMAC-SHA256(key=license_key, msg=challenge) ──
    //    The client must compute the same HMAC using the license key stored on the
    //    USB device; only the physical USB holder can produce a matching response.
    unsigned char hmac_out[EVP_MAX_MD_SIZE];
    unsigned int  hmac_len = 0;

    unsigned char* result = HMAC(
        EVP_sha256(),
        license_key.data(), static_cast<int>(license_key.size()),
        reinterpret_cast<const unsigned char*>(challenge.data()), challenge.size(),
        hmac_out, &hmac_len
    );

    if (!result || hmac_len == 0) {
        THEMIS_ERROR("USBAdminAuthenticator: HMAC computation failed: {}", ERR_error_string(ERR_get_error(), nullptr));
        return false;
    }

    // Encode expected response as lowercase hex
    std::ostringstream expected_oss;
    for (unsigned int i = 0; i < hmac_len; ++i) {
        expected_oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hmac_out[i]);
    }
    const std::string expected_response = expected_oss.str();

    // ── 6. Constant-time comparison to prevent timing attacks ─────────────────
    if (response.size() != expected_response.size()) {
        THEMIS_WARN("USBAdminAuthenticator: challenge-response rejected — response length mismatch");
        return false;
    }

    // CRYPTO_memcmp returns 0 iff both buffers are identical (OpenSSL constant-time compare)
    bool valid = (CRYPTO_memcmp(response.data(), expected_response.data(), response.size()) == 0);

    if (!valid) {
        THEMIS_WARN("USBAdminAuthenticator: challenge-response rejected — HMAC mismatch");
    } else {
        THEMIS_DEBUG("USBAdminAuthenticator: challenge-response validated successfully");
    }
    return valid;
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

