#include "ingestion/ingestion_manager.h"
#include "ingestion/huggingface_connector.h"
#include "ingestion/filesystem_ingester.h"
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
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
                stats.error_message = "Source not found: " + source_id;
                return stats;
            }
            config = it->second;
        }
        
        if (!config.enabled) {
            stats.error_message = "Source disabled: " + source_id;
            return stats;
        }
        
        // Create connector based on type
        std::unique_ptr<ISourceConnector> connector;
        
        try {
            switch (config.type) {
                case SourceType::HUGGINGFACE: {
                    auto hf_connector = std::make_unique<HuggingFaceConnector>();
                    if (!hf_connector->initialize(config)) {
                        stats.error_message = "Failed to initialize HuggingFace connector";
                        return stats;
                    }
                    connector = std::move(hf_connector);
                    break;
                }
                
                case SourceType::FILESYSTEM: {
                    auto fs_ingester = std::make_unique<FileSystemIngester>();
                    if (!fs_ingester->initialize(config)) {
                        stats.error_message = "Failed to initialize filesystem ingester";
                        return stats;
                    }
                    connector = std::move(fs_ingester);
                    break;
                }
                
                case SourceType::API:
                case SourceType::DATABASE:
                default:
                    stats.error_message = "Connector type not yet implemented: " + 
                                        std::to_string(static_cast<int>(config.type));
                    return stats;
            }
            
            // Check availability
            if (!connector->isAvailable()) {
                stats.error_message = "Source not available: " + source_id;
                return stats;
            }
            
            // Invoke ingestion
            stats = connector->ingest(target_collection_, progress_callback);
            
            auto end_time = std::chrono::steady_clock::now();
            stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
            
        } catch (const std::exception& e) {
            stats.error_message = "Exception during ingestion: " + std::string(e.what());
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
        
        // Ingest each source
        for (const auto& config : enabled_sources) {
            auto stats = ingestSource(config.source_id, progress_callback);
            report.source_stats[config.source_id] = stats;
            report.total_documents += stats.documents_processed;
            report.total_failures += stats.documents_failed;
            report.total_time_seconds += stats.elapsed_seconds;
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
    
private:
    std::string db_connection_;
    std::string target_collection_;
    bool parallel_enabled_;
    size_t max_threads_;
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

} // namespace ingestion
} // namespace themis
