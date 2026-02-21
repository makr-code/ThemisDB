/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_manager.cpp                              ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1200                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/ingestion_manager.h"
#include "ingestion/huggingface_connector.h"
#include "ingestion/filesystem_ingester.h"
#include "ingestion/api_connector.h"
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>
#include <filesystem>
#include <fstream>

namespace themis {
namespace ingestion {

// ============================================================================
// Correlation ID generator (thread-safe, no external UUID lib)
// ============================================================================
namespace {
static std::string generateCorrelationId() {
    static std::atomic<uint64_t> counter{0};
    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    auto seq = ++counter;
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << static_cast<uint64_t>(ts)
       << '-'
       << std::setw(8) << (seq & 0xFFFFFFFF);
    return ss.str();
}

/// Map SourceType to a short string label for Prometheus
static std::string sourceTypeLabel(SourceType t) {
    switch (t) {
        case SourceType::HUGGINGFACE: return "HUGGINGFACE";
        case SourceType::FILESYSTEM:  return "FILESYSTEM";
        case SourceType::API:         return "API";
        case SourceType::DATABASE:    return "DATABASE";
        default:                      return "UNKNOWN";
    }
}

/// Map IngestionErrorCode to its integer string for a metric label
static std::string errorCodeLabel(IngestionErrorCode c) {
    return std::to_string(static_cast<int>(c));
}
} // anonymous namespace

// ============================================================================
// CheckpointStore
// ============================================================================

namespace {
/// Sanitise a source_id so it is safe as part of a file name.
static std::string sanitiseSourceId(const std::string& sid) {
    std::string out;
    out.reserve(sid.size());
    for (char c : sid) {
        if (std::isalnum(static_cast<unsigned char>(c)) ||
            c == '-' || c == '_' || c == '.') {
            out += c;
        } else {
            out += '_';
        }
    }
    return out.empty() ? "default" : out;
}

/// Format a time_point as a simple ISO-8601-like string (UTC).
static std::string formatTimestamp(std::chrono::system_clock::time_point tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    return buf;
}
} // anonymous namespace

CheckpointStore::CheckpointStore(const std::string& checkpoint_dir)
    : dir_(checkpoint_dir) {}

std::string CheckpointStore::checkpointPath(const std::string& source_id) const {
    namespace fs = std::filesystem;
    return (fs::path(dir_) / (sanitiseSourceId(source_id) + ".checkpoint")).string();
}

bool CheckpointStore::write(const IngestionCheckpoint& cp) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        std::ofstream f(checkpointPath(cp.source_id), std::ios::trunc);
        if (!f) return false;
        f << "source_id="       << cp.source_id       << '\n'
          << "processed_count=" << cp.processed_count  << '\n'
          << "byte_offset="     << cp.byte_offset       << '\n'
          << "cursor="          << cp.cursor            << '\n'
          << "timestamp="       << cp.timestamp         << '\n';
        return f.good();
    } catch (...) {
        return false;
    }
}

bool CheckpointStore::read(const std::string& source_id,
                            IngestionCheckpoint& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        std::ifstream f(checkpointPath(source_id));
        if (!f) return false;
        out = IngestionCheckpoint{};
        std::string line;
        while (std::getline(f, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "source_id")       out.source_id       = val;
            else if (key == "processed_count") {
                try { out.processed_count = std::stoull(val); } catch (...) {}
            } else if (key == "byte_offset") {
                try { out.byte_offset = std::stoull(val); } catch (...) {}
            } else if (key == "cursor")    out.cursor          = val;
            else if (key == "timestamp")   out.timestamp       = val;
        }
        // A valid checkpoint must have a non-empty source_id
        return !out.source_id.empty();
    } catch (...) {
        return false;
    }
}

bool CheckpointStore::clear(const std::string& source_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        return std::filesystem::remove(checkpointPath(source_id));
    } catch (...) {
        return false;
    }
}

bool CheckpointStore::exists(const std::string& source_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::filesystem::exists(checkpointPath(source_id));
}

// ============================================================================
// Token-bucket rate limiter (simple, no external dep)
// ============================================================================
class TokenBucket {
public:
    explicit TokenBucket(double requests_per_second)
        : rate_(requests_per_second)
        , tokens_(requests_per_second > 0.0 ? requests_per_second : 0.0)
        , last_refill_(std::chrono::steady_clock::now()) {}

