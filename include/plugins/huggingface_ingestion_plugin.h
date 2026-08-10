/**
 * @file huggingface_ingestion_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: huggingface_ingestion_plugin.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "content/async_ingestion_worker.h"
#include <curl/curl.h>
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <chrono>
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

        /// @brief Optional HuggingFace API token.
        ///
        /// If empty, the plugin reads the @c HUGGINGFACE_TOKEN environment
        /// variable at construction time.  Tokens are never written to logs.
        std::string auth_token;

        // Schema mapping
        std::string text_field = "text";
        std::string label_field = "label";
        std::map<std::string, std::string> custom_fields;
        
        // Caching
        std::string cache_dir = "./cache/huggingface";
        bool use_cache = true;

        /// @brief Path for resume/checkpoint JSON state file.
        ///
        /// If non-empty, the plugin persists the ingestion offset to this
        /// file after each batch so that an interrupted job can be resumed
        /// from the last successful offset.  Leave empty to disable.
        std::string checkpoint_file;
        
        // Rate limiting
        size_t max_requests_per_second = 10;
        
        // Retry configuration
        size_t max_retries = 3;
        size_t retry_delay_ms = 1000;

        /// @brief Directory for Model Hub downloads (Feature 3).
        std::string model_download_dir = "./models/huggingface";

        /// @brief Enable Prometheus per-batch ingestion metrics (Feature 4).
        bool enable_metrics = false;
        
        json toJson() const;
        static Config fromJson(const json& j);
    };
    
    // -----------------------------------------------------------------------
    // Checkpoint support (Feature 2)
    // -----------------------------------------------------------------------

    /**
     * @brief Persisted state for resuming an interrupted ingestion.
     *
     * Written to `Config::checkpoint_file` (JSON) after every batch so that
     * a restarted job can continue from `next_offset` without re-processing
     * already-ingested rows.  Cleared on successful job completion.
     */
    struct CheckpointState {
        std::string job_id;       ///< Job that owns this checkpoint
        std::string dataset_name; ///< Dataset being ingested
        std::string split;        ///< Dataset split being ingested
        size_t      next_offset{0};  ///< First row to fetch on resume
        size_t      total_rows{0};   ///< Estimated total (0 = unknown)
        int64_t     updated_at{0};   ///< UNIX ms of last write

        json toJson() const;
        static CheckpointState fromJson(const json& j);
    };

    // -----------------------------------------------------------------------
    // Model Hub support (Feature 3)
    // -----------------------------------------------------------------------

    /**
     * @brief Result of downloading a model artifact from the HuggingFace Hub.
     */
    struct ModelDownloadResult {
        std::string repo_id;      ///< e.g. "meta-llama/Llama-2-7b-chat-hf"
        std::string filename;     ///< e.g. "model.Q4_K_M.gguf"
        std::string local_path;   ///< Absolute path of the downloaded file
        size_t      bytes{0};     ///< File size in bytes
        std::string sha256;       ///< Hex-encoded SHA-256 of the file
        bool        from_cache{false};

        json toJson() const;
    };

    /**
     * @brief Download a model file from the HuggingFace Model Hub.
     *
     * Supports any file format (GGUF, safetensors, bin).  The downloaded
     * artifact is stored at `output_dir / filename`.  If the same file already
     * exists and its SHA-256 matches the Hub's reported hash the cached copy is
     * returned without re-downloading.
     *
     * @param repo_id  HuggingFace repository identifier (e.g. "TheBloke/Llama-2-7B-GGUF")
     * @param filename File to download (e.g. "llama-2-7b.Q4_K_M.gguf")
     * @param output_dir Directory where the file should be saved
     * @return Download result including local path and SHA-256
     * @throws std::runtime_error on network error, authentication failure, or
     *         SHA-256 mismatch after download
     */
    ModelDownloadResult downloadModelWeights(
        const std::string& repo_id,
        const std::string& filename,
        const std::string& output_dir
    );

    // -----------------------------------------------------------------------
    // Parallel ingestion (Feature 5)
    // -----------------------------------------------------------------------

    /**
     * @brief A single dataset specification inside a parallel ingestion request.
     */
    struct DatasetSpec {
        std::string dataset_name;           ///< HuggingFace dataset identifier
        std::string split = "train";        ///< Dataset split
        int         priority{0};            ///< Higher value = processed first
        json        extra_config = json::object(); ///< Per-dataset overrides
    };

    /**
     * @brief Submit multiple datasets for parallel ingestion.
     *
     * Jobs are dispatched to a thread pool of `concurrency` threads and
     * ordered by `DatasetSpec::priority` (higher first).  Each job runs
     * independently; a failure in one dataset does not abort others.
     *
     * @param datasets     Datasets to ingest
     * @param concurrency  Maximum number of datasets to ingest simultaneously
     *                     (clamped to [1, hardware_concurrency])
     * @return Vector of job IDs, one per DatasetSpec, in the original order
     * @throws std::invalid_argument if `datasets` is empty
     */
    std::vector<std::string> submitParallelDatasetJobs(
        const std::vector<DatasetSpec>& datasets,
        size_t concurrency = 4
    );

    explicit HuggingFaceIngestionPlugin(
        const Config& config,
        std::shared_ptr<content::ContentManager> content_manager
    );
    
    ~HuggingFaceIngestionPlugin();
    
    /**
     * @brief Register plugin with AsyncIngestionWorker
     * 
     * This adds a new job handler for HUGGINGFACE job type.
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
        
        json toJson() const;
    };
    
    DatasetMetadata getDatasetMetadata(const std::string& dataset_name);
    
    /**
     * @brief Estimate dataset size
     */
    size_t estimateDatasetSize(const std::string& dataset_name);

    /// @brief Compute SHA-256 of a local file; returns 64-char hex string.
    ///
    /// Exposed as public static to allow callers to verify downloaded artefacts
    /// without constructing a full plugin instance.
    static std::string computeFileSha256(const std::string& path);

