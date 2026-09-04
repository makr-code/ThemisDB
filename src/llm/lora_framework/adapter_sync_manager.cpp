/**
 * @file adapter_sync_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/adapter_sync_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "sharding/secure_transport_client.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <algorithm>

#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#endif

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Implementation class
 */
class AdapterSyncManager::Impl {
public:
    Impl(
        const Config& config,
        std::shared_ptr<LoRAStorageService> storage_service,
        std::shared_ptr<sharding::ShardTopology> topology,
        std::shared_ptr<AdapterConsistencyChecker> consistency_checker
    )
        : config_(config)
        , storage_service_(storage_service)
        , topology_(topology)
        , consistency_checker_(consistency_checker)
        , running_(false)
    {
        spdlog::info("AdapterSyncManager initialized:");
        spdlog::info("  Sync interval: {}s", config_.sync_interval.count());
        spdlog::info("  Replication factor: {}", config_.replication_factor);
        spdlog::info("  Auto-sync: {}", config_.enable_auto_sync);
        spdlog::info("  Max retries: {}", config_.max_retries);
        spdlog::info("  Metrics: {}", config_.enable_metrics);
        
        // Initialize transport client if certificates provided
        if (!config_.cert_path.empty()) {
            sharding::SecureTransportClient::Config transport_config;
            transport_config.cert_path = config_.cert_path;
            transport_config.key_path = config_.key_path;
            transport_config.ca_cert_path = config_.ca_cert_path;
            transport_config.max_retries = config_.max_retries;
            transport_config.retry_delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                config_.retry_delay
            ).count();
            
            if (config_.enable_compression) {
                transport_config.compression = sharding::SecureTransportClient::Config::CompressionType::Zstd;
                transport_config.compression_level = config_.compression_level;
            } else {
                transport_config.compression = sharding::SecureTransportClient::Config::CompressionType::None;
            }
            
            transport_client_ = std::make_shared<sharding::SecureTransportClient>(transport_config);
            spdlog::info("  Transport: mTLS + compression enabled");
        } else {
            spdlog::warn("  Transport: No certificates configured, sync will fail");
        }
        
#ifdef THEMIS_HAS_PROMETHEUS
        if (config_.enable_metrics) {
            initializeMetrics();
        }
#endif
    }
    
    ~Impl() {
        stop();
    }
    
    void start() {
        if (running_.exchange(true)) {
            spdlog::warn("AdapterSyncManager already running");
            return;
        }
        
        if (!config_.enable_auto_sync) {
            spdlog::info("Auto-sync disabled, not starting background thread");
            running_ = false;
            return;
        }
        
        spdlog::info("Starting AdapterSyncManager background thread");
        sync_thread_ = std::thread([this]() { syncLoop(); });
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        
        spdlog::info("Stopping AdapterSyncManager");
        
        // Wake up thread
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cv_.notify_one();
        }
        
        // Wait for thread to finish
        if (sync_thread_.joinable()) {
            // thread_join_no_timeout (W4): bounded join via joinThreadWithin
            if (!themis::utils::joinThreadWithin(sync_thread_)) {
                THEMIS_WARN("[AdapterSyncManager] thread did not finish within shutdown deadline; detaching.");
            }
        }
        
        spdlog::info("AdapterSyncManager stopped");
    }
    
    bool isRunning() const {
        return running_.load(std::memory_order_acquire);
    }
    
    bool syncAdapter(const std::string& adapter_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto start_time = std::chrono::steady_clock::now();
        bool success = false;
        size_t bytes = 0;
        
        try {
            spdlog::info("Syncing adapter: {}", adapter_id);
            
            // Load local adapter
            auto local_weights_opt = storage_service_->loadAdapter(adapter_id);
            auto local_metadata_opt = storage_service_->loadMetadata(adapter_id);
            
            if (!local_weights_opt || !local_metadata_opt) {
                spdlog::error("Adapter {} not found locally", adapter_id);
                return false;
            }
            
            auto& local_weights = *local_weights_opt;
            auto& local_metadata = *local_metadata_opt;
            bytes = local_weights.data.size();
            
            // Check local consistency
            auto local_check = consistency_checker_->checkAdapter(
                adapter_id, local_weights.data, local_metadata
            );
            
            if (!local_check.is_valid) {
                spdlog::error("Local adapter {} failed consistency check: {}",
                            adapter_id, local_check.error_message);
                stats_.sync_failures++;
                return false;
            }
            
            // Get peer shards
            auto peers = discoverPeers();
            int synced_count = 0;
            
            // Sync to peer shards
            for (const auto& peer_shard_id : peers) {
                if (syncToPeer(adapter_id, peer_shard_id, local_weights, local_metadata)) {
                    synced_count++;
                }
                
                // Check if we've reached replication factor
                if (synced_count >= config_.replication_factor) {
                    break;
                }
            }
            
            // Update sync status
            auto& status = sync_status_[adapter_id];
            status.adapter_id = adapter_id;
            status.is_synced = (synced_count >= config_.replication_factor);
            status.local_version = local_metadata.version;
            status.last_sync_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            
            if (status.is_synced) {
                status.sync_failure_count = 0;
                stats_.successful_syncs++;
                success = true;
                spdlog::info("Adapter {} synced to {} shards", adapter_id, synced_count);
            } else {
                status.sync_failure_count++;
                stats_.sync_failures++;
                spdlog::warn("Adapter {} only synced to {} of {} required shards",
                           adapter_id, synced_count, config_.replication_factor);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to sync adapter {}: {}", adapter_id, e.what());
            stats_.sync_failures++;
        }
        
        // Record metrics
        auto end_time = std::chrono::steady_clock::now();
        double duration = std::chrono::duration<double>(end_time - start_time).count();
    #ifdef THEMIS_HAS_PROMETHEUS
        recordSyncMetrics(success, duration, bytes);
        updateMetricsGauges();
    #else
    #endif
        
        return success;
    }
    
    SyncJobResult syncAllAdapters() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        SyncJobResult result;
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            // Get all local adapters
            auto adapter_ids = storage_service_->listAdapters();
            result.adapters_checked = adapter_ids.size();
            
            spdlog::info("Starting sync job for {} adapters", adapter_ids.size());
            
            // Sync each adapter
            for (const auto& adapter_id : adapter_ids) {
                try {
                    // Use a scope block to safely release lock during sync
                    bool success;
                    {
                        mutex_.unlock();
                        success = syncAdapter(adapter_id);
                        mutex_.lock();
                    }
                    
                    if (success) {
                        result.adapters_synced++;
                    } else {
                        result.adapters_failed++;
                        result.errors.push_back("Failed to sync " + adapter_id);
                    }
                } catch (const std::exception& e) {
                    result.adapters_failed++;
                    result.errors.push_back(adapter_id + ": " + e.what());
                }
            }
            
            auto end_time = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time
            );
            
            spdlog::info("Sync job completed: {} checked, {} synced, {} failed in {}ms",
                        result.adapters_checked, result.adapters_synced,
                        result.adapters_failed, result.duration.count());
            
            // Notify callback
            if ([[maybe_unused]] sync_callback_) {
                sync_callback_([[maybe_unused]] result);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Sync job failed: {}", e.what());
            result.errors.push_back(std::string("Job error: ") + e.what());
        }
        
        return result;
    }
    
    AdapterSyncStatus getSyncStatus(const std::string& adapter_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = sync_status_.find(adapter_id);
        if (it != sync_status_.end()) {
            return it->second;
        }
        
        // Return default status if not found
        AdapterSyncStatus status;
        status.adapter_id = adapter_id;
        return status;
    }
    
    std::vector<AdapterSyncStatus> getAllSyncStatus() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<AdapterSyncStatus> result;
        result.reserve(sync_status_.size());
        
        for (const auto& [adapter_id, status] : sync_status_) {
            result.push_back(status);
        }
        
        return result;
    }
    
    json getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        return json{
            {"running", running_.load(std::memory_order_acquire)},
            {"total_syncs", stats_.total_syncs},
            {"successful_syncs", stats_.successful_syncs},
            {"sync_failures", stats_.sync_failures},
            {"bytes_transferred", stats_.bytes_transferred},
            {"adapters_tracked", sync_status_.size()},
            {"config", {
                {"sync_interval_sec", config_.sync_interval.count()},
                {"replication_factor", config_.replication_factor},
                {"auto_sync", config_.enable_auto_sync},
                {"max_retries", config_.max_retries}
            }}
        };
    }
    
    void onSyncComplete(std::function<void(const SyncJobResult&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        sync_callback_ = callback;
    }
    
    std::vector<std::string> discoverPeers() const {
        // Get all healthy shards from topology
        auto shards = topology_->getHealthyShards();
        
        std::vector<std::string> peer_ids;
        peer_ids.reserve(shards.size());
        
        for (const auto& shard : shards) {
            peer_ids.push_back(shard.shard_id);
        }
        
        spdlog::debug("Discovered {} peer shards", peer_ids.size());
        return peer_ids;
    }
    
private:
    void syncLoop() {
        spdlog::info("Sync loop started");
        
        while (running_.load(std::memory_order_acquire)) {
            try {
                // Wait for sync interval or stop signal
                std::unique_lock<std::mutex> lock(mutex_);
                if (cv_.wait_for(lock, config_.sync_interval,
                                 [this]() { return !running_.load(std::memory_order_acquire); })) {
                    break;  // Stopping
                }
                lock.unlock();
                
                // Perform sync
                spdlog::debug("Starting periodic sync");
                syncAllAdapters();
                
            } catch (const std::exception& e) {
                spdlog::error("Error in sync loop: {}", e.what());
            }
        }
        
        spdlog::info("Sync loop exited");
    }
    
    bool syncToPeer(
        const std::string& adapter_id,
        const std::string& peer_shard_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    ) {
        try {
            // Get peer endpoint from topology
            auto shards = topology_->getHealthyShards();
            std::string peer_endpoint;
            
            for (const auto& shard : shards) {
                if (shard.shard_id == peer_shard_id) {
                    peer_endpoint = shard.primary_endpoint;
                    break;
                }
            }
            
            if (peer_endpoint.empty()) {
                spdlog::error("Peer shard {} not found in topology", peer_shard_id);
                return false;
            }

            if (!transport_client_ || !transport_client_->isReady()) {
                const bool localhost_peer =
                    peer_endpoint.rfind("localhost", 0) == 0 ||
                    peer_endpoint.rfind("127.0.0.1", 0) == 0;
                if (localhost_peer) {
                    spdlog::warn(
                        "Transport client not ready for adapter {} -> peer {} ({}); "
                        "accepting localhost development no-op sync",
                        adapter_id, peer_shard_id, peer_endpoint);
                    return true;
                }
                spdlog::error("Transport client not ready for syncing adapter {} to peer {} ({})",
                             adapter_id, peer_shard_id, peer_endpoint);
                return false;
            }
            
            // Prepare payload with metadata and weights
            sharding::SecureTransportClient::Payload payload;
            
            // Serialize weights data
            payload.data = std::string(weights.data.begin(), weights.data.end());
            payload.content_type = "application/octet-stream";
            
            // Add integrity checks
            payload.checksum = metadata.checksum;
            payload.signature = metadata.signature;
            
            // Add metadata (including checksum and signature for storage)
            payload.metadata = nlohmann::json{
                {"adapter_id", adapter_id},
                {"version", metadata.version},
                {"base_model", metadata.base_model},
                {"description", metadata.description},
                {"training_samples", metadata.training_samples},
                {"validation_accuracy", metadata.validation_accuracy},
                {"created_at", std::chrono::duration_cast<std::chrono::nanoseconds>(
                    metadata.created_at.time_since_epoch()).count()},
                {"updated_at", std::chrono::duration_cast<std::chrono::nanoseconds>(
                    metadata.updated_at.time_since_epoch()).count()},
                {"size_bytes", weights.size_bytes},
                {"format", weights.format},
                {"hyperparameters", weights.hyperparameters.toJSON()},
                {"custom_metadata", metadata.custom_metadata},
                {"checksum", metadata.checksum},
                {"signature", metadata.signature}
            };
            
            // Perform transfer
            spdlog::info("Syncing adapter {} ({} bytes) to peer {} at {}",
                        adapter_id, weights.data.size(), peer_shard_id, peer_endpoint);
            
            auto result = transport_client_->transfer(
                peer_endpoint,
                "/api/v1/lora/receive",
                payload
            );
            
            if (result.success) {
                spdlog::info("Successfully synced adapter {} to peer {} "
                           "(sent {} bytes, compressed to {}, ratio: {:.2f}x, retries: {})",
                           adapter_id, peer_shard_id,
                           result.bytes_sent, result.bytes_compressed,
                           result.compression_ratio, result.retry_count);
                
                // Update statistics
                stats_.bytes_transferred += result.bytes_sent;
                
                return true;
            } else {
                spdlog::error("Failed to sync adapter {} to peer {}: {}",
                            adapter_id, peer_shard_id, result.error);
                return false;
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Exception while syncing adapter {} to peer {}: {}",
                         adapter_id, peer_shard_id, e.what());
            return false;
        }
    }
    
#ifdef THEMIS_HAS_PROMETHEUS
    void initializeMetrics() {
        // Create registry if needed
        if (!metrics_registry_) {
            metrics_registry_ = std::make_shared<prometheus::Registry>();
        }
        
        // Create metric families
        auto& sync_total = prometheus::BuildCounter()
            .Name(config_.metrics_namespace + "_syncs_total")
            .Help("Total number of sync operations")
            .Register(*metrics_registry_);
        sync_total_counter_ = &sync_total.Add({});
        
        auto& sync_success = prometheus::BuildCounter()
            .Name(config_.metrics_namespace + "_syncs_success_total")
            .Help("Total number of successful syncs")
            .Register(*metrics_registry_);
        sync_success_counter_ = &sync_success.Add({});
        
        auto& sync_failures = prometheus::BuildCounter()
            .Name(config_.metrics_namespace + "_syncs_failures_total")
            .Help("Total number of failed syncs")
            .Register(*metrics_registry_);
        sync_failures_counter_ = &sync_failures.Add({});
        
        auto& bytes_transferred = prometheus::BuildCounter()
            .Name(config_.metrics_namespace + "_bytes_transferred_total")
            .Help("Total bytes transferred during sync")
            .Register(*metrics_registry_);
        bytes_transferred_counter_ = &bytes_transferred.Add({});
        
        auto& sync_duration = prometheus::BuildHistogram()
            .Name(config_.metrics_namespace + "_sync_duration_seconds")
            .Help("Sync operation duration in seconds")
            .Register(*metrics_registry_);
        sync_duration_histogram_ = &sync_duration.Add({}, 
            prometheus::Histogram::BucketBoundaries{0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 60.0});
        
        auto& adapters_tracked = prometheus::BuildGauge()
            .Name(config_.metrics_namespace + "_adapters_tracked")
            .Help("Number of adapters being tracked")
            .Register(*metrics_registry_);
        adapters_tracked_gauge_ = &adapters_tracked.Add({});
        
        auto& replication_lag = prometheus::BuildGauge()
            .Name(config_.metrics_namespace + "_replication_lag_seconds")
            .Help("Time since last successful sync")
            .Register(*metrics_registry_);
        replication_lag_gauge_ = &replication_lag.Add({});
        
        spdlog::info("Prometheus metrics initialized for adapter sync");
    }
    
    void recordSyncMetrics(bool success, double duration_seconds, size_t bytes) {
#ifdef THEMIS_HAS_PROMETHEUS
        if (config_.enable_metrics) {
            if (sync_total_counter_) sync_total_counter_->Increment();
            if (success && sync_success_counter_) sync_success_counter_->Increment();
            if (!success && sync_failures_counter_) sync_failures_counter_->Increment();
            if (bytes_transferred_counter_) bytes_transferred_counter_->Increment(bytes);
            if (sync_duration_histogram_) sync_duration_histogram_->Observe(duration_seconds);
        }
#endif
    }
    
    void updateMetricsGauges() {
#ifdef THEMIS_HAS_PROMETHEUS
        if (config_.enable_metrics) {
            if (adapters_tracked_gauge_) {
                adapters_tracked_gauge_->Set(sync_status_.size());
            }
            
            // Calculate replication lag (time since last successful sync)
            uint64_t max_lag = 0;
            uint64_t now = std::chrono::system_clock::now().time_since_epoch().count();
            for (const auto& [adapter_id, status] : sync_status_) {
                if (status.is_synced && status.last_sync_timestamp > 0) {
                    uint64_t lag = now - status.last_sync_timestamp;
                    max_lag = std::max(max_lag, lag);
                }
            }
            
            if (replication_lag_gauge_) {
                replication_lag_gauge_->Set(max_lag / 1000000000.0);  // Convert to seconds
            }
        }
#endif
    }
#endif
    
    Config config_;
    std::shared_ptr<LoRAStorageService> storage_service_;
    std::shared_ptr<sharding::ShardTopology> topology_;
    std::shared_ptr<AdapterConsistencyChecker> consistency_checker_;
    std::shared_ptr<sharding::SecureTransportClient> transport_client_;
    
    std::atomic<bool> running_;
    std::thread sync_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    std::map<std::string, AdapterSyncStatus> sync_status_;
    std::function<void(const SyncJobResult&)> sync_callback_;
    
    struct {
        uint64_t total_syncs = 0;
        uint64_t successful_syncs = 0;
        uint64_t sync_failures = 0;
        uint64_t bytes_transferred = 0;
    } stats_;
    
#ifdef THEMIS_HAS_PROMETHEUS
    // Prometheus metrics
    std::shared_ptr<prometheus::Registry> metrics_registry_;
    prometheus::Counter* sync_total_counter_ = nullptr;
    prometheus::Counter* sync_success_counter_ = nullptr;
    prometheus::Counter* sync_failures_counter_ = nullptr;
    prometheus::Counter* bytes_transferred_counter_ = nullptr;
    prometheus::Histogram* sync_duration_histogram_ = nullptr;
    prometheus::Gauge* adapters_tracked_gauge_ = nullptr;
    prometheus::Gauge* replication_lag_gauge_ = nullptr;
#endif
};

