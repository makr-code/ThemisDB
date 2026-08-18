/**
 * @file oci_registry_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "plugins/oci_registry_client.h"
#include <stdexcept>
#include "utils/logger.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <regex>
#include <sstream>

namespace themis {
namespace plugins {

namespace fs = std::filesystem;
using json = nlohmann::json;
using errors::ErrorCode;

// ============================================================================
// curl write callbacks
// ============================================================================

namespace {

size_t curlWriteString(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* buf = static_cast<std::string*>(userp);
    buf->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

struct CurlFileWriter {
    std::ofstream file;
    bool ok = true;
};

size_t curlWriteFile(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* writer = static_cast<CurlFileWriter*>(userp);
    writer->file.write(static_cast<char*>(contents),
                       static_cast<std::streamsize>(size * nmemb));
    if (!writer->file) {
        writer->ok = false;
        return 0;  // Signal error to libcurl
    }
    return size * nmemb;
}

// Collect response headers into a map (lower-case key -> value).
size_t curlHeaderCallback(char* buffer, size_t size, size_t nitems, void* userp) {
    auto* headers = static_cast<std::unordered_map<std::string, std::string>*>(userp);
    std::string line(buffer, size * nitems);
    // Trim trailing CRLF
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    auto colon = line.find(':');
    if (colon != std::string::npos) {
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        // Lower-case key
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        // Trim leading spaces from value
        while (!val.empty() && val.front() == ' ') val.erase(val.begin());
        (*headers)[key] = val;
    }
    return size * nitems;
}

// RAII deleter for EVP_MD_CTX to ensure cleanup on all paths (including exceptions).
struct EvpMdCtxDeleter {
    void operator()(EVP_MD_CTX* p) const noexcept { if (p) EVP_MD_CTX_free(p); }
};

// Compute hex-encoded SHA-256 of a file.
std::string sha256HexFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};

    std::unique_ptr<EVP_MD_CTX, EvpMdCtxDeleter> ctx(EVP_MD_CTX_new());
    if (!ctx) return {};

    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        return {};
    }

    std::array<char, 8192> buf{};
    while (f.read(buf.data(), static_cast<std::streamsize>(buf.size())) || f.gcount() > 0) {
        if (EVP_DigestUpdate(ctx.get(), buf.data(), static_cast<size_t>(f.gcount())) != 1) {
            return {};
        }
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digest_len) != 1) {
        return {};
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::setw(2) << static_cast<unsigned>(digest[i]);
    }
    return oss.str();
}

} // anonymous namespace

// ============================================================================
// OciReference
// ============================================================================

Result<OciReference> OciReference::parse(const std::string& raw) {
    if (raw.empty()) {
        return Err<OciReference>(ErrorCode::ERR_PLUGIN_OCI_INVALID_REFERENCE,
                                 "OCI reference string is empty");
    }

    OciReference ref;

    // Split off optional @digest suffix first.
    std::string remainder = raw;
    {
        auto at_pos = remainder.rfind('@');
        if (at_pos != std::string::npos) {
            ref.digest   = remainder.substr(at_pos + 1);
            remainder    = remainder.substr(0, at_pos);
        }
    }

    // Split off optional :tag suffix.
    // Be careful: registry host may contain a port like "localhost:5000".
    // Only the LAST colon after the last '/' is the tag separator.
    {
        auto slash_pos = remainder.rfind('/');
        std::string name_part;
        std::string registry_part;

        if (slash_pos == std::string::npos) {
            // No slash at all – treat as plain image name with implicit registry.
            name_part     = remainder;
            registry_part = "registry-1.docker.io";
        } else {
            // Check whether the portion before the first '/' looks like a
            // hostname (contains '.' or ':', or is "localhost").
            auto first_slash = remainder.find('/');
            std::string potential_host = remainder.substr(0, first_slash);
            bool has_dot   = potential_host.find('.') != std::string::npos;
            bool has_colon = potential_host.find(':') != std::string::npos;
            bool is_localhost = (potential_host == "localhost");

            if (has_dot || has_colon || is_localhost) {
                registry_part = potential_host;
                name_part     = remainder.substr(first_slash + 1);
            } else {
                registry_part = "registry-1.docker.io";
                name_part     = remainder;
            }
        }

        // Extract tag from name_part (last ':' after last '/')
        auto colon_pos = name_part.rfind(':');
        if (colon_pos != std::string::npos) {
            ref.tag       = name_part.substr(colon_pos + 1);
            name_part     = name_part.substr(0, colon_pos);
        }

        ref.registry = registry_part;
        ref.name     = name_part;
    }

    if (ref.name.empty()) {
        return Err<OciReference>(ErrorCode::ERR_PLUGIN_OCI_INVALID_REFERENCE,
                                 "OCI reference has empty repository name: " + raw);
    }
    if (ref.tag.empty() && ref.digest.empty()) {
        ref.tag = "latest";
    }

    return Ok(ref);
}

std::string OciReference::toString() const {
    std::string s = registry + "/" + name;
    if (!tag.empty())    s += ":" + tag;
    if (!digest.empty()) s += "@" + digest;
    return s;
}

// ============================================================================
// OciRegistryClient
// ============================================================================

OciRegistryClient::OciRegistryClient() {
    // curl_global_init is idempotent when called multiple times in the same
    // process, but the application is expected to have called it already.
    // We call it here for safety; CURL_GLOBAL_DEFAULT is safe to call once.
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

OciRegistryClient::~OciRegistryClient() = default;

void OciRegistryClient::setAuth(const std::string& registry, OciAuthConfig auth) {
    std::lock_guard<std::mutex> lk(mutex_);
    auth_configs_[registry] = std::move(auth);
}

void OciRegistryClient::setTimeout(long timeout_seconds) {
    std::lock_guard<std::mutex> lk(mutex_);
    timeout_seconds_ = timeout_seconds;
}

// ============================================================================
// Internal helpers
// ============================================================================

/* static */ std::string OciRegistryClient::registryBaseUrl(const std::string& registry) {
    // Use HTTPS unless the host is localhost or a loopback address.
    bool is_local = (registry.rfind("localhost", 0) == 0) ||
                    (registry.rfind("127.", 0) == 0) ||
                    (registry.rfind("[::1]", 0) == 0);
    std::string scheme = is_local ? "http" : "https";
    return scheme + "://" + registry;
}

