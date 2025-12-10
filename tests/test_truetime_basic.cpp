// Copyright 2025 ThemisDB
// Test for TrueTime and Distributed Transactions

#include "sharding/truetime.h"
#include "sharding/distributed_transaction.h"
#include <iostream>
#include <cassert>

using namespace themis::sharding;

void test_truetime_basic() {
    std::cout << "Testing TrueTime basic functionality..." << std::endl;
    
    // Create TrueTime with default config
    TrueTime::Config config;
    config.base_uncertainty_us = 1000; // 1ms
    TrueTime tt(config);
    
    // Test now()
    auto interval = tt.now();
    assert(interval.latest >= interval.earliest);
    assert(interval.uncertainty().count() > 0);
    
    std::cout << "  Current time interval: [" 
              << interval.earliest.count() << ", " 
              << interval.latest.count() << "]" << std::endl;
    std::cout << "  Uncertainty: " << interval.uncertainty().count() << " ns" << std::endl;
    
    // Test interval comparisons
    auto interval2 = tt.now();
    
    // interval2 should be after interval (or maybe overlapping)
    if (interval2.definitelyAfter(interval)) {
        std::cout << "  interval2 is definitely after interval" << std::endl;
    } else if (interval2.maybeOverlaps(interval)) {
        std::cout << "  interval2 may overlap with interval" << std::endl;
    }
    
    std::cout << "✓ TrueTime basic test passed" << std::endl;
}

void test_distributed_transaction_basic() {
    std::cout << "\nTesting Distributed Transaction basic functionality..." << std::endl;
    
    // Create TrueTime
    TrueTime::Config tt_config;
    tt_config.base_uncertainty_us = 1000;
    auto truetime = std::make_shared<TrueTime>(tt_config);
    
    // Create transaction coordinator
    DistributedTransactionCoordinator::Config txn_config;
    auto coordinator = std::make_shared<DistributedTransactionCoordinator>(
        truetime, txn_config
    );
    
    // Begin transaction
    std::vector<std::string> shards = {"shard1", "shard2"};
    std::string txn_id = coordinator->beginTransaction(shards);
    
    std::cout << "  Started transaction: " << txn_id << std::endl;
    
    // Check transaction state
    auto state = coordinator->getTransactionState(txn_id);
    assert(state.has_value());
    assert(state.value() == TransactionState::ACTIVE);
    
    std::cout << "  Transaction state: ACTIVE" << std::endl;
    
    // Get statistics
    auto stats = coordinator->getStatistics();
    std::cout << "  Coordinator stats: " << stats.dump() << std::endl;
    
    std::cout << "✓ Distributed transaction basic test passed" << std::endl;
}

int main() {
    try {
        std::cout << "=== TrueTime and Distributed Transaction Tests ===" << std::endl;
        
        test_truetime_basic();
        test_distributed_transaction_basic();
        
        std::cout << "\n=== All tests passed ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
