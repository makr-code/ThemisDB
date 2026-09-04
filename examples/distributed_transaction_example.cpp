/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_transaction_example.cpp                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     501                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Example: Using Distributed Transaction Coordinator with 2PC
//
// This example demonstrates how to use the distributed transaction coordinator
// to perform atomic operations across multiple shards.

#include "sharding/distributed_transaction.h"
#include "sharding/two_phase_commit_participant.h"
#include "sharding/truetime.h"
#include "utils/logger.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <map>
#include <string>
#include <vector>

using namespace themis::sharding;

// Example 1: Simple two-shard transaction
void example_simple_transaction() {
    std::cout << "\n=== Example 1: Simple Two-Shard Transaction ===\n";
    
    // Initialize TrueTime and coordinator
    auto truetime = std::make_shared<TrueTime>();
    
    DistributedTransactionCoordinator::Config config;
    config.prepare_timeout_ms = 10000;  // 10 seconds
    config.commit_timeout_ms = 10000;
    
    auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
        truetime, config
    );
    
    // Begin transaction across two shards
    std::vector<std::string> shard_ids = {"shard1", "shard2"};
    std::string txn_id = coordinator->beginTransaction(shard_ids);
    
    std::cout << "Started transaction: " << txn_id << "\n";
    
    // Add operations to shard1 (e.g., debit account)
    nlohmann::json debit_op = {
        {"type", "update"},
        {"table", "accounts"},
        {"key", "account:alice"},
        {"field", "balance"},
        {"operation", "subtract"},
        {"amount", 100.00}
    };
    coordinator->addOperation(txn_id, "shard1", debit_op);
    
    // Add operations to shard2 (e.g., credit account)
    nlohmann::json credit_op = {
        {"type", "update"},
        {"table", "accounts"},
        {"key", "account:bob"},
        {"field", "balance"},
        {"operation", "add"},
        {"amount", 100.00}
    };
    coordinator->addOperation(txn_id, "shard2", credit_op);
    
    // Commit with 2PC - either both operations succeed or both fail
    std::cout << "Committing transaction...\n";
    bool success = coordinator->commit(txn_id);
    
    if (success) {
        std::cout << "✓ Transaction committed successfully!\n";
        std::cout << "  $100 transferred from Alice to Bob atomically\n";
    } else {
        std::cout << "✗ Transaction aborted - no changes applied\n";
    }
}

// Example 2: Multi-shard e-commerce order
void example_ecommerce_order() {
    std::cout << "\n=== Example 2: Multi-Shard E-commerce Order ===\n";
    
    auto truetime = std::make_shared<TrueTime>();
    DistributedTransactionCoordinator::Config config;
    auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
        truetime, config
    );
    
    // Order involves 3 shards: inventory, orders, payments
    std::vector<std::string> shard_ids = {"inventory_shard", "orders_shard", "payments_shard"};
    std::string txn_id = coordinator->beginTransaction(shard_ids);
    
    std::cout << "Processing order transaction: " << txn_id << "\n";
    
    // 1. Reserve inventory
    nlohmann::json inventory_op = {
        {"type", "update"},
        {"table", "inventory"},
        {"product_id", "product:12345"},
        {"operation", "decrement"},
        {"quantity", 1}
    };
    coordinator->addOperation(txn_id, "inventory_shard", inventory_op);
    
    // 2. Create order record
    nlohmann::json order_op = {
        {"type", "insert"},
        {"table", "orders"},
        {"order_id", "order:98765"},
        {"customer_id", "customer:555"},
        {"product_id", "product:12345"},
        {"quantity", 1},
        {"amount", 299.99}
    };
    coordinator->addOperation(txn_id, "orders_shard", order_op);
    
    // 3. Process payment
    nlohmann::json payment_op = {
        {"type", "insert"},
        {"table", "payments"},
        {"payment_id", "payment:777"},
        {"order_id", "order:98765"},
        {"amount", 299.99},
        {"status", "completed"}
    };
    coordinator->addOperation(txn_id, "payments_shard", payment_op);
    
    // Commit - all 3 operations succeed or all fail
    std::cout << "Committing order...\n";
    bool success = coordinator->commit(txn_id);
    
    if (success) {
        std::cout << "✓ Order processed successfully!\n";
        std::cout << "  - Inventory reserved\n";
        std::cout << "  - Order created\n";
        std::cout << "  - Payment processed\n";
    } else {
        std::cout << "✗ Order failed - inventory/order/payment rolled back\n";
    }
}

