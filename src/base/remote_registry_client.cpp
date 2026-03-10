/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            remote_registry_client.cpp                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     494                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • bfe82c39d  2026-02-27  fix(base): code-audit fixes for RemoteRegistryClient ║
    • 27a08eb54  2026-02-27  feat(base): implement remote plugin loading from authenti... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Remote plugin registry client implementation.
//
// See include/themis/base/remote_registry_client.h for the public API.

#include "themis/base/remote_registry_client.h"

#include <curl/curl.h>
#include <openssl/evp.h>
#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace themis {
namespace modules {

namespace {

// Maximum number of retries permitted regardless of what the config says.
// Prevents integer overflow in the exponential-backoff shift calculation.
constexpr int kMaxAllowedRetries = 10;

// Maximum bit-shift used in the backoff formula (500 ms × 2^5 = 16 000 ms).
constexpr int kMaxBackoffShift = 5;

// CURL write callback: accumulates response body into a std::string.
size_t writeStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), total);
    return total;
}

// CURL write callback: writes to an open std::ofstream.
size_t writeFileCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    static_cast<std::ofstream*>(userp)->write(static_cast<char*>(contents),
                                              static_cast<std::streamsize>(total));
    return total;
}

// Compute the SHA-256 hex digest of a file using OpenSSL EVP.
std::string sha256File(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return {};

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    std::array<char, 8192> buf{};
    while (file.read(buf.data(), static_cast<std::streamsize>(buf.size())) ||
           file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buf.data(),
                             static_cast<size_t>(file.gcount())) != 1) {
            EVP_MD_CTX_free(ctx);
            return {};
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(digest[i]);
    }
    return oss.str();
}

} // anonymous namespace

// =============================================================================
// Constructor / Destructor
// =============================================================================

RemoteRegistryClient::RemoteRegistryClient(const RegistryConfig& config)
    : config_(config) {
    spdlog::info("RemoteRegistryClient: registry_url='{}' verify_ssl={}",
                 config_.registry_url, config_.verify_ssl);
}

RemoteRegistryClient::~RemoteRegistryClient() = default;

// =============================================================================
// Registry queries
// =============================================================================

std::vector<RegistryPluginEntry> RemoteRegistryClient::listPlugins() {
    const std::string url = config_.registry_url + "/plugins";
    spdlog::debug("RemoteRegistryClient::listPlugins GET {}", url);

    std::string body;
    try {
        body = httpGet(url);
    } catch (const std::exception& ex) {
        spdlog::error("RemoteRegistryClient::listPlugins: HTTP error: {}", ex.what());
        return {};
    }

    std::vector<RegistryPluginEntry> entries;
    try {
        auto j = nlohmann::json::parse(body);
        if (!j.is_array()) {
            spdlog::error("RemoteRegistryClient::listPlugins: expected JSON array");
            return {};
        }
        for (const auto& item : j) {
            RegistryPluginEntry entry;
            if (parseEntry(item, entry)) {
                entries.push_back(std::move(entry));
            }
        }
    } catch (const nlohmann::json::exception& ex) {
        spdlog::error("RemoteRegistryClient::listPlugins: JSON parse error: {}", ex.what());
    }
    spdlog::info("RemoteRegistryClient::listPlugins: found {} plugin(s)", entries.size());
    return entries;
}

std::optional<RegistryPluginEntry> RemoteRegistryClient::fetchPlugin(
    const std::string& name) {
    const std::string url = config_.registry_url + "/plugins/" + name;
    spdlog::debug("RemoteRegistryClient::fetchPlugin GET {}", url);

    std::string body;
    try {
        body = httpGet(url);
    } catch (const std::exception& ex) {
        spdlog::error("RemoteRegistryClient::fetchPlugin '{}': HTTP error: {}",
                      name, ex.what());
        return std::nullopt;
    }

    try {
        auto j = nlohmann::json::parse(body);
        RegistryPluginEntry entry;
        if (parseEntry(j, entry)) {
            return entry;
        }
    } catch (const nlohmann::json::exception& ex) {
        spdlog::error("RemoteRegistryClient::fetchPlugin '{}': JSON parse error: {}",
                      name, ex.what());
    }
    return std::nullopt;
}

// =============================================================================
// Download
// =============================================================================