    /// Consume one token, blocking until available when rate > 0.
    void consume() {
        if (rate_ <= 0.0) return;  // unlimited

        std::unique_lock<std::mutex> lock(mutex_);
        refill();
        while (tokens_ < 1.0) {
            // Calculate wait duration until next token
            double wait_secs = (1.0 - tokens_) / rate_;
            auto wait = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::duration<double>(wait_secs));
            lock.unlock();
            std::this_thread::sleep_for(wait);
            lock.lock();
            refill();
        }
        tokens_ -= 1.0;
    }

    bool isEnabled() const { return rate_ > 0.0; }

private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        double elapsed =
            std::chrono::duration<double>(now - last_refill_).count();
        tokens_ = std::min(rate_, tokens_ + elapsed * rate_);
        last_refill_ = now;
    }

    double rate_;
    double tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    std::mutex mutex_;
};

// ============================================================================
// Pimpl implementation
// ============================================================================
class IngestionManager::Impl {
public:
    explicit Impl(const std::string& db_connection) 
        : db_connection_(db_connection)
        , target_collection_("legal_documents")
        , parallel_enabled_(false)
        , dry_run_(false)
        , max_threads_(std::thread::hardware_concurrency()) {
    }
    
    ~Impl() = default;
    
    bool registerSource(const SourceConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sources_.find(config.source_id) != sources_.end()) {
            return false;
        }
        sources_[config.source_id] = config;
        return true;
    }
    
    bool unregisterSource(const std::string& source_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return sources_.erase(source_id) > 0;
    }
    
    IngestionStats ingestSource(const std::string& source_id,
                               ProgressCallback progress_callback) {
        IngestionStats stats;
        stats.correlation_id = generateCorrelationId();
        auto start_time = std::chrono::steady_clock::now();
        
        // Find source configuration
        SourceConfig config;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sources_.find(source_id);
            if (it == sources_.end()) {
                stats.addError(IngestionErrorCode::SOURCE_NOT_FOUND,
                               IngestionErrorSeverity::ERROR,
                               "Source not found: " + source_id, source_id);
                return stats;
            }
            config = it->second;
        }
        
        if (!config.enabled) {
            stats.addError(IngestionErrorCode::SOURCE_DISABLED,
                           IngestionErrorSeverity::WARNING,
                           "Source disabled: " + source_id, source_id);
            return stats;
        }

        // Apply per-source request-rate throttle (token bucket)
        // The byte-quota is checked after ingestion when bytes_processed is known.
        if (rate_limit_config_.enabled && rate_limit_config_.requests_per_second > 0.0) {
            checkRateLimit(source_id, 0, stats);
        }
        
        // Create connector based on type
        std::unique_ptr<ISourceConnector> connector;
        
        try {
            switch (config.type) {
                case SourceType::HUGGINGFACE: {
                    auto hf_connector = std::make_unique<HuggingFaceConnector>();
                    hf_connector->setRetryConfig(retry_config_);
                    if (!hf_connector->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize HuggingFace connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(hf_connector);
                    break;
                }
                
                case SourceType::FILESYSTEM: {
                    auto fs_ingester = std::make_unique<FileSystemIngester>();
                    if (!fs_ingester->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize filesystem ingester",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(fs_ingester);
                    break;
                }
                
                case SourceType::API: {
                    auto api_connector = std::make_unique<GenericApiConnector>();
                    api_connector->setRetryConfig(retry_config_);
                    if (!api_connector->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize API connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(api_connector);
                    break;
                }

                case SourceType::DATABASE:
                default:
                    stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                                   IngestionErrorSeverity::ERROR,
                                   "Connector type not yet implemented: " +
                                   std::to_string(static_cast<int>(config.type)),
                                   source_id);
                    return stats;
            }
            
            // Check availability
            if (!connector->isAvailable()) {
                stats.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,
                               IngestionErrorSeverity::ERROR,
                               "Source not available: " + source_id, source_id);
                return stats;
            }

            if (dry_run_) {
                stats.documents_processed = connector->getDocumentCount();
                stats.documents_failed    = 0;
            } else {
                // Incremental mode: read checkpoint to get the resume offset.
                // CheckpointStore is captured as a shared_ptr under the lock so
                // it remains valid even if setCheckpointDir() is called concurrently.
                if (incremental_mode_) {
                    std::shared_ptr<CheckpointStore> cs;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        cs = checkpoint_store_shared_;
                    }
                    if (cs) {
                        IngestionCheckpoint cp;
                        // checkpoint_offset is available for connector-level
                        // resume extensions; currently informational.
                        if (cs->read(source_id, cp)) {
                            stats.addError(IngestionErrorCode::OK,
                                           IngestionErrorSeverity::INFO,
                                           "Resuming from checkpoint: " +
                                           std::to_string(cp.processed_count) +
                                           " docs already processed",
                                           source_id);
                        }
                    }
                }

                // Preserve the correlation_id assigned at the start of this run;
                // ingest() returns a fresh IngestionStats that doesn't carry it.
                const std::string corr_id = stats.correlation_id;
                stats = connector->ingest(target_collection_, progress_callback);
                stats.correlation_id = corr_id;
                quarantineFailures(stats, source_id);

                // Check byte-hour quota now that bytes_processed is known
                if (rate_limit_config_.enabled &&
                    rate_limit_config_.max_bytes_per_hour > 0) {
                    checkRateLimit(source_id, stats.bytes_processed, stats);
                }

                // Write checkpoint after a successful run
                if (incremental_mode_ && stats.documents_failed == 0) {
                    std::shared_ptr<CheckpointStore> cs;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        cs = checkpoint_store_shared_;
                    }
                    if (cs) {
                        IngestionCheckpoint cp;
                        cp.source_id       = source_id;
                        cp.processed_count = stats.documents_processed;
                        cp.timestamp       = formatTimestamp(
                            std::chrono::system_clock::now());
                        cs->write(cp);
                    }
                }
            }
            
            auto end_time = std::chrono::steady_clock::now();
            stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
            if (stats.elapsed_seconds > 0.0 && stats.documents_processed > 0) {
                stats.metrics.throughput_docs_per_sec =
                    static_cast<double>(stats.documents_processed) / stats.elapsed_seconds;
            }
            
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception during ingestion: " + std::string(e.what()),
                           source_id);
        }
        
        return stats;
    }
    
    IngestionReport ingestAll(ProgressCallback progress_callback) {
        IngestionReport report;
        report.dry_run = dry_run_;
        
        std::vector<SourceConfig> enabled_sources;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& pair : sources_) {
                if (pair.second.enabled) {
                    enabled_sources.push_back(pair.second);
                }
            }
        }
        
        std::sort(enabled_sources.begin(), enabled_sources.end(),
                 [](const SourceConfig& a, const SourceConfig& b) {
                     return a.priority > b.priority;
                 });

        if (parallel_enabled_ && enabled_sources.size() > 1) {
            const size_t concurrency =
                std::min(max_threads_, enabled_sources.size());

            std::vector<std::future<std::pair<std::string, IngestionStats>>> futures;
            futures.reserve(enabled_sources.size());

            size_t submitted = 0;
            while (submitted < enabled_sources.size()) {
                size_t wave_end = std::min(submitted + concurrency,
                                           enabled_sources.size());
                for (size_t i = submitted; i < wave_end; ++i) {
                    const auto& cfg = enabled_sources[i];
                    futures.push_back(
                        std::async(std::launch::async,
                            [this, cfg, progress_callback]() {
                                return std::make_pair(
                                    cfg.source_id,
                                    ingestSource(cfg.source_id, progress_callback));
                            }));
                }
                for (size_t i = submitted; i < wave_end; ++i) {
                    auto [sid, stats] = futures[i].get();
                    report.source_stats[sid] = stats;
                    report.total_documents += stats.documents_processed;
                    report.total_failures  += stats.documents_failed;
                    report.total_time_seconds += stats.elapsed_seconds;
                }
                submitted = wave_end;
            }
        } else {
            for (const auto& config : enabled_sources) {
                auto stats = ingestSource(config.source_id, progress_callback);
                report.source_stats[config.source_id] = stats;
                report.total_documents += stats.documents_processed;
                report.total_failures  += stats.documents_failed;
                report.total_time_seconds += stats.elapsed_seconds;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            report.quarantine = quarantine_;
        }
        
        return report;
    }
    
    std::vector<SourceConfig> getRegisteredSources() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<SourceConfig> result;
        for (const auto& pair : sources_) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    void setTargetCollection(const std::string& collection_name) {
        target_collection_ = collection_name;
    }
    
    void setParallelProcessing(bool enabled, size_t max_threads) {
        parallel_enabled_ = enabled;
        if (max_threads > 0) {
            max_threads_ = max_threads;
        }
    }

    void setRetryConfig(const RetryConfig& config) {
        retry_config_ = config;
    }

    void setDryRun(bool enabled) { dry_run_ = enabled; }
    bool isDryRun() const { return dry_run_; }

    void setRateLimitConfig(const RateLimitConfig& config) {
        rate_limit_config_ = config;
        // Reset per-source buckets so they're rebuilt with the new rate on next use
        per_source_buckets_.clear();
    }

    std::vector<QuarantineEntry> getQuarantineItems() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return quarantine_;
    }

    bool dismissQuarantineItem(const std::string& item_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(quarantine_.begin(), quarantine_.end(),
            [&item_path](const QuarantineEntry& e) {
                return e.item_path == item_path;
            });
        if (it == quarantine_.end()) {
            return false;
        }
        quarantine_.erase(it);
        return true;
    }

    void clearQuarantine() {
        std::lock_guard<std::mutex> lock(mutex_);
        quarantine_.clear();
    }

    bool pauseSource(const std::string& source_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sources_.find(source_id);
        if (it == sources_.end()) return false;
        it->second.enabled = false;
        return true;
    }

    bool resumeSource(const std::string& source_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sources_.find(source_id);
        if (it == sources_.end()) return false;
        it->second.enabled = true;
        return true;
    }

    SourcePreview previewSource(const std::string& source_id,
                                size_t max_documents) const {
        SourcePreview preview;
        preview.source_id = source_id;

        // Cap to avoid memory exhaustion
        static constexpr size_t kMaxPreviewCap = 100;
        max_documents = std::min(max_documents, kMaxPreviewCap);

        SourceConfig config;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sources_.find(source_id);
            if (it == sources_.end()) return preview;
            config = it->second;
        }

        // Only FILESYSTEM preview is currently implemented
        if (config.type != SourceType::FILESYSTEM) {
            preview.total_available = 0;
            return preview;
        }

        namespace fs = std::filesystem;
        const fs::path root(config.location);
        if (!fs::exists(root)) return preview;

        auto addDoc = [&](const fs::path& p) {
            std::ifstream f(p, std::ios::binary);
            if (!f) return;
            std::string content{std::istreambuf_iterator<char>(f),
                                 std::istreambuf_iterator<char>()};
            if (!content.empty()) {
                preview.documents.push_back(std::move(content));
            }
        };

        if (fs::is_regular_file(root)) {
            preview.total_available = 1;
            addDoc(root);
        } else if (fs::is_directory(root)) {
            // Single pass: count all files and collect up to max_documents.
            for (auto& entry : fs::recursive_directory_iterator(root)) {
                if (!entry.is_regular_file()) continue;
                ++preview.total_available;
                if (preview.documents.size() < max_documents) {
                    addDoc(entry.path());
                }
            }
        }

        if (preview.total_available > max_documents) {
            preview.truncated = true;
        }
        return preview;
    }

    // ── Checkpoint / incremental ingestion ───────────────────────────────────

    void setCheckpointDir(const std::string& dir) {
        // Use shared_ptr so the store can be safely shared with ingestSource()
        // threads without a race when setCheckpointDir() is called concurrently.
        auto new_store = std::make_shared<CheckpointStore>(dir);
        std::lock_guard<std::mutex> lock(mutex_);
        checkpoint_store_shared_ = std::move(new_store);
    }

    void enableIncrementalMode(bool enabled) {
        incremental_mode_ = enabled;
    }

    bool isIncrementalMode() const { return incremental_mode_; }

    bool getCheckpoint(const std::string& source_id,
                       IngestionCheckpoint& out) const {
        std::shared_ptr<CheckpointStore> cs;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cs = checkpoint_store_shared_;
        }
        if (!cs) return false;
        return cs->read(source_id, out);
    }

    bool clearCheckpoint(const std::string& source_id) {
        std::shared_ptr<CheckpointStore> cs;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cs = checkpoint_store_shared_;
        }
        if (!cs) return false;
        return cs->clear(source_id);
    }

