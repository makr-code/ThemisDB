/**
 * @file adapter_sync_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_config.h"
#include "adapter_consistency_checker.h"
#include "lora_metrics.h"
#include "sharding/shard_topology.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/secure_transport_client.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>

using json = nlohmann::json;

#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#endif

namespace themis {
namespace llm {
namespace lora {

// Forward declarations
class LoRAStorageService;

/**
 * @brief Sync status for an adapter
 */
struct AdapterSyncStatus {
    virtual ~AdapterSyncStatus() = default;
    std::string adapter_id;
    bool is_synced = false;
    std::string local_version;               // Version string
    std::string remote_version;              // Version string
    uint64_t last_sync_timestamp = 0;
    int sync_failure_count = 0;
    std::string last_error;
    std::vector<std::string> synced_shards;  // List of shards in sync
    std::vector<std::string> pending_shards; // List of shards pending sync
};

/**
 * @brief Sync job result
 */
struct SyncJobResult {
    virtual ~SyncJobResult() = default;
    int adapters_checked = 0;
    int adapters_synced = 0;
    int adapters_failed = 0;
    int total_bytes_transferred = 0;
    std::chrono::milliseconds duration{0};
    std::vector<std::string> errors;
};

/**
 * @brief Adapter Sync Manager
 * 
 * Manages automatic synchronization of LoRA adapters across shards:
 * - Periodic sync with configurable interval
 * - Peer detection and health checking
 * - Retry logic with exponential backoff
 * - Resume interrupted syncs
 * - Conflict resolution
 * - Multi-LLM support
 */
class AdapterSyncManager {
public:
    /**
     * @brief Configuration for sync manager
     */
    struct Config {
        // Sync interval
        std::chrono::seconds sync_interval{300};  // 5 minutes default
        
        // Replication settings
        int replication_factor = 3;               // Number of replicas
        bool enable_auto_sync = true;             // Auto-sync on interval
        bool enable_on_write_sync = false;        // Sync immediately after write
        
        // Retry settings
        int max_retries = 3;
        std::chrono::seconds retry_delay{10};
        bool enable_exponential_backoff = true;
        
        // Conflict resolution
        std::string conflict_resolution = "newest_wins";  // "newest_wins", "manual"
        
        // Performance
        int max_concurrent_syncs = 4;
        int max_transfer_rate_mbps = 100;  // Rate limit
        
        // Multi-LLM support
        bool enable_multi_llm = false;
        std::vector<std::string> llm_models;  // Filter by LLM models
        
        // Metrics
        bool enable_metrics = true;               // Enable Prometheus metrics
        std::string metrics_namespace = "themis_lora_sync";
        
        // Transport settings (mTLS, compression, retry)
        std::string cert_path;                    // mTLS certificate path
        std::string key_path;                     // mTLS key path
        std::string ca_cert_path;                 // mTLS CA certificate path
        bool enable_compression = true;           // Enable compression for transfer
        int compression_level = 3;                // Compression level (1-22 for Zstd)
    };
    
    /**
     * @brief Construct sync manager
     * @param config Configuration
     * @param storage_service LoRA storage service
     * @param topology Shard topology
     * @param consistency_checker Consistency checker
     */
    AdapterSyncManager(
        const Config& config,
        std::shared_ptr<LoRAStorageService> storage_service,
        std::shared_ptr<sharding::ShardTopology> topology,
        std::shared_ptr<AdapterConsistencyChecker> consistency_checker
    );
    
    ~AdapterSyncManager();
    
    // Disable copy
    AdapterSyncManager(const AdapterSyncManager&) = delete;
    AdapterSyncManager& operator=(const AdapterSyncManager&) = delete;
    
    /**
     * @brief Start automatic synchronization
     */
    void start();
    
    /**
     * @brief Stop automatic synchronization
     */
    void stop();
    
    /**
     * @brief Check if sync is running
     */
    bool isRunning() const;
    
    /**
     * @brief Trigger manual sync for specific adapter
     * @param adapter_id Adapter identifier
     * @return true if sync initiated successfully
     */
    bool syncAdapter(const std::string& adapter_id);
    
    /**
     * @brief Trigger manual sync for all adapters
     * @return Sync job result
     */
    SyncJobResult syncAllAdapters();
    
    /**
     * @brief Get sync status for adapter
     * @param adapter_id Adapter identifier
     * @return Sync status
     */
    AdapterSyncStatus getSyncStatus(const std::string& adapter_id) const;
    
    /**
     * @brief Get sync status for all adapters
     * @return Vector of sync statuses
     */
    std::vector<AdapterSyncStatus> getAllSyncStatus() const;
    
    /**
     * @brief Get sync statistics
     * @return JSON with statistics
     */
    json getStats() const;
    
    /**
     * @brief Register callback for sync events
     * @param callback Function called on sync completion
     */
    void onSyncComplete(std::function<void(const SyncJobResult&)> callback);
    
    /**
     * @brief Discover peer shards
     * @return List of peer shard IDs
     */
    std::vector<std::string> discoverPeers() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
