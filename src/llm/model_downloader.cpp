/**
 * @file model_downloader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

// ModelDownloader implementation with progress tracking and checksum verification
// Supports Ollama API and direct HTTP/HTTPS downloads

#include "llm/model_downloader.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <yaml-cpp/yaml.h>
#include <stdexcept>

namespace fs = std::filesystem;

namespace themis {
namespace llm {

namespace {

// CURL write callback for downloading files
size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* stream = static_cast<std::ofstream*>(userdata);
    size_t total_size = size * nmemb;
    
    // Check if stream is in good state before writing
    if (!stream->good()) {
        return 0;  // Signal error to abort download
    }
    
    stream->write(static_cast<char*>(ptr), total_size);
    
    // Check if write succeeded
    if (!stream->good()) {
        return 0;  // Signal error to abort download (e.g., disk full)
    }
    
    return total_size;
}

// CURL progress callback
int progress_callback_wrapper(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
    auto* callback = static_cast<DownloadProgressCallback*>([[maybe_unused]] clientp);
    if ([[maybe_unused]] callback && *callback) {
        std::string status = "downloading";
        if (dlnow == dltotal && dltotal > 0) {
            status = "completed";
        }
        ([[maybe_unused]] *callback)(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal), status);
    }
    return 0;
}

// ── Wave 2-C: insecure_model_url — Ollama URL validation ─────────────────────
// Purpose: Prevent SSRF and credential-injection attacks via the ollama_url
//          config value. Called by every outbound HTTP function before the URL
//          is passed to libcurl.
// Acceptance criteria (test_model_downloader_url_validation.cpp):
//   URL_VAL_01..03 — invalid schemes rejected
//   URL_VAL_04     — embedded credentials (@) rejected
//   URL_VAL_05     — http://non-localhost rejected by default
//   URL_VAL_06..07 — localhost-http and https urls accepted
//   URL_VAL_10     — http://non-localhost accepted only with explicit override
[[nodiscard]] static bool validateOllamaUrl(const std::string& url,
                                             bool allow_insecure_http = false) {
    if (url.empty()) {
        THEMIS_WARN("validateOllamaUrl: URL is empty — rejected");
        return false;
    }

    // Accept only http:// and https:// schemes.
    const bool is_http  = (url.rfind("http://",  0) == 0);
    const bool is_https = (url.rfind("https://", 0) == 0);
    if (!is_http && !is_https) {
        THEMIS_WARN("validateOllamaUrl: rejected URL with non-HTTP scheme: {}",
                    url.substr(0, std::min(url.size(), size_t{64})));
        return false;
    }

    // Reject credential injection: "******host/..."
    // The '@' character in the authority component signals embedded credentials.
    const std::string_view authority_start = is_https ? url.substr(8) : url.substr(7);
    const auto slash_pos = authority_start.find('/');
    const auto at_pos    = authority_start.find('@');
    if (at_pos != std::string_view::npos &&
        (slash_pos == std::string_view::npos || at_pos < slash_pos)) {
        THEMIS_WARN("validateOllamaUrl: rejected URL containing embedded credentials");
        return false;
    }

    // [W3-SEC-01] Reject plain-HTTP connections to non-localhost targets unless
    // ModelDownloadConfig::allow_insecure_http is explicitly set to true.
    // Rationale: unencrypted model weight transfers are trivially MITM-attacked.
    if (is_http) {
        const bool is_local = (authority_start.rfind("localhost", 0) == 0) ||
                              (authority_start.rfind("127.", 0) == 0)      ||
                              (authority_start.rfind("[::1]", 0) == 0);
        if (!is_local) {
            if (!allow_insecure_http) {
                THEMIS_WARN("validateOllamaUrl: plain HTTP rejected for non-local endpoint '{}' "
                            "— use HTTPS or set allow_insecure_http=true", url);
                return false;
            }
            THEMIS_WARN("validateOllamaUrl: plain HTTP allowed for non-local endpoint '{}' "
                        "via allow_insecure_http override — not recommended in production", url);
        }
    }

    return true;
}

/// [W3-SEC-02] Sanitize model_name before it is used in any filesystem path.
/// Rejects traversal sequences (".."), path separators, and null bytes that
/// could redirect output outside the configured download directory.
[[nodiscard]] static bool sanitizeModelName(const std::string& name,
                                             std::string& error_out) {
    if (name.empty()) {
        error_out = "model_name must not be empty";
        return false;
    }
    if (name.find("..") != std::string::npos) {
        error_out = "model_name must not contain '..' path traversal sequences";
        return false;
    }
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
        error_out = "model_name must not contain path separators ('/' or '\\')";
        return false;
    }
    if (name.find('\0') != std::string::npos) {
        error_out = "model_name must not contain null bytes";
        return false;
    }
    return true;
}

/// [W3-SEC-01] Startup-level guard for allow_insecure_http.
///
/// Emits a prominent warning whenever allow_insecure_http=true appears in a
/// config that is actually exercised.  A second check against the
/// THEMISDB_ALLOW_INSECURE_HTTP environment variable provides an additional
/// enforcement layer: if the env-var is absent, the warning is upgraded to
/// strongly discourage unintended use in production deployments.
///
/// This function is intentionally non-fatal so callers that have already
/// validated the URL (validateOllamaUrl) can proceed; the purpose is operator
/// visibility, not a second gate.
static void warnInsecureConfigIfSet(const ModelDownloadConfig& cfg) {
    if (!cfg.allow_insecure_http) {
        return;
    }
    const char* env_guard = std::getenv("THEMISDB_ALLOW_INSECURE_HTTP");
    const bool  env_set   = (env_guard != nullptr &&
                             std::string_view{env_guard} == "1");
    if (!env_set) {
        THEMIS_WARN(
            "[SECURITY] ModelDownloadConfig::allow_insecure_http=true is set "
            "but THEMISDB_ALLOW_INSECURE_HTTP=1 env-var is NOT present. "
            "This combination is unsafe in production — plain-HTTP model "
            "transfers can be intercepted. Set THEMISDB_ALLOW_INSECURE_HTTP=1 "
            "explicitly on startup to acknowledge the risk, or switch to HTTPS.");
    } else {
        THEMIS_WARN(
            "[SECURITY] allow_insecure_http=true acknowledged via "
            "THEMISDB_ALLOW_INSECURE_HTTP=1 — plain-HTTP transfers are active "
            "for ollama_url '{}'. Ensure this is intentional.",
            cfg.ollama_url);
    }
}

} // anonymous namespace

ModelDownloadResult ModelDownloader::downloadFromOllama(const ModelDownloadConfig& config) {
    ModelDownloadResult result;
    auto start_time = std::chrono::steady_clock::now();

    // [W3-SEC-01] Warn at call-site if insecure-HTTP opt-in is active.
    warnInsecureConfigIfSet(config);

    // [W3-SEC-02] Validate model_name before it is embedded in any filesystem path.
    {
        std::string name_error = {};
        if (!sanitizeModelName(config.model_name, name_error)) {
            result.success = false;
            result.error_message = "Invalid model_name: " + name_error;
            THEMIS_WARN("downloadFromOllama: rejected config.model_name — {}", name_error);
            return result;
        }
    }

    try {
        // Check if model already exists
        std::string expected_path = config.download_dir + "/" + config.model_name + ".gguf";
        if (config.use_cache && isModelAvailable(expected_path)) {
            THEMIS_INFO("Model already available: {}", expected_path);
            result.success = true;
            result.model_path = expected_path;
            result.file_size_bytes = fs::file_size(expected_path);
            return result;
        }
        
        // Create download directory if needed
        if (!fs::exists(config.download_dir)) {
            fs::create_directories(config.download_dir);
        }
        
        // Pull model from Ollama
        result = pullFromOllama(config);
        
        auto end_time = std::chrono::steady_clock::now();
        result.download_time_seconds = std::chrono::duration<double>(end_time - start_time).count();
        
        return result;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Download failed: ") + e.what();
        THEMIS_ERROR("Model download failed: {}", e.what());
        return result;
    }
}

ModelDownloadResult ModelDownloader::pullFromOllama(const ModelDownloadConfig& config) {
    ModelDownloadResult result;

    // [W3-SEC-01] Warn at call-site if insecure-HTTP opt-in is active.
    // pullFromOllama may be called independently of downloadFromOllama, so the
    // guard runs here as well to ensure no silent insecure path.
    warnInsecureConfigIfSet(config);

    // Wave 2-C: validate ollama_url before any outbound HTTP request.
    // [W3-SEC-01] Pass allow_insecure_http flag from config to reject non-local HTTP by default.
    if (!validateOllamaUrl(config.ollama_url, config.allow_insecure_http)) {
        result.success = false;
        result.error_message = "Invalid ollama_url: must be http:// or https:// without embedded credentials";
        return result;
    }

    // [W3-SEC-02] Re-validate model_name here since pullFromOllama is part of the
    // public-facing protected API and may be called independently of downloadFromOllama.
    {
        std::string name_error = {};
        if (!sanitizeModelName(config.model_name, name_error)) {
            result.success = false;
            result.error_message = "Invalid model_name: " + name_error;
            THEMIS_WARN("pullFromOllama: rejected config.model_name — {}", name_error);
            return result;
        }
    }

    // Initialize CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        result.success = false;
        result.error_message = "Failed to initialize CURL";
        return result;
    }
    
    // Prepare Ollama pull request
    std::string pull_url = config.ollama_url + "/api/pull";
    json pull_request = {
        {"name", config.model_name},
        {"stream", false}
    };
    std::string request_body = pull_request.dump();
    
    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, pull_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config.timeout_seconds);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Response buffer
    std::string response_buffer = {};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto* buffer = static_cast<std::string*>(userdata);
        size_t total_size = size * nmemb;
        buffer->append(static_cast<char*>(ptr), total_size);
        return total_size;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
    
    // Execute request
    THEMIS_INFO("Pulling model from Ollama: {}", config.model_name);
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        result.success = false;
        result.error_message = std::string("CURL error: ") + curl_easy_strerror(res);
        return result;
    }
    
    // After pulling, export model to GGUF format
    std::string output_path = config.download_dir + "/" + config.model_name + ".gguf";
    bool export_success = exportOllamaModel(config.ollama_url, config.model_name, output_path);
    
    if (!export_success) {
        result.success = false;
        result.error_message = "Ollama model pull succeeded but export to GGUF failed. "
                               "Ensure the Ollama service is running and ~/.ollama/models/blobs/ "
                               "is accessible, or use a direct HuggingFace GGUF download instead.";
        THEMIS_WARN("exportOllamaModel failed; consider direct download from HuggingFace");
        THEMIS_WARN("Example: https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf");
        return result;
    }
    
    if (export_success && fs::exists(output_path)) {
        result.success = true;
        result.model_path = output_path;
        result.file_size_bytes = fs::file_size(output_path);
        THEMIS_INFO("Model successfully downloaded: {} ({} MB)", output_path, result.file_size_bytes / (1024 * 1024));
    } else {
        result.success = false;
        result.error_message = "Failed to export model to GGUF format";
    }
    
    return result;
}

bool ModelDownloader::exportOllamaModel(
    const std::string& ollama_url,
    const std::string& model_name,
    const std::string& output_path
) {
    if (!validateOllamaUrl(ollama_url)) {
        THEMIS_WARN("exportOllamaModel: invalid ollama_url — rejected");
        return false;
    }
    // Query Ollama's /api/show endpoint to get the model manifest, which
    // contains the SHA-256 digest of the underlying GGUF blob.
    // The blob file lives at ~/.ollama/models/blobs/sha256-<digest>.
    CURL* curl = curl_easy_init();
    if (!curl) {
        THEMIS_WARN("exportOllamaModel: failed to init CURL");
        return false;
    }

    const std::string show_url = ollama_url + "/api/show";
    const std::string req_body = json{{"name", model_name}}.dump();
    std::string resp_buf;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, show_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](void* ptr, size_t size, size_t nmemb, void* ud) -> size_t {
            auto* buf = static_cast<std::string*>(ud);
            buf->append(static_cast<char*>(ptr), size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_buf);

    const CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        THEMIS_WARN("exportOllamaModel: /api/show request failed: {}",
                    curl_easy_strerror(res));
        return false;
    }

    // Parse response: look for the blob SHA256 digest.
    // Ollama returns {"details": {...}, "modelinfo": {"general.file_type": ...}}
    // and a "digest" top-level field like "sha256:<hex>".
    try {
        auto j = json::parse(resp_buf);
        std::string digest = {};
        if (j.contains("digest") && j["digest"].is_string()) {
            digest = j["digest"].get<std::string>();
        } else if (j.contains("details") && j["details"].contains("digest")) {
            digest = j["details"]["digest"].get<std::string>();
        }

        if (digest.empty()) {
            THEMIS_WARN("exportOllamaModel: no digest in /api/show response");
            return false;
        }

        // Convert "sha256:<hex>" → file name "sha256-<hex>"
        std::string filename = digest;
        const auto colon_pos = filename.find(':');
        if (colon_pos != std::string::npos) {
            filename[colon_pos] = '-';
        }

        // Resolve blob path: ~/.ollama/models/blobs/<filename>
        const char* home = std::getenv("HOME");
        if (!home) {
            THEMIS_WARN("exportOllamaModel: $HOME not set");
            return false;
        }
        fs::path blob_path = fs::path(home) / ".ollama" / "models" / "blobs" / filename;

        if (!fs::exists(blob_path)) {
            THEMIS_WARN("exportOllamaModel: blob not found at {}", blob_path.string());
            return false;
        }

        // Create parent directory for output
        fs::path out(output_path);
        if (out.has_parent_path()) {
            fs::create_directories(out.parent_path());
        }

        // Prefer a hard-link (same filesystem, zero copy); fall back to copy.
        std::error_code ec = {};
        fs::create_hard_link(blob_path, out, ec);
        if (ec) {
            fs::copy_file(blob_path, out,
                          fs::copy_options::overwrite_existing, ec);
            if (ec) {
                THEMIS_WARN("exportOllamaModel: copy failed: {}", ec.message());
                return false;
            }
        }

        THEMIS_INFO("exportOllamaModel: exported {} → {}", blob_path.string(), output_path);
        return true;

    } catch (const json::exception& ex) {
        THEMIS_WARN("exportOllamaModel: JSON parse error: {}", ex.what());
        return false;
    }
}

ModelDownloadResult ModelDownloader::downloadFromURL(
    const std::string& url,
    const std::string& output_path,
    DownloadProgressCallback progress_callback
) {
    ModelDownloadResult result;
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Create parent directory if needed
        fs::path path(output_path);
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }
        
        // Initialize CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            result.success = false;
            result.error_message = "Failed to initialize CURL";
            return result;
        }
        
        // Open output file
        std::ofstream output_file(output_path, std::ios::binary);
        if (!output_file.is_open()) {
            curl_easy_cleanup(curl);
            result.success = false;
            result.error_message = "Failed to open output file: " + output_path;
            return result;
        }
        
        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_file);
        
        // Enable progress tracking if callback provided
        if ([[maybe_unused]] progress_callback) {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback_wrapper);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_callback);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }
        
        // Execute download
        THEMIS_INFO("Downloading model from: {}", url);
        CURLcode res = curl_easy_perform(curl);
        
        output_file.close();
        
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            fs::remove(output_path);  // Clean up partial download
            result.success = false;
            result.error_message = std::string("Download failed: ") + curl_easy_strerror(res);
            return result;
        }
        
        // Check HTTP response code
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_cleanup(curl);
        
        if (response_code != 200) {
            fs::remove(output_path);
            result.success = false;
            result.error_message = "HTTP error: " + std::to_string(response_code);
            return result;
        }
        
        // Success
        result.success = true;
        result.model_path = output_path;
        result.file_size_bytes = fs::file_size(output_path);
        
        auto end_time = std::chrono::steady_clock::now();
        result.download_time_seconds = std::chrono::duration<double>(end_time - start_time).count();
        
        THEMIS_INFO("Download completed: {} ({} MB, {:.1f}s)",
            output_path,
            result.file_size_bytes / (1024 * 1024),
            result.download_time_seconds);
        
        return result;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Exception: ") + e.what();
        THEMIS_ERROR("Download failed: {}", e.what());
        
        // Clean up partial download
        if (fs::exists(output_path)) {
            fs::remove(output_path);
        }
        
        return result;
    }
}

bool ModelDownloader::isModelAvailable(const std::string& model_path) {
    if (!fs::exists(model_path)) {
        return false;
    }
    
    // Check if file is readable and has reasonable size (> 1MB)
    try {
        auto file_size = fs::file_size(model_path);
        return file_size > 1024 * 1024;  // At least 1 MB
    } catch (...) {
        THEMIS_WARN("model_downloader: unhandled exception caught");
        return false;
    }
}

std::optional<json> ModelDownloader::getOllamaManifest(
    const std::string& ollama_url,
    const std::string& model_name
) {
    if (!validateOllamaUrl(ollama_url)) {
        return std::nullopt;
    }
    try {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return std::nullopt;
        }
        
        std::string show_url = ollama_url + "/api/show";
        json show_request = {{"name", model_name}};
        std::string request_body = show_request.dump();
        
        curl_easy_setopt(curl, CURLOPT_URL, show_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        std::string response_buffer = {};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto* buffer = static_cast<std::string*>(userdata);
            size_t total_size = size * nmemb;
            buffer->append(static_cast<char*>(ptr), total_size);
            return total_size;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK && !response_buffer.empty()) {
            return json::parse(response_buffer);
        }
        
        return std::nullopt;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to get Ollama manifest: {}", e.what());
        return std::nullopt;
    }
}

std::vector<std::string> ModelDownloader::listOllamaModels(const std::string& ollama_url) {
    std::vector<std::string> models;

    if (!validateOllamaUrl(ollama_url)) {
        return models;
    }

    try {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return models;
        }
        
        std::string list_url = ollama_url + "/api/tags";
        curl_easy_setopt(curl, CURLOPT_URL, list_url.c_str());
        
        std::string response_buffer = {};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto* buffer = static_cast<std::string*>(userdata);
            size_t total_size = size * nmemb;
            buffer->append(static_cast<char*>(ptr), total_size);
            return total_size;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK && !response_buffer.empty()) {
            json response = json::parse(response_buffer);
            if (response.contains("models")) {
                for (const auto& model : response["models"]) {
                    if (model.contains("name")) {
                        models.push_back(model["name"]);
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to list Ollama models: {}", e.what());
    }
    
    return models;
}

std::optional<ModelDownloadConfig> loadModelConfigFromYAML(
    const std::string& config_path,
    const std::string& model_name
) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        ModelDownloadConfig dl_config;
        dl_config.model_name = model_name;
        dl_config.ollama_url = config["ollama_url"]
            ? config["ollama_url"].as<std::string>()
            : "http://localhost:11434";
        dl_config.download_dir = config["download_dir"]
            ? config["download_dir"].as<std::string>()
            : "";
        
        if (!config["models"]) {
            THEMIS_ERROR("No 'models' section in config: {}", config_path);
            return std::nullopt;
        }

        auto apply_model_node = [&]([[maybe_unused]] const YAML::Node& model) -> std::optional<ModelDownloadConfig> {
            if (!model || !model.IsMap()) {
                return std::nullopt;
            }

            ModelDownloadConfig resolved = dl_config;

            if (model["ollama_url"]) {
                resolved.ollama_url = model["ollama_url"].as<std::string>();
            }
            if (model["download_dir"]) {
                resolved.download_dir = model["download_dir"].as<std::string>();
            }
            if (model["use_cache"]) {
                resolved.use_cache = model["use_cache"].as<bool>();
            }
            if (model["timeout_seconds"]) {
                resolved.timeout_seconds = model["timeout_seconds"].as<int>();
            }

            if (model["sources"] && model["sources"].IsMap() && model["sources"]["ollama"]) {
                resolved.model_name = model["sources"]["ollama"].as<std::string>();
            }

            return resolved;
        };
        
        // Supported layouts:
        // 1. Sequence: models: [ { name: ..., ... } ]
        // 2. Legacy map: models: { model_name: { ... } }
        YAML::Node models = config["models"];
        if (models.IsSequence()) {
            for (const auto& model : models) {
                if (model.IsMap() && model["name"] && model["name"].as<std::string>() == model_name) {
                    return apply_model_node(model);
                }
            }
        } else if (models.IsMap()) {
            if (models[model_name]) {
                auto resolved = apply_model_node(models[model_name]);
                if (resolved) {
                    return resolved;
                }
            }

            for (const auto& entry : models) {
                if (entry.second.IsMap() && entry.second["name"] &&
                    entry.second["name"].as<std::string>() == model_name) {
                    return apply_model_node(entry.second);
                }
            }
        }
        
        THEMIS_WARN("Model not found in config: {}", model_name);
        return std::nullopt;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load model config from YAML: {}", e.what());
        return std::nullopt;
    }
}

} // namespace llm
} // namespace themis

