/**
 * @file vault_key_provider.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=18, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/vault_key_provider.h"
#include <stdexcept>
#include "security/key_provider.h"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <mutex>
#include <chrono>
#include <random>
#include <thread>
#include <memory>

using json = nlohmann::json;

namespace themis {

// ============================================================================
// RAII Wrappers for CURL Resources
// ============================================================================

namespace {
    // RAII wrapper for CURL handles
    struct CURLDeleter {
        void operator()(CURL* p) const { if (p) curl_easy_cleanup(p); }
    };
    using CURL_ptr = std::unique_ptr<CURL, CURLDeleter>;

    // RAII wrapper for curl_slist headers
    struct CURLSListDeleter {
        void operator()(curl_slist* p) const { if (p) curl_slist_free_all(p); }
    };
    using CURLSList_ptr = std::unique_ptr<curl_slist, CURLSListDeleter>;

    bool starts_with(const std::string& value, const char* prefix) {
        return value.rfind(prefix, 0) == 0;
    }

    std::string extract_url_host(const std::string& url) {
        const size_t scheme_pos = url.find("://");
        const size_t host_start = (scheme_pos == std::string::npos) ? 0 : scheme_pos + 3;
        if (host_start >= static_cast<int>(url.size())) {
            return {};
        }

        if (url[host_start] == '[') {
            const size_t end_bracket = url.find(']', host_start + 1);
            if (end_bracket == std::string::npos) {
                return {};
            }
            return url.substr(host_start + 1, end_bracket - host_start - 1);
        }

        const size_t host_end = url.find_first_of(":/", host_start);
        return url.substr(host_start, host_end == std::string::npos ? std::string::npos
                                                                    : host_end - host_start);
    }

    bool is_loopback_host(const std::string& host) {
        return host == "localhost" || host == "127.0.0.1" || host == "::1";
    }

    bool is_loopback_url(const std::string& url) {
        return is_loopback_host(extract_url_host(url));
    }

    VaultKeyProvider::Config validate_vault_config(VaultKeyProvider::Config config) {
        if (config.vault_addr.empty()) {
            throw KeyOperationException("Vault address must not be empty");
        }
        if (config.vault_token.empty()) {
            throw KeyOperationException("Vault token must not be empty");
        }
        if (config.request_timeout_ms <= 0) {
            throw KeyOperationException("Vault request timeout must be greater than zero");
        }

        const bool uses_https = starts_with(config.vault_addr, "https://");
        const bool uses_http = starts_with(config.vault_addr, "http://");
        const bool loopback = is_loopback_url(config.vault_addr);

        if (!uses_https && !(uses_http && loopback)) {
            throw KeyOperationException(
                "Vault endpoint must use HTTPS; plain HTTP is only allowed for loopback development endpoints",
                -1,
                config.vault_addr,
                false);
        }

        if (!config.verify_ssl && !loopback) {
            throw KeyOperationException(
                "Vault TLS verification may only be disabled for loopback development endpoints",
                -1,
                config.vault_addr,
                false);
        }

        return config;
    }
}

// CURL write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Base64 decode helper
static std::vector<uint8_t> base64_decode(const std::string& encoded) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::vector<uint8_t> result = {};

    result.reserve(encoded.size() * 3 / 4);
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) {
      T[base64_chars[i]] = i;
    }
    
    int val = 0, valb = -8;
    for (unsigned char c : encoded) {
        if (T[c] == -1) {
          break;
        }
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return result;
}

// Base64 encode helper
static std::string base64_encode(const std::vector<uint8_t>& data) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string result = {};
    result.reserve(((static_cast<int>(data.size()) + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
      result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (result.size() % 4) {
      result.push_back('=');
    }
    return result;
}

// Internal implementation (PIMPL pattern)
struct VaultKeyProvider::Impl {
    Config config;
    CURL* curl;
    std::timed_mutex mutex;
    
    // Cache structure: "key_id:version" -> {key_bytes, expiry_time}
    struct CacheEntry {
        std::vector<uint8_t> key_bytes;
        int64_t expiry_ms;
        int64_t last_access_ms;
    };
    std::map<std::string, CacheEntry> cache;
    
    // Metrics
    size_t total_requests = 0;
    size_t cache_hits = 0;
    // Optional test hook to override HTTP behavior in unit tests
    std::function<std::string(const std::string&, const std::string&, const std::string&)> test_request_override;
    
    Impl(const Config& cfg) : config(cfg), curl(nullptr) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (!curl) {
            throw KeyOperationException("Failed to initialize libcurl");
        }
        
        // Set common options
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, config.request_timeout_ms);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config.verify_ssl ? 1L : 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config.verify_ssl ? 2L : 0L);
        test_request_override = nullptr;
    }
    
    ~Impl() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
    
    std::string performRequest(const std::string& url, const std::string& method,
                                const std::string& body = "") {
        // If a test hook is set, call it (bypass curl). Useful for unit tests.
        if (test_request_override) {
            return test_request_override(url, method, body);
        }

        // Duplicate the shared handle under the lock to get a private copy whose
        // common options (TLS, timeout …) are already configured.  This lets us
        // release the mutex before performing the blocking network call so that
        // other threads can proceed with cache lookups or rotate their own handles
        // concurrently.
        CURL_ptr local_curl_raw = nullptr;
        {
            std::lock_guard<std::timed_mutex> lock(mutex);
            CURL* raw_handle = curl_easy_duphandle(curl);
            if (!raw_handle) {
                throw KeyOperationException("curl_easy_duphandle failed", -1, std::string(), true);
            }
            local_curl_raw = CURL_ptr(raw_handle);
        }

        // Per-request setup on the private handle (no lock needed – local_curl is
        // not shared).
        std::string response = {};
        CURL* local_curl = local_curl_raw.get();
        curl_easy_setopt(local_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(local_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(local_curl, CURLOPT_WRITEDATA, &response);

        if (method == "GET") {
            curl_easy_setopt(local_curl, CURLOPT_HTTPGET, 1L);
        } else if (method == "POST") {
            curl_easy_setopt(local_curl, CURLOPT_POST, 1L);
            curl_easy_setopt(local_curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "LIST") {
            curl_easy_setopt(local_curl, CURLOPT_CUSTOMREQUEST, "LIST");
        }

        // Create headers list with RAII wrapper
        curl_slist* raw_headers = nullptr;
        raw_headers = curl_slist_append(raw_headers, ("X-Vault-Token: " + config.vault_token).c_str());
        if (!raw_headers) {
            throw KeyOperationException("Failed to create HTTP headers", -1, std::string(), false);
        }
        raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");
        if (!raw_headers) {
            throw KeyOperationException("Failed to append Content-Type header", -1, std::string(), false);
        }
        CURLSList_ptr headers(raw_headers);

        curl_easy_setopt(local_curl, CURLOPT_HTTPHEADER, headers.get());

        // Perform request – mutex NOT held, allowing concurrent cache reads.
        // RAII will ensure cleanup even if exceptions occur.
        CURLcode res = curl_easy_perform(local_curl);

        long http_code = 0;
        if (res == CURLE_OK) {
            if (curl_easy_getinfo(local_curl, CURLINFO_RESPONSE_CODE, &http_code) != CURLE_OK) {
                throw KeyOperationException("curl_easy_getinfo failed", -1, std::string(), false);
            }
        }
        // local_curl_raw and headers are automatically cleaned up here via RAII

        if (res != CURLE_OK) {
            bool transient = false;
            switch (res) {
                case CURLE_OPERATION_TIMEDOUT:
                [[fallthrough]];
case CURLE_COULDNT_CONNECT:
                [[fallthrough]];
case CURLE_COULDNT_RESOLVE_HOST:
                [[fallthrough]];
case CURLE_PARTIAL_FILE:
                    transient = true; break;
                default: transient = false; break;
            }
            throw KeyOperationException(std::string("CURL error: ") + curl_easy_strerror(res), -1, std::string(), transient);
        }

        if (http_code == 404) {
            throw KeyNotFoundException("key", 0);
        } else if (http_code == 403) {
            throw KeyOperationException("Vault authentication failed (403 Forbidden)", (int)http_code, response, false);
        } else if (http_code >= 500) {
            throw KeyOperationException("Vault server error (HTTP " + std::to_string(http_code) + ")", (int)http_code, response, true);
        } else if (http_code >= 400) {
            throw KeyOperationException("Vault request failed (HTTP " + std::to_string(http_code) + "): " + response, (int)http_code, response, false);
        }

        return response;
    }
    
    void evictExpiredCache() {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        for (auto it = cache.begin(); it != cache.end();) {
            if (it->second.expiry_ms < now) {
                it = cache.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void evictLRU() {
        if (static_cast<int>(cache.size()) < (size_t)config.cache_capacity) {
            return;
        }
        
        // Find oldest accessed entry
        auto oldest = cache.begin();
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if (it->second.last_access_ms < oldest->second.last_access_ms) {
                oldest = it;
            }
        }
        cache.erase(oldest);
    }
};

VaultKeyProvider::VaultKeyProvider(const Config& config) 
    : impl_(std::make_unique<Impl>(validate_vault_config(config))) 
{}

VaultKeyProvider::VaultKeyProvider(
    const std::string& vault_addr,
    const std::string& vault_token,
    const std::string& kv_mount_path
) : VaultKeyProvider([&]() {
        Config config;
        config.vault_addr = vault_addr;
        config.vault_token = vault_token;
        config.kv_mount_path = kv_mount_path;
        return config;
    }()) {}

VaultKeyProvider::~VaultKeyProvider() = default;

std::string VaultKeyProvider::httpGet(const std::string& path) {
    std::string url = impl_->config.vault_addr + path;
    return impl_->performRequest(url, "GET");
}

std::string VaultKeyProvider::httpPost(const std::string& path, const std::string& body) {
    std::string url = impl_->config.vault_addr + path;
    return impl_->performRequest(url, "POST", body);
}

std::string VaultKeyProvider::httpList(const std::string& path) {
    std::string url = impl_->config.vault_addr + path;
    return impl_->performRequest(url, "LIST");
}

void VaultKeyProvider::setTestRequestOverride(std::function<std::string(const std::string&, const std::string&, const std::string&)> fn) {
    impl_->test_request_override = std::move(fn);
}

std::string VaultKeyProvider::readSecret(const std::string& key_id, uint32_t version) {
    std::string path = {};
    if (impl_->config.kv_version == "v2") {
        path = "/v1/" + impl_->config.kv_mount_path + "/data/keys/" + key_id;
        if (version > 0) {
            path += "?version=" + std::to_string(version);
        }
    } else {
        // KV v1
        path = "/v1/" + impl_->config.kv_mount_path + "/keys/" + key_id;
    }
    
    try {
        return httpGet(path);
    } catch (const KeyNotFoundException&) {
        throw KeyNotFoundException(key_id, version);
    }
}

std::string VaultKeyProvider::readSecretMetadata(const std::string& key_id) {
    if (impl_->config.kv_version != "v2") {
        throw KeyOperationException("Metadata only available in KV v2");
    }
    
    std::string path = "/v1/" + impl_->config.kv_mount_path + "/metadata/keys/" + key_id;
    try {
        return httpGet(path);
    } catch (const KeyNotFoundException&) {
        throw KeyNotFoundException(key_id, 0);
    }
}

void VaultKeyProvider::writeSecret(const std::string& key_id, const std::string& key_b64, uint32_t version) {
    json payload = {};
    
    if (impl_->config.kv_version == "v2") {
        payload["data"] = {
            {"key", key_b64},
            {"algorithm", "AES-256-GCM"},
            {"version", version}
        };
    } else {
        payload = {
            {"key", key_b64},
            {"algorithm", "AES-256-GCM"},
            {"version", version}
        };
    }
    
    std::string path = "/v1/" + impl_->config.kv_mount_path + 
                       (impl_->config.kv_version == "v2" ? "/data/keys/" : "/keys/") + key_id;
    
    httpPost(path, payload.dump());
}

std::vector<std::string> VaultKeyProvider::listSecrets() {
    std::string path = "/v1/" + impl_->config.kv_mount_path + 
                       (impl_->config.kv_version == "v2" ? "/metadata/keys" : "/keys");

    std::string response = {};
    // Some Vault setups / client stacks return the key list for KV v2 when using
    // a GET with the query parameter `?list=true` instead of the HTTP LIST verb.
    if (impl_->config.kv_version == "v2") {
        response = httpGet(path + "?list=true");
    } else {
        response = httpList(path);
    }

    try {
        json j = json::parse(response);

        std::vector<std::string> keys = {};

        if (j.contains("data") && j["data"].contains("keys")) {
            keys.reserve(j["data"]["keys"].size());
            for (const auto& key : j["data"]["keys"]) {
                keys.push_back(key.get<std::string>());
            }
        }
        return keys;
    } catch (const std::exception& e) {
        throw KeyOperationException("Failed to parse Vault key list response: " + std::string(e.what()), -1, response, false);
    }
}

std::vector<uint8_t> VaultKeyProvider::parseKeyFromVaultResponse(const std::string& json_response) {
    try {
        json j = json::parse(json_response);
         
        std::string key_b64 = {};
        if (impl_->config.kv_version == "v2") {
            if (!j.contains("data") || !j["data"].contains("data")) {
                throw KeyOperationException("Invalid Vault response format (missing data.data)");
            }
            key_b64 = j["data"]["data"]["key"].get<std::string>();
        } else {
            if (!j.contains("data")) {
                throw KeyOperationException("Invalid Vault response format (missing data)");
            }
            key_b64 = j["data"]["key"].get<std::string>();
        }
         
        auto key_bytes = base64_decode(key_b64);
        if (key_bytes.empty()) {
            throw KeyOperationException("Vault returned an empty key - refusing to use zero-length key material");
        }
        return key_bytes;
    } catch (const KeyOperationException&) {
        throw;
    } catch (const std::exception& e) {
        throw KeyOperationException("Failed to parse Vault response: " + std::string(e.what()), -1, json_response, false);
    }
}

KeyMetadata VaultKeyProvider::parseMetadataFromVaultResponse(const std::string& json_response) {
    try {
        json j = json::parse(json_response);
         
        KeyMetadata meta = {};
         
        if (impl_->config.kv_version == "v2") {
            if (!j.contains("data")) {
                throw KeyOperationException("Invalid Vault metadata response");
            }
             
            const auto& data = j["data"];
             
            // Get current version
            if (data.contains("current_version")) {
                meta.version = data["current_version"].get<uint32_t>();
            }
             
            // Get creation time from versions
            if (data.contains("versions") && !data["versions"].empty()) {
                auto version_key = std::to_string(meta.version);
                if (data["versions"].contains(version_key)) {
                    const auto& version_data = data["versions"][version_key];
                    if (version_data.contains("created_time")) {
                        // Parse RFC3339 timestamp (simplified)
                        std::string created = version_data["created_time"].get<std::string>();
                        // For now, use current time (proper parsing would use strptime)
                        meta.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count();
                    }
                }
            }
        }
         
        meta.algorithm = "AES-256-GCM";
        meta.status = KeyStatus::ACTIVE;
        meta.expires_at_ms = 0;
         
        return meta;
    } catch (const std::exception& e) {
        throw KeyOperationException("Failed to parse Vault metadata response: " + std::string(e.what()), -1, json_response, false);
    }
}

std::string VaultKeyProvider::makeCacheKey(const std::string& key_id, uint32_t version) const {
    return key_id + ":" + std::to_string(version);
}

std::vector<uint8_t> VaultKeyProvider::getKey(const std::string& key_id) {
    return getKey(key_id, 0);  // 0 = latest version
}

std::vector<uint8_t> VaultKeyProvider::getKey(const std::string& key_id, uint32_t version) {
    std::unique_lock<std::timed_mutex> lock(impl_->mutex, std::defer_lock);
    if (!lock.try_lock_for(std::chrono::seconds(5))) {
        throw KeyOperationException("Cache lock timeout on entry", -1, std::string(), true);
    }
    
    impl_->total_requests++;
    
    // Check cache
    std::string cache_key = makeCacheKey(key_id, version);
    auto it = impl_->cache.find(cache_key);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (it != impl_->cache.end() && it->second.expiry_ms > now) {
        // Cache hit
        impl_->cache_hits++;
        it->second.last_access_ms = now;
        return it->second.key_bytes;
    }
    
    // Cache miss - fetch from Vault: release lock so performRequest can acquire it.
    lock.unlock();
    std::string response = readSecret(key_id, version);
    std::vector<uint8_t> key_bytes = parseKeyFromVaultResponse(response);
    if (!lock.try_lock_for(std::chrono::seconds(5))) {
        throw KeyOperationException("Cache lock timeout after Vault fetch", -1, std::string(), true);
    }
    
    // Store in cache
    impl_->evictExpiredCache();
    impl_->evictLRU();
    
    Impl::CacheEntry entry;
    entry.key_bytes = key_bytes;
    entry.expiry_ms = now + (impl_->config.cache_ttl_seconds * 1000);
    entry.last_access_ms = now;
    
    impl_->cache[cache_key] = entry;
    
    return key_bytes;
}

uint32_t VaultKeyProvider::rotateKey(const std::string& key_id) {
    // Get current metadata to find latest version
    std::string metadata_response = readSecretMetadata(key_id);
    KeyMetadata meta = parseMetadataFromVaultResponse(metadata_response);
     
    uint32_t new_version = meta.version + 1;
     
    // Generate new random key
    std::vector<uint8_t> new_key(32);  // 256 bits
    if (RAND_bytes(new_key.data(), 32) != 1) {
        throw KeyOperationException("Failed to generate random key material for rotation", -1, std::string(), false);
    }
     
    std::string key_b64 = base64_encode(new_key);
     
    // Write new version to Vault
    writeSecret(key_id, key_b64, new_version);
     
    // Invalidate cache for this key_id
    std::lock_guard<std::timed_mutex> lock(impl_->mutex);
    for (auto it = impl_->cache.begin(); it != impl_->cache.end();) {
        if (it->first.find(key_id + ":") == 0) {
            it = impl_->cache.erase(it);
        } else {
            ++it;
        }
    }
     
    return new_version;
}

std::vector<KeyMetadata> VaultKeyProvider::listKeys() {
    std::vector<std::string> key_ids = listSecrets();
    std::vector<KeyMetadata> result = {};

    result.reserve(key_ids.size());
    
    for (const auto& key_id : key_ids) {
        try {
            KeyMetadata meta = getKeyMetadata(key_id, 0);
            meta.key_id = key_id;
            result.push_back(meta);
        } catch (const std::exception&) {
            // Skip keys that can't be read
            continue;
        }
    }
    
    return result;
}

SigningResult VaultKeyProvider::sign(const std::string& key_id, const std::vector<uint8_t>& data) {
    // Build path for transit sign: /v1/<transit_mount>/sign/<key_id>
    std::string mount = impl_->config.transit_mount.empty() ? std::string("transit") : impl_->config.transit_mount;
    std::string path = "/v1/" + mount + "/sign/" + key_id;

    // Prepare JSON payload
    nlohmann::json payload;
    payload["input"] = base64_encode(data);

    // Retry/backoff parameters from config
    int max_retries = impl_->config.transit_max_retries > 0 ? impl_->config.transit_max_retries : 3;
    int base_backoff_ms = impl_->config.transit_backoff_ms > 0 ? impl_->config.transit_backoff_ms : 200;

    std::random_device rd = {};
    std::mt19937 rng(rd());
    std::uniform_real_distribution<double> jitter_dist(0.5, 1.5);

    int attempt = 0;
    while (true) {
        ++attempt;
        try {
            std::string response = httpPost(path, payload.dump());
            // Parse response and extract signature
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(response);
            } catch (const std::exception& e) {
                throw KeyOperationException("Failed to parse Vault transit response: " + std::string(e.what()), -1, response, false);
            }
             
            std::string sig_b64 = {};
            if (j.contains("data") && j["data"].contains("signature")) {
                sig_b64 = j["data"]["signature"].get<std::string>();
            } else if (j.contains("data") && j["data"].contains("signatures") && j["data"]["signatures"].is_array()) {
                sig_b64 = j["data"]["signatures"][0].get<std::string>();
            } else if (j.contains("data") && j["data"].contains("signed")) {
                sig_b64 = j["data"]["signed"].get<std::string>();
            }

            if (!sig_b64.empty()) {
                if (sig_b64.rfind("vault:", 0) == 0) {
                    size_t pos = sig_b64.find(':', 6);
                    if (pos != std::string::npos && pos + 1 < sig_b64.size()) {
                        sig_b64 = sig_b64.substr(pos + 1);
                    }
                }
                SigningResult res;
                res.signature = base64_decode(sig_b64);
                res.algorithm = "VAULT+TRANSIT";
                return res;
            }

            // If response parsed but no signature -> permanent error
            throw KeyOperationException("Vault transit sign returned unexpected payload", -1, response, false);

        } catch (const KeyOperationException& koe) {
            // If already a KeyOperationException, decide whether to retry based on transient flag
            if (koe.transient() && attempt <= max_retries) {
                double factor = jitter_dist(rng);
                int sleep_ms = static_cast<int>(base_backoff_ms * factor);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                base_backoff_ms *= 2; // exponential base for next attempt
                continue;
            }
            throw;
        } catch (const std::exception& ex) {
            // Treat network/parse errors as transient for retry purposes for a few attempts
            bool is_transient = true;
            if (attempt <= max_retries) {
                double factor = jitter_dist(rng);
                int sleep_ms = static_cast<int>(base_backoff_ms * factor);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                base_backoff_ms *= 2;
                continue;
            }
            throw KeyOperationException(std::string("Vault transit sign failed: ") + ex.what(), -1, std::string(), is_transient);
        }
    }
}

KeyMetadata VaultKeyProvider::getKeyMetadata(const std::string& key_id, uint32_t version) {
    std::string response = readSecretMetadata(key_id);
    KeyMetadata meta = parseMetadataFromVaultResponse(response);
    meta.key_id = key_id;
    
    if (version > 0) {
        meta.version = version;
    }
    
    return meta;
}

void VaultKeyProvider::deleteKey(const std::string& key_id, uint32_t version) {
    // In Vault KV v2, deletion is done via DELETE /metadata/keys/:path
    // This soft-deletes the key (marks as deleted, can be recovered)
    
    if (impl_->config.kv_version != "v2") {
        throw KeyOperationException("Key deletion only supported in KV v2");
    }
    
    // Check if key is deprecated first (safety check)
    try {
        KeyMetadata meta = getKeyMetadata(key_id, version);
        if (meta.status == KeyStatus::ACTIVE) {
            throw KeyOperationException("Cannot delete ACTIVE key. Deprecate it first via rotation.");
        }
    } catch (const KeyNotFoundException&) {
        // Key already doesn't exist, that's fine
        return;
    }
    
    // Perform deletion via Vault API
    std::string path = "/v1/" + impl_->config.kv_mount_path + "/metadata/keys/" + key_id;

    // Duplicate the shared handle under the lock, then release before the
    // blocking network call (same pattern as performRequest).
    std::string url = impl_->config.vault_addr + path;
    std::string vault_token = {};
    CURL_ptr local_curl_raw = nullptr;
    {
        std::lock_guard<std::timed_mutex> lock(impl_->mutex);
        CURL* raw_handle = curl_easy_duphandle(impl_->curl);
        if (!raw_handle) {
            throw KeyOperationException("curl_easy_duphandle failed during deleteKey");
        }
        local_curl_raw = CURL_ptr(raw_handle);
        vault_token  = impl_->config.vault_token;
    }

    std::string response = {};
    CURL* local_curl = local_curl_raw.get();
    curl_easy_setopt(local_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(local_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(local_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(local_curl, CURLOPT_WRITEDATA, &response);

    curl_slist* raw_headers = nullptr;
    raw_headers = curl_slist_append(raw_headers, ("X-Vault-Token: " + vault_token).c_str());
    if (!raw_headers) {
        throw KeyOperationException("Failed to create HTTP headers for deleteKey", -1, std::string(), false);
    }
    CURLSList_ptr headers(raw_headers);
    curl_easy_setopt(local_curl, CURLOPT_HTTPHEADER, headers.get());

    // Perform request with RAII ensuring cleanup
    CURLcode res = curl_easy_perform(local_curl);
    // RAII wrappers automatically clean up local_curl_raw and headers

    if (res != CURLE_OK) {
        throw KeyOperationException(std::string("Failed to delete key: ") + curl_easy_strerror(res));
    }
    
    // Clear from cache (re-acquire mutex for cache modification)
    std::lock_guard<std::timed_mutex> cache_lock(impl_->mutex);
    for (auto it = impl_->cache.begin(); it != impl_->cache.end();) {
        if (it->first.find(key_id + ":") == 0) {
            it = impl_->cache.erase(it);
        } else {
            ++it;
        }
    }
}

bool VaultKeyProvider::hasKey(const std::string& key_id, uint32_t version) {
    try {
        getKeyMetadata(key_id, version);
        return true;
    } catch (const KeyNotFoundException&) {
        return false;
    }
}

uint32_t VaultKeyProvider::createKeyFromBytes(
    const std::string& key_id,
    const std::vector<uint8_t>& key_bytes,
    const KeyMetadata& metadata) {
    
    if (static_cast<int>(key_bytes.size()) != 32) {
        throw KeyOperationException("Key must be exactly 32 bytes (256 bits)");
    }
    
    // In Vault, we store the key as base64 and metadata as JSON
    // For KV v2: PUT /v1/{mount}/data/{key_id}
    std::string path = "/v1/" + impl_->config.kv_mount_path;
    if (impl_->config.kv_version == "v2") {
        path += "/data/keys/" + key_id;
    } else {
        path += "/keys/" + key_id;
    }
    
    // Base64 encode the key
    std::string key_b64 = base64_encode(key_bytes);
    
    // Create JSON payload
    nlohmann::json payload;
    if (impl_->config.kv_version == "v2") {
        payload["data"]["key"] = key_b64;
    payload["data"]["created_at_ms"] = metadata.created_at_ms;
        payload["data"]["algorithm"] = metadata.algorithm;
        payload["data"]["status"] = static_cast<int>(metadata.status);
    } else {
        payload["key"] = key_b64;
    payload["created_at_ms"] = metadata.created_at_ms;
        payload["algorithm"] = metadata.algorithm;
        payload["status"] = static_cast<int>(metadata.status);
    }
    
    std::string payload_str = payload.dump();

    // Duplicate the shared handle under the lock, then release before the
    // blocking network call (same pattern as performRequest).
    std::string url = impl_->config.vault_addr + path;
    std::string vault_token = {};
    std::string kv_version = {};
    CURL_ptr local_curl_raw = nullptr;
    {
        std::lock_guard<std::timed_mutex> lock(impl_->mutex);
        CURL* raw_handle = curl_easy_duphandle(impl_->curl);
        if (!raw_handle) {
            throw KeyOperationException("curl_easy_duphandle failed during createKey");
        }
        local_curl_raw = CURL_ptr(raw_handle);
        vault_token = impl_->config.vault_token;
        kv_version  = impl_->config.kv_version;
    }

    std::string response = {};
    CURL* local_curl = local_curl_raw.get();
    curl_easy_setopt(local_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(local_curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(local_curl, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(local_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(local_curl, CURLOPT_WRITEDATA, &response);

    curl_slist* raw_headers = nullptr;
    raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");
    if (!raw_headers) {
        throw KeyOperationException("Failed to create HTTP headers for createKey", -1, std::string(), false);
    }
    raw_headers = curl_slist_append(raw_headers, ("X-Vault-Token: " + vault_token).c_str());
    if (!raw_headers) {
        throw KeyOperationException("Failed to append Vault-Token header to createKey", -1, std::string(), false);
    }
    CURLSList_ptr headers(raw_headers);
    curl_easy_setopt(local_curl, CURLOPT_HTTPHEADER, headers.get());

    // Perform request with RAII ensuring cleanup
    CURLcode res = curl_easy_perform(local_curl);
    // RAII wrappers automatically clean up local_curl_raw and headers

    if (res != CURLE_OK) {
        throw KeyOperationException(std::string("Failed to create key: ") + curl_easy_strerror(res));
    }

    // Parse response to get version
    try {
        nlohmann::json resp_json = nlohmann::json::parse(response);
        if (kv_version == "v2" && resp_json.contains("data") && resp_json["data"].contains("version")) {
            return resp_json["data"]["version"].get<uint32_t>();
        }
    } catch (const std::exception&) {
        // If parsing fails, return version 1 as default
    }

    return 1;
}

void VaultKeyProvider::clearCache() {
    std::lock_guard<std::timed_mutex> lock(impl_->mutex);
    impl_->cache.clear();
}

VaultKeyProvider::CacheStats VaultKeyProvider::getCacheStats() const {
    std::lock_guard<std::timed_mutex> lock(impl_->mutex);
    
    CacheStats stats;
    stats.total_requests = impl_->total_requests;
    stats.cache_hits = impl_->cache_hits;
    stats.cache_size = impl_->cache.size();
    stats.hit_rate = impl_->total_requests > 0 
        ? (double)impl_->cache_hits / impl_->total_requests 
        : 0.0;
    
    return stats;
}

} // namespace themis