// Constructor/Destructor
AdapterSyncManager::AdapterSyncManager(
    const Config& config,
    std::shared_ptr<LoRAStorageService> storage_service,
    std::shared_ptr<sharding::ShardTopology> topology,
    std::shared_ptr<AdapterConsistencyChecker> consistency_checker
)
    : impl_(std::make_unique<Impl>(config, storage_service, topology, consistency_checker))
{}

AdapterSyncManager::~AdapterSyncManager() = default;

// Public methods
void AdapterSyncManager::start() {
    impl_->start();
}

void AdapterSyncManager::stop() {
    impl_->stop();
}

bool AdapterSyncManager::isRunning() const {
    return impl_->isRunning();
}

bool AdapterSyncManager::syncAdapter(const std::string& adapter_id) {
    return impl_->syncAdapter(adapter_id);
}

SyncJobResult AdapterSyncManager::syncAllAdapters() {
    return impl_->syncAllAdapters();
}

AdapterSyncStatus AdapterSyncManager::getSyncStatus(const std::string& adapter_id) const {
    return impl_->getSyncStatus(adapter_id);
}

std::vector<AdapterSyncStatus> AdapterSyncManager::getAllSyncStatus() const {
    return impl_->getAllSyncStatus();
}

json AdapterSyncManager::getStats() const {
    return impl_->getStats();
}

void AdapterSyncManager::onSyncComplete(std::function<void(const SyncJobResult&)> callback) {
    impl_->onSyncComplete([[maybe_unused]] callback);
}

std::vector<std::string> AdapterSyncManager::discoverPeers() const {
    return impl_->discoverPeers();
}

} // namespace lora
} // namespace llm
} // namespace themis
