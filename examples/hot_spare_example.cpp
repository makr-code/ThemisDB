/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hot_spare_example.cpp                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     236                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Hot Spare Management Example
 * 
 * Demonstrates hot spare configuration and usage for automatic failover
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "sharding/hot_spare_manager.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"

using namespace themisdb::sharding;

// Simulated storage backend
class SimpleStorage {
public:
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data;
    
    bool write(const std::string& shard_id, const std::string& doc_id, 
               const std::vector<uint8_t>& content) {
        data[shard_id][doc_id] = content;
        std::cout << "Written to shard " << shard_id << ": " << doc_id << std::endl;
        return true;
    }
    
    std::optional<std::vector<uint8_t>> read(const std::string& shard_id, 
                                              const std::string& doc_id) {
        if (data.count(shard_id) && data[shard_id].count(doc_id)) {
            return data[shard_id][doc_id];
        }
        return std::nullopt;
    }
    
    std::vector<std::string> listDocuments(const std::string& shard_id) {
        std::vector<std::string> docs = {};

        if (data.count(shard_id)) {
            for (const auto& [doc_id, _] : data[shard_id]) {
                docs.push_back(doc_id);
            }
        }
        return docs;
    }
};

int main() {
    std::cout << "=== Hot Spare Management Example ===" << std::endl << std::endl;
    
    // Step 1: Configure redundancy with hot spares
    std::cout << "[1] Configuring redundancy strategy..." << std::endl;
    RedundancyConfig redundancy_config;
    redundancy_config.mode = RedundancyMode::MIRROR;
    redundancy_config.replication_factor = 3;
    redundancy_config.hot_spare.enable = true;
    redundancy_config.hot_spare.spare_shards = {"spare-1", "spare-2", "spare-3"};
    
    RedundancyStrategy strategy(redundancy_config);
    ShardTopology topology;
    
    std::cout << "  ✓ Redundancy mode: MIRROR (replication factor: 3)" << std::endl;
    std::cout << "  ✓ Hot spares configured: 3" << std::endl << std::endl;
    
    // Step 2: Configure hot spare manager
    std::cout << "[2] Configuring hot spare manager..." << std::endl;
    HotSpareConfig spare_config;
    spare_config.enable = true;
    spare_config.spare_shards = {"spare-1", "spare-2", "spare-3"};
    spare_config.auto_rebuild = true;
    spare_config.rebuild_priority = RebuildPriority::HIGH;
    spare_config.rebuild_throttle_mbps = 100;
    spare_config.health_check_interval = std::chrono::seconds(30);
    spare_config.max_concurrent_rebuilds = 2;
    
    // Set up alert callback
    spare_config.enable_alerts = true;
    spare_config.alert_callback = [](const std::string& message) {
        std::cout << "  🔔 ALERT: " << message << std::endl;
    };
    
    HotSpareManager spare_manager(spare_config, strategy, topology);
    
    std::cout << "  ✓ Rebuild priority: HIGH" << std::endl;
    std::cout << "  ✓ Rebuild throttle: 100 MB/s" << std::endl;
    std::cout << "  ✓ Health check interval: 30s" << std::endl << std::endl;
    
    // Step 3: Start the manager
    std::cout << "[3] Starting hot spare manager..." << std::endl;
    spare_manager.start();
    std::cout << "  ✓ Manager started" << std::endl << std::endl;
    
    // Step 4: Set up storage and handlers
    SimpleStorage storage;
    ConsistentHashRing ring(100);
    
    // Add primary shards to ring
    ring.addNode("shard-1");
    ring.addNode("shard-2");
    ring.addNode("shard-3");
    
    // Write some test data
    std::cout << "[4] Writing test data..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::string doc_id = "doc-" + std::to_string(i);
        std::vector<uint8_t> data(100, 'A' + i);
        storage.write("shard-1", doc_id, data);
    }
    std::cout << "  ✓ Written 10 documents to shard-1" << std::endl << std::endl;
    
    // Step 5: Display spare status
    std::cout << "[5] Checking spare status..." << std::endl;
    auto available_spares = spare_manager.getAvailableSpares();
    std::cout << "  Available spares: " << available_spares.size() << std::endl;
    for (const auto& spare_id : available_spares) {
        std::cout << "    - " << spare_id << std::endl;
    }
    std::cout << std::endl;
    
    // Step 6: Simulate shard failure
    std::cout << "[6] Simulating shard-1 failure..." << std::endl;
    std::cout << "  ⚠️  Shard-1 has failed!" << std::endl;
    
    // Define handlers
    auto write_handler = [&storage](const std::string& shard_id, 
                                    const std::string& doc_id,
                                    const std::vector<uint8_t>& data) {
        return storage.write(shard_id, doc_id, data);
    };
    
    auto read_handler = [&storage](const std::string& shard_id, 
                                   const std::string& doc_id) {
        return storage.read(shard_id, doc_id);
    };
    
    auto doc_iterator = [&storage](const std::string& shard_id) {
        return storage.listDocuments(shard_id);
    };
    
    // Activate spare
    auto failover_start = std::chrono::steady_clock::now();
    bool success = spare_manager.activateSpare(
        "shard-1",
        ring,
        read_handler,
        write_handler,
        doc_iterator
    );
    auto failover_end = std::chrono::steady_clock::now();
    auto failover_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        failover_end - failover_start);
    
    if (success) {
        std::cout << "  ✓ Spare activated successfully!" << std::endl;
        std::cout << "  ✓ Failover time: " << failover_duration.count() << "ms" << std::endl;
    } else {
        std::cout << "  ✗ Failover failed!" << std::endl;
    }
    std::cout << std::endl;
    
    // Step 7: Monitor rebuild progress
    std::cout << "[7] Monitoring rebuild progress..." << std::endl;
    auto rebuild_status = spare_manager.getRebuildStatus();
    if (rebuild_status.is_rebuilding) {
        std::cout << "  Active rebuilds: " << rebuild_status.active_rebuilds << std::endl;
        std::cout << "  Overall progress: " << rebuild_status.overall_progress << "%" << std::endl;
        std::cout << "  Average throughput: " << rebuild_status.average_throughput_mbps 
                  << " MB/s" << std::endl;
    } else {
        std::cout << "  No active rebuilds" << std::endl;
    }
    std::cout << std::endl;
    
    // Step 8: Display statistics
    std::cout << "[8] Statistics:" << std::endl;
    auto stats = spare_manager.getStats();
    std::cout << "  Total failovers: " << stats.total_failovers << std::endl;
    std::cout << "  Successful failovers: " << stats.successful_failovers << std::endl;
    std::cout << "  Failed failovers: " << stats.failed_failovers << std::endl;
    std::cout << "  Average failover time: " << stats.avg_failover_time.count() << "ms" << std::endl;
    std::cout << "  Available spares: " << stats.spares_available << std::endl;
    std::cout << "  Active spares: " << stats.spares_active << std::endl;
    std::cout << "  Rebuilding spares: " << stats.spares_rebuilding << std::endl;
    std::cout << std::endl;
    
    // Step 9: Display failover history
    std::cout << "[9] Failover history:" << std::endl;
    auto history = spare_manager.getFailoverHistory(5);
    if (history.empty()) {
        std::cout << "  No failover events" << std::endl;
    } else {
        for (const auto& event : history) {
            std::cout << "  - " << event.failed_shard_id 
                      << " → " << event.spare_shard_id
                      << " (" << event.failover_duration.count() << "ms)" << std::endl;
        }
    }
    std::cout << std::endl;
    
    // Step 10: Export Prometheus metrics
    std::cout << "[10] Prometheus metrics:" << std::endl;
    std::string metrics = spare_manager.exportPrometheusMetrics();
    std::cout << metrics << std::endl;
    
    // Cleanup
    std::cout << "[11] Stopping hot spare manager..." << std::endl;
    spare_manager.stop();
    std::cout << "  ✓ Manager stopped" << std::endl;
    
    std::cout << std::endl << "=== Example Complete ===" << std::endl;
    
    return 0;
}