// Example 3: Read-only transaction (optimized, no 2PC)
void example_readonly_transaction() {
    std::cout << "\n=== Example 3: Read-Only Transaction (Optimized) ===\n";
    
    auto truetime = std::make_shared<TrueTime>();
    DistributedTransactionCoordinator::Config config;
    auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
        truetime, config
    );
    
    // Read from multiple shards at consistent snapshot
    std::vector<std::string> shard_ids = {"user_shard", "profile_shard", "activity_shard"};
    
    nlohmann::json read_operations = {
        {"queries", {
            {{"shard", "user_shard"}, {"query", "SELECT * FROM users WHERE id = 123"}},
            {{"shard", "profile_shard"}, {"query", "SELECT * FROM profiles WHERE user_id = 123"}},
            {{"shard", "activity_shard"}, {"query", "SELECT * FROM activities WHERE user_id = 123 LIMIT 10"}}
        }}
    };
    
    std::cout << "Executing snapshot read across 3 shards...\n";
    
    // Execute - no locking, no 2PC, just consistent snapshot
    auto results = coordinator->executeReadOnly(shard_ids, read_operations);
    
    std::cout << "✓ Read completed successfully!\n";
    std::cout << "  Results from " << results.size() << " shards\n";
    std::cout << "  All data read at same consistent snapshot timestamp\n";
    
    // Process results
    for (auto& [shard_id, shard_result] : results.items()) {
        if (shard_result["status"] == "success") {
            std::cout << "  ✓ " << shard_id << ": success\n";
        } else {
            std::cout << "  ✗ " << shard_id << ": " << shard_result["error"] << "\n";
        }
    }
}

// Example 4: Explicit abort
void example_abort_transaction() {
    std::cout << "\n=== Example 4: Explicit Transaction Abort ===\n";
    
    auto truetime = std::make_shared<TrueTime>();
    DistributedTransactionCoordinator::Config config;
    auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
        truetime, config
    );
    
    std::vector<std::string> shard_ids = {"shard1", "shard2"};
    std::string txn_id = coordinator->beginTransaction(shard_ids);
    
    std::cout << "Started transaction: " << txn_id << "\n";
    
    // Add some operations
    nlohmann::json op = {{"type", "insert"}, {"key", "test"}};
    coordinator->addOperation(txn_id, "shard1", op);
    
    // Business logic decides to abort (e.g., validation failed)
    std::cout << "Business validation failed - aborting transaction...\n";
    
    bool aborted = coordinator->abort(txn_id);
    if (aborted) {
        std::cout << "✓ Transaction aborted successfully\n";
        std::cout << "  No changes were applied to any shard\n";
    }
    
    // Try to commit after abort - should fail
    bool commit_after_abort = coordinator->commit(txn_id);
    if (!commit_after_abort) {
        std::cout << "✓ Correctly prevented commit after abort\n";
    }
}

// Example 5: Transaction state tracking
void example_transaction_state() {
    std::cout << "\n=== Example 5: Transaction State Tracking ===\n";
    
    auto truetime = std::make_shared<TrueTime>();
    DistributedTransactionCoordinator::Config config;
    auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
        truetime, config
    );
    
    std::vector<std::string> shard_ids = {"shard1"};
    std::string txn_id = coordinator->beginTransaction(shard_ids);
    
    // Check initial state
    auto state = coordinator->getTransactionState(txn_id);
    if (state && *state == TransactionState::ACTIVE) {
        std::cout << "✓ Transaction state: ACTIVE\n";
    }
    
    // Add operation and commit
    nlohmann::json op = {{"type", "insert"}};
    coordinator->addOperation(txn_id, "shard1", op);
    
    coordinator->commit(txn_id);
    
    // Check final state
    state = coordinator->getTransactionState(txn_id);
    if (state) {
        std::cout << "✓ Transaction final state: ";
        switch (*state) {
            case TransactionState::COMMITTED:
                std::cout << "COMMITTED\n";
                break;
            case TransactionState::ABORTED:
                std::cout << "ABORTED\n";
                break;
            default:
                std::cout << "OTHER\n";
        }
    }
}

