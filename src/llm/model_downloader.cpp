#include "llm/model_downloader.h"
#include "utils/logger.h"
#include "utils/checksum_utils.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <iomanip>

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#ifdef THEMIS_ENABLE_CURL
#include <curl/curl.h>
#endif

#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

namespace themis {
namespace llm {

#ifdef THEMIS_ENABLE_CURL
namespace {

// CURL callback for writing data to file
size_t writeFileCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// CURL callback for progress reporting
int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, 
                     curl_off_t ultotal, curl_off_t ulnow) {
    auto* callback = static_cast<DownloadProgressCallback*>(clientp);
    if (callback && *callback) {
        std::ostringstream status;
        if (dltotal > 0) {
            double percent = (double)dlnow / (double)dltotal * 100.0;
            status << "Downloading: " << std::fixed << std::setprecision(1) 
                   << percent << "% (" << dlnow << "/" << dltotal << " bytes)";
        } else {
            status << "Downloading: " << dlnow << " bytes";
        }
        (*callback)(dlnow, dltotal, status.str());
    }
    return 0;
}

// CURL callback for receiving header data
size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total_size = size * nitems;
    auto* response_data = static_cast<std::string*>(userdata);
    response_data->append(buffer, total_size);
    return total_size;
}

// CURL callback for writing response to string
size_t writeStringCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t total_size = size * nmemb;
    userp->append(static_cast<char*>(contents), total_size);
    return total_size;
}
#endif

} // anonymous namespace

// ============================================================================
// ModelDownloader Implementation
// ============================================================================

ModelDownloadResult ModelDownloader::downloadFromOllama(const ModelDownloadConfig& config) {
    ModelDownloadResult result;
    auto start_time = std::chrono::steady_clock::now();

    LOG_INFO("Downloading model '{}' from Ollama at {}", config.model_name, config.ollama_url);

    // Check if model exists locally and use_cache is enabled
    std::string model_filename = config.model_name;
    std::replace(model_filename.begin(), model_filename.end(), ':', '_');
    std::replace(model_filename.begin(), model_filename.end(), '/', '_');
    model_filename += ".gguf";
    
    fs::path output_path = fs::path(config.download_dir) / model_filename;

    if (config.use_cache && fs::exists(output_path)) {
        LOG_INFO("Model already cached at: {}", output_path.string());
        result.success = true;
        result.model_path = output_path.string();
        result.file_size_bytes = fs::file_size(output_path);
        result.download_time_seconds = 0.0;
        return result;
    }

    // Create download directory if it doesn't exist
    if (!fs::exists(config.download_dir)) {
        try {
            fs::create_directories(config.download_dir);
        } catch (const std::exception& e) {
            result.error_message = std::string("Failed to create download directory: ") + e.what();
            LOG_ERROR("{}", result.error_message);
            return result;
        }
    }

    // Pull model from Ollama
    result = pullFromOllama(config);
    
    if (!result.success) {
        return result;
    }

    // Calculate download time
    auto end_time = std::chrono::steady_clock::now();
    result.download_time_seconds = std::chrono::duration<double>(end_time - start_time).count();

    LOG_INFO("Model '{}' downloaded successfully to {} ({} bytes, {:.2f}s)", 
             config.model_name, result.model_path, result.file_size_bytes, 
             result.download_time_seconds);

    return result;
}

ModelDownloadResult ModelDownloader::pullFromOllama(const ModelDownloadConfig& config) {
#ifdef THEMIS_ENABLE_CURL
    ModelDownloadResult result;

    // Prepare output path
    std::string model_filename = config.model_name;
    std::replace(model_filename.begin(), model_filename.end(), ':', '_');
    std::replace(model_filename.begin(), model_filename.end(), '/', '_');
    model_filename += ".gguf";
    
    fs::path output_path = fs::path(config.download_dir) / model_filename;
    std::string temp_path = output_path.string() + ".tmp";

    // Prepare Ollama API endpoint for pulling
    std::string pull_url = config.ollama_url + "/api/pull";
    
    // Create JSON request body
    json request_body;
    request_body["name"] = config.model_name;
    request_body["stream"] = true;
    std::string request_str = request_body.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        result.error_message = "Failed to initialize CURL";
        LOG_ERROR("{}", result.error_message);
        return result;
    }

    std::string response_data;
    
    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, pull_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(config.timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ThemisDB-ModelDownloader/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Set request headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        result.error_message = std::string("CURL error: ") + curl_easy_strerror(res);
        LOG_ERROR("{}", result.error_message);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return result;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        result.error_message = "Ollama API returned HTTP " + std::to_string(http_code);
        LOG_ERROR("{}", result.error_message);
        return result;
    }

    // Parse streaming response
    std::istringstream response_stream(response_data);
    std::string line;
    bool pull_complete = false;

    while (std::getline(response_stream, line)) {
        if (line.empty()) continue;
        
        try {
            json j = json::parse(line);
            
            if (j.contains("status")) {
                std::string status = j["status"];
                
                if (config.progress_callback) {
                    size_t completed = 0;
                    size_t total = 0;
                    
                    if (j.contains("completed") && j.contains("total")) {
                        completed = j["completed"].get<size_t>();
                        total = j["total"].get<size_t>();
                    }
                    
                    config.progress_callback(completed, total, status);
                }
                
                if (status == "success") {
                    pull_complete = true;
                }
            }
            
            if (j.contains("error")) {
                result.error_message = j["error"].get<std::string>();
                LOG_ERROR("Ollama pull error: {}", result.error_message);
                return result;
            }
        } catch (const json::exception& e) {
            LOG_WARN("Failed to parse Ollama response line: {}", e.what());
        }
    }

    if (!pull_complete) {
        result.error_message = "Model pull did not complete successfully";
        LOG_ERROR("{}", result.error_message);
        return result;
    }

    // Now export the model to GGUF format
    if (!exportOllamaModel(config.ollama_url, config.model_name, output_path.string())) {
        result.error_message = "Failed to export model to GGUF format";
        LOG_ERROR("{}", result.error_message);
        return result;
    }

    result.success = true;
    result.model_path = output_path.string();
    result.file_size_bytes = fs::file_size(output_path);

    return result;
