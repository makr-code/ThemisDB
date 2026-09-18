/**
 * @file async_ingestion_worker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class IngestionJobStatus {
    QUEUED,      // Waiting in queue
    PROCESSING,  // Currently being processed
    COMPLETED,   // Successfully completed
    FAILED,      // Failed with error
    CANCELLED    // Cancelled by user
};

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

class AsyncIngestionWorker {
public:
    explicit AsyncIngestionWorker(
        std::shared_ptr<ContentManager> content_manager,
        AsyncIngestionConfig config = AsyncIngestionConfig{}
    );
    
    ~AsyncIngestionWorker() noexcept;
    
    /**
     * @brief Start.
     */
    void start();
    
    void stop(bool wait_for_completion = true);
    
    bool isRunning() const { return running_.load(); }
    
    std::string submitFile(
        const std::string& blob,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    std::string submitStream(
        std::istream& stream,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    std::future<std::string> ingestStream(
        std::istream& stream,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    std::string submitArchive(
        const std::string& blob,
        const std::string& filename,
        const std::string& user_context = "",
        const json& config = json::object()
    );
    
    std::string submitBatch(
        const std::vector<std::pair<std::string, std::string>>& files,
        const std::string& user_context = "",
        const json& config = json::object()
    );
    
    /**
     * @brief Get Job Status.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    std::optional<IngestionJob> getJobStatus(const std::string& job_id);
    
    /**
     * @brief Cancel Job.
     * @param[in] job_id Identifier of the job.
     * @return True when the operation succeeds.
     */
    bool cancelJob(const std::string& job_id);
    
    std::vector<IngestionJob> getAllJobs(
        std::optional<IngestionJobStatus> status = std::nullopt
    );
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    json getStatistics();
    
    void clearCompletedJobs(int64_t older_than_ms = 0);
    
    void setCompletionCallback(
        const std::string& job_id,
        std::function<void(const IngestionJob&)> callback
    );
    
    void registerJobHandler(
        IngestionJobType job_type,
        std::function<void(IngestionJob&)> handler
    );
    
    // ========================================================================
    // Plugin Management API (NEW)
    // ========================================================================
    
    /**
     * @brief Register Plugin.
     * @param[in] plugin Input parameter.
     */
    void registerPlugin(std::shared_ptr<IngestionPlugin> plugin);
    
    /**
     * @brief Unregister Plugin.
     * @param[in] plugin_name Name of the plugin.
     */
    void unregisterPlugin(const std::string& plugin_name);
    
    /**
     * @brief List Plugins.
     * @return Return value.
     */
    std::vector<std::string> listPlugins() const;
    
    /**
     * @brief Get Plugin.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::shared_ptr<IngestionPlugin> getPlugin(const std::string& name) const;
    
    std::string submitSourceJob(
        const IngestionSource& source,
        const json& additional_config = json::object(),
        const std::string& user_context = ""
    );
    
    /**
     * @brief Load Sources From Config.
     * @param[in] config_path Path to the retention policy configuration file.
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
    /**
     * @brief Worker Loop.
     * @param[in] worker_id Identifier of the worker.
     */
    void workerLoop(int worker_id);
    
    // Job processing
    /**
     * @brief Process Job.
     * @param[in,out] job Input/output parameter.
     */
    void processJob(IngestionJob& job);
    /**
     * @brief Process Single File.
     * @param[in,out] job Input/output parameter.
     */
    void processSingleFile(IngestionJob& job);
    /**
     * @brief Process Stream File.
     * @param[in,out] job Input/output parameter.
     */
    void processStreamFile(IngestionJob& job);  // Stream-based large file ingestion
    /**
     * @brief Process Archive.
     * @param[in,out] job Input/output parameter.
     */
    void processArchive(IngestionJob& job);
    /**
     * @brief Process Batch Files.
     * @param[in,out] job Input/output parameter.
     */
    void processBatchFiles(IngestionJob& job);
    /**
     * @brief Process Plugin Job.
     * @param[in,out] job Input/output parameter.
     */
    void processPluginJob(IngestionJob& job);  // NEW: Plugin-based processing
    
    // Helpers
    /**
     * @brief Generate Job Id.
     * @return Return value.
     */
    std::string generateJobId();
    /**
     * @brief Update Job Status.
     * @param[in] job_id Identifier of the job.
     * @param[in] status Input parameter.
     */
    void updateJobStatus(const std::string& job_id, IngestionJobStatus status);
    /**
     * @brief Update Job Progress.
     * @param[in] job_id Identifier of the job.
     * @param[in] processed Input parameter.
     * @param[in] total Input parameter.
     */
    void updateJobProgress(const std::string& job_id, int processed, int total);
    /**
     * @brief Get Current Time Ms.
     * @return Return value.
     */
    int64_t getCurrentTimeMs();
    
    // Auto-cleanup thread
    std::thread cleanup_thread_;
    /**
     * @brief Cleanup Loop.
     */
    void cleanupLoop();
};

} // namespace content
} // namespace themis
