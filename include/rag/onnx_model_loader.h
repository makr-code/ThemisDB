/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            onnx_model_loader.h                                ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:25:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     201                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file onnx_model_loader.h
 * @brief ONNX Runtime model loading and management utilities
 * 
 * Provides utilities for loading and managing ONNX models for the quality control system.
 * Supports model downloading, caching, and validation.
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <filesystem>
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief ONNX model metadata
 */
struct ONNXModelInfo {
    std::string model_name;           ///< Model name/identifier
    std::string model_path;           ///< Local path to ONNX file
    std::string tokenizer_path;       ///< Path to tokenizer config
    std::string model_url;            ///< Download URL (if applicable)
    size_t model_size_bytes;          ///< Model file size
    std::string checksum;             ///< SHA256 checksum for validation
    std::vector<int64_t> input_shape; ///< Expected input shape
    std::vector<std::string> output_names;  ///< Output tensor names
};

/**
 * @brief ONNX model loader configuration
 */
struct ONNXModelLoaderConfig {
    std::string cache_dir = "./models";  ///< Directory for model caching
    bool verify_checksum = true;         ///< Verify model checksums
    bool auto_download = false;          ///< Auto-download missing models
    int download_timeout_sec = 300;      ///< Download timeout (5 minutes)
    bool create_cache_dir = true;        ///< Create cache dir if missing
};

/**
 * @brief ONNX Model Loader
 * 
 * Manages ONNX model loading, caching, and validation. Supports:
 * - Local model loading
 * - Model downloading from URLs
 * - Checksum verification
 * - Model caching
 * - Model info management
 */
class ONNXModelLoader {
public:
    /**
     * @brief Construct loader with configuration
     */
    explicit ONNXModelLoader(const ONNXModelLoaderConfig& config = {});
    
    /**
     * @brief Destructor
     */
    ~ONNXModelLoader();
    
    /**
     * @brief Load model from local path
     * @param model_path Path to ONNX model file
     * @return Model info if successful, nullopt otherwise
     */
    std::optional<ONNXModelInfo> loadModel(const std::string& model_path);
    
    /**
     * @brief Load or download model
     * @param model_name Model name/identifier
     * @param model_url URL to download from (if not cached)
     * @param expected_checksum Expected SHA256 checksum (optional)
     * @return Model info if successful, nullopt otherwise
     */
    std::optional<ONNXModelInfo> loadOrDownloadModel(
        const std::string& model_name,
        const std::string& model_url,
        const std::string& expected_checksum = ""
    );
    
    /**
     * @brief Check if model exists in cache
     * @param model_name Model name/identifier
     * @return true if model is cached
     */
    bool isModelCached(const std::string& model_name) const;
    
    /**
     * @brief Get cached model path
     * @param model_name Model name/identifier
     * @return Path to cached model, or empty string if not found
     */
    std::string getCachedModelPath(const std::string& model_name) const;
    
    /**
     * @brief Validate model file integrity
     * @param model_path Path to model file
     * @param expected_checksum Expected SHA256 checksum
     * @return true if checksum matches
     */
    bool validateModelChecksum(const std::string& model_path, const std::string& expected_checksum);
    
    /**
     * @brief Get list of cached models
     * @return Vector of cached model names
     */
    std::vector<std::string> listCachedModels() const;
    
    /**
     * @brief Clear model cache
     * @param model_name Optional model name to clear (clears all if empty)
     * @return Number of models cleared
     */
    size_t clearCache(const std::string& model_name = "");
    
    /**
     * @brief Get loader configuration
     */
    const ONNXModelLoaderConfig& getConfig() const { return config_; }
    
    /**
     * @brief Register predefined model
     * @param info Model information
     */
    void registerModel(const ONNXModelInfo& info);
    
    /**
     * @brief Get predefined model info
     * @param model_name Model name
     * @return Model info if found, nullopt otherwise
     */
    std::optional<ONNXModelInfo> getModelInfo(const std::string& model_name) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    ONNXModelLoaderConfig config_;
    
    // Predefined model registry
    std::unordered_map<std::string, ONNXModelInfo> model_registry_;
    
    void initializeDefaultModels();
    std::string computeChecksum(const std::string& file_path);
    bool downloadFile(const std::string& url, const std::string& dest_path, int timeout_sec);
};

/**
 * @brief Factory for common NLI models
 */
class NLIModelFactory {
public:
    /**
     * @brief Get DeBERTa-v3-large-mnli model info
     */
    static ONNXModelInfo getDebertaV3LargeMNLI();
    
    /**
     * @brief Get RoBERTa-large-mnli model info
     */
    static ONNXModelInfo getRobertaLargeMNLI();
    
    /**
     * @brief Get BART-large-mnli model info
     */
    static ONNXModelInfo getBartLargeMNLI();
    
    /**
     * @brief Get all supported NLI models
     */
    static std::vector<ONNXModelInfo> getAllSupportedModels();
};

} // namespace themis::rag::judge
