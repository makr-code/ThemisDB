/**
 * @file license_info.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: license_info.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 818
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=8, M=6, L=0
 * PR History (last 5): #4518 [WIP] Update developer docu... (2026-04-12) | #4351 feat(themis): integration t... (2026-03-20) | #3830 feat(themis): Modular Build... (2026-03-12) | #3429 [WIP] Add full modularizati... (2026-03-12) | #3410 feat(themis): Dynamic featu... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB License Information Implementation
 * ============================================
 * Provides access to compile-time embedded license data.
 */

#include "themis/license_info.h"
#include <stdexcept>
#include "utils/openssl_deleter.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <array>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

// Optional CURL for online license validation
#if __has_include(<curl/curl.h>)
#  include <curl/curl.h>
#  define THEMIS_HAVE_CURL 1
#endif

// Platform-specific machine fingerprint helpers
#if defined(__linux__)
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <net/if.h>
#  include <unistd.h>
#  include <ifaddrs.h>
#  include <netinet/in.h>
#elif defined(_WIN32)
#  include <windows.h>
#  include <iphlpapi.h>
#  pragma comment(lib, "iphlpapi.lib")
#endif

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
// RSA PUBLIC KEY FOR LICENSE VERIFICATION
// ============================================================================
// This is the ThemisDB public key used to verify license signatures
// Production licenses are signed with the corresponding private key
// For development/testing, this can be overridden at build time

#ifndef THEMIS_LICENSE_PUBLIC_KEY_PEM
#define THEMIS_LICENSE_PUBLIC_KEY_PEM \
"-----BEGIN PUBLIC KEY-----\n" \
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0Z3VS" \
"QscQaIyIKDiREBnYUmDZXEsCg5HmYgLzGEcNdHd/IxA5vp3Qr\n" \
"H5jGxW5qxFmFrEfNdEJ8ZNFxQqI9p5m0KqR3yqEhWBYyBvO6" \
"oEGHxH2QzJKqZqAjF0YhLfNzM4pW\n" \
"YjJ3MxDGqKFxYjH5NxRqJ3pYxGhLqMzJhKqZxFjH3QxDhJqZh" \
"FjH3QxLqMzJhKqZxFjH3QxDhJqZ\n" \
"hFjH3QxLqMzJhKqZxFjH3QxDhJqZhFjH3QxLqMzJhKqZxFjH3" \
"QxDhJqZhFjH3QxLqMzJhKqZxFj\n" \
"H3QxDhJqZhFjH3QxLqMzJhKqZxFjH3QxDhJqZhFjH3QxLqMzJ" \
"hKqZxFjH3QxDhJqZhFjH3QxLq\n" \
"MzJhKqZxFjH3QxDhJqZhFjH3QxLqMzJhKqZxFwIDAQAB\n" \
"-----END PUBLIC KEY-----\n"
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

// Helper: Base64 decode
static std::vector<uint8_t> base64Decode(const std::string& encoded) {
    BIO* bmem = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    if (!bmem) return {};
    BIO* b64 = BIO_new(BIO_f_base64());
    if (!b64) { BIO_free(bmem); return {}; }
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    auto bio = themis::utils::BIOPtr(BIO_push(b64, bmem));  // BIO_push returns top of chain
    
    std::vector<uint8_t> output(encoded.size());
    int decoded_size = BIO_read(bio.get(), output.data(), static_cast<int>(output.size()));
    
    if (decoded_size < 0) {
        return {};
    }
    output.resize(decoded_size);
    return output;
}

