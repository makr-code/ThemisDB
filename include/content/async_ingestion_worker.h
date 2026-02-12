#pragma once

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

// Forward declarations
class ContentManager;

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
    SINGLE_FILE,   // Single file upload
    ARCHIVE,       // Archive extraction and ingestion
    BATCH_FILES,   // Multiple files (directory upload)
    URL_FETCH,     // Fetch from URL (future)
    HUGGINGFACE    // Fetch from HuggingFace Hub
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
    json config;       // Job-specific configuration
    std::string user_context;
    
    // Progress tracking
    int64_t created_at;
    int64_t started_at;
    int64_t completed_at;
    int total_items;     // Total files to process
    int processed_items; // Files processed so far
    float progress;      // 0.0 to 1.0
    
    // Result
    std::string error_message;
    std::vector<std::string> content_ids;  // IDs of ingested content
    json result_metadata;
    
    // Callback (optional)
    std::function<void(const IngestionJob&)> on_complete;
};

/**
 * @brief Configuration for async ingestion worker
 */
struct AsyncIngestionConfig {
    size_t worker_thread_count = 2;       // Number of parallel workers
    size_t max_queue_size = 1000;         // Max jobs in queue
    bool enable_auto_cleanup = true;      // Auto-cleanup completed jobs
    int64_t job_retention_ms = 3600000;   // Keep completed jobs for 1 hour
    bool verbose_logging = false;
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
    
    ~AsyncIngestionWorker();
    
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
     * @brief Register a custom job handler
     * 
     * Allows plugins to register handlers for custom job types.
     * 
     * @param job_type Job type to handle
     * @param handler Function to process jobs of this type
     */
    void registerJobHandler(
        IngestionJobType job_type,
        std::function<void(IngestionJob&)> handler
    );

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
    std::condition_variable queue_cv_;
    
    // Job tracking
    std::map<std::string, IngestionJob> job_history_;
    std::mutex history_mutex_;
    
    // Custom job handlers (for plugins)
    std::map<IngestionJobType, std::function<void(IngestionJob&)>> job_handlers_;
    std::mutex handlers_mutex_;
    
    // Statistics
    std::atomic<uint64_t> total_jobs_processed_;
    std::atomic<uint64_t> total_jobs_failed_;
    std::atomic<uint64_t> total_items_processed_;
    
    // Worker thread function
    void workerLoop(int worker_id);
    
    // Job processing
    void processJob(IngestionJob& job);
    void processSingleFile(IngestionJob& job);
    void processArchive(IngestionJob& job);
    void processBatchFiles(IngestionJob& job);
    
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
