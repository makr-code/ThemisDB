/**
 * @file huggingface_ingestion_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "content/async_ingestion_worker.h"
#include <curl/curl.h>
#include <string>
#include <memory>
#include <map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace plugins {

using json = nlohmann::json;

/**
 * @brief HuggingFace Ingestion Plugin
 * 
 * Fetches datasets from HuggingFace Hub via REST API.
 * Integrates with AsyncIngestionWorker for background processing.
 * 
 * Features:
 * - Streaming for large datasets
 * - Automatic retry on network errors
 * - Local caching
 * - Incremental updates
 * 
 * Example usage:
 * ```cpp
 * HuggingFaceIngestionPlugin::Config config;
 * config.dataset_name = "lexlms/ger_legal_data";
 * config.split = "train";
 * config.cache_dir = "./cache/huggingface";
 * 
 * auto plugin = std::make_shared<HuggingFaceIngestionPlugin>(
 *     config, content_manager
 * );
 * 
 * AsyncIngestionWorker worker(content_manager);
 * plugin->registerWithWorker(worker);
 * worker.start();
 * 
 * auto job_id = plugin->submitDatasetJob("lexlms/ger_legal_data");
 * ```
 */
class HuggingFaceIngestionPlugin {
public:
    /**
     * @brief Configuration for HuggingFace plugin
     */
    struct Config {
        std::string dataset_name;           ///< e.g., "lexlms/ger_legal_data"
        std::string split = "train";        ///< train/test/validation
        bool streaming = true;              ///< Stream large datasets
        size_t chunk_size = 1000;           ///< Rows per API request
        std::string auth_token;             ///< Optional HF token
        
        // Schema mapping
        std::string text_field = "text";
        std::string label_field = "label";
        std::map<std::string, std::string> custom_fields;
        
        // Caching
        std::string cache_dir = "./cache/huggingface";
        bool use_cache = true;
        
        // Rate limiting
        size_t max_requests_per_second = 10;
        
        // Retry configuration
        size_t max_retries = 3;
        size_t retry_delay_ms = 1000;
        
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        json toJson() const;
        /**
         * @brief TBD: Describe fromJson.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static Config fromJson(const json& j);
    };
    
    /**
     * @brief TBD: Describe HuggingFaceIngestionPlugin.
     * @param[in] config Input parameter.
     * @param[in] content_manager Input parameter.
     * @return Return value.
     */
    explicit HuggingFaceIngestionPlugin(
        const Config& config,
        std::shared_ptr<content::ContentManager> content_manager
    );
    
    ~HuggingFaceIngestionPlugin();
    
    /**
     * @brief Register plugin with AsyncIngestionWorker
     * 
     * This adds a new job handler for HUGGINGFACE job type.
     * @param[in,out] worker Input/output parameter.
     */
    void registerWithWorker(content::AsyncIngestionWorker& worker);
    
    /**
     * @brief Submit a dataset ingestion job
     * 
     * @param dataset_name HuggingFace dataset identifier
     * @param split Dataset split (train/test/validation)
     * @param config Optional job configuration
     * @return Job ID for tracking
     */
    std::string submitDatasetJob(
        const std::string& dataset_name,
        const std::string& split = "",
        const json& config = json::object()
    );
    
    /**
     * @brief Get dataset metadata from HuggingFace
     */
    struct DatasetMetadata {
        std::string dataset_id;
        std::string description;
        size_t total_rows;
        std::vector<std::string> splits;
        std::map<std::string, std::string> columns;
        
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        json toJson() const;
    };
    
    /**
     * @brief TBD: Describe getDatasetMetadata.
     * @param[in] dataset_name Input parameter.
     * @return Return value.
     */
    DatasetMetadata getDatasetMetadata(const std::string& dataset_name);
    
    /**
     * @brief Estimate dataset size
     * @param[in] dataset_name Input parameter.
     * @return Return value.
     */
    size_t estimateDatasetSize(const std::string& dataset_name);

private:
    Config config_;
    std::shared_ptr<content::ContentManager> content_manager_;
    content::AsyncIngestionWorker* worker_ = nullptr;  // Non-owning pointer
    CURL* curl_handle_;
    
    // Phase 2A Data Race Protection: Mutex guards for concurrent access
    mutable std::mutex config_state_mutex_;  ///< Protects config_state_ concurrent read-modify-write
    
    // Phase 2A Progress Tracking: Atomic counters for thread-safe updates
    std::atomic<size_t> progress_rows_processed_{0};  ///< Number of rows processed
    std::atomic<size_t> progress_errors_count_{0};    ///< Number of errors encountered
    std::atomic<size_t> progress_batches_completed_{0}; ///< Number of batches completed
    
    /**
     * @brief HTTP helpers
     * @param[in] url Input parameter.
     * @return Return value.
     */
    std::string httpGet(const std::string& url);
    /**
     * @brief TBD: Describe httpGetJson.
     * @param[in] url Input parameter.
     * @return Return value.
     */
    json httpGetJson(const std::string& url);
    
    // Dataset fetching
    struct FetchResult {
        std::vector<json> documents;
        bool has_more;
        size_t offset;
    };
    
    /**
     * @brief TBD: Describe fetchBatch.
     * @param[in] dataset_name Input parameter.
     * @param[in] split Input parameter.
     * @param[in] offset Input parameter.
     * @param[in] limit Input parameter.
     * @return Return value.
     */
    FetchResult fetchBatch(
        const std::string& dataset_name,
        const std::string& split,
        size_t offset,
        size_t limit
    );
    
    /**
     * @brief Caching
     * @param[in] dataset_name Input parameter.
     * @param[in] split Input parameter.
     * @return Return value.
     */
    std::string getCachePath(const std::string& dataset_name, const std::string& split) const;
    /**
     * @brief TBD: Describe loadFromCache.
     * @param[in] dataset_name Input parameter.
     * @param[in] split Input parameter.
     * @param[in,out] docs Input/output parameter.
     * @return True on success.
     */
    bool loadFromCache(const std::string& dataset_name, const std::string& split, std::vector<json>& docs);
    /**
     * @brief TBD: Describe saveToCache.
     * @param[in] dataset_name Input parameter.
     * @param[in] split Input parameter.
     * @param[in] docs Input parameter.
     */
    void saveToCache(const std::string& dataset_name, const std::string& split, const std::vector<json>& docs);
    
    /**
     * @brief Rate limiting
     */
    void waitForRateLimit();
    std::chrono::steady_clock::time_point last_request_time_;
    
    /**
     * @brief Job processing (static so it can be used as callback)
     * @param[in,out] job Input/output parameter.
     * @param[in,out] plugin Input/output parameter.
     */
    static void processHuggingFaceJob(
        content::IngestionJob& job,
        HuggingFaceIngestionPlugin* plugin
    );
    
    /**
     * @brief Helper to convert HF document to ContentManager format
     * @param[in] doc Input parameter.
     * @param[in] dataset_name Input parameter.
     * @param[in] index Input parameter.
     * @return Return value.
     */
    json documentToContentSpec(const json& doc, const std::string& dataset_name, size_t index);
};

} // namespace plugins
} // namespace themis