bool verifyLicenseSignature(const LicenseData& license) {
    std::string license_edition_normalized = license.edition;
    for (auto& ch : license_edition_normalized) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    const bool is_hyperscaler_license = (license_edition_normalized == "HYPERSCALER");

    // Hyperscaler licenses must always carry a cryptographic signature.
        // ✓ SECURITY FIX #7: Legacy Path Governance Marker
        // Legacy Compatibility Path: v1.7.0 License Validation Grace Period
        // Purpose: Development/non-Hyperscaler editions skip signature requirement
        // Activation: license.signature.empty() && !is_hyperscaler_license
        // Behavior Delta: Hyperscaler licenses MUST have cryptographic signature; others optional
        // Approver: @makr-code (PR #3410)
        // Removal Target: v1.8.0 – Enforce signatures for all license editions
        // Other editions keep legacy development behavior.
    // Other editions keep legacy development behavior.
    if (license.signature.empty()) {
        return !is_hyperscaler_license;
    }
    
    // Construct the data that was signed (canonical format)
    std::ostringstream data_stream;
    data_stream << license.license_key
                << "|" << license.organization_name
                << "|" << license.organization_id
                << "|" << license.issued_date
                << "|" << license.expiry_date
                << "|" << license.max_nodes
                << "|" << license.max_cores
                << "|" << license.max_storage_tb
                << "|" << license.edition;
    std::string data_to_verify = data_stream.str();
    
    // Load the embedded public key
    auto key_bio = themis::utils::make_bio_mem_buf(THEMIS_LICENSE_PUBLIC_KEY_PEM, -1);
    if (!key_bio) {
        return false;
    }
    
    auto public_key = themis::utils::EVPKeyPtr(PEM_read_bio_PUBKEY(key_bio.get(), nullptr, nullptr, nullptr));
    
    if (!public_key) {
        return false;
    }
    
    // Decode the base64 signature
    std::vector<uint8_t> signature_bytes = base64Decode(license.signature);
    if (signature_bytes.empty()) {
        return false;
    }
    
    // Verify the signature using SHA-256
    auto ctx = themis::utils::make_evp_md_ctx();
    if (!ctx) {
        return false;
    }
    
    bool valid = false;
    
    // ✓ SECURITY FIX #5: Exception-safe signature verification
    // Ensure all verification failures are logged and handled explicitly
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, public_key.get()) != 1) {
        spdlog::error("verifyLicenseSignature: EVP_DigestVerifyInit failed");
        return false;
    }
    
    if (EVP_DigestVerifyUpdate(ctx.get(), data_to_verify.data(), data_to_verify.size()) != 1) {
        spdlog::error("verifyLicenseSignature: EVP_DigestVerifyUpdate failed");
        return false;
    }
    
    int verify_result = EVP_DigestVerifyFinal(ctx.get(), signature_bytes.data(), signature_bytes.size());
    if (verify_result == 1) {
        valid = true;
    } else if (verify_result == 0) {
        // Signature verification failed - this is expected for invalid signatures
        spdlog::debug("verifyLicenseSignature: Signature verification failed (invalid signature)");
        valid = false;
    } else {
        spdlog::error("verifyLicenseSignature: EVP_DigestVerifyFinal error ({})", verify_result);
        valid = false;
    }
    
    return valid;
}

// ============================================================================
// MACHINE FINGERPRINT HELPERS
// ============================================================================

// Compute a hex-encoded SHA-256 of the primary MAC address (or a fallback)
static std::string computeFingerprintHash(const std::string& raw) {
    unsigned char digest[EVP_MAX_MD_SIZE] = {};
    unsigned int  dlen = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx) {
        EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx, raw.data(), raw.size());
        EVP_DigestFinal_ex(ctx, digest, &dlen);
        EVP_MD_CTX_free(ctx);
    }
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < dlen; ++i)
        hex << std::setw(2) << static_cast<unsigned>(digest[i]);
    return hex.str();
}

static std::string getPrimaryMacAddress() {
#if defined(__linux__)
    struct ifaddrs* ifa_list = nullptr;
    if (getifaddrs(&ifa_list) != 0) return "00:00:00:00:00:00";

    std::string result;
    for (struct ifaddrs* ifa = ifa_list; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_name) continue;
        if (std::string(ifa->ifa_name) == "lo") continue;

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) continue;

        struct ifreq ifr{};
        std::strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // ensure null termination
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
            const auto* mac = reinterpret_cast<const unsigned char*>(ifr.ifr_hwaddr.sa_data);
            char buf[32];
            std::snprintf(buf, sizeof(buf),
                "%02x:%02x:%02x:%02x:%02x:%02x",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            result = buf;
        }
        close(sock);
        if (!result.empty()) break;
    }
    freeifaddrs(ifa_list);
    return result.empty() ? "00:00:00:00:00:00" : result;