#else
    ModelDownloadResult result;
    result.error_message = "CURL support not enabled - cannot download from Ollama";
    LOG_ERROR("{}", result.error_message);
    return result;
#endif
}

bool ModelDownloader::exportOllamaModel(const std::string& ollama_url, 
                                        const std::string& model_name,
                                        const std::string& output_path) {
#ifdef THEMIS_ENABLE_CURL
    // Use Ollama's show endpoint to get model blob/layers info
    std::string show_url = ollama_url + "/api/show";
    
    json request_body;
    request_body["name"] = model_name;
    std::string request_str = request_body.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL");
        return false;
    }

    std::string response_data;
    
    curl_easy_setopt(curl, CURLOPT_URL, show_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("CURL error: {}", curl_easy_strerror(res));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        LOG_ERROR("Ollama show API returned HTTP {}", http_code);
        return false;
    }

    try {
        json model_info = json::parse(response_data);
        
        // In a real implementation, we would:
        // 1. Get the model file location from Ollama's local storage
        // 2. Copy or link the GGUF file to our output path
        // 3. For now, we'll use a simpler approach
        
        // Ollama stores models in ~/.ollama/models on Linux/Mac
        // or %USERPROFILE%\.ollama\models on Windows
        std::string ollama_models_dir;
        
        #ifdef _WIN32
        const char* userprofile = std::getenv("USERPROFILE");
        if (userprofile) {
            ollama_models_dir = std::string(userprofile) + "\\.ollama\\models";
        }
        #else
        const char* home = std::getenv("HOME");
        if (home) {
            ollama_models_dir = std::string(home) + "/.ollama/models";
        }
        #endif

        if (ollama_models_dir.empty() || !fs::exists(ollama_models_dir)) {
            LOG_ERROR("Could not locate Ollama models directory. Ollama may not be installed or accessible.");
            return false;
        }

        // Try to find the model blob
        // Ollama uses content-addressable storage with SHA256 digests
        if (model_info.contains("details") && model_info["details"].contains("parent_model")) {
            // Complex model with parent - requires proper blob handling
            LOG_ERROR("Model export using Ollama's blob storage is not yet fully implemented");
            LOG_ERROR("Please use Ollama's native tools to export this model, or wait for full implementation");
            return false;
        }

    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse Ollama model info: {}", e.what());
        return false;
    }

    LOG_ERROR("Model export from Ollama is not yet fully implemented. Use Ollama CLI to export the model manually.");
    return false;
#else
    LOG_ERROR("CURL support not enabled");
    return false;
#endif
}

ModelDownloadResult ModelDownloader::downloadFromURL(const std::string& url,
                                                      const std::string& output_path,
                                                      DownloadProgressCallback progress_callback) {
#ifdef THEMIS_ENABLE_CURL
    ModelDownloadResult result;
    auto start_time = std::chrono::steady_clock::now();

    LOG_INFO("Downloading model from URL: {}", url);

    // Create parent directories if needed
    fs::path path(output_path);
    if (path.has_parent_path()) {
        try {
            fs::create_directories(path.parent_path());
        } catch (const std::exception& e) {
            result.error_message = std::string("Failed to create directory: ") + e.what();
            LOG_ERROR("{}", result.error_message);
            return result;
        }
    }

    // Download to temporary file first
    std::string temp_path = output_path + ".tmp";
    FILE* fp = fopen(temp_path.c_str(), "wb");
    if (!fp) {
        result.error_message = "Failed to open output file for writing";
        LOG_ERROR("{}", result.error_message);
        return result;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        fs::remove(temp_path);
        result.error_message = "Failed to initialize CURL";
        LOG_ERROR("{}", result.error_message);
        return result;
    }

    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3600L); // 1 hour timeout for large models
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ThemisDB-ModelDownloader/1.0");

    // Enable progress reporting
    if (progress_callback) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_callback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }

    // Perform download
    CURLcode res = curl_easy_perform(curl);

    fclose(fp);

    if (res != CURLE_OK) {
        result.error_message = std::string("CURL error: ") + curl_easy_strerror(res);
        LOG_ERROR("{}", result.error_message);
        curl_easy_cleanup(curl);
        fs::remove(temp_path);
        return result;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_easy_cleanup(curl);

    if (http_code != 200) {
        result.error_message = "HTTP error: " + std::to_string(http_code);
        LOG_ERROR("{}", result.error_message);
        fs::remove(temp_path);
        return result;
    }

    // Move temp file to final location
    try {
        fs::rename(temp_path, output_path);
    } catch (const std::exception& e) {
        result.error_message = std::string("Failed to rename temp file: ") + e.what();
        LOG_ERROR("{}", result.error_message);
        fs::remove(temp_path);
        return result;
    }

    auto end_time = std::chrono::steady_clock::now();
    result.success = true;
    result.model_path = output_path;
    result.file_size_bytes = fs::file_size(output_path);
    result.download_time_seconds = std::chrono::duration<double>(end_time - start_time).count();

    LOG_INFO("Model downloaded successfully to {} ({} bytes, {:.2f}s)", 
             output_path, result.file_size_bytes, result.download_time_seconds);

    return result;
#else
    ModelDownloadResult result;
    result.error_message = "CURL support not enabled";
    LOG_ERROR("{}", result.error_message);
    return result;
#endif
}