// Example 6: Statistics and monitoring
void example_statistics() {
    std::cout << "\n=== Example 6: Statistics and Monitoring ===\n";
    
    auto truetime = std::make_shared<TrueTime>();
    DistributedTransactionCoordinator::Config config;
    auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
        truetime, config
    );
    
    // Execute several transactions
    for (int i = 0; i < 5; ++i) {
        std::vector<std::string> shard_ids = {"shard1", "shard2"};
        std::string txn_id = coordinator->beginTransaction(shard_ids);
        
        nlohmann::json op = {{"type", "insert"}, {"key", "key" + std::to_string(i)}};
        coordinator->addOperation(txn_id, "shard1", op);
        coordinator->addOperation(txn_id, "shard2", op);
        
        coordinator->commit(txn_id);
    }
    
    // Execute some read-only transactions
    for (int i = 0; i < 3; ++i) {
        std::vector<std::string> shard_ids = {"shard1"};
        coordinator->executeReadOnly(shard_ids, nlohmann::json{});
    }
    
    // Get statistics
    auto stats = coordinator->getStatistics();
    
    std::cout << "Coordinator Statistics:\n";
    std::cout << "  Total transactions: " << stats["total_transactions"] << "\n";
    std::cout << "  Committed: " << stats["committed_transactions"] << "\n";
    std::cout << "  Aborted: " << stats["aborted_transactions"] << "\n";
    std::cout << "  Read-only: " << stats["readonly_transactions"] << "\n";
    std::cout << "  Currently active: " << stats["active_transactions"] << "\n";
}