private:
    /// Consume a token from the per-source bucket (creates bucket if needed).
    /// Returns false and records a QUOTA_EXCEEDED error if the byte limit is breached.
    bool checkRateLimit(const std::string& source_id,
                        size_t bytes_this_call,
                        IngestionStats& stats) {
        if (!rate_limit_config_.enabled) return true;

        // Per-source token bucket.
        // A shared_ptr is used so that after unlocking the mutex the bucket
        // remains alive even if another thread removes it from the map.
        if (rate_limit_config_.requests_per_second > 0.0) {
            std::shared_ptr<TokenBucket> bucket;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = per_source_buckets_.find(source_id);
                if (it == per_source_buckets_.end()) {
                    auto inserted = per_source_buckets_.emplace(
                        source_id,
                        std::make_shared<TokenBucket>(
                            rate_limit_config_.requests_per_second));
                    it = inserted.first;
                }
                bucket = it->second;
            }
            bucket->consume();
        }

        // Byte-hour quota
        if (rate_limit_config_.max_bytes_per_hour > 0) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& tracker = bytes_this_hour_[source_id];
            tracker.bytes += bytes_this_call;

            // Reset counter if an hour has passed
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::hours>(
                    now - tracker.window_start).count() >= 1) {
                tracker.bytes = bytes_this_call;
                tracker.window_start = now;
            }

            if (tracker.bytes > rate_limit_config_.max_bytes_per_hour) {
                stats.addError(IngestionErrorCode::QUOTA_EXCEEDED,
                               IngestionErrorSeverity::WARNING,
                               "Byte-per-hour quota exceeded for source: " + source_id,
                               source_id);
                stats.metrics.quota_violations++;
                return false;
            }
        }
        return true;
    }

    void quarantineFailures(const IngestionStats& stats,
                            const std::string& source_id) {
        for (const auto& err : stats.errors) {
            if (err.isFatal()) {
                QuarantineEntry entry;
                entry.source_id    = source_id;
                entry.item_path    = err.details.empty()
                    ? ("unknown_item_from_" + source_id)
                    : err.details;
                entry.error_code   = err.code;
                entry.error_message = err.message;
                entry.retry_count  = stats.metrics.retry_count;

                std::lock_guard<std::mutex> lock(mutex_);
                quarantine_.push_back(std::move(entry));
            }
        }
    }

    // Byte-hour tracking per source
    struct ByteWindowTracker {
        size_t bytes = 0;
        std::chrono::steady_clock::time_point window_start =
            std::chrono::steady_clock::now();
    };

    std::string db_connection_;
    std::string target_collection_;
    bool parallel_enabled_;
    bool dry_run_;
    bool incremental_mode_ = false;
    size_t max_threads_;
    RetryConfig retry_config_;
    RateLimitConfig rate_limit_config_;
    std::unordered_map<std::string, std::shared_ptr<TokenBucket>> per_source_buckets_;
    std::unordered_map<std::string, ByteWindowTracker> bytes_this_hour_;
    std::unordered_map<std::string, SourceConfig> sources_;
    std::vector<QuarantineEntry> quarantine_;
    std::shared_ptr<CheckpointStore> checkpoint_store_shared_;  ///< null = no checkpointing
    mutable std::mutex mutex_;
};

