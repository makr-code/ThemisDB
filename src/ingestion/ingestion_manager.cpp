#include "ingestion/ingestion_manager.h"
#include "ingestion/huggingface_connector.h"
#include "ingestion/filesystem_ingester.h"
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>

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

        // Apply rate limiting (token bucket)
        if (rate_limit_config_.enabled && rate_limit_config_.requests_per_second > 0.0) {
            if (!token_bucket_) {
                token_bucket_ = std::make_unique<TokenBucket>(
                    rate_limit_config_.requests_per_second);
            }
            token_bucket_->consume();
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
                
                case SourceType::API:
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
                // Preserve the correlation_id assigned at the start of this run;
                // ingest() returns a fresh IngestionStats that doesn't carry it.
                const std::string corr_id = stats.correlation_id;
                stats = connector->ingest(target_collection_, progress_callback);
                stats.correlation_id = corr_id;
                quarantineFailures(stats, source_id);
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
        // Reset bucket so it's rebuilt on next use
        token_bucket_.reset();
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
    
private:
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

    std::string db_connection_;
    std::string target_collection_;
    bool parallel_enabled_;
    bool dry_run_;
    size_t max_threads_;
    RetryConfig retry_config_;
    RateLimitConfig rate_limit_config_;
    std::unique_ptr<TokenBucket> token_bucket_;
    std::unordered_map<std::string, SourceConfig> sources_;
    std::vector<QuarantineEntry> quarantine_;
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

} // namespace ingestion
} // namespace themis