PluginDownloadResult RemoteRegistryClient::downloadPlugin(
    const RegistryPluginEntry& entry) {
    PluginDownloadResult result;
    result.plugin_name = entry.name;
    result.version     = entry.version;

    if (entry.download_url.empty()) {
        result.error_message = "download_url is empty for plugin '" + entry.name + "'";
        spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
        return result;
    }

    // Determine local file name: <name>-<version>.<platform_ext>
#if defined(_WIN32)
    const std::string ext = ".dll";
#elif defined(__APPLE__)
    const std::string ext = ".dylib";
#else
    const std::string ext = ".so";
#endif
    const std::string filename = entry.name + "-" + entry.version + ext;
    std::filesystem::path dest_dir(config_.download_dir);

    // Create download directory if it doesn't exist.
    std::error_code ec;
    std::filesystem::create_directories(dest_dir, ec);
    if (ec) {
        result.error_message = "Cannot create download directory '" +
                               config_.download_dir + "': " + ec.message();
        spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
        return result;
    }

    const std::string local_path = (dest_dir / filename).string();

    spdlog::info("RemoteRegistryClient::downloadPlugin: downloading '{}' v{} -> {}",
                 entry.name, entry.version, local_path);

    if (!httpGetBinary(entry.download_url, local_path)) {
        result.error_message = "Failed to download '" + entry.download_url + "'";
        spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
        return result;
    }

    // Verify integrity only when a hash was provided.
    if (!entry.sha256.empty()) {
        if (!verifyIntegrity(local_path, entry.sha256)) {
            result.error_message = "SHA-256 mismatch for '" + entry.name +
                                   "': download may be corrupted or tampered";
            spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
            std::filesystem::remove(local_path, ec);
            return result;
        }
        spdlog::info("RemoteRegistryClient::downloadPlugin: integrity OK for '{}'",
                     entry.name);
    } else {
        spdlog::warn("RemoteRegistryClient::downloadPlugin: no SHA-256 provided for "
                     "'{}', skipping integrity check", entry.name);
    }

    result.success    = true;
    result.local_path = local_path;
    return result;
}

// =============================================================================
// Combined download + load
// =============================================================================

ModuleVerificationResult RemoteRegistryClient::downloadAndLoad(
    const RegistryPluginEntry& entry, ModuleLoader& loader) {
    auto dl = downloadPlugin(entry);

    if (!dl.success) {
        ModuleVerificationResult mvr;
        mvr.success      = false;
        mvr.errorCode    = ModuleErrorCode::MODULE_NOT_FOUND;
        mvr.errorCategory = ErrorCategory::TRANSIENT;
        mvr.errorMessage = dl.error_message;
        mvr.modulePath   = entry.download_url;
        return mvr;
    }

    return loader.loadModule(dl.local_path, entry.name);
}

// =============================================================================
// Private helpers
// =============================================================================

std::string RemoteRegistryClient::buildAuthorizationHeader() const {
    if (!config_.auth_token.empty()) {
        return "Authorization: Bearer " + config_.auth_token;
    }
    if (!config_.api_key.empty()) {
        return "X-API-Key: " + config_.api_key;
    }
    return {};
}

std::string RemoteRegistryClient::httpGet(const std::string& url) {
    const std::string auth_header = buildAuthorizationHeader();

    // Clamp max_retries to [0, kMaxAllowedRetries] to prevent overflow in the backoff shift.
    const int max_retries = std::max(0, std::min(config_.max_retries, kMaxAllowedRetries));
    const int attempts    = max_retries + 1;

    std::string last_error;

    for (int attempt = 0; attempt < attempts; ++attempt) {
        if (attempt > 0) {
            // Exponential backoff: 500 ms, 1000 ms, 2000 ms, … capped at 16 s.
            const int shift_amount = std::min(attempt - 1, kMaxBackoffShift);
            const int backoff_ms   = 500 * (1 << shift_amount);
            spdlog::warn("RemoteRegistryClient::httpGet: retry {}/{} after {}ms for {}",
                         attempt, max_retries, backoff_ms, url);
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("curl_easy_init() failed");
        }

        std::string body;
        struct curl_slist* headers = nullptr;

        if (!auth_header.empty()) {
            headers = curl_slist_append(headers, auth_header.c_str());
        }
        headers = curl_slist_append(headers, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(config_.timeout_ms));
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config_.verify_ssl ? 1L : 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config_.verify_ssl ? 2L : 0L);
        if (!config_.ca_bundle_path.empty()) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, config_.ca_bundle_path.c_str());
        }
        if (!config_.pinned_public_key.empty()) {
            curl_easy_setopt(curl, CURLOPT_PINNEDPUBLICKEY,
                             config_.pinned_public_key.c_str());
        }

        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            last_error = std::string("CURL error: ") + curl_easy_strerror(res);
            // Transient network error – retry
            continue;
        }

        // Authentication / authorisation failures are permanent; do not retry.
        if (http_code == 401 || http_code == 403) {
            throw std::runtime_error("Registry authentication failed (HTTP " +
                                     std::to_string(http_code) + ")");
        }
        if (http_code == 404) {
            throw std::runtime_error("Resource not found (HTTP 404): " + url);
        }
        if (http_code >= 500) {
            // Server error – transient, retry
            last_error = "Unexpected HTTP status " + std::to_string(http_code) +
                         " for " + url;
            continue;
        }
        if (http_code < 200 || http_code >= 300) {
            throw std::runtime_error("Unexpected HTTP status " +
                                     std::to_string(http_code) + " for " + url);
        }

        return body;
    }

    throw std::runtime_error(last_error.empty()
                                 ? "httpGet failed after retries"
                                 : last_error);
}