// ============================================================================
// Public API implementation
// ============================================================================
IngestionManager::IngestionManager(const std::string& db_connection)
    : impl_(std::make_unique<Impl>(db_connection)) {
}

IngestionManager::~IngestionManager() = default;

bool IngestionManager::registerSource(const SourceConfig& config) {
    return impl_->registerSource(config);
}

bool IngestionManager::unregisterSource(const std::string& source_id) {
    return impl_->unregisterSource(source_id);
}

IngestionStats IngestionManager::ingestSource(const std::string& source_id,
                                             ProgressCallback progress_callback) {
    return impl_->ingestSource(source_id, progress_callback);
}

IngestionReport IngestionManager::ingestAll(ProgressCallback progress_callback) {
    return impl_->ingestAll(progress_callback);
}

std::vector<SourceConfig> IngestionManager::getRegisteredSources() const {
    return impl_->getRegisteredSources();
}

void IngestionManager::setTargetCollection(const std::string& collection_name) {
    impl_->setTargetCollection(collection_name);
}

void IngestionManager::setParallelProcessing(bool enabled, size_t max_threads) {
    impl_->setParallelProcessing(enabled, max_threads);
}

void IngestionManager::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void IngestionManager::setDryRun(bool enabled) {
    impl_->setDryRun(enabled);
}