Result<std::string> OciRegistryClient::httpGet(
    const std::string& url,
    const std::vector<std::string>& extra_headers)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "curl_easy_init() failed");
    }

    std::string response_body;
    std::unordered_map<std::string, std::string> response_headers;

    long timeout = 30;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        timeout = timeout_seconds_;
    }

    curl_slist* headers_list = nullptr;
    for (const auto& h : extra_headers) {
        headers_list = curl_slist_append(headers_list, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlHeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (headers_list) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
    }

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers_list);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                std::string("curl error: ") + curl_easy_strerror(res));
    }
    if (http_code == 404) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_MANIFEST_NOT_FOUND,
                                "HTTP 404 for URL: " + url);
    }
    if (http_code == 401 || http_code == 403) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "HTTP " + std::to_string(http_code) +
                                " (auth required) for URL: " + url);
    }
    if (http_code < 200 || http_code >= 300) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "HTTP " + std::to_string(http_code) + " for URL: " + url);
    }

    return Ok(response_body);
}

Result<void> OciRegistryClient::httpGetToFile(
    const std::string& url,
    const std::string& dest_path,
    const std::vector<std::string>& extra_headers)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        return ErrVoid(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED, "curl_easy_init() failed");
    }

    CurlFileWriter writer;
    writer.file.open(dest_path, std::ios::binary | std::ios::trunc);
    if (!writer.file) {
        curl_easy_cleanup(curl);
        return ErrVoid(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                       "Cannot open dest file for writing: " + dest_path);
    }

    long timeout = 30;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        timeout = timeout_seconds_;
    }

    curl_slist* headers_list = nullptr;
    for (const auto& h : extra_headers) {
        headers_list = curl_slist_append(headers_list, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (headers_list) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
    }

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers_list);
    curl_easy_cleanup(curl);
    writer.file.close();

    if (res != CURLE_OK) {
        fs::remove(dest_path);
        return ErrVoid(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                       std::string("curl error: ") + curl_easy_strerror(res));
    }
    if (!writer.ok) {
        fs::remove(dest_path);
        return ErrVoid(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                       "Write error while downloading to: " + dest_path);
    }
    if (http_code == 404) {
        fs::remove(dest_path);
        return ErrVoid(ErrorCode::ERR_PLUGIN_OCI_MANIFEST_NOT_FOUND,
                       "HTTP 404 for blob URL: " + url);
    }
    if (http_code < 200 || http_code >= 300) {
        fs::remove(dest_path);
        return ErrVoid(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                       "HTTP " + std::to_string(http_code) + " for blob URL: " + url);
    }

    return OkVoid();
}

