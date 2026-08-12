/**
 * @file async_ingestion_worker.h
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
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>
#include <future>
#include <map>
#include <sstream>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

// Forward declarations
class ContentManager;
class IngestionPlugin;
struct IngestionSource;

/**
 * @brief Ingestion job status
 */
enum class IngestionJobStatus {
    QUEUED,      // Waiting in queue
    PROCESSING,  // Currently being processed
    COMPLETED,   // Successfully completed
    FAILED,      // Failed with error
    CANCELLED    // Cancelled by user
};

/**
 * @brief Ingestion job type
 */
enum class IngestionJobType {
    SINGLE_FILE,        // Single file upload
    STREAM_FILE,        // Stream-based ingestion for large files
    ARCHIVE,            // Archive extraction and ingestion
    BATCH_FILES,        // Multiple files (directory upload)
    URL_FETCH,          // Fetch from URL (future)
    HUGGINGFACE,        // HuggingFace datasets
    FILESYSTEM_BULK,    // Recursive filesystem scan
    DATABASE_EXPORT,    // Database via JDBC/ODBC
    REST_API            // Generic REST API
};

/**
 * @brief Ingestion job metadata
 */
struct IngestionJob {
    std::string job_id;
    IngestionJobType type;
    IngestionJobStatus status;
    std::string filename;
    std::string blob;  // Binary data
    std::istream* stream = nullptr;  // Stream for STREAM_FILE jobs (not owned)
    json config;       // Job-specific configuration
    std::string user_context;
    
    // Progress tracking — explicit zero defaults prevent undefined behaviour when a
    // creation path sets only a subset of these fields (CON-014).
    int64_t created_at = 0;      ///< Unix-ms timestamp when the job was enqueued
    int64_t started_at = 0;      ///< Unix-ms timestamp when a worker picked up the job (0 = not started)
    int64_t completed_at = 0;    ///< Unix-ms timestamp when the job finished (0 = not completed)
    int total_items = 0;         ///< Total files to process (−1 = unknown until extracted)
    int processed_items = 0;     ///< Files processed so far
    float progress = 0.0f;       ///< Normalised progress in [0.0, 1.0]
    
    // Result
    std::string error_message;
    std::vector<std::string> content_ids;  // IDs of ingested content
    json result_metadata;
    
    // Callback (optional)
    std::function<void(const IngestionJob&)> on_complete;

    // Promise for ingestStream() callers (optional)
    std::shared_ptr<std::promise<std::string>> completion_promise;
};

/**
 * @brief Configuration for async ingestion worker
 */
struct AsyncIngestionConfig {
    size_t worker_thread_count = 2;       // Number of parallel workers
    size_t max_queue_size = 1000;         // Absolute queue capacity (hard limit)
    size_t max_queue_depth = 1000;        // Back-pressure threshold: callers block when exceeded
    bool enable_auto_cleanup = true;      // Auto-cleanup completed jobs
    int64_t job_retention_ms = 3600000;   // Keep completed jobs for 1 hour
    bool verbose_logging = false;
    size_t batch_size = 64;              // Number of items processed per batch
    int retry_attempts = 3;              // Max retries on transient failures
};

/**
 * @brief Asynchronous Ingestion Worker (Plugin)
 * 
 * Background worker pool for processing ingestion jobs.
 * Designed for development, testing, and showcase purposes.
 * 
 * Features:
 * - Multi-threaded processing
 * - Job queue with priority
 * - Progress tracking
 * - Cancellation support
 * - Completion callbacks
 * 
 * Thread-Safety: Fully thread-safe
 */
class AsyncIngestionWorker {
public:
    explicit AsyncIngestionWorker(
        std::shared_ptr<ContentManager> content_manager,
        AsyncIngestionConfig config = AsyncIngestionConfig{}
    );
    
    ~AsyncIngestionWorker() noexcept;
    
    /**
     * @brief Start the worker threads
     */
    void start();
    
    /**
     * @brief Stop the worker threads (graceful shutdown)
     * 
     * @param wait_for_completion If true, waits for all jobs to finish
     */
    void stop(bool wait_for_completion = true);
    
