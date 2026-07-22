/*
 * ThemisDB | File: test_batch_write_optimizer.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */
// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


#include <gtest/gtest.h>
#include "storage/batch_write_optimizer.h"
#include <rocksdb/options.h>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>

using namespace themis;

class BatchWriteOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures
    }
};

TEST_F(BatchWriteOptimizerTest, DefaultConfigIsAsync) {
    BatchWriteOptimizer::Config config;
    EXPECT_EQ(config.durability, BatchWriteOptimizer::DurabilityMode::Async);
    EXPECT_FALSE(config.disable_wal);
}

TEST_F(BatchWriteOptimizerTest, SyncModeEnablesSyncWrites) {
    BatchWriteOptimizer::Config config;
    config.durability = BatchWriteOptimizer::DurabilityMode::Sync;
    
    BatchWriteOptimizer optimizer(config);
    auto opts = optimizer.getOptimizedWriteOptions();
    
    EXPECT_TRUE(opts.sync);
    EXPECT_FALSE(opts.disableWAL);
}

TEST_F(BatchWriteOptimizerTest, AsyncModeDisablesSyncButKeepsWAL) {
    BatchWriteOptimizer::Config config;
    config.durability = BatchWriteOptimizer::DurabilityMode::Async;
    
    BatchWriteOptimizer optimizer(config);
    auto opts = optimizer.getOptimizedWriteOptions();
    
    EXPECT_FALSE(opts.sync);
    EXPECT_FALSE(opts.disableWAL);
}

TEST_F(BatchWriteOptimizerTest, NoSyncModeWarns) {
    BatchWriteOptimizer::Config config;
    config.durability = BatchWriteOptimizer::DurabilityMode::NoSync;
    
    // Should log warning but not crash
    BatchWriteOptimizer optimizer(config);
    auto opts = optimizer.getOptimizedWriteOptions();
    
    EXPECT_FALSE(opts.sync);
    EXPECT_FALSE(opts.disableWAL);
}

TEST_F(BatchWriteOptimizerTest, DisableWALWarns) {
    BatchWriteOptimizer::Config config;
    config.disable_wal = true;
    
    // Should log warning but not crash
    BatchWriteOptimizer optimizer(config);
    auto opts = optimizer.getOptimizedWriteOptions();
    
    EXPECT_TRUE(opts.disableWAL);
}

TEST_F(BatchWriteOptimizerTest, StatisticsInitiallyZero) {
    BatchWriteOptimizer optimizer;
    auto stats = optimizer.getStats();
    
    EXPECT_EQ(stats.total_batches_written, 0u);
    EXPECT_EQ(stats.total_items_written, 0u);
    EXPECT_EQ(stats.avg_batch_size, 0.0);
    EXPECT_EQ(stats.total_write_time_ms, 0.0);
    EXPECT_EQ(stats.avg_write_latency_ms, 0.0);
    EXPECT_EQ(stats.throughput_items_per_sec, 0.0);
}

TEST_F(BatchWriteOptimizerTest, RecordBatchWriteUpdatesStatistics) {
    BatchWriteOptimizer optimizer;
    
    // Record some batch writes
    optimizer.recordBatchWrite(100, 10.0);  // 100 items, 10ms
    optimizer.recordBatchWrite(200, 20.0);  // 200 items, 20ms
    optimizer.recordBatchWrite(300, 30.0);  // 300 items, 30ms
    
    auto stats = optimizer.getStats();
    
    EXPECT_EQ(stats.total_batches_written, 3u);
    EXPECT_EQ(stats.total_items_written, 600u);
    EXPECT_EQ(stats.total_write_time_ms, 60.0);
    EXPECT_DOUBLE_EQ(stats.avg_batch_size, 200.0);  // 600/3
    EXPECT_DOUBLE_EQ(stats.avg_write_latency_ms, 20.0);  // 60/3
    
    // Throughput: 600 items / 60ms = 10,000 items/sec
    EXPECT_NEAR(stats.throughput_items_per_sec, 10000.0, 1.0);
}