Result<std::string> OciRegistryClient::obtainBearerToken(
    const std::string& registry,
    const std::string& scope)
{
    // Perform a challenge request to /v2/ to get the WWW-Authenticate header.
    std::string challenge_url = registryBaseUrl(registry) + "/v2/";

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "curl_easy_init() failed during token challenge");
    }

    std::unordered_map<std::string, std::string> resp_headers;
    std::string body;

    curl_easy_setopt(curl, CURLOPT_URL, challenge_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlHeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp_headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // Parse WWW-Authenticate: Bearer realm="...",service="...",scope="..."
    auto it = resp_headers.find("www-authenticate");
    if (it == resp_headers.end()) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "No WWW-Authenticate header from registry: " + registry);
    }

    const std::string& www_auth = it->second;
    std::string realm, service;

    auto extract = [&](const std::string& key) -> std::string {
        std::regex re(key + "=\"([^\"]*)\"");
        std::smatch m;
        if (std::regex_search(www_auth, m, re)) return m[1].str();
        return {};
    };

    realm   = extract("realm");
    service = extract("service");

    if (realm.empty()) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "Cannot parse realm from WWW-Authenticate: " + www_auth);
    }

    // Build token endpoint URL.
    std::string token_url = realm + "?service=" + service + "&scope=" + scope;

    // Build auth headers.
    std::vector<std::string> token_headers;
    OciAuthConfig auth;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto ait = auth_configs_.find(registry);
        if (ait != auth_configs_.end()) auth = ait->second;
    }

    CURL* token_curl = curl_easy_init();
    if (!token_curl) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "curl_easy_init() failed for token fetch");
    }

    std::string token_body;
    curl_easy_setopt(token_curl, CURLOPT_URL, token_url.c_str());
    curl_easy_setopt(token_curl, CURLOPT_WRITEFUNCTION, curlWriteString);
    curl_easy_setopt(token_curl, CURLOPT_WRITEDATA, &token_body);
    curl_easy_setopt(token_curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(token_curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (!auth.username.empty()) {
        std::string userpass = auth.username + ":" + auth.password;
        curl_easy_setopt(token_curl, CURLOPT_USERPWD, userpass.c_str());
        curl_easy_setopt(token_curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    }

    CURLcode tres = curl_easy_perform(token_curl);
    long tcode = 0;
    curl_easy_getinfo(token_curl, CURLINFO_RESPONSE_CODE, &tcode);
    curl_easy_cleanup(token_curl);

    if (tres != CURLE_OK) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                std::string("Token fetch curl error: ") + curl_easy_strerror(tres));
    }
    if (tcode < 200 || tcode >= 300) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "Token endpoint returned HTTP " + std::to_string(tcode));
    }

    try {
        auto tj = json::parse(token_body);
        // Docker Hub returns "token"; ECR/GHCR return "access_token"
        if (tj.contains("token"))        return Ok(tj["token"].get<std::string>());
        if (tj.contains("access_token")) return Ok(tj["access_token"].get<std::string>());
    } catch (...) {}

    return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                            "Cannot parse token from registry response");
}