bool IngestionManager::isDryRun() const {
    return impl_->isDryRun();
}

void IngestionManager::setRateLimitConfig(const RateLimitConfig& config) {
    impl_->setRateLimitConfig(config);
}

std::vector<QuarantineEntry> IngestionManager::getQuarantineItems() const {
    return impl_->getQuarantineItems();
}

bool IngestionManager::dismissQuarantineItem(const std::string& item_path) {
    return impl_->dismissQuarantineItem(item_path);
}

void IngestionManager::clearQuarantine() {
    impl_->clearQuarantine();
}

void IngestionManager::setCheckpointDir(const std::string& checkpoint_dir) {
    impl_->setCheckpointDir(checkpoint_dir);
}

void IngestionManager::enableIncrementalMode(bool enabled) {
    impl_->enableIncrementalMode(enabled);
}

bool IngestionManager::isIncrementalMode() const {
    return impl_->isIncrementalMode();
}

bool IngestionManager::getCheckpoint(const std::string& source_id,
                                      IngestionCheckpoint& out) const {
    return impl_->getCheckpoint(source_id, out);
}

bool IngestionManager::clearCheckpoint(const std::string& source_id) {
    return impl_->clearCheckpoint(source_id);
}

// ============================================================================
// IngestionMetricsExporter
// ============================================================================

