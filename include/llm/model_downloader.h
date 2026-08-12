/**
 * @file model_downloader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Model download progress callback
 * @param bytes_downloaded Bytes downloaded so far
 * @param total_bytes Total bytes to download (0 if unknown)
 * @param status Status message
 */
using DownloadProgressCallback = std::function<void(size_t bytes_downloaded, size_t total_bytes, const std::string& status)>;

/**
 * @brief Configuration for model downloading
 */
struct ModelDownloadConfig {
    std::string model_name;           // Model identifier (e.g., "llama-2-7b")
    std::string ollama_url;           // Ollama API endpoint (e.g., "http://localhost:11434")
    std::string download_dir;         // Directory to save model
    bool use_cache = true;            // Use cached model if available
    int timeout_seconds = 300;        // Download timeout
    DownloadProgressCallback progress_callback;  // Optional progress tracking
};

/**
 * @brief Result of model download operation
 */
struct ModelDownloadResult {
    virtual ~ModelDownloadResult() = default;
    bool success = false;
    std::string model_path;           // Path to downloaded model
    std::string error_message;
    size_t file_size_bytes = 0;
    double download_time_seconds = 0.0;
};

/**
 * @brief Utility class for downloading LLM models from remote sources
 * 
 * Supports:
 * - Ollama API (http://localhost:11434/api/pull)
 * - Direct HTTP/HTTPS downloads
 * - Model caching to avoid re-downloads
 * - Progress tracking
 * 
 * Example:
 * ```cpp
 * ModelDownloadConfig config;
 * config.model_name = "llama2:7b";
 * config.ollama_url = "http://ollama-server:11434";
 * config.download_dir = "/models";
 * 
 * ModelDownloader downloader;
 * auto result = downloader.downloadFromOllama(config);
 * 
 * if (result.success) {
 *     // Load model from result.model_path
 * }
 * ```
 */
class ModelDownloader {
public:
    ModelDownloader() = default;
    ~ModelDownloader() = default;
    
    /**
     * @brief Download model from Ollama API
     * 
     * Uses Ollama's /api/pull endpoint to fetch models.
     * Models are stored in GGUF format compatible with llama.cpp.
     * 
     * @param config Download configuration
     * @return Download result with model path or error
     */
    ModelDownloadResult downloadFromOllama(const ModelDownloadConfig& config);
    
    /**
     * @brief Download model from direct URL
     * 
     * Downloads GGUF model file from HTTP/HTTPS URL.
     * 
     * @param url Direct download URL
     * @param output_path Path to save model
     * @param progress_callback Optional progress tracking
     * @return Download result
     */
    ModelDownloadResult downloadFromURL(
        const std::string& url,
        const std::string& output_path,
        DownloadProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Check if model exists locally
     * 
     * @param model_path Path to check
     * @return true if model file exists and is readable
     */
    static bool isModelAvailable(const std::string& model_path);
    
    /**
     * @brief Get Ollama model manifest
     * 
     * Queries Ollama API for model information without downloading.
     * 
     * @param ollama_url Ollama API endpoint
     * @param model_name Model identifier
     * @return Model manifest JSON or empty on error
     */
    static std::optional<json> getOllamaManifest(
        const std::string& ollama_url,
        const std::string& model_name
    );
    
    /**
     * @brief List available models from Ollama
     * 
     * @param ollama_url Ollama API endpoint
     * @return List of available model names
     */
    static std::vector<std::string> listOllamaModels(const std::string& ollama_url);
    
private:
    /**
     * @brief Pull model from Ollama API
     * Internal helper for downloadFromOllama
     */
    ModelDownloadResult pullFromOllama(const ModelDownloadConfig& config);
    
    /**
     * @brief Export model from Ollama to GGUF
     * After pulling, export to ThemisDB-compatible GGUF format
     */
    bool exportOllamaModel(
        const std::string& ollama_url,
        const std::string& model_name,
        const std::string& output_path
    );
};

/**
 * @brief Load model configuration from YAML
 * 
 * Reads model configuration from llm-models.yaml and returns
 * download configuration for specified model.
 * 
 * @param config_path Path to YAML config file
 * @param model_name Model name to look up
 * @return Download configuration or nullopt if not found
 */
std::optional<ModelDownloadConfig> loadModelConfigFromYAML(
    const std::string& config_path,
    const std::string& model_name
);

} // namespace llm
} // namespace themis