/* static */ bool OciRegistryClient::verifyDigest(
    const std::string& file_path,
    const std::string& expected_digest)
{
    // expected_digest is in the form "sha256:<hex>".
    if (expected_digest.size() <= 7 ||
        expected_digest.substr(0, 7) != "sha256:") {
        return false;
    }
    std::string expected_hex = expected_digest.substr(7);
    std::string actual_hex   = sha256HexFile(file_path);
    return !actual_hex.empty() && (actual_hex == expected_hex);
}

// ============================================================================
// Public API
// ============================================================================

Result<OciManifest> OciRegistryClient::fetchManifest(const OciReference& ref) {
    const std::string base = registryBaseUrl(ref.registry);
    const std::string name_enc = ref.name;  // URL path segments don't need percent-encoding here
    std::string ref_str = ref.digest.empty() ? ref.tag : ref.digest;

    std::string url = base + "/v2/" + name_enc + "/manifests/" + ref_str;

    // Accepted media types (OCI + Docker v2 for compatibility).
    std::vector<std::string> accept_headers = {
        "Accept: application/vnd.oci.image.manifest.v1+json",
        "Accept: application/vnd.docker.distribution.manifest.v2+json",
    };

    // Inject authentication.
    OciAuthConfig auth;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = auth_configs_.find(ref.registry);
        if (it != auth_configs_.end()) auth = it->second;
    }

    if (!auth.bearer_token.empty()) {
        accept_headers.push_back("Authorization: Bearer " + auth.bearer_token);
    } else if (!auth.username.empty()) {
        // For registries that require token auth, attempt token flow.
        std::string scope = "repository:" + ref.name + ":pull";
        auto token_res = obtainBearerToken(ref.registry, scope);
        if (token_res.has_value()) {
            accept_headers.push_back("Authorization: Bearer " + *token_res);
        }
        // If token acquisition fails, proceed without auth (may still work for public repos).
    }

    auto body_res = httpGet(url, accept_headers);
    if (!body_res.has_value()) {
        return Err<OciManifest>(body_res.error().code(), body_res.error().context());
    }

    // Parse manifest JSON.
    OciManifest manifest;
    manifest.raw_json = *body_res;
    try {
        auto mj = json::parse(*body_res);

        manifest.schema_version = mj.value("schemaVersion", 2);
        manifest.media_type     = mj.value("mediaType", "");

        if (mj.contains("config") && mj["config"].is_object()) {
            const auto& cfg = mj["config"];
            manifest.config.media_type = cfg.value("mediaType", "");
            manifest.config.digest     = cfg.value("digest", "");
            manifest.config.size       = cfg.value("size", int64_t{0});
        }

        if (mj.contains("layers") && mj["layers"].is_array()) {
            for (const auto& layer : mj["layers"]) {
                OciManifestLayer l;
                l.media_type = layer.value("mediaType", "");
                l.digest     = layer.value("digest", "");
                l.size       = layer.value("size", int64_t{0});
                manifest.layers.push_back(std::move(l));
            }
        }
    } catch (const std::exception& e) {
        return Err<OciManifest>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                std::string("Failed to parse OCI manifest JSON: ") + e.what());
    }

    THEMIS_INFO("OciRegistryClient: fetched manifest for {} ({} layers)",
                ref.toString(), manifest.layers.size());

    return Ok(manifest);
}

