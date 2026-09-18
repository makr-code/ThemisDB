/**
 * @file model_downloader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

using DownloadProgressCallback = std::function<void(size_t bytes_downloaded, size_t total_bytes, const std::string& status)>;

struct ModelDownloadConfig {
    std::string model_name;           // Model identifier (e.g., "llama-2-7b")
    std::string ollama_url;           // Ollama API endpoint (e.g., "http://localhost:11434")
    std::string download_dir;         // Directory to save model
    bool use_cache = true;            // Use cached model if available
    int timeout_seconds = 300;        // Download timeout
    DownloadProgressCallback progress_callback;  // Optional progress tracking
    bool allow_insecure_http = false;
};

struct ModelDownloadResult {
    /**
     * @brief Model Download Result.
     * @return Return value.
     */
    virtual ~ModelDownloadResult() = default;
    bool success = false;
    std::string model_path;           // Path to downloaded model
    std::string error_message;
    size_t file_size_bytes = 0;
    double download_time_seconds = 0.0;
};

class ModelDownloader {
public:
    ModelDownloader() = default;
    ~ModelDownloader() = default;
    
    /**
     * @brief Download From Ollama.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    ModelDownloadResult downloadFromOllama(const ModelDownloadConfig& config);
    
    ModelDownloadResult downloadFromURL(
        const std::string& url,
        const std::string& output_path,
        DownloadProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Is Model Available.
     * @param[in] model_path Path to the model.
     * @return True when the operation succeeds.
     */
    static bool isModelAvailable(const std::string& model_path);
    
    /**
     * @brief Get Ollama Manifest.
     * @param[in] ollama_url Input parameter.
     * @param[in] model_name Name of the model.
     * @return Return value.
     */
    static std::optional<json> getOllamaManifest(
        const std::string& ollama_url,
        const std::string& model_name
    );
    
    /**
     * @brief List Ollama Models.
     * @param[in] ollama_url Input parameter.
     * @return Return value.
     */
    static std::vector<std::string> listOllamaModels(const std::string& ollama_url);
    
private:
    /**
     * @brief Pull From Ollama.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    ModelDownloadResult pullFromOllama(const ModelDownloadConfig& config);
    
    /**
     * @brief Export Ollama Model.
     * @param[in] ollama_url Input parameter.
     * @param[in] model_name Name of the model.
     * @param[in] output_path Path to the output.
     * @return True when the operation succeeds.
     */
    bool exportOllamaModel(
        const std::string& ollama_url,
        const std::string& model_name,
        const std::string& output_path
    );
};

/**
 * @brief Load Model Config From YAML.
 * @param[in] config_path Path to the retention policy configuration file.
 * @param[in] model_name Name of the model.
 * @return Return value.
 */
std::optional<ModelDownloadConfig> loadModelConfigFromYAML(
    const std::string& config_path,
    const std::string& model_name
);

} // namespace llm
} // namespace themis