bool ModelDownloader::isModelAvailable(const std::string& model_path) {
    if (!fs::exists(model_path)) {
        return false;
    }

    // Check if file is readable
    std::ifstream file(model_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Check minimum file size (at least 1MB for a valid model)
    try {
        auto size = fs::file_size(model_path);
        if (size < 1024 * 1024) {
            return false;
        }
    } catch (...) {
        return false;
    }

    return true;
}

std::optional<json> ModelDownloader::getOllamaManifest(const std::string& ollama_url,
                                                        const std::string& model_name) {
#ifdef THEMIS_ENABLE_CURL
    std::string show_url = ollama_url + "/api/show";
    
    json request_body;
    request_body["name"] = model_name;
    std::string request_str = request_body.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL");
        return std::nullopt;
    }

    std::string response_data;
    
    curl_easy_setopt(curl, CURLOPT_URL, show_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("CURL error: {}", curl_easy_strerror(res));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return std::nullopt;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        LOG_ERROR("Ollama API returned HTTP {}", http_code);
        return std::nullopt;
    }

    try {
        return json::parse(response_data);
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse Ollama manifest: {}", e.what());
        return std::nullopt;
    }
#else
    LOG_ERROR("CURL support not enabled");
    return std::nullopt;
#endif
}

std::vector<std::string> ModelDownloader::listOllamaModels(const std::string& ollama_url) {
#ifdef THEMIS_ENABLE_CURL
    std::vector<std::string> models;
    std::string list_url = ollama_url + "/api/tags";

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL");
        return models;
    }

    std::string response_data;
    
    curl_easy_setopt(curl, CURLOPT_URL, list_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("CURL error: {}", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return models;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_easy_cleanup(curl);

    if (http_code != 200) {
        LOG_ERROR("Ollama API returned HTTP {}", http_code);
        return models;
    }

    try {
        json response = json::parse(response_data);
        if (response.contains("models")) {
            for (const auto& model : response["models"]) {
                if (model.contains("name")) {
                    models.push_back(model["name"].get<std::string>());
                }
            }
        }
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse Ollama models list: {}", e.what());
    }

    return models;
#else
    LOG_ERROR("CURL support not enabled");
    return std::vector<std::string>();
#endif
}

// ============================================================================
// YAML Configuration Loader
// ============================================================================

std::optional<ModelDownloadConfig> loadModelConfigFromYAML(const std::string& config_path,
                                                             const std::string& model_name) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        
        if (!config["models"] || !config["models"][model_name]) {
            LOG_ERROR("Model '{}' not found in configuration file", model_name);
            return std::nullopt;
        }

        YAML::Node model_config = config["models"][model_name];
        
        ModelDownloadConfig dl_config;
        dl_config.model_name = model_name;

        if (model_config["ollama_url"]) {
            dl_config.ollama_url = model_config["ollama_url"].as<std::string>();
        } else if (config["ollama_url"]) {
            dl_config.ollama_url = config["ollama_url"].as<std::string>();
        } else {
            dl_config.ollama_url = "http://localhost:11434";
        }

        if (model_config["download_dir"]) {
            dl_config.download_dir = model_config["download_dir"].as<std::string>();
        } else if (config["download_dir"]) {
            dl_config.download_dir = config["download_dir"].as<std::string>();
        } else {
            dl_config.download_dir = "./models";
        }

        if (model_config["use_cache"]) {
            dl_config.use_cache = model_config["use_cache"].as<bool>();
        }

        if (model_config["timeout_seconds"]) {
            dl_config.timeout_seconds = model_config["timeout_seconds"].as<int>();
        }

        return dl_config;
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Failed to parse YAML configuration: {}", e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load model configuration: {}", e.what());
        return std::nullopt;
    }
}

} // namespace llm
} // namespace themis