bool RemoteRegistryClient::httpGetBinary(const std::string& url,
                                         const std::string& out_path) {
    const std::string auth_header = buildAuthorizationHeader();

    // Clamp max_retries to [0, kMaxAllowedRetries] to prevent overflow in the backoff shift.
    const int max_retries = std::max(0, std::min(config_.max_retries, kMaxAllowedRetries));
    const int attempts    = max_retries + 1;

    for (int attempt = 0; attempt < attempts; ++attempt) {
        if (attempt > 0) {
            const int shift_amount = std::min(attempt - 1, kMaxBackoffShift);
            const int backoff_ms   = 500 * (1 << shift_amount);
            spdlog::warn("RemoteRegistryClient::httpGetBinary: retry {}/{} after {}ms",
                         attempt, max_retries, backoff_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            spdlog::error("RemoteRegistryClient::httpGetBinary: curl_easy_init() failed");
            return false;
        }

        std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            curl_easy_cleanup(curl);
            spdlog::error("RemoteRegistryClient::httpGetBinary: cannot open '{}' for writing",
                          out_path);
            return false;
        }

        struct curl_slist* headers = nullptr;
        if (!auth_header.empty()) {
            headers = curl_slist_append(headers, auth_header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(config_.timeout_ms));
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config_.verify_ssl ? 1L : 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config_.verify_ssl ? 2L : 0L);
        if (!config_.ca_bundle_path.empty()) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, config_.ca_bundle_path.c_str());
        }
        if (!config_.pinned_public_key.empty()) {
            curl_easy_setopt(curl, CURLOPT_PINNEDPUBLICKEY,
                             config_.pinned_public_key.c_str());
        }

        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        out.close();

        if (res != CURLE_OK) {
            spdlog::error("RemoteRegistryClient::httpGetBinary: CURL error: {}",
                          curl_easy_strerror(res));
            // Remove the incomplete file before retrying.
            std::error_code ec;
            std::filesystem::remove(out_path, ec);
            continue;
        }

        if (http_code < 200 || http_code >= 300) {
            spdlog::error("RemoteRegistryClient::httpGetBinary: HTTP {} for {}",
                          http_code, url);
            // Remove the incomplete file.
            std::error_code ec;
            std::filesystem::remove(out_path, ec);
            if (http_code >= 500) {
                // Server error – transient, retry
                continue;
            }
            return false;
        }

        return true;
    }

    spdlog::error("RemoteRegistryClient::httpGetBinary: all {} attempt(s) failed for {}",
                  attempts, url);
    return false;
}

/*static*/ bool RemoteRegistryClient::verifyIntegrity(
    const std::string& file_path, const std::string& expected_sha256) {
    const std::string actual = sha256File(file_path);
    if (actual.empty()) {
        spdlog::error("RemoteRegistryClient::verifyIntegrity: could not hash '{}'",
                      file_path);
        return false;
    }
    const bool ok = (actual == expected_sha256);
    if (!ok) {
        spdlog::error("RemoteRegistryClient::verifyIntegrity: hash mismatch for '{}' "
                      "(expected={}, actual={})", file_path, expected_sha256, actual);
    }
    return ok;
}

/*static*/ bool RemoteRegistryClient::parseEntry(const nlohmann::json& obj,
                                                  RegistryPluginEntry& out) {
    if (!obj.is_object()) return false;
    if (!obj.contains("name") || !obj.contains("download_url")) {
        return false;
    }
    out.name         = obj.value("name", "");
    out.version      = obj.value("version", "");
    out.description  = obj.value("description", "");
    out.download_url = obj.value("download_url", "");
    out.sha256       = obj.value("sha256", "");
    out.min_themis_version = obj.value("min_themis_version", "");
    return !out.name.empty() && !out.download_url.empty();
}

} // namespace modules
} // namespace themis