TEST_F(BatchWriteOptimizerTest, RecommendedConfigForProduction) {
    auto config = BatchWriteOptimizer::recommendedConfigForUseCase("production");
    
    EXPECT_EQ(config.durability, BatchWriteOptimizer::DurabilityMode::Async);
    EXPECT_FALSE(config.disable_wal);
}

TEST_F(BatchWriteOptimizerTest, RecommendedConfigForBulkLoad) {
    auto config = BatchWriteOptimizer::recommendedConfigForUseCase("bulk_load");
    
    EXPECT_EQ(config.durability, BatchWriteOptimizer::DurabilityMode::NoSync);
    EXPECT_FALSE(config.disable_wal);  // Keep WAL for safety
}

TEST_F(BatchWriteOptimizerTest, RecommendedConfigForBenchmark) {
    auto config = BatchWriteOptimizer::recommendedConfigForUseCase("benchmark");
    
    EXPECT_EQ(config.durability, BatchWriteOptimizer::DurabilityMode::NoSync);
    EXPECT_TRUE(config.disable_wal);  // Maximum speed
}

TEST_F(BatchWriteOptimizerTest, RecommendedConfigForCritical) {
    auto config = BatchWriteOptimizer::recommendedConfigForUseCase("critical");
    
    EXPECT_EQ(config.durability, BatchWriteOptimizer::DurabilityMode::Sync);
    EXPECT_FALSE(config.disable_wal);
}

TEST_F(BatchWriteOptimizerTest, RecommendedConfigForUnknownUseCaseDefaultsToProduction) {
    auto config = BatchWriteOptimizer::recommendedConfigForUseCase("unknown_use_case");
    
    EXPECT_EQ(config.durability, BatchWriteOptimizer::DurabilityMode::Async);
}

TEST_F(BatchWriteOptimizerTest, MultipleInstancesIndependentStats) {
    BatchWriteOptimizer optimizer1;
    BatchWriteOptimizer optimizer2;
    
    optimizer1.recordBatchWrite(100, 10.0);
    optimizer2.recordBatchWrite(200, 20.0);
    
    auto stats1 = optimizer1.getStats();
    auto stats2 = optimizer2.getStats();
    
    EXPECT_EQ(stats1.total_batches_written, 1u);
    EXPECT_EQ(stats1.total_items_written, 100u);
    
    EXPECT_EQ(stats2.total_batches_written, 1u);
    EXPECT_EQ(stats2.total_items_written, 200u);
}

TEST_F(BatchWriteOptimizerTest, ConcurrentRecordBatchWriteSafe) {
    BatchWriteOptimizer optimizer;
    
    // Test thread safety by recording from multiple threads
    std::vector<std::thread> worker_threads;
    const int num_threads = 10;
    const int writes_per_thread = 100;
    
    for (int i = 0; i < num_threads; ++i) {
        worker_threads.emplace_back([&optimizer, writes_per_thread]() {
            for (int j = 0; j < writes_per_thread; ++j) {
                optimizer.recordBatchWrite(10, 1.0);
            }
        });
    }
    
    for (auto& t : worker_threads) {
        t.join();
    }
    
    auto stats = optimizer.getStats();
    EXPECT_EQ(stats.total_batches_written, num_threads * writes_per_thread);
    EXPECT_EQ(stats.total_items_written, num_threads * writes_per_thread * 10);
}

// Benchmark: Measure overhead of optimizer
TEST_F(BatchWriteOptimizerTest, DISABLED_BenchmarkOverhead) {
    BatchWriteOptimizer optimizer;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000000; ++i) {
        auto opts = optimizer.getOptimizedWriteOptions();
        (void)opts;  // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    double avg_ns = static_cast<double>(duration) / 1000000.0;
    std::cout << "Average getOptimizedWriteOptions() overhead: " 
              << avg_ns << " ns" << std::endl;
    
    // Should be < 100ns per call
    EXPECT_LT(avg_ns, 100.0);
}