Result<std::string> OciRegistryClient::pullPluginBinary(
    const OciReference& ref,
    const std::string& dest_dir)
{
    // Ensure destination directory exists.
    std::error_code ec;
    fs::create_directories(dest_dir, ec);
    if (ec) {
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "Cannot create dest_dir: " + dest_dir + ": " + ec.message());
    }

    // 1. Fetch manifest to discover layers.
    auto manifest_res = fetchManifest(ref);
    if (!manifest_res.has_value()) {
        return Err<std::string>(manifest_res.error().code(), manifest_res.error().context());
    }
    const OciManifest& manifest = *manifest_res;

    // 2. Find the plugin binary layer.
    const OciManifestLayer* plugin_layer = nullptr;
    for (const auto& layer : manifest.layers) {
        if (layer.media_type == THEMIS_PLUGIN_LAYER_MEDIA_TYPE) {
            plugin_layer = &layer;
            break;
        }
    }
    if (!plugin_layer) {
        // Fallback: use the first layer (generic OCI artifact).
        if (!manifest.layers.empty()) {
            plugin_layer = &manifest.layers.front();
            THEMIS_WARN("OciRegistryClient: no layer with media type '{}' found; "
                        "using first layer (type: {})",
                        THEMIS_PLUGIN_LAYER_MEDIA_TYPE, plugin_layer->media_type);
        } else {
            return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                    "OCI manifest for " + ref.toString() + " has no layers");
        }
    }

    // 3. Build destination file path from digest (sha256:<hex> -> <hex>.plugin).
    std::string digest = plugin_layer->digest;
    std::string digest_hex = (digest.size() > 7 && digest.substr(0, 7) == "sha256:")
                             ? digest.substr(7)
                             : digest;

    // Determine file extension based on current platform.
#if defined(_WIN32)
    static constexpr const char* EXT = ".dll";
#elif defined(__APPLE__)
    static constexpr const char* EXT = ".dylib";
#else
    static constexpr const char* EXT = ".so";
#endif

    std::string plugin_name = ref.name;
    // Use only the last path component as the base filename.
    {
        auto slash = plugin_name.rfind('/');
        if (slash != std::string::npos) plugin_name = plugin_name.substr(slash + 1);
    }

    std::string dest_path = dest_dir + "/" + plugin_name + "_" +
                            digest_hex.substr(0, 12) + EXT;

    // 4. Skip download if the file already exists with correct digest.
    if (fs::exists(dest_path) && verifyDigest(dest_path, plugin_layer->digest)) {
        THEMIS_INFO("OciRegistryClient: using cached plugin binary at {}", dest_path);
        return Ok(dest_path);
    }

    // 5. Build blob URL and download.
    const std::string base = registryBaseUrl(ref.registry);
    std::string blob_url = base + "/v2/" + ref.name + "/blobs/" + plugin_layer->digest;

    std::vector<std::string> auth_headers;
    {
        OciAuthConfig auth;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            auto it = auth_configs_.find(ref.registry);
            if (it != auth_configs_.end()) auth = it->second;
        }
        if (!auth.bearer_token.empty()) {
            auth_headers.push_back("Authorization: Bearer " + auth.bearer_token);
        } else if (!auth.username.empty()) {
            std::string scope = "repository:" + ref.name + ":pull";
            auto token_res = obtainBearerToken(ref.registry, scope);
            if (token_res.has_value()) {
                auth_headers.push_back("Authorization: Bearer " + *token_res);
            }
        }
    }

    std::string tmp_path = dest_path + ".tmp";
    THEMIS_INFO("OciRegistryClient: downloading plugin blob {} -> {}", plugin_layer->digest, dest_path);

    auto dl_res = httpGetToFile(blob_url, tmp_path, auth_headers);
    if (!dl_res.has_value()) {
        return Err<std::string>(dl_res.error().code(), dl_res.error().context());
    }

    // 6. Verify digest.
    if (!verifyDigest(tmp_path, plugin_layer->digest)) {
        fs::remove(tmp_path);
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_HASH_MISMATCH,
                                "Digest mismatch for downloaded plugin blob: " + plugin_layer->digest);
    }

    // 7. Atomically rename temp file to final path.
    fs::rename(tmp_path, dest_path, ec);
    if (ec) {
        fs::remove(tmp_path);
        return Err<std::string>(ErrorCode::ERR_PLUGIN_OCI_PULL_FAILED,
                                "Failed to rename temp file to: " + dest_path + ": " + ec.message());
    }

    THEMIS_INFO("OciRegistryClient: plugin binary pulled to {}", dest_path);
    return Ok(dest_path);
}

} // namespace plugins
} // namespace themis
