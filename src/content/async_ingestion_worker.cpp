/**
 * @file async_ingestion_worker.cpp
 * @brief Content processor module for async ingestion worker operations.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=4, C=1, H=3, M=4, L=0
 * @note Status: Production Ready; Core ingestion loop functional; backpressure handling deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/async_ingestion_worker.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>

#include "config/config_path_resolver.h"
#include "config/config_schema_validator.h"
#include "content/content_manager.h"
#include "content/ingestion_plugin.h"
#include "utils/logger.h"
#include <stdexcept>

using namespace std::chrono;

namespace themis {
namespace content {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

std::string generateRandomJobId() {
    static thread_local std::mt19937_64 rng{static_cast<uint64_t>(steady_clock::now().time_since_epoch().count())};

    auto u64 = rng();
    std::ostringstream oss = {};
    oss << "job_" << std::hex << std::setw(16) << std::setfill('0') << u64;
    return oss.str();
}

std::string jobTypeToString(IngestionJobType type) {
    switch (type) {
        case IngestionJobType::SINGLE_FILE:
            return "SINGLE_FILE";
        case IngestionJobType::STREAM_FILE:
            return "STREAM_FILE";
        case IngestionJobType::ARCHIVE:
            return "ARCHIVE";
        case IngestionJobType::BATCH_FILES:
            return "BATCH_FILES";
        case IngestionJobType::URL_FETCH:
            return "URL_FETCH";
        case IngestionJobType::HUGGINGFACE:
            return "HUGGINGFACE";
        case IngestionJobType::FILESYSTEM_BULK:
            return "FILESYSTEM_BULK";
        case IngestionJobType::DATABASE_EXPORT:
            return "DATABASE_EXPORT";
        case IngestionJobType::REST_API:
            return "REST_API";
        default:
            return "UNKNOWN";
    }
}

std::string jobStatusToString(IngestionJobStatus status) {
    switch (status) {
        case IngestionJobStatus::QUEUED:
            return "QUEUED";
        case IngestionJobStatus::PROCESSING:
            return "PROCESSING";
        case IngestionJobStatus::COMPLETED:
            return "COMPLETED";
        case IngestionJobStatus::FAILED:
            return "FAILED";
        case IngestionJobStatus::CANCELLED:
            return "CANCELLED";
        default:
            return "UNKNOWN";
    }
}

} // anonymous namespace

// ============================================================================
// AsyncIngestionWorker Implementation
// ============================================================================

AsyncIngestionWorker::AsyncIngestionWorker(std::shared_ptr<ContentManager> content_manager, AsyncIngestionConfig config)
    : content_manager_(content_manager), config_(std::move(config)), running_(false), shutdown_requested_(false),
      total_jobs_processed_(0), total_jobs_failed_(0), total_items_processed_(0), total_backpressure_events_(0),
      queue_depth_high_watermark_(0) {
    if (!content_manager_) {
        throw std::invalid_argument("ContentManager cannot be null");
    }
}

AsyncIngestionWorker::~AsyncIngestionWorker() noexcept {
    try {
        stop(false);  // Force stop without waiting
    } catch (...) {
        // Exceptions must not propagate from destructors (C++ standard §15.5.1).
        // stop() can throw if a mutex operation or promise::set_exception fails;
        // any such failure is silently absorbed here to prevent std::terminate().
        THEMIS_WARN("AsyncIngestionWorker::~AsyncIngestionWorker: exception swallowed during stop()");
    }
}

void AsyncIngestionWorker::start() {
    if (running_.load()) {
        THEMIS_WARN("AsyncIngestionWorker already running");
        return;
    }

    running_.store(true);
    shutdown_requested_.store(false);

    // Start worker threads
    for (size_t i = 0; i < config_.worker_thread_count; ++i) {
        workers_.emplace_back(&AsyncIngestionWorker::workerLoop, this, static_cast<int>(i));
    }

    // Start cleanup thread if auto-cleanup enabled
    if (config_.enable_auto_cleanup) {
        cleanup_thread_ = std::thread(&AsyncIngestionWorker::cleanupLoop, this);
    }

    THEMIS_INFO("AsyncIngestionWorker started with {} worker threads", config_.worker_thread_count);
}

void AsyncIngestionWorker::stop(bool wait_for_completion) {
    if (!running_.load()) {
        return;
    }

    THEMIS_INFO("Stopping AsyncIngestionWorker (wait_for_completion={})", wait_for_completion);

    if (!wait_for_completion) {
        // Cancel all queued jobs
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!job_queue_.empty()) {
            auto job = job_queue_.front();
            job_queue_.pop();
            job.status        = IngestionJobStatus::CANCELLED;
            job.error_message = "Worker shutdown requested";

            // Cancel any attached promise
            if (job.completion_promise) {
                job.completion_promise->set_exception(
                    std::make_exception_ptr(std::runtime_error("Worker shutdown requested")));
            }

            std::lock_guard<std::mutex> hist_lock(history_mutex_);
            job_history_[job.job_id] = job;
        }
    }

    shutdown_requested_.store(true);
    queue_cv_.notify_all();
    backpressure_cv_.notify_all(); // Wake blocked submitters so they can observe shutdown

    // Wait for workers to finish
    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    // Stop cleanup thread
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }

    running_.store(false);

    THEMIS_INFO("AsyncIngestionWorker stopped");
}

std::string AsyncIngestionWorker::submitFile(const std::string &blob, const std::string &filename,
                                             const std::string &mime_type, const std::string &user_context,
                                             const json &config) {
    if (!running_.load()) {
        throw std::runtime_error("Worker not running");
    }

    IngestionJob job;
    job.job_id              = generateJobId();
    job.type                = IngestionJobType::SINGLE_FILE;
    job.status              = IngestionJobStatus::QUEUED;
    job.filename            = filename;
    job.blob                = blob;
    job.config              = config;
    job.config["mime_type"] = mime_type;
    job.user_context = user_context;
    job.created_at = getCurrentTimeMs();
    job.total_items = 1;
    // started_at, completed_at, processed_items, progress default to 0/0.0f (CON-014)

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        if (static_cast<int>(job_queue_.size()) > = config_.max_queue_size) {
            throw std::runtime_error("Job queue full");
        }

        job_queue_.push(job);

        // Add to history
        std::lock_guard<std::mutex> hist_lock(history_mutex_);
        job_history_[job.job_id] = job;
    }

    queue_cv_.notify_one();

    if (config_.verbose_logging) {
        THEMIS_INFO("Job submitted: {} ({})", job.job_id, filename);
    }

    return job.job_id;
}

std::string AsyncIngestionWorker::submitStream(std::istream &stream, const std::string &filename,
                                               const std::string &mime_type, const std::string &user_context,
                                               const json &config) {
    if (!running_.load()) {
        throw std::runtime_error("Worker not running");
    }

    IngestionJob job;
    job.job_id              = generateJobId();
    job.type                = IngestionJobType::STREAM_FILE;
    job.status              = IngestionJobStatus::QUEUED;
    job.filename            = filename;
    job.stream              = &stream;
    job.config              = config;
    job.config["mime_type"] = mime_type;
    job.user_context = user_context;
    job.created_at   = getCurrentTimeMs();
    job.total_items  = 1;
    // started_at, completed_at, processed_items, progress default to 0/0.0f (CON-014)

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        // Count a back-pressure event if the queue is already at capacity
        if ((job_queue_.size() + inflight_count_.load(std::memory_order_relaxed)) >= config_.max_queue_depth) {
            total_backpressure_events_.fetch_add(1, std::memory_order_relaxed);
        }
        // Block until queue depth is below the back-pressure threshold
        backpressure_cv_.wait(lock, [this] {
            return (job_queue_.size() + inflight_count_.load(std::memory_order_relaxed)) < config_.max_queue_depth
                   || !running_.load() || shutdown_requested_.load();
        });
        if (!running_.load() || shutdown_requested_.load()) {
            throw std::runtime_error("Worker shutting down");
        }
        job_queue_.push(job);
        // Update queue depth high-watermark
        size_t depth   = job_queue_.size();
        size_t old_hwm = queue_depth_high_watermark_.load(std::memory_order_relaxed);
        while (depth > old_hwm
               && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {
        }
        std::lock_guard<std::mutex> hist_lock(history_mutex_);
        job_history_[job.job_id] = job;
    }

    queue_cv_.notify_one();

    if (config_.verbose_logging) {
        THEMIS_INFO("Stream job submitted: {} ({})", job.job_id, filename);
    }

    return job.job_id;
}

std::future<std::string> AsyncIngestionWorker::ingestStream(std::istream &stream, const std::string &filename,
                                                            const std::string &mime_type,
                                                            const std::string &user_context, const json &config) {
    if (!running_.load()) {
        throw std::runtime_error("Worker not running");
    }

    auto promise                    = std::make_shared<std::promise<std::string>>();
    std::future<std::string> future = promise->get_future();

    IngestionJob job;
    job.job_id              = generateJobId();
    job.type                = IngestionJobType::STREAM_FILE;
    job.status              = IngestionJobStatus::QUEUED;
    job.filename            = filename;
    job.stream              = &stream;
    job.config              = config;
    job.config["mime_type"] = mime_type;
    job.user_context        = user_context;
    job.created_at          = getCurrentTimeMs();
    job.started_at          = 0;
    job.completed_at        = 0;
    job.total_items         = 1;
    job.processed_items     = 0;
    job.progress            = 0.0f;
    job.completion_promise  = promise;

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        // Count a back-pressure event if the queue is already at capacity
        if ((job_queue_.size() + inflight_count_.load(std::memory_order_relaxed)) >= config_.max_queue_depth) {
            total_backpressure_events_.fetch_add(1, std::memory_order_relaxed);
        }
        // Block until queue depth is below the back-pressure threshold
        backpressure_cv_.wait(lock, [this] {
            return (job_queue_.size() + inflight_count_.load(std::memory_order_relaxed)) < config_.max_queue_depth
                   || !running_.load() || shutdown_requested_.load();
        });
        if (!running_.load() || shutdown_requested_.load()) {
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Worker shutting down")));
            return future;
        }
        job_queue_.push(job);
        // Update queue depth high-watermark
        size_t depth   = job_queue_.size();
        size_t old_hwm = queue_depth_high_watermark_.load(std::memory_order_relaxed);
        while (depth > old_hwm
               && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {
        }
        std::lock_guard<std::mutex> hist_lock(history_mutex_);
        job_history_[job.job_id] = job;
    }

    queue_cv_.notify_one();

    if (config_.verbose_logging) {
        THEMIS_INFO("Async stream job submitted: {} ({})", job.job_id, filename);
    }

    return future;
}

std::string AsyncIngestionWorker::submitArchive(const std::string &blob, const std::string &filename,
                                                const std::string &user_context, const json &config) {
    if (!running_.load()) {
        throw std::runtime_error("Worker not running");
    }

    IngestionJob job;
    job.job_id = generateJobId();
    job.type = IngestionJobType::ARCHIVE;
    job.status = IngestionJobStatus::QUEUED;
    job.filename = filename;
    job.blob = blob;
    job.config = config;
    job.user_context = user_context;
    job.created_at = getCurrentTimeMs();
    job.total_items = -1;  // Unknown until extracted
    // started_at, completed_at, processed_items, progress default to 0/0.0f (CON-014)
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        if (static_cast<int>(job_queue_.size()) > = config_.max_queue_size) {
            throw std::runtime_error("Job queue full");
        }

        job_queue_.push(job);

        // Add to history
        std::lock_guard<std::mutex> hist_lock(history_mutex_);
        job_history_[job.job_id] = job;
    }

    queue_cv_.notify_one();

    if (config_.verbose_logging) {
        THEMIS_INFO("Archive job submitted: {} ({})", job.job_id, filename);
    }

    return job.job_id;
}

std::string AsyncIngestionWorker::submitBatch(const std::vector<std::pair<std::string, std::string>> &files,
                                              const std::string &user_context, const json &config) {
    if (!running_.load()) {
        throw std::runtime_error("Worker not running");
    }

    IngestionJob job;
    job.job_id = generateJobId();
    job.type = IngestionJobType::BATCH_FILES;
    job.status = IngestionJobStatus::QUEUED;
    job.filename = "batch_" + std::to_string(files.size()) + "_files";
    job.config = config;
    job.user_context = user_context;
    job.created_at = getCurrentTimeMs();
    job.total_items = static_cast<int>(files.size());
    // started_at, completed_at, processed_items, progress default to 0/0.0f (CON-014)
    
    // Store file list in config
    json file_list = json::array();
    for (const auto &[filename, blob] : files) {
        file_list.push_back({{"filename", filename}, {"size", blob.size()}});
    }
    job.config["files"]  = file_list;
    job.config["_blobs"] = json::array(); // Will be filled with actual blobs

    // Store blobs separately (not in job metadata)
    for (const auto &[filename, blob] : files) {
        job.config["_blobs"].push_back({{"filename", filename}, {"blob", blob}});
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        if (static_cast<int>(job_queue_.size()) > = config_.max_queue_size) {
            throw std::runtime_error("Job queue full");
        }

        job_queue_.push(job);

        // Add to history (without blobs for efficiency)
        std::lock_guard<std::mutex> hist_lock(history_mutex_);
        auto hist_job = job;
        hist_job.config.erase("_blobs"); // Don't store blobs in history
        job_history_[job.job_id] = hist_job;
    }

    queue_cv_.notify_one();

    if (config_.verbose_logging) {
        THEMIS_INFO("Batch job submitted: {} ({} files)", job.job_id, files.size());
    }

    return job.job_id;
}

std::optional<IngestionJob> AsyncIngestionWorker::getJobStatus(const std::string &job_id) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = job_history_.find(job_id);
    if (it != job_history_.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool AsyncIngestionWorker::cancelJob(const std::string &job_id) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = job_history_.find(job_id);
    if (it == job_history_.end()) {
        return false;
    }

    if (it->second.status == IngestionJobStatus::QUEUED) {
        it->second.status        = IngestionJobStatus::CANCELLED;
        it->second.error_message = "Cancelled by user";
        return true;
    }

    // Cannot cancel running/completed jobs
    return false;
}

std::vector<IngestionJob> AsyncIngestionWorker::getAllJobs(std::optional<IngestionJobStatus> status) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    std::vector<IngestionJob> result = {};

    for (const auto &[job_id, job] : job_history_) {
        if (!status.has_value() || job.status == status.value()) {
            result.push_back(job);
        }
    }

    return result;
}

json AsyncIngestionWorker::getStatistics() {
    std::lock_guard<std::mutex> q_lock(queue_mutex_);
    std::lock_guard<std::mutex> h_lock(history_mutex_);

    size_t queued = 0, processing = 0, completed = 0, failed = 0, cancelled = 0;

    for (const auto &[_, job] : job_history_) {
        switch (job.status) {
            case IngestionJobStatus::QUEUED:
                queued++;
                break;
            case IngestionJobStatus::PROCESSING:
                processing++;
                break;
            case IngestionJobStatus::COMPLETED:
                completed++;
                break;
            case IngestionJobStatus::FAILED:
                failed++;
                break;
            case IngestionJobStatus::CANCELLED:
                cancelled++;
                break;
        }
    }

    return json{{"running", running_.load()},
                {"worker_count", config_.worker_thread_count},
                {"queue_size", job_queue_.size()},
                {"max_queue_size", config_.max_queue_size},
                {"max_queue_depth", config_.max_queue_depth},
                {"jobs",
                 {{"queued", queued},
                  {"processing", processing},
                  {"completed", completed},
                  {"failed", failed},
                  {"cancelled", cancelled},
                  {"total", job_history_.size()}}},
                {"stats",
                 {{"total_processed", total_jobs_processed_.load()},
                  {"total_failed", total_jobs_failed_.load()},
                  {"total_items", total_items_processed_.load()}}},
                {"backpressure",
                 {{"events_total", total_backpressure_events_.load()},
                  {"queue_depth_high_watermark", queue_depth_high_watermark_.load()}}}};
}

void AsyncIngestionWorker::clearCompletedJobs(int64_t older_than_ms) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    int64_t cutoff = (older_than_ms > 0) ? (getCurrentTimeMs() - older_than_ms) : std::numeric_limits<int64_t>::max();

    auto it = job_history_.begin();
    while (it != job_history_.end()) {
        const auto &job = it->second;
        if ((job.status == IngestionJobStatus::COMPLETED || job.status == IngestionJobStatus::FAILED
             || job.status == IngestionJobStatus::CANCELLED)
            && job.completed_at < cutoff) {
            it = job_history_.erase(it);
        } else {
            ++it;
        }
    }
}

void AsyncIngestionWorker::setCompletionCallback(const std::string &job_id,
                                                 std::function<void(const IngestionJob &)> callback) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = job_history_.find(job_id);
    if (it != job_history_.end()) {
        it->second.on_complete = callback;
    }
}

void AsyncIngestionWorker::registerJobHandler(IngestionJobType job_type, std::function<void(IngestionJob &)> handler) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] handlers_mutex_);
    job_handlers_[job_type] = handler;

    if (config_.verbose_logging) {
        THEMIS_INFO("Registered custom handler for job type: {}", jobTypeToString(job_type));
    }
}

// ============================================================================
// Worker Thread Implementation
// ============================================================================

void AsyncIngestionWorker::workerLoop([[maybe_unused]] int worker_id) {
    if (config_.verbose_logging) {
        THEMIS_INFO("Worker {} started", worker_id);
    }

    while (true) {
        IngestionJob job;

        // Wait for a job
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            queue_cv_.wait(lock, [this] { return !job_queue_.empty() || shutdown_requested_.load(); });

            if (shutdown_requested_.load() && job_queue_.empty()) {
                break;
            }

            if (job_queue_.empty()) {
                continue;
            }

            job = job_queue_.front();
            job_queue_.pop();
            inflight_count_.fetch_add(1, std::memory_order_relaxed);
        }

        // Check if cancelled
        {
            std::lock_guard<std::mutex> lock(history_mutex_);
            auto it = job_history_.find(job.job_id);
            if (it != job_history_.end() && it->second.status == IngestionJobStatus::CANCELLED) {
                // Cancel any attached promise
                if (job.completion_promise) {
                    job.completion_promise->set_exception(std::make_exception_ptr(std::runtime_error("Job cancelled")));
                }
                {
                    std::lock_guard<std::mutex> bp_lock(queue_mutex_);
                    inflight_count_.fetch_sub(1, std::memory_order_relaxed);
                }
                backpressure_cv_.notify_one();
                continue;
            }
        }

        // Process job
        job.status     = IngestionJobStatus::PROCESSING;
        job.started_at = getCurrentTimeMs();
        updateJobStatus(job.job_id, IngestionJobStatus::PROCESSING);

        if (config_.verbose_logging) {
            THEMIS_INFO("Worker {} processing job {}: {}", worker_id, job.job_id, job.filename);
        }

        try {
            processJob(job);

            job.status       = IngestionJobStatus::COMPLETED;
            job.completed_at = getCurrentTimeMs();
            total_jobs_processed_.fetch_add(1);

            if (config_.verbose_logging) {
                THEMIS_INFO("Worker {} completed job {} ({} items)", worker_id, job.job_id, job.content_ids.size());
            }

            // Fulfill promise for ingestStream() callers
            if (job.completion_promise) {
                std::string result_id = job.content_ids.empty() ? "" : job.content_ids.front();
                job.completion_promise->set_value(result_id);
            }

        } catch (const std::exception &e) {
            job.status        = IngestionJobStatus::FAILED;
            job.error_message = std::string("Exception: ") + e.what();
            job.completed_at  = getCurrentTimeMs();
            total_jobs_failed_.fetch_add(1);

            THEMIS_ERROR("Worker {} failed job {}: {}", worker_id, job.job_id, e.what());

            // Break promise for ingestStream() callers
            if (job.completion_promise) {
                job.completion_promise->set_exception(std::current_exception());
            }
        }

        // Update history and call callback
        {
            std::lock_guard<std::mutex> lock(history_mutex_);
            job_history_[job.job_id] = job;

            if (job.on_complete) {
                try {
                    job.on_complete(job);
                } catch (const std::exception &e) {
                    THEMIS_ERROR("Completion callback failed for job {}: {}", job.job_id, e.what());
                }
            }
        }

        // Release inflight slot and notify any blocked submitters
        {
            std::lock_guard<std::mutex> bp_lock(queue_mutex_);
            inflight_count_.fetch_sub(1, std::memory_order_relaxed);
        }
        backpressure_cv_.notify_one();
    }

    if (config_.verbose_logging) {
        THEMIS_INFO("Worker {} stopped", worker_id);
    }
}

void AsyncIngestionWorker::processJob(IngestionJob &job) {
    // Check for custom handler first
    {
        std::lock_guard<std::mutex> lock([[maybe_unused]] handlers_mutex_);
        auto it = job_handlers_.find([[maybe_unused]] job.type);
        if ([[maybe_unused]] it != job_handlers_.end()) {
            it->second(job);
            return;
        }
    }

    // Fall back to built-in handlers
    switch (job.type) {
        case IngestionJobType::SINGLE_FILE:
            processSingleFile(job);
            break;
        case IngestionJobType::STREAM_FILE:
            processStreamFile(job);
            break;
        case IngestionJobType::ARCHIVE:
            processArchive(job);
            break;
        case IngestionJobType::BATCH_FILES:
            processBatchFiles(job);
            break;
        case IngestionJobType::HUGGINGFACE:
        [[fallthrough]];\n        case IngestionJobType::FILESYSTEM_BULK:
        [[fallthrough]];\n        case IngestionJobType::DATABASE_EXPORT:
        [[fallthrough]];\n        case IngestionJobType::REST_API:
            // Plugin-based job types
            processPluginJob(job);
            break;
        default:
            throw std::runtime_error("Unsupported job type: " + jobTypeToString(job.type));
    }
}

void AsyncIngestionWorker::processSingleFile(IngestionJob &job) {
    std::string mime_type = job.config.value("mime_type", "");

    auto result = content_manager_->ingestRawBlob(job.blob, job.filename, mime_type, job.user_context, job.config);

    if (!result.success) {
        throw std::runtime_error(result.error_message);
    }

    job.content_ids.push_back(result.primary_content_id);
    job.content_ids.insert(job.content_ids.end(), result.extracted_content_ids.begin(),
                           result.extracted_content_ids.end());
    job.result_metadata = result.metadata;
    job.processed_items = 1;
    job.progress        = 1.0f;

    total_items_processed_.fetch_add(1);
}

void AsyncIngestionWorker::processStreamFile(IngestionJob &job) {
    if (!job.stream) {
        throw std::runtime_error("Stream job has no stream pointer");
    }

    std::string mime_type = job.config.value("mime_type", "");

    auto result = content_manager_->ingestStream(*job.stream, job.filename, mime_type, job.user_context, job.config);

    if (!result.success) {
        throw std::runtime_error(result.error_message);
    }

    job.content_ids.push_back(result.primary_content_id);
    job.result_metadata = result.metadata;
    job.processed_items = 1;
    job.progress        = 1.0f;

    total_items_processed_.fetch_add(1);
}

void AsyncIngestionWorker::processArchive(IngestionJob &job) {
    auto result = content_manager_->ingestRawBlob(job.blob, job.filename,
                                                  "", // Auto-detect
                                                  job.user_context, job.config);

    if (!result.success) {
        throw std::runtime_error(result.error_message);
    }

    job.content_ids.push_back(result.primary_content_id);
    job.content_ids.insert(job.content_ids.end(), result.extracted_content_ids.begin(),
                           result.extracted_content_ids.end());
    job.result_metadata = result.metadata;
    job.total_items     = static_cast<int>(result.extracted_content_ids.size()) + 1;
    job.processed_items = job.total_items;
    job.progress        = 1.0f;

    total_items_processed_.fetch_add(job.total_items);
}

void AsyncIngestionWorker::processBatchFiles(IngestionJob &job) {
    if (!job.config.contains("_blobs")) {
        throw std::runtime_error("Batch job missing file blobs");
    }

    auto blobs    = job.config["_blobs"];
    int processed = 0;

    for (const auto &file_entry : blobs) {
        std::string filename = file_entry["filename"];
        std::string blob     = file_entry["blob"];

        try {
            auto result = content_manager_->ingestRawBlob(blob, filename,
                                                          "", // Auto-detect
                                                          job.user_context, job.config);

            if (result.success) {
                job.content_ids.push_back(result.primary_content_id);
                job.content_ids.insert(job.content_ids.end(), result.extracted_content_ids.begin(),
                                       result.extracted_content_ids.end());
            }
        } catch (const std::exception &e) {
            THEMIS_WARN("Failed to ingest file {} in batch: {}", filename, e.what());
        }

        processed++;
        job.processed_items = processed;
        job.progress        = static_cast<float>(processed) / job.total_items;
        updateJobProgress(job.job_id, processed, job.total_items);

        total_items_processed_.fetch_add(1);
    }
}

// ============================================================================
// Plugin Management API (NEW)
// ============================================================================

void AsyncIngestionWorker::registerPlugin(std::shared_ptr<IngestionPlugin> plugin) {
    if (!plugin) {
        throw std::invalid_argument("Plugin cannot be null");
    }

    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto name = plugin->name();
    if (plugins_.find(name) != plugins_.end()) {
        THEMIS_WARN("Plugin '{}' already registered, replacing", name);
    }

    plugins_[name] = plugin;
    THEMIS_INFO("Registered plugin: {} (version {})", name, plugin->version());
}

void AsyncIngestionWorker::unregisterPlugin(const std::string &plugin_name) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto it = plugins_.find(plugin_name);
    if (it != plugins_.end()) {
        plugins_.erase(it);
        THEMIS_INFO("Unregistered plugin: {}", plugin_name);
    } else {
        THEMIS_WARN("Plugin '{}' not found, nothing to unregister", plugin_name);
    }
}

std::vector<std::string> AsyncIngestionWorker::listPlugins() const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    std::vector<std::string> names = {};

    names.reserve(plugins_.size());
    for (const auto &[name, plugin] : plugins_) {
        names.push_back(name);
    }
    return names;
}

std::shared_ptr<IngestionPlugin> AsyncIngestionWorker::getPlugin(const std::string &name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second;
    }
    return nullptr;
}

std::string AsyncIngestionWorker::submitSourceJob(const IngestionSource &source, const json &additional_config,
                                                  const std::string &user_context) {
    if (!running_.load()) {
        throw std::runtime_error("Worker not running");
    }

    // For source-backed jobs, a registered plugin is preferred so we can
    // estimate work size. Tests and custom integrations may instead provide
    // a direct job-type handler, in which case plugin lookup is optional.
    bool has_custom_handler = false;
    {
        std::lock_guard<std::mutex> lock([[maybe_unused]] handlers_mutex_);
        has_custom_handler = job_handlers_.find([[maybe_unused]] source.type) != job_handlers_.end();
    }

    // Find plugin
    std::shared_ptr<IngestionPlugin> plugin;
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        auto it = plugins_.find(source.plugin_name);
        if (it == plugins_.end()) {
            if ([[maybe_unused]] !has_custom_handler) {
                throw std::runtime_error("Plugin not found: " + source.plugin_name);
            }
        } else {
            plugin = it->second;
        }
    }

    // Create job
    IngestionJob job;
    job.job_id           = generateJobId();
    job.type             = source.type;
    job.status           = IngestionJobStatus::QUEUED;
    job.filename         = source.location;
    job.config           = source.config;
    job.config["source"] = source.toJson();
    job.config.merge_patch(additional_config);
    job.user_context = user_context;
    job.created_at = getCurrentTimeMs();
    // started_at, completed_at, processed_items, progress default to 0/0.0f (CON-014)
    
    // Estimate size when a plugin is available; otherwise default to a single
    // logical item for handler-driven source jobs.
    if (plugin) {
        try {
            job.total_items = static_cast<int>(plugin->estimateJobSize(job));
        } catch (const std::exception &e) {
            THEMIS_WARN("Plugin {} failed to estimate job size: {}", source.plugin_name, e.what());
            job.total_items = -1; // Unknown
        }
    } else {
        job.total_items = 1;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        if (static_cast<int>(job_queue_.size()) > = config_.max_queue_size) {
            throw std::runtime_error("Job queue full");
        }

        job_queue_.push(job);

        // Add to history
        std::lock_guard<std::mutex> hist_lock(history_mutex_);
        job_history_[job.job_id] = job;
    }

    queue_cv_.notify_one();

    if (config_.verbose_logging) {
        THEMIS_INFO("Source job submitted: {} (plugin: {}, type: {})", job.job_id, source.plugin_name,
                    jobTypeToString(source.type));
    }

    return job.job_id;
}

void AsyncIngestionWorker::loadSourcesFromConfig(const std::string &config_path) {
    // Allow explicit absolute file paths (e.g. tests using temp files).
    // Otherwise, resolve via ConfigPathResolver for mapped repository configs.
    std::string resolved_path = {};
    std::filesystem::path requested_path(config_path);
    if (requested_path.is_absolute() && std::filesystem::exists(requested_path)) {
        resolved_path = requested_path.lexically_normal().string();
    } else {
        resolved_path = themis::config::ConfigPathResolver::resolve(config_path);
    }

    // Load and parse the YAML config file
    nlohmann::json cfg = themis::config::ConfigSchemaValidator::loadAsJson(resolved_path);

    // Apply optional worker pool settings from the config
    if (cfg.contains("worker_threads") && cfg["worker_threads"].is_number_unsigned()) {
        config_.worker_thread_count = cfg["worker_threads"].get<size_t>();
    }
    if (cfg.contains("queue_depth") && cfg["queue_depth"].is_number_unsigned()) {
        config_.max_queue_depth = cfg["queue_depth"].get<size_t>();
    }
    if (cfg.contains("batch_size") && cfg["batch_size"].is_number_unsigned()) {
        config_.batch_size = cfg["batch_size"].get<size_t>();
    }
    if (cfg.contains("retry_attempts") && cfg["retry_attempts"].is_number_integer()) {
        config_.retry_attempts = cfg["retry_attempts"].get<int>();
    }

    // Submit each source listed under the "sources" key
    if (!cfg.contains("sources") || !cfg["sources"].is_array()) {
        if (config_.verbose_logging) {
            THEMIS_INFO("loadSourcesFromConfig: no sources array in {}", resolved_path);
        }
        return;
    }

    int submitted = 0;
    for (const auto &src_json : cfg["sources"]) {
        try {
            auto source          = IngestionSource::fromJson(src_json);
            std::string user_ctx = src_json.value("user_context", "");
            submitSourceJob(source, json::object(), user_ctx);
            ++submitted;
        } catch (const std::exception &e) {
            THEMIS_WARN("loadSourcesFromConfig: failed to submit source from {}: {}", resolved_path, e.what());
        }
    }

    THEMIS_INFO("loadSourcesFromConfig: submitted {} source job(s) from {}", submitted, resolved_path);
}

void AsyncIngestionWorker::processPluginJob(IngestionJob &job) {
    // Extract source config
    if (!job.config.contains("source")) {
        throw std::runtime_error("Plugin job missing source configuration");
    }

    auto source_json = job.config["source"];
    auto source      = IngestionSource::fromJson(source_json);

    // Find plugin
    std::shared_ptr<IngestionPlugin> plugin;
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        auto it = plugins_.find(source.plugin_name);
        if (it == plugins_.end()) {
            throw std::runtime_error("Plugin not found: " + source.plugin_name);
        }
        plugin = it->second;
    }

    // Process via plugin
    plugin->processJob(job);
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string AsyncIngestionWorker::generateJobId() {
    return generateRandomJobId();
}

void AsyncIngestionWorker::updateJobStatus(const std::string &job_id, IngestionJobStatus status) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = job_history_.find(job_id);
    if (it != job_history_.end()) {
        it->second.status = status;
    }
}

void AsyncIngestionWorker::updateJobProgress(const std::string &job_id, int processed, int total) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = job_history_.find(job_id);
    if (it != job_history_.end()) {
        it->second.processed_items = processed;
        it->second.total_items     = total;
        it->second.progress        = total > 0 ? static_cast<float>(processed) / total : 0.0f;
    }
}

int64_t AsyncIngestionWorker::getCurrentTimeMs() {
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void AsyncIngestionWorker::cleanupLoop() {
    while (!shutdown_requested_.load()) {
        // Sleep for 5 minutes
        std::this_thread::sleep_for(std::chrono::minutes(5));

        if (shutdown_requested_.load()) {
            break;
        }

        // Cleanup old completed jobs
        clearCompletedJobs(config_.job_retention_ms);
    }
}

} // namespace content
} // namespace themis