namespace {
static std::string promEscapeLabel(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"')  out += "\\\"";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

/// Write a Prometheus metric line with multiple labels
static void writeMetricMultiLabel(
        std::ostream& os,
        const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& labels,
        double value) {
    os << name << '{';
    bool first = true;
    for (const auto& [k, v] : labels) {
        if (!first) os << ',';
        os << k << "=\"" << promEscapeLabel(v) << '"';
        first = false;
    }
    os << "} " << value << '\n';
}

static void writeMetric(std::ostream& os,
                        const std::string& name,
                        const std::string& label_key,
                        const std::string& label_val,
                        double value) {
    writeMetricMultiLabel(os, name, {{label_key, label_val}}, value);
}
} // anonymous namespace

std::string IngestionMetricsExporter::exportText(
        const IngestionReport& report) const {
    std::ostringstream os;

    // Per-source metrics – use source_type from source_stats key if available
    for (const auto& [sid, stats] : report.source_stats) {
        // source_type is not directly in IngestionStats, so we pass empty
        os << exportText(stats, sid);
    }

    const std::string agg_label = "source_id";
    const std::string agg_val   = "__all__";

    os << "# HELP " << prefix_ << "_total_documents "
       << "Total documents ingested across all sources\n"
       << "# TYPE " << prefix_ << "_total_documents counter\n";
    writeMetric(os, prefix_ + "_total_documents",
                agg_label, agg_val,
                static_cast<double>(report.total_documents));

    os << "# HELP " << prefix_ << "_total_failures "
       << "Total failed documents across all sources\n"
       << "# TYPE " << prefix_ << "_total_failures counter\n";
    writeMetric(os, prefix_ + "_total_failures",
                agg_label, agg_val,
                static_cast<double>(report.total_failures));

    os << "# HELP " << prefix_ << "_total_time_seconds "
       << "Total wall-clock time for all sources (seconds)\n"
       << "# TYPE " << prefix_ << "_total_time_seconds gauge\n";
    writeMetric(os, prefix_ + "_total_time_seconds",
                agg_label, agg_val,
                report.total_time_seconds);

    os << "# HELP " << prefix_ << "_quarantine_size "
       << "Number of items currently in quarantine\n"
       << "# TYPE " << prefix_ << "_quarantine_size gauge\n";
    writeMetric(os, prefix_ + "_quarantine_size",
                agg_label, agg_val,
                static_cast<double>(report.quarantine.size()));

    return os.str();
}

std::string IngestionMetricsExporter::exportText(
        const IngestionStats& stats,
        const std::string& source_id,
        const std::string& source_type) const {
    std::ostringstream os;

    // Base labels always present; source_type added when non-empty
    std::vector<std::pair<std::string,std::string>> base_labels = {
        {"source_id", source_id}
    };
    if (!source_type.empty()) {
        base_labels.push_back({"source_type", source_type});
    }

    auto writeStat = [&](const std::string& suffix,
                         const std::string& help,
                         const std::string& type,
                         double value) {
        const std::string metric = prefix_ + suffix;
        os << "# HELP " << metric << ' ' << help << '\n'
           << "# TYPE " << metric << ' ' << type << '\n';
        writeMetricMultiLabel(os, metric, base_labels, value);
    };

    writeStat("_docs_processed_total",
              "Documents successfully processed", "counter",
              static_cast<double>(stats.documents_processed));
    writeStat("_docs_failed_total",
              "Documents that failed to ingest", "counter",
              static_cast<double>(stats.documents_failed));
    writeStat("_bytes_processed_total",
              "Bytes processed", "counter",
              static_cast<double>(stats.bytes_processed));
    writeStat("_elapsed_seconds",
              "Elapsed ingestion time in seconds", "gauge",
              stats.elapsed_seconds);
    writeStat("_retry_total",
              "Total retried requests", "counter",
              static_cast<double>(stats.metrics.retry_count));
    writeStat("_errors_total",
              "Total errors encountered", "counter",
              static_cast<double>(stats.metrics.error_count));
    writeStat("_throughput_docs_per_sec",
              "Document throughput (docs/second)", "gauge",
              stats.metrics.throughput_docs_per_sec);

    // Per-error-code breakdown
    if (!stats.errors.empty()) {
        const std::string ec_metric = prefix_ + "_errors_by_code_total";
        os << "# HELP " << ec_metric
           << " Error count broken down by error_code\n"
           << "# TYPE " << ec_metric << " counter\n";

        // Count occurrences per error code
        std::unordered_map<int, size_t> code_counts;
        for (const auto& err : stats.errors) {
            code_counts[static_cast<int>(err.code)]++;
        }
        for (const auto& [code_int, cnt] : code_counts) {
            auto labels = base_labels;
            labels.push_back({"error_code", std::to_string(code_int)});
            writeMetricMultiLabel(os, ec_metric, labels,
                                  static_cast<double>(cnt));
        }
    }

    return os.str();
}

// ============================================================================
// IngestionBuilder
// ============================================================================

IngestionBuilder::IngestionBuilder(const std::string& db_connection)
    : opts_(std::make_unique<Opts>()) {
    opts_->db_connection = db_connection;
}

IngestionBuilder::~IngestionBuilder() = default;
IngestionBuilder::IngestionBuilder(IngestionBuilder&&) noexcept = default;
IngestionBuilder& IngestionBuilder::operator=(IngestionBuilder&&) noexcept = default;

IngestionBuilder& IngestionBuilder::withHuggingFaceSource(
        const std::string& source_id,
        const std::string& dataset,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::HUGGINGFACE;
    cfg.location  = dataset;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withFilesystemSource(
        const std::string& source_id,
        const std::string& path,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = path;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withRetryConfig(const RetryConfig& config) {
    opts_->retry_config = config;
    return *this;
}

IngestionBuilder& IngestionBuilder::withRateLimitConfig(const RateLimitConfig& config) {
    opts_->rate_limit_config = config;
    return *this;
}

IngestionBuilder& IngestionBuilder::withParallelProcessing(bool enabled,
                                                            size_t max_threads) {
    opts_->parallel_enabled = enabled;
    opts_->max_threads      = max_threads;
    return *this;
}

IngestionBuilder& IngestionBuilder::withTargetCollection(
        const std::string& collection) {
    opts_->target_collection = collection;
    return *this;
}

IngestionBuilder& IngestionBuilder::withDryRun(bool enabled) {
    opts_->dry_run = enabled;
    return *this;
}

std::unique_ptr<IngestionManager> IngestionBuilder::build() {
    auto mgr = std::make_unique<IngestionManager>(opts_->db_connection);

    mgr->setRetryConfig(opts_->retry_config);
    mgr->setRateLimitConfig(opts_->rate_limit_config);
    mgr->setParallelProcessing(opts_->parallel_enabled, opts_->max_threads);
    mgr->setTargetCollection(opts_->target_collection);
    mgr->setDryRun(opts_->dry_run);

    for (const auto& src : opts_->sources) {
        mgr->registerSource(src);
    }

    return mgr;
}

// ============================================================================
// IngestionManager::previewSource  (public wrapper around Impl)
// ============================================================================

SourcePreview IngestionManager::previewSource(const std::string& source_id,
                                               size_t max_documents) const {
    return impl_->previewSource(source_id, max_documents);
}

bool IngestionManager::pauseSource(const std::string& source_id) {
    return impl_->pauseSource(source_id);
}

bool IngestionManager::resumeSource(const std::string& source_id) {
    return impl_->resumeSource(source_id);
}

// ============================================================================
// IngestionAdminApi
// ============================================================================

IngestionAdminApi::IngestionAdminApi(IngestionManager& manager)
    : mgr_(manager) {}

std::vector<SourceStatus> IngestionAdminApi::listSources() const {
    std::vector<SourceStatus> result;
    for (const auto& cfg : mgr_.getRegisteredSources()) {
        SourceStatus s;
        s.source_id  = cfg.source_id;
        s.type       = cfg.type;
        s.enabled    = cfg.enabled;

        // Attempt a lightweight availability probe via previewSource(0)
        try {
            auto preview = mgr_.previewSource(cfg.source_id, 0);
            s.available = true;
            s.doc_count = preview.total_available;
        } catch (...) {
            s.available = false;
        }

        result.push_back(std::move(s));
    }
    return result;
}

IngestionStats IngestionAdminApi::startSource(const std::string& source_id) {
    return mgr_.ingestSource(source_id);
}

bool IngestionAdminApi::pauseSource(const std::string& source_id) {
    return mgr_.pauseSource(source_id);
}

bool IngestionAdminApi::resumeSource(const std::string& source_id) {
    return mgr_.resumeSource(source_id);
}

std::vector<QuarantineEntry> IngestionAdminApi::listQuarantine() const {
    return mgr_.getQuarantineItems();
}

bool IngestionAdminApi::retryQuarantineItem(const std::string& item_path) {
    // Find which source the quarantine entry belongs to
    auto items = mgr_.getQuarantineItems();
    std::string source_id;
    for (const auto& entry : items) {
        if (entry.item_path == item_path) {
            source_id = entry.source_id;
            break;
        }
    }
    if (source_id.empty()) return false;

    // Remove from quarantine and re-run the source
    mgr_.dismissQuarantineItem(item_path);
    mgr_.ingestSource(source_id);
    return true;
}

bool IngestionAdminApi::dismissQuarantineItem(const std::string& item_path) {
    return mgr_.dismissQuarantineItem(item_path);
}

std::string IngestionAdminApi::healthJson() const {
    auto sources    = mgr_.getRegisteredSources();
    auto quarantine = mgr_.getQuarantineItems();

    size_t total     = sources.size();
    size_t enabled   = 0;
    for (const auto& s : sources) {
        if (s.enabled) ++enabled;
    }
    size_t qsize = quarantine.size();

    // Determine overall status
    std::string status = "healthy";
    if (qsize > 0) status = "degraded";
    if (enabled == 0 && total > 0) status = "unhealthy";

    std::ostringstream os;
    os << "{"
       << "\"status\":\"" << status << "\","
       << "\"registered_sources\":" << total << ","
       << "\"enabled_sources\":" << enabled << ","
       << "\"quarantine_size\":" << qsize
       << "}";
    return os.str();
}

} // namespace ingestion
} // namespace themis

