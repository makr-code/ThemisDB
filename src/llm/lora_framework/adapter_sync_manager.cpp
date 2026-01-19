#include "llm/lora_framework/adapter_sync_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <algorithm>

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
            sync_thread_.join();
        }
        
        spdlog::info("AdapterSyncManager stopped");
    }
    
    bool isRunning() const {
        return running_.load();
    }
    
    bool syncAdapter(const std::string& adapter_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
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
            status.last_sync_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            
            if (status.is_synced) {
                status.sync_failure_count = 0;
                stats_.successful_syncs++;
                spdlog::info("Adapter {} synced to {} shards", adapter_id, synced_count);
            } else {
                status.sync_failure_count++;
                stats_.sync_failures++;
                spdlog::warn("Adapter {} only synced to {} of {} required shards",
                           adapter_id, synced_count, config_.replication_factor);
            }
            
            return status.is_synced;
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to sync adapter {}: {}", adapter_id, e.what());
            stats_.sync_failures++;
            return false;
        }
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
                    // Release lock during sync to allow concurrent operations
                    mutex_.unlock();
                    bool success = syncAdapter(adapter_id);
                    mutex_.lock();
                    
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
            if (sync_callback_) {
                sync_callback_(result);
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
            {"running", running_.load()},
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
        
        while (running_.load()) {
            try {
                // Wait for sync interval or stop signal
                std::unique_lock<std::mutex> lock(mutex_);
                if (cv_.wait_for(lock, config_.sync_interval,
                                 [this]() { return !running_.load(); })) {
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
        // In a real implementation, this would:
        // 1. Serialize adapter data
        // 2. Send via RPC to peer shard
        // 3. Wait for acknowledgment
        // 4. Handle retries
        
        // For now, just log the operation
        spdlog::debug("Syncing adapter {} to peer {}", adapter_id, peer_shard_id);
        
        // Simulate success (in production, use actual RPC)
        stats_.bytes_transferred += weights.data.size();
        
        return true;  // Assume success for now
    }
    
    Config config_;
    std::shared_ptr<LoRAStorageService> storage_service_;
    std::shared_ptr<sharding::ShardTopology> topology_;
    std::shared_ptr<AdapterConsistencyChecker> consistency_checker_;
    
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
    impl_->onSyncComplete(callback);
}

std::vector<std::string> AdapterSyncManager::discoverPeers() const {
    return impl_->discoverPeers();
}

} // namespace lora
} // namespace llm
} // namespace themis
