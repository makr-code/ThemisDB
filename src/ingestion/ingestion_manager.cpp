#include "ingestion/ingestion_manager.h"
#include "ingestion/huggingface_connector.h"
#include "ingestion/filesystem_ingester.h"
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>

namespace themis {
namespace ingestion {

// Pimpl implementation
class IngestionManager::Impl {
public:
    explicit Impl(const std::string& db_connection) 
        : db_connection_(db_connection)
        , target_collection_("legal_documents")
        , parallel_enabled_(false)
        , max_threads_(std::thread::hardware_concurrency()) {
    }
    
    ~Impl() = default;
    
    bool registerSource(const SourceConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if source already registered
        if (sources_.find(config.source_id) != sources_.end()) {
            return false; // Already exists
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
            
            // Invoke ingestion
            stats = connector->ingest(target_collection_, progress_callback);
            
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
            // Parallel ingestion: launch one future per source, bounded by max_threads_
            const size_t concurrency =
                std::min(max_threads_, enabled_sources.size());

            std::vector<std::future<std::pair<std::string, IngestionStats>>> futures;
            futures.reserve(enabled_sources.size());

            // Semaphore-like throttle: submit in waves of `concurrency`
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
                // Collect wave results before starting next wave
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
            // Sequential ingestion
            for (const auto& config : enabled_sources) {
                auto stats = ingestSource(config.source_id, progress_callback);
                report.source_stats[config.source_id] = stats;
                report.total_documents += stats.documents_processed;
                report.total_failures  += stats.documents_failed;
                report.total_time_seconds += stats.elapsed_seconds;
            }
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
    
private:
    std::string db_connection_;
    std::string target_collection_;
    bool parallel_enabled_;
    size_t max_threads_;
    RetryConfig retry_config_;
    std::unordered_map<std::string, SourceConfig> sources_;
    mutable std::mutex mutex_;
};

// Public API implementation
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

} // namespace ingestion
} // namespace themis