#elif defined(_WIN32)
    IP_ADAPTER_INFO adapter_info[16];
    DWORD buf_len = sizeof(adapter_info);
    if (GetAdaptersInfo(adapter_info, &buf_len) != ERROR_SUCCESS)
        return "00:00:00:00:00:00";
    const auto* mac = adapter_info[0].Address;
    char buf[32];
    std::snprintf(buf, sizeof(buf),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
#else
    return "00:00:00:00:00:00";
#endif
}

// ============================================================================
// LicenseClient::Impl
// ============================================================================

class LicenseClient::Impl {
public:
    explicit Impl(const LicenseClientConfig& cfg) : cfg_(cfg) {}

    LicenseActivationResult activate() {
        LicenseActivationResult result;

        // Get embedded license as baseline
        auto embedded = getEmbeddedLicense();
        if (!embedded) {
            result.success       = false;
            result.status        = "invalid";
            result.error_message = "No embedded license found";
            return result;
        }

#ifdef THEMIS_HAVE_CURL
        if (!cfg_.server_url.empty()) {
            return performOnlineRequest("activate", *embedded);
        }
#endif

        // Offline path
        if (!cfg_.allow_offline) {
            result.success       = false;
            result.status        = "offline";
            result.error_message = "Offline activation not permitted";
            return result;
        }

        // Fall back to embedded license validation
        if (isLicenseValid(*embedded)) {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cached_license_     = embedded;
            last_check_time_    = std::chrono::steady_clock::now();
            result.success      = true;
            result.status       = "active";
            result.refreshed_license = embedded;
        } else {
            result.success       = false;
            result.status        = "expired";
            result.error_message = "Embedded license has expired";
        }
        return result;
    }

    LicenseActivationResult validate() {
        // Check cache in a scoped block so the mutex is released before activate()
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);

            // Return cached result if fresh (< 1 hour)
            if (cached_license_) {
                auto age = std::chrono::steady_clock::now() - last_check_time_;
                if (age < std::chrono::hours(1)) {
                    LicenseActivationResult result;
                    result.success = isLicenseValid(*cached_license_);
                    result.status  = result.success ? "active" : "expired";
                    result.refreshed_license = cached_license_;
                    return result;
                }
            }
        } // lock released here

        // Re-validate (calls activate() which may re-acquire the mutex)
        return activate();
    }

    std::optional<LicenseData> getCachedLicense() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cached_license_;
    }

    LicenseActivationResult refresh() {
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cached_license_ = std::nullopt; // force re-check
        }
        return activate();
    }

    static std::string getMachineFingerprint() {
        const std::string mac = getPrimaryMacAddress();
        return computeFingerprintHash("ThemisDB:" + mac);
    }

