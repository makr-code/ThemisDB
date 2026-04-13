/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_lora_sync.cpp                              ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     238                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file example_lora_sync.cpp
 * @brief Example usage of automatic cross-shard LoRA synchronization
 * 
 * This example demonstrates:
 * - Setting up AdapterSyncManager
 * - Configuring replication and sync intervals
 * - Manual and automatic synchronization
 * - Monitoring sync status and metrics
 */

#include "llm/lora_framework/adapter_consistency_checker.h"
#include "llm/lora_framework/adapter_sync_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "sharding/shard_topology.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::llm::lora;
using namespace themis::sharding;

int main() {
    std::cout << "=== ThemisDB LoRA Cross-Shard Synchronization Example ===" << std::endl;
    
    // ========================================================================
    // 1. Create LoRA Storage Service
    // ========================================================================
    std::cout << "\n1. Creating LoRA Storage Service..." << std::endl;
    
    LoRAStorageService::Config storage_config;
    storage_config.backend = LoRAStorageService::Backend::FileSystem;
    storage_config.filesystem_path = "/tmp/example_lora_sync";
    storage_config.enable_versioning = true;
    storage_config.enable_compression = true;
    
    auto storage_service = std::make_shared<LoRAStorageService>(storage_config);
    std::cout << "✓ Storage service created" << std::endl;
    
    // ========================================================================
    // 2. Create Shard Topology
    // ========================================================================
    std::cout << "\n2. Setting up Shard Topology..." << std::endl;
    
    ShardTopology::Config topo_config;
    topo_config.cluster_name = "example_cluster";
    auto topology = std::make_shared<ShardTopology>(topo_config);
    
    // Add example shards
    for (int i = 1; i <= 5; i++) {
        ShardInfo shard;
        shard.shard_id = "shard_" + std::to_string(i);
        shard.primary_endpoint = "localhost:808" + std::to_string(i);
        shard.datacenter = (i <= 3) ? "dc1" : "dc2";
        shard.is_healthy = true;
        topology->addShard(shard);
        std::cout << "  Added " << shard.shard_id << " (" << shard.datacenter << ")" << std::endl;
    }
    
    std::cout << "✓ Topology configured with " << topology->getShardCount() << " shards" << std::endl;
    
    // ========================================================================
    // 3. Create Consistency Checker
    // ========================================================================
    std::cout << "\n3. Creating Consistency Checker..." << std::endl;
    
    AdapterConsistencyChecker::Config checker_config;
    checker_config.enable_checksums = true;
    checker_config.enable_signatures = true;
    checker_config.strict_mode = false;
    
    auto consistency_checker = std::make_shared<AdapterConsistencyChecker>(checker_config);
    std::cout << "✓ Consistency checker created" << std::endl;
    
    // ========================================================================
    // 4. Create Sync Manager
    // ========================================================================
    std::cout << "\n4. Creating Sync Manager..." << std::endl;
    
    AdapterSyncManager::Config sync_config;
    sync_config.sync_interval = std::chrono::seconds(60);    // 1 minute for demo
    sync_config.replication_factor = 3;                      // 3-way replication
    sync_config.enable_auto_sync = true;                     // Auto-sync enabled
    sync_config.enable_on_write_sync = false;
    sync_config.max_retries = 3;
    sync_config.retry_delay = std::chrono::seconds(5);
    sync_config.enable_exponential_backoff = true;
    sync_config.conflict_resolution = "newest_wins";
    sync_config.max_concurrent_syncs = 4;
    sync_config.enable_metrics = true;
    
    auto sync_manager = std::make_unique<AdapterSyncManager>(
        sync_config, storage_service, topology, consistency_checker
    );
    
    std::cout << "✓ Sync manager created" << std::endl;
    std::cout << "  Sync interval: " << sync_config.sync_interval.count() << "s" << std::endl;
    std::cout << "  Replication factor: " << sync_config.replication_factor << std::endl;
    
    // ========================================================================
    // 5. Register Sync Callback
    // ========================================================================
    std::cout << "\n5. Registering sync callback..." << std::endl;
    
    sync_manager->onSyncComplete([](const SyncJobResult& result) {
        std::cout << "\n>>> Sync job completed:" << std::endl;
        std::cout << "    Checked: " << result.adapters_checked << std::endl;
        std::cout << "    Synced: " << result.adapters_synced << std::endl;
        std::cout << "    Failed: " << result.adapters_failed << std::endl;
        std::cout << "    Duration: " << result.duration.count() << "ms" << std::endl;
        
        if (!result.errors.empty()) {
            std::cout << "    Errors:" << std::endl;
            for (const auto& error : result.errors) {
                std::cout << "      - " << error << std::endl;
            }
        }
    });
    
    std::cout << "✓ Callback registered" << std::endl;
    
    // ========================================================================
    // 6. Create Example Adapters
    // ========================================================================
    std::cout << "\n6. Creating example adapters..." << std::endl;
    
    for (int i = 1; i <= 3; i++) {
        std::string adapter_id = "adapter_" + std::to_string(i);
        
        // Create adapter weights
        AdapterWeights weights;
        weights.data = std::vector<uint8_t>(1024, i);  // 1KB of data
        weights.size_bytes = weights.data.size();
        weights.format = "safetensors";
        
        // Create metadata
        AdapterMetadata metadata;
        metadata.adapter_id = adapter_id;
        metadata.base_model = "llama-2-7b";
        metadata.version = "1.0.0";
        metadata.created_at = std::chrono::system_clock::now();
        metadata.updated_at = metadata.created_at;
        metadata.checksum = consistency_checker->calculateChecksum(weights.data);
        metadata.signature = consistency_checker->generateSignature(weights.data);
        
        // Save adapter
        if (storage_service->saveAdapter(adapter_id, weights, metadata)) {
            std::cout << "  ✓ Created " << adapter_id << std::endl;
        } else {
            std::cout << "  ✗ Failed to create " << adapter_id << std::endl;
        }
    }
    
    // ========================================================================
    // 7. Manual Sync
    // ========================================================================
    std::cout << "\n7. Performing manual sync..." << std::endl;
    
    auto result = sync_manager->syncAllAdapters();
    std::cout << "✓ Manual sync completed" << std::endl;
    std::cout << "  Checked: " << result.adapters_checked << std::endl;
    std::cout << "  Synced: " << result.adapters_synced << std::endl;
    std::cout << "  Failed: " << result.adapters_failed << std::endl;
    
    // ========================================================================
    // 8. Check Sync Status
    // ========================================================================
    std::cout << "\n8. Checking sync status..." << std::endl;
    
    auto all_status = sync_manager->getAllSyncStatus();
    for (const auto& status : all_status) {
        std::cout << "  " << status.adapter_id << ":" << std::endl;
        std::cout << "    Synced: " << (status.is_synced ? "yes" : "no") << std::endl;
        std::cout << "    Version: " << status.local_version << std::endl;
        std::cout << "    Failures: " << status.sync_failure_count << std::endl;
    }
    
    // ========================================================================
    // 9. View Statistics
    // ========================================================================
    std::cout << "\n9. Viewing statistics..." << std::endl;
    
    auto stats = sync_manager->getStats();
    std::cout << "  Total syncs: " << stats["total_syncs"] << std::endl;
    std::cout << "  Successful: " << stats["successful_syncs"] << std::endl;
    std::cout << "  Failures: " << stats["sync_failures"] << std::endl;
    std::cout << "  Bytes transferred: " << stats["bytes_transferred"] << std::endl;
    std::cout << "  Adapters tracked: " << stats["adapters_tracked"] << std::endl;
    
    // ========================================================================
    // 10. Start Auto-Sync (Optional)
    // ========================================================================
    std::cout << "\n10. Starting automatic sync..." << std::endl;
    sync_manager->start();
    
    if (sync_manager->isRunning()) {
        std::cout << "✓ Auto-sync is running" << std::endl;
        std::cout << "  Will sync every " << sync_config.sync_interval.count() << " seconds" << std::endl;
        
        // Wait a bit to see auto-sync in action (optional)
        std::cout << "\nWaiting 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    // ========================================================================
    // 11. Cleanup
    // ========================================================================
    std::cout << "\n11. Cleaning up..." << std::endl;
    sync_manager->stop();
    std::cout << "✓ Sync manager stopped" << std::endl;
    
    std::cout << "\n=== Example completed successfully ===" << std::endl;
    return 0;
}