// Example 7: Shard-side participant setup and direct 2PC protocol walkthrough
//
// Demonstrates how each Raft-leader shard constructs a TwoPhaseCommitParticipant
// and how the PREPARE → COMMIT protocol is driven through it.
//
// In a real deployment the participant is attached to ShardRPCServer and the
// coordinator calls sendPrepare()/sendCommit() over the network.  Here we invoke
// the participant's public API directly (simulating what the coordinator's RPC
// layer would do) so the full protocol flow is visible without network setup.
void example_participant_setup() {
    std::cout << "\n=== Example 7: Shard-Side Participant Setup ===\n";

    // ── In-memory key-value store simulating shard storage ───────────────────
    // NOTE: MockStore objects must outlive the TwoPhaseCommitParticipant
    // instances that hold lambdas capturing them by reference.

    struct MockStore {
        std::mutex mu;
        // key → {value, commit_timestamp}
        std::map<std::string, std::pair<std::string, int64_t>> data;
        std::map<std::string, std::vector<std::string>> lock_table; // txn_id → locked keys

        bool validateAndLock(const std::string& txn_id, const nlohmann::json& ops) {
            std::lock_guard<std::mutex> g(mu);
            // Check for write-write conflicts: abort if a key is already locked
            // by a different active transaction (mimics row-level locking).
            for (const auto& op : ops) {
                std::string key = op.value("key", "");
                if (key.empty()) {
                  continue;
                }
                for (const auto& [other_txn, locked_keys] : lock_table) {
                    if (other_txn == txn_id) {
                      continue;
                    }
                    for (const auto& lk : locked_keys) {
                        if (lk == key) {
                            std::cout << "  CONFLICT: key '" << key
                                      << "' locked by " << other_txn << " – voting ABORT\n";
                            return false; // conflict: vote ABORT
                        }
                    }
                }
                lock_table[txn_id].push_back(key);
            }
            return true; // no conflicts: vote COMMIT
        }

        bool apply(const std::string& /*txn_id*/, const nlohmann::json& ops, int64_t ts) {
            std::lock_guard<std::mutex> g(mu);
            for (const auto& op : ops) {
                if (op.value("type", "") == "put") {
                    std::string key = op.value("key", "");
                    std::string val = op.value("value", "");
                    // Store value alongside commit timestamp for temporal ordering
                    auto it = data.find(key);
                    if (it == data.end() || it->second.second < ts) {
                        data[key] = {val, ts};
                    }
                }
            }
            return true;
        }

        void release(const std::string& txn_id) {
            std::lock_guard<std::mutex> g(mu);
            lock_table.erase(txn_id);
        }
    };

    // One MockStore per shard – declared before participants so they outlive them
    MockStore store_alpha, store_beta;

    // ── Build participants (one per shard) ───────────────────────────────────

    TwoPhaseCommitParticipant::Config pcfg;
    pcfg.prepare_timeout_ms = 5000;
    pcfg.sync_wal_writes    = false; // no disk I/O in this demo

    // Lambda callbacks wire the generic 2PC protocol to the concrete storage engine.
    // 'store' is captured by reference; its lifetime is guaranteed to exceed the
    // participant (both are stack-allocated in this function).
    auto make_participant = [&pcfg](const std::string& shard_id, MockStore& store) {
        return std::make_shared<TwoPhaseCommitParticipant>(
            shard_id, pcfg,
            [&store](const std::string& txn, const nlohmann::json& ops) {
                return store.validateAndLock(txn, ops);
            },
            [&store](const std::string& txn, const nlohmann::json& ops, int64_t ts) {
                return store.apply(txn, ops, ts);
            },
            [&store](const std::string& txn) {
                store.release(txn);
            }
        );
    };

    auto participant_alpha = make_participant("shard-alpha", store_alpha);
    auto participant_beta  = make_participant("shard-beta",  store_beta);

    // ── Run the 2PC protocol directly ────────────────────────────────────────
    // (This mirrors what DistributedTransactionCoordinator::commit() does
    //  via sendPrepare() / sendCommit() over the wire.)

    const std::string txn_id = "txn-demo-7";
    std::cout << "Transaction: " << txn_id << "\n";

    nlohmann::json op_alpha = {{"type","put"},{"key","user:1"},{"value","Alice"}};
    nlohmann::json op_beta  = {{"type","put"},{"key","user:2"},{"value","Bob"}};

    // Serialize operation payloads as the coordinator would before sending over RPC
    nlohmann::json payload_alpha;  payload_alpha["operations"] = {op_alpha};
    nlohmann::json payload_beta;   payload_beta["operations"]  = {op_beta};

    // Phase 1: PREPARE – each shard validates, locks, and votes
    bool vote_a = participant_alpha->onPrepare(txn_id, "coordinator", payload_alpha.dump());
    bool vote_b = participant_beta ->onPrepare(txn_id, "coordinator", payload_beta.dump());
    std::cout << "PREPARE votes – shard-alpha: " << (vote_a ? "COMMIT" : "ABORT")
              << ", shard-beta: " << (vote_b ? "COMMIT" : "ABORT") << "\n";

    // Phase 2: COMMIT if unanimous, ABORT otherwise (coordinator decision rule)
    if (vote_a && vote_b) {
        bool ok_a = participant_alpha->onCommit(txn_id);
        bool ok_b = participant_beta ->onCommit(txn_id);
        std::cout << "COMMIT applied – shard-alpha: " << ok_a
                  << ", shard-beta: " << ok_b << "\n";
    } else {
        participant_alpha->onAbort(txn_id);
        participant_beta ->onAbort(txn_id);
        std::cout << "Transaction ABORTED (at least one shard voted ABORT)\n";
    }

    // Verify final state on each shard
    {
        std::lock_guard<std::mutex> g(store_alpha.mu);
        auto it = store_alpha.data.find("user:1");
        if (it != store_alpha.data.end()) {
            std::cout << "shard-alpha user:1 = " << it->second.first
                      << " (committed at ts=" << it->second.second << ")\n";
        } else {
            std::cout << "shard-alpha user:1 = <not found>\n";
        }
    }
    {
        std::lock_guard<std::mutex> g(store_beta.mu);
        auto it = store_beta.data.find("user:2");
        if (it != store_beta.data.end()) {
            std::cout << "shard-beta  user:2 = " << it->second.first
                      << " (committed at ts=" << it->second.second << ")\n";
        } else {
            std::cout << "shard-beta  user:2 = <not found>\n";
        }
    }

    // Participant statistics
    auto stats_a = participant_alpha->getStatistics();
    auto stats_b = participant_beta ->getStatistics();
    std::cout << "shard-alpha – prepares: " << stats_a["total_prepares"]
              << ", commits: " << stats_a["total_commits"] << "\n";
    std::cout << "shard-beta  – prepares: " << stats_b["total_prepares"]
              << ", commits: " << stats_b["total_commits"] << "\n";
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "Distributed Transaction Coordinator Examples\n";
    std::cout << "=================================================\n";
    
    try {
        example_simple_transaction();
        example_ecommerce_order();
        example_readonly_transaction();
        example_abort_transaction();
        example_transaction_state();
        example_statistics();
        example_participant_setup();
        
        std::cout << "\n=================================================\n";
        std::cout << "All examples completed successfully!\n";
        std::cout << "=================================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