private:
#ifdef THEMIS_HAVE_CURL
    // CURL response accumulator
    static size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* out = static_cast<std::string*>(userdata);
        out->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    LicenseActivationResult performOnlineRequest(const std::string& action,
                                                  const LicenseData& license) {
        LicenseActivationResult result;

        CURL* curl = curl_easy_init();
        if (!curl) {
            result.success       = false;
            result.status        = "offline";
            result.error_message = "curl_easy_init failed";
            return handleOfflineFallback(license, result);
        }

        const std::string url = cfg_.server_url + "/" + action;

        // Build minimal JSON body
        std::ostringstream body;
        body << "{"
             << "\"license_key\":\"" << license.license_key << "\","
             << "\"machine_fingerprint\":\"" << getMachineFingerprint() << "\","
             << "\"edition\":\"" << license.edition << "\""
             << "}";
        const std::string body_str = body.str();

        std::string response_body;
        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,
            static_cast<long>(cfg_.timeout.count()));
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!cfg_.api_key.empty()) {
            const std::string auth = "Authorization: Bearer " + cfg_.api_key;
            headers = curl_slist_append(headers, auth.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            result.success       = false;
            result.status        = "offline";
            result.error_message = curl_easy_strerror(res);
            return handleOfflineFallback(license, result);
        }

        if (http_code == 200) {
            // Parse JSON response body from the license server.
            // The server returns a JSON object whose fields map 1:1 to LicenseData.
            // All fields are optional in the parse — if the server omits one,
            // we fall back to the corresponding value from the embedded license.
            //
            // Field name conventions used by each backend:
            //   FastAPI license-server   WordPress plugin
            //   ────────────────────     ────────────────
            //   expiry_date              end_date
            //   issued_date              start_date
            //   (both send "edition" and "organization")
            LicenseData refreshed = license;  // start with embedded as baseline
            try {
                auto j = nlohmann::json::parse(response_body);
                if (j.contains("license_key")    && j["license_key"].is_string())
                    refreshed.license_key     = j["license_key"].get<std::string>();
                if (j.contains("edition")        && j["edition"].is_string())
                    refreshed.edition         = j["edition"].get<std::string>();
                if (j.contains("tier")           && j["tier"].is_string())
                    refreshed.edition         = j["tier"].get<std::string>();  // alias
                if (j.contains("organization")   && j["organization"].is_string())
                    refreshed.organization_name = j["organization"].get<std::string>();
                // Accept "expiry_date" (FastAPI server) or "end_date" (WordPress plugin).
                if (j.contains("expiry_date")    && j["expiry_date"].is_string())
                    refreshed.expiry_date     = j["expiry_date"].get<std::string>().substr(0, 10);
                else if (j.contains("end_date")  && j["end_date"].is_string())
                    refreshed.expiry_date     = j["end_date"].get<std::string>().substr(0, 10);
                // Accept "issued_date" (FastAPI server) or "start_date" (WordPress plugin).
                if (j.contains("issued_date")    && j["issued_date"].is_string())
                    refreshed.issued_date     = j["issued_date"].get<std::string>().substr(0, 10);
                else if (j.contains("start_date") && j["start_date"].is_string())
                    refreshed.issued_date     = j["start_date"].get<std::string>().substr(0, 10);
                if (j.contains("status")         && j["status"].is_string())
                    result.status             = j["status"].get<std::string>();
                else
                    result.status = "active";
                // Populate LicenseData.signature from the server's HMAC so the
                // cached license carries the server-signed proof.
                if (j.contains("signature")      && j["signature"].is_string())
                    refreshed.signature       = j["signature"].get<std::string>();
                // Top-level limits (FastAPI server sends max_nodes at top level, not nested).
                // Parsed first so that a nested "limits" object (WordPress plugin / future)
                // can override individual values — nested takes priority.
                if (j.contains("max_nodes")      && j["max_nodes"].is_number_integer())
                    refreshed.max_nodes      = j["max_nodes"].get<int>();
                if (j.contains("max_cores")      && j["max_cores"].is_number_integer())
                    refreshed.max_cores      = j["max_cores"].get<int>();
                if (j.contains("max_storage_tb") && j["max_storage_tb"].is_number_integer())
                    refreshed.max_storage_tb = j["max_storage_tb"].get<int>();
                if (j.contains("limits") && j["limits"].is_object()) {
                    const auto& lim = j["limits"];
                    if (lim.contains("max_nodes")      && lim["max_nodes"].is_number_integer())
                        refreshed.max_nodes      = lim["max_nodes"].get<int>();
                    if (lim.contains("max_cores")      && lim["max_cores"].is_number_integer())
                        refreshed.max_cores      = lim["max_cores"].get<int>();
                    if (lim.contains("max_storage_tb") && lim["max_storage_tb"].is_number_integer())
                        refreshed.max_storage_tb = lim["max_storage_tb"].get<int>();
                }
            } catch (const nlohmann::json::exception&) {
                // Malformed JSON response — keep the embedded baseline.
                result.status = "active";
            }
            result.success = true;
            result.refreshed_license = refreshed;
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cached_license_  = refreshed;
            last_check_time_ = std::chrono::steady_clock::now();
        } else if (http_code == 402) {
            result.success       = false;
            result.status        = "expired";
            result.error_message = "License expired (server response 402)";
        } else {
            result.success       = false;
            result.status        = "invalid";
            result.error_message = "Server returned HTTP " + std::to_string(http_code);
        }
        return result;
    }