    /**
     * @brief Check if worker is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * @brief Submit a single file for ingestion
     * 
     * @param blob Binary file data
     * @param filename Original filename
     * @param mime_type Optional MIME type
     * @param user_context User context for auth/encryption
     * @param config Optional job configuration
     * @return Job ID for tracking
     */
    std::string submitFile(
        const std::string& blob,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    /**
     * @brief Submit a stream for chunked ingestion (large-file support)
     *
     * Reads content from the stream in configurable chunks
     * (see ContentManager::ingestStream for config keys).
     * The stream must remain valid until the job completes when
     * wait_for_completion = true; for async jobs the caller is
     * responsible for the stream lifetime.
     *
     * Blocks the calling thread when the queue depth reaches
     * config_.max_queue_depth until a worker dequeues a job.
     *
     * @param stream       Input stream positioned at the start of the content
     * @param filename     Original filename (for MIME detection and metadata)
     * @param mime_type    Optional MIME type override
     * @param user_context User context for auth/encryption
     * @param config       Optional job configuration (chunk_size_bytes, etc.)
     * @return Job ID for tracking
     */
    std::string submitStream(
        std::istream& stream,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    /**
     * @brief Submit a stream for async ingestion with back-pressure
     *
     * Blocks the calling thread when the queue depth reaches
     * config_.max_queue_depth until a worker dequeues a job.
     * Returns a future that resolves to the primary ContentId
     * (std::string) once the ingestion job completes.
     *
     * The stream must remain valid until the returned future is ready.
     *
     * @param stream       Input stream positioned at the start of the content
     * @param filename     Original filename (for MIME detection and metadata)
     * @param mime_type    Optional MIME type override
     * @param user_context User context for auth/encryption
     * @param config       Optional job configuration (chunk_size_bytes, etc.)
     * @return std::future<std::string> resolving to the primary ContentId
     */
    std::future<std::string> ingestStream(
        std::istream& stream,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    /**
     * @brief Submit an archive for extraction and ingestion
     * 
     * @param blob Archive binary data
     * @param filename Archive filename
     * @param user_context User context
     * @param config Archive configuration (strategy, password, etc.)
     * @return Job ID for tracking
     */
    std::string submitArchive(
        const std::string& blob,
        const std::string& filename,
        const std::string& user_context = "",
        const json& config = json::object()
    );
    
    /**
     * @brief Submit multiple files for batch ingestion
     * 
     * @param files Vector of {filename, blob} pairs
     * @param user_context User context
     * @param config Batch configuration
     * @return Job ID for tracking
     */
    std::string submitBatch(
        const std::vector<std::pair<std::string, std::string>>& files,
        const std::string& user_context = "",
        const json& config = json::object()
    );
    
    /**
     * @brief Get job status
     * 
     * @param job_id Job identifier
     * @return Job details if found
     */
    std::optional<IngestionJob> getJobStatus(const std::string& job_id);
    
    /**
     * @brief Cancel a queued or running job
     * 
     * @param job_id Job identifier
     * @return True if job was cancelled
     */
    bool cancelJob(const std::string& job_id);
    
    /**
     * @brief Get all jobs (optionally filtered by status)
     * 
     * @param status Optional status filter
     * @return Vector of jobs
     */
    std::vector<IngestionJob> getAllJobs(
        std::optional<IngestionJobStatus> status = std::nullopt
    );
    
    /**
     * @brief Get queue statistics
     * 
     * @return JSON with queue stats
     */
    json getStatistics();
    
    /**
     * @brief Clear completed jobs from history
     * 
     * @param older_than_ms Clear jobs older than this (milliseconds)
     */
    void clearCompletedJobs(int64_t older_than_ms = 0);
    
    /**
     * @brief Register completion callback for a job
     * 
     * @param job_id Job identifier
     * @param callback Function to call when job completes
     */
    void setCompletionCallback(
        const std::string& job_id,
        std::function<void(const IngestionJob&)> callback
    );
    
    /**
     * @brief Register a custom handler for a specific job type
     * 
     * Allows external code to handle specific job types with custom logic.
     * 
     * @param job_type The type of job to register handler for
     * @param handler Callback function to process jobs of this type
     */
    void registerJobHandler(
        IngestionJobType job_type,
        std::function<void(IngestionJob&)> handler
    );
    
    // ========================================================================
    // Plugin Management API (NEW)
    // ========================================================================
    
    /**
     * @brief Register an ingestion plugin
     * 
     * Plugins extend the worker with new data sources.
     * 
     * Example:
     * ```cpp
     * auto hf_plugin = std::make_shared<HuggingFacePlugin>(...);
     * worker.registerPlugin(hf_plugin);
     * ```
     * 
     * @param plugin Plugin to register
     */
    void registerPlugin(std::shared_ptr<IngestionPlugin> plugin);
    
    /**
     * @brief Unregister a plugin
     * 
     * @param plugin_name Name of plugin to remove
     */
    void unregisterPlugin(const std::string& plugin_name);
    
    /**
     * @brief List registered plugins
     * 
     * @return Vector of plugin names
     */
    std::vector<std::string> listPlugins() const;
    
    /**
     * @brief Get plugin by name
     * 
     * @param name Plugin name
     * @return Plugin pointer or nullptr if not found
     */
    std::shared_ptr<IngestionPlugin> getPlugin(const std::string& name) const;
    
    /**
     * @brief Submit a multi-source job
     * 
     * Creates a job that will be processed by the registered plugin.
     * 
     * @param source Source configuration
     * @param additional_config Optional additional configuration
     * @param user_context Optional user context for audit attribution
     * @return Job ID for tracking
     */
    std::string submitSourceJob(
        const IngestionSource& source,
        const json& additional_config = json::object(),
        const std::string& user_context = ""
    );
    
    /**
     * @brief Load sources from configuration file
     * 
     * YAML format:
     * ```yaml
     * sources:
     *   - source_id: hf_legal
     *     plugin_name: huggingface
     *     type: HUGGINGFACE
     *     location: lexlms/ger_legal_data
     *     priority: 5
     * ```
     * 
     * @param config_path Path to YAML config file
     */
    void loadSourcesFromConfig(const std::string& config_path);

private:
    std::shared_ptr<ContentManager> content_manager_;
    AsyncIngestionConfig config_;
    
    // Worker threads
    std::vector<std::thread> workers_;
    std::atomic<bool> running_;
    std::atomic<bool> shutdown_requested_;
    
    // Job queue
    std::queue<IngestionJob> job_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;        // Signals workers when jobs are available
    std::condition_variable backpressure_cv_; // Signals callers when queue has space
    
    // Job tracking
    std::map<std::string, IngestionJob> job_history_;
    std::mutex history_mutex_;
    
    // Custom job handlers (for plugins)
    std::map<IngestionJobType, std::function<void(IngestionJob&)>> job_handlers_;
    std::mutex handlers_mutex_;
    
    // Statistics — initialised to 0 here for clarity; the constructor's member-
    // initializer list duplicates these, but in-class defaults guard against any
    // future delegating constructor that omits the initializer-list entry (CON-016).
    std::atomic<uint64_t> total_jobs_processed_{0};
    std::atomic<uint64_t> total_jobs_failed_{0};
    std::atomic<uint64_t> total_items_processed_{0};

    // Back-pressure metrics
    std::atomic<uint64_t> total_backpressure_events_{0};  ///< Number of times a caller was blocked by back-pressure
    std::atomic<uint64_t> queue_depth_high_watermark_{0}; ///< Peak queue depth observed since start
    std::atomic<size_t>   inflight_count_{0};              ///< Jobs currently being processed (dequeued but not completed)
    
    // Plugin registry (NEW)
    std::map<std::string, std::shared_ptr<IngestionPlugin>> plugins_;
    mutable std::mutex plugins_mutex_;
    
    // Worker thread function
    void workerLoop(int worker_id);
    
    // Job processing
    void processJob(IngestionJob& job);
    void processSingleFile(IngestionJob& job);
    void processStreamFile(IngestionJob& job);  // Stream-based large file ingestion
    void processArchive(IngestionJob& job);
    void processBatchFiles(IngestionJob& job);
    void processPluginJob(IngestionJob& job);  // NEW: Plugin-based processing
    
    // Helpers
    std::string generateJobId();
    void updateJobStatus(const std::string& job_id, IngestionJobStatus status);
    void updateJobProgress(const std::string& job_id, int processed, int total);
    int64_t getCurrentTimeMs();
    
    // Auto-cleanup thread
    std::thread cleanup_thread_;
    void cleanupLoop();
};

} // namespace content
} // namespace themis
