/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_sharding_example.cpp                   ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     172                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * Example: Distributed Sharding with Consensus and Cross-Shard Transactions
 * 
 * This example demonstrates:
 * - Setting up a consensus module (Raft)
 * - Initializing cross-shard transaction coordinator
 * - Executing distributed transactions
 * - Handling failures and monitoring
 */

#include "sharding/consensus_factory.h"
#include "sharding/cross_shard_transaction.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace themisdb::sharding;

// Helper function to generate transaction ID
std::string generateTransactionId() {
    static std::atomic<uint64_t> counter{1};
    return "txn_" + std::to_string(counter++);
}

// Example 1: Basic consensus setup and leader election
void example1_consensus_setup() {
    std::cout << "\n=== Example 1: Consensus Setup ===" << std::endl;
    
    // Configure Raft consensus
    ConsensusConfig config;
    config.type = ConsensusType::RAFT;
    config.node_id = "node1";
    config.cluster_nodes = {"node1", "node2", "node3"};
    config.heartbeat_interval = std::chrono::milliseconds(500);
    config.election_timeout_min = std::chrono::milliseconds(1000);
    config.election_timeout_max = std::chrono::milliseconds(2000);
    
    // Create consensus module
    auto consensus = ConsensusFactory::create(config);
    if (!consensus) {
        std::cerr << "Failed to create consensus module" << std::endl;
        return;
    }
    
    // Initialize and start
    if (!consensus->initialize(config.node_id, config.cluster_nodes)) {
        std::cerr << "Failed to initialize consensus" << std::endl;
        return;
    }
    
    if (!consensus->start()) {
        std::cerr << "Failed to start consensus" << std::endl;
        return;
    }
    
    // Wait for leader election
    std::cout << "Waiting for leader election..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check status
    std::cout << "Is leader: " << (consensus->isLeader() ? "yes" : "no") << std::endl;
    std::cout << "Leader ID: " << consensus->getLeaderId() << std::endl;
    std::cout << "Current state: " << static_cast<int>(consensus->getState()) << std::endl;
    
    // Get statistics
    auto stats = consensus->getStats();
    std::cout << "Cluster size: " << stats.cluster_size << std::endl;
    std::cout << "Reachable nodes: " << stats.reachable_nodes << std::endl;
    std::cout << "Current term: " << stats.current_term << std::endl;
    
    consensus->stop();
    std::cout << "Consensus stopped" << std::endl;
}

// Example 2: Two-Phase Commit transaction
void example2_two_phase_commit() {
    std::cout << "\n=== Example 2: Two-Phase Commit ===" << std::endl;
    
    // Setup consensus
    ConsensusConfig consensus_config;
    consensus_config.type = ConsensusType::RAFT;
    consensus_config.node_id = "node1";
    consensus_config.cluster_nodes = {"node1", "node2", "node3"};
    
    auto consensus = ConsensusFactory::create(consensus_config);
    consensus->initialize(consensus_config.node_id, consensus_config.cluster_nodes);
    consensus->start();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Setup transaction coordinator
    CrossShardTransactionConfig txn_config;
    txn_config.default_protocol = TransactionProtocol::TWO_PHASE_COMMIT;
    txn_config.default_isolation = IsolationLevel::SNAPSHOT_ISOLATION;
    
    auto coordinator = std::make_shared<CrossShardTransactionCoordinator>(
        txn_config, consensus
    );
    coordinator->initialize();
    coordinator->start();
    
    // Begin transaction
    std::string txn_id = generateTransactionId();
    std::cout << "Beginning transaction: " << txn_id << std::endl;
    
    coordinator->beginTransaction(txn_id);
    
    // Add shard participants
    coordinator->addParticipant(txn_id, "shard1", "node1:8080", 
        {"PUT /users/alice balance=1000"});
    coordinator->addParticipant(txn_id, "shard2", "node2:8080", 
        {"PUT /accounts/acc123 user=alice"});
    
    std::cout << "Added 2 participants to transaction" << std::endl;
    
    // Execute 2PC
    if (coordinator->prepare(txn_id)) {
        if (coordinator->commit(txn_id)) {
            std::cout << "Transaction committed successfully!" << std::endl;
        }
    }
    
    coordinator->stop();
    consensus->stop();
}

// Main function
int main(int argc, char** argv) {
    std::cout << "==================================================" << std::endl;
    std::cout << "ThemisDB v1.4 Distributed Sharding Examples" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    try {
        example1_consensus_setup();
        example2_two_phase_commit();
        
        std::cout << "\n==================================================" << std::endl;
        std::cout << "Examples completed!" << std::endl;
        std::cout << "==================================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