#endif // THEMIS_HAVE_CURL

    LicenseActivationResult handleOfflineFallback(const LicenseData& license,
                                                   LicenseActivationResult& base) {
        if (!cfg_.allow_offline) {
            base.error_message += " (offline fallback disabled)";
            return base;
        }

        // Check grace period using last_check_time_
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            if (last_check_time_ != std::chrono::steady_clock::time_point{}) {
                auto offline_duration = std::chrono::steady_clock::now() - last_check_time_;
                auto offline_days = std::chrono::duration_cast<std::chrono::hours>(
                    offline_duration).count() / 24;

                if (offline_days > cfg_.grace_period_days) {
                    base.success       = false;
                    base.status        = "expired";
                    base.error_message = "Grace period exceeded (" +
                        std::to_string(cfg_.grace_period_days) + " days)";
                    return base;
                }
                base.grace_days_remaining =
                    cfg_.grace_period_days - static_cast<int>(offline_days);
            }
        }

        if (isLicenseValid(license)) {
            base.success = true;
            base.status  = "grace";
            base.refreshed_license = license;
        } else {
            base.success       = false;
            base.status        = "expired";
            base.error_message = "Embedded license expired during offline grace period";
        }
        return base;
    }

    LicenseClientConfig              cfg_;
    mutable std::mutex               cache_mutex_;
    std::optional<LicenseData>       cached_license_;
    std::chrono::steady_clock::time_point last_check_time_{};
};

// ============================================================================
// LicenseClient public API
// ============================================================================

LicenseClient::LicenseClient(const LicenseClientConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

LicenseClient::~LicenseClient() = default;

LicenseActivationResult LicenseClient::activate() {
    return impl_->activate();
}

LicenseActivationResult LicenseClient::validate() {
    return impl_->validate();
}

std::optional<LicenseData> LicenseClient::getCachedLicense() const {
    return impl_->getCachedLicense();
}

LicenseActivationResult LicenseClient::refresh() {
    return impl_->refresh();
}

std::string LicenseClient::getMachineFingerprint() {
    return Impl::getMachineFingerprint();
}

// ============================================================================
// LicenseInfo (v1.7.1)
// ============================================================================

LicenseInfo::LicenseInfo(const LicenseData& data, int grace_period_days)
    : data_(data), grace_period_days_(grace_period_days) {}

int LicenseInfo::remaining_grace_days() const {
    if (data_.expiry_date.empty()) {
        return 0;
    }

    // Parse ISO-8601 "YYYY-MM-DD"
    int year = 0, month = 0, day = 0;
    if (std::sscanf(data_.expiry_date.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
        return 0;
    }

    // Build expiry time_t (midnight UTC on the given date)
    struct tm expiry_tm{};
    expiry_tm.tm_year  = year - 1900;
    expiry_tm.tm_mon   = month - 1;
    expiry_tm.tm_mday  = day;
    expiry_tm.tm_hour  = 0;
    expiry_tm.tm_min   = 0;
    expiry_tm.tm_sec   = 0;
    expiry_tm.tm_isdst = 0;

#ifdef _WIN32
    time_t expiry_time = _mkgmtime(&expiry_tm);
#else
    time_t expiry_time = timegm(&expiry_tm);
#endif
    if (expiry_time == static_cast<time_t>(-1)) {
        return 0;
    }

    auto now        = std::chrono::system_clock::now();
    auto expiry_tp  = std::chrono::system_clock::from_time_t(expiry_time);

    if (now <= expiry_tp) {
        // License has not yet expired — the full grace window is available.
        return grace_period_days_;
    }

    auto elapsed_since_expiry = now - expiry_tp;
    auto days_since_expiry    = static_cast<int>(
        std::chrono::duration_cast<std::chrono::hours>(elapsed_since_expiry).count() / 24);

    int remaining = grace_period_days_ - days_since_expiry;
    return remaining > 0 ? remaining : 0;
}

} // namespace license
} // namespace themis

