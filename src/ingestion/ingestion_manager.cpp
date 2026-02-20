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

namespace themis {
namespace ingestion {

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
                // Dry-run: count documents only, no actual insertion
                stats.documents_processed = connector->getDocumentCount();
                stats.documents_failed    = 0;
            } else {
                // Real ingestion
                stats = connector->ingest(target_collection_, progress_callback);
                // Quarantine items that failed after all retries
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
        
        // Get all enabled sources sorted by priority
        std::vector<SourceConfig> enabled_sources;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& pair : sources_) {
                if (pair.second.enabled) {
                    enabled_sources.push_back(pair.second);
                }
            }
        }
        
        // Sort by priority (higher first)
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

        // Attach current quarantine snapshot to the report
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
    /// Move FATAL errors into the persistent quarantine list
    void quarantineFailures(const IngestionStats& stats,
                            const std::string& source_id) {
        for (const auto& err : stats.errors) {
            if (err.isFatal()) {
                QuarantineEntry entry;
                entry.source_id    = source_id;
                // Use err.details as item path when available (usually contains the
                // specific file path / URL that failed). Fall back to a descriptive
                // placeholder rather than the bare source_id to avoid ambiguity when
                // multiple items from the same source fail with no details.
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
/// Escape label value for Prometheus exposition format
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

static void writeMetric(std::ostream& os,
                         const std::string& name,
                         const std::string& label_key,
                         const std::string& label_val,
                         double value) {
    os << name << '{' << label_key << "=\""
       << promEscapeLabel(label_val) << "\"} " << value << '\n';
}
} // anonymous namespace

std::string IngestionMetricsExporter::exportText(
        const IngestionReport& report) const {
    std::ostringstream os;

    // Per-source metrics
    for (const auto& [sid, stats] : report.source_stats) {
        os << exportText(stats, sid);
    }

    // Aggregate metrics
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
        const std::string& source_id) const {
    std::ostringstream os;
    const std::string lk = "source_id";

    os << "# HELP " << prefix_ << "_docs_processed_total "
       << "Documents successfully processed\n"
       << "# TYPE " << prefix_ << "_docs_processed_total counter\n";
    writeMetric(os, prefix_ + "_docs_processed_total",
                lk, source_id,
                static_cast<double>(stats.documents_processed));

    os << "# HELP " << prefix_ << "_docs_failed_total "
       << "Documents that failed to ingest\n"
       << "# TYPE " << prefix_ << "_docs_failed_total counter\n";
    writeMetric(os, prefix_ + "_docs_failed_total",
                lk, source_id,
                static_cast<double>(stats.documents_failed));

    os << "# HELP " << prefix_ << "_bytes_processed_total "
       << "Bytes processed\n"
       << "# TYPE " << prefix_ << "_bytes_processed_total counter\n";
    writeMetric(os, prefix_ + "_bytes_processed_total",
                lk, source_id,
                static_cast<double>(stats.bytes_processed));

    os << "# HELP " << prefix_ << "_elapsed_seconds "
       << "Elapsed ingestion time in seconds\n"
       << "# TYPE " << prefix_ << "_elapsed_seconds gauge\n";
    writeMetric(os, prefix_ + "_elapsed_seconds",
                lk, source_id,
                stats.elapsed_seconds);

    os << "# HELP " << prefix_ << "_retry_total "
       << "Total retried requests\n"
       << "# TYPE " << prefix_ << "_retry_total counter\n";
    writeMetric(os, prefix_ + "_retry_total",
                lk, source_id,
                static_cast<double>(stats.metrics.retry_count));

    os << "# HELP " << prefix_ << "_errors_total "
       << "Total errors encountered\n"
       << "# TYPE " << prefix_ << "_errors_total counter\n";
    writeMetric(os, prefix_ + "_errors_total",
                lk, source_id,
                static_cast<double>(stats.metrics.error_count));

    os << "# HELP " << prefix_ << "_throughput_docs_per_sec "
       << "Document throughput (docs/second)\n"
       << "# TYPE " << prefix_ << "_throughput_docs_per_sec gauge\n";
    writeMetric(os, prefix_ + "_throughput_docs_per_sec",
                lk, source_id,
                stats.metrics.throughput_docs_per_sec);

    return os.str();
}

} // namespace ingestion
} // namespace themis