private:
    Config config_;
    std::shared_ptr<content::ContentManager> content_manager_;
    content::AsyncIngestionWorker* worker_ = nullptr;  // Non-owning pointer
    CURL* curl_handle_;
    
    // HTTP helpers
    std::string httpGet(const std::string& url);
    json httpGetJson(const std::string& url);
    
    // Dataset fetching
    struct FetchResult {
        std::vector<json> documents;
        bool has_more;
        size_t offset;
    };
    
    FetchResult fetchBatch(
        const std::string& dataset_name,
        const std::string& split,
        size_t offset,
        size_t limit
    );
    
    // Caching
    std::string getCachePath(const std::string& dataset_name, const std::string& split) const;
    bool loadFromCache(const std::string& dataset_name, const std::string& split, std::vector<json>& docs);
    void saveToCache(const std::string& dataset_name, const std::string& split, const std::vector<json>& docs);
    
    // Rate limiting
    void waitForRateLimit();
    std::chrono::steady_clock::time_point last_request_time_;
    
    // -----------------------------------------------------------------------
    // Token auth (Feature 1)
    // -----------------------------------------------------------------------

    /// @brief Resolved API token (from config or HUGGINGFACE_TOKEN env var).
    ///
    /// Populated once at construction; never logged.
    std::string resolved_token_;

    /// @brief Build and apply the Authorization header onto the curl handle.
    ///
    /// No-op when `resolved_token_` is empty (unauthenticated requests).
    /// @param headers  Existing CURL slist to append the header to.
    /// @return Updated slist (caller must free with curl_slist_free_all).
    curl_slist* applyAuthHeader(curl_slist* headers) const;

    // -----------------------------------------------------------------------
    // Checkpoint (Feature 2)
    // -----------------------------------------------------------------------

    /// @brief Write `state` to `config_.checkpoint_file`.  No-op if path empty.
    void saveCheckpoint(const CheckpointState& state);
    /// @brief Load checkpoint for `dataset_name`/`split`.
    /// @return loaded state, or default-constructed (next_offset==0) if absent.
    CheckpointState loadCheckpoint(const std::string& dataset_name,
                                   const std::string& split) const;
    /// @brief Delete the checkpoint file (called on successful job completion).
    void clearCheckpoint();

    // -----------------------------------------------------------------------
    // Prometheus metrics (Feature 4)
    // -----------------------------------------------------------------------

    /// @brief Accumulated ingestion metrics for a single job run.
    struct BatchMetrics {
        size_t batches_fetched{0};
        size_t rows_ingested{0};
        size_t cache_hits{0};
        size_t cache_misses{0};
        double total_fetch_ms{0.0};
    };

    /// @brief Emit per-batch Prometheus counters/gauges via MetricsCollector.
    ///
    /// Only called when `config_.enable_metrics` is true.
    void emitBatchMetrics(const BatchMetrics& m,
                          const std::string& dataset_name,
                          const std::string& split) const;

    // -----------------------------------------------------------------------
    // Model Hub helpers (Feature 3)
    // -----------------------------------------------------------------------

    /// @brief Fetch the expected SHA-256 from the HuggingFace Hub file metadata.
    std::string fetchModelFileSha256(const std::string& repo_id,
                                     const std::string& filename);

    // Job processing (static so it can be used as callback)
    static void processHuggingFaceJob(
        content::IngestionJob& job,
        HuggingFaceIngestionPlugin* plugin
    );
    
    // Helper to convert HF document to ContentManager format
    json documentToContentSpec(const json& doc, const std::string& dataset_name, size_t index);
};

} // namespace plugins
} // namespace themis
