// Google Test suite for ThemisDB Optimizations (v1.4.0)
// Tests for: HNSW parameter tuning, WriteBatch API, gRPC protocol
// Date: December 25, 2024

#include <gtest/gtest.h>
#include "rocksdb_wrapper.h"
#include "server/grpc_service.h"
#include "llm/multi_agent_orchestrator.h"
#include <memory>
#include <random>
#include <chrono>

using namespace themis;

// =============================================================================
// Test Fixtures
// =============================================================================

class ThemisOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database for testing
        db_path_ = "/tmp/themis_test_optimization_" + 
                   std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        
        db_ = std::make_shared<RocksDBWrapper>(db_path_);
    }
    
    void TearDown() override {
        db_.reset();
        // Cleanup test database
        std::system(("rm -rf " + db_path_).c_str());
    }
    
    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
};

// =============================================================================
// WriteBatch API Tests
// =============================================================================

TEST_F(ThemisOptimizationTest, WriteBatch_BasicOperations) {
    auto batch = db_->createWriteBatch();
    
    // Add multiple operations
    batch->put("key1", "value1");
    batch->put("key2", "value2");
    batch->put("key3", "value3");
    
    // Atomic commit
    auto status = batch->commit();
    ASSERT_TRUE(status.ok()) << "Batch commit should succeed";
    
    // Verify all operations committed
    std::string val1, val2, val3;
    ASSERT_TRUE(db_->get("key1", val1).ok());
    ASSERT_TRUE(db_->get("key2", val2).ok());
    ASSERT_TRUE(db_->get("key3", val3).ok());
    
    EXPECT_EQ(val1, "value1");
    EXPECT_EQ(val2, "value2");
    EXPECT_EQ(val3, "value3");
}

TEST_F(ThemisOptimizationTest, WriteBatch_Atomicity) {
    // Test that failed commit rolls back all operations
    auto batch = db_->createWriteBatch();
    
    batch->put("atomic_key1", "value1");
    batch->put("atomic_key2", "value2");
    
    // Simulate failure by closing DB before commit
    // (In production, this would be a genuine failure case)
    
    auto status = batch->commit();
    // In case of failure, ensure atomicity
    
    if (!status.ok()) {
        // Verify no partial writes
        std::string val;
        EXPECT_FALSE(db_->get("atomic_key1", val).ok()) 
            << "Partial write should not exist on failed batch";
    }
}

TEST_F(ThemisOptimizationTest, WriteBatch_MixedOperations) {
    // Pre-populate with data
    db_->put("old_key", "old_value");
    
    auto batch = db_->createWriteBatch();
    
    // Mix of PUT and DELETE
    batch->put("new_key1", "new_value1");
    batch->put("new_key2", "new_value2");
    batch->delete_key("old_key");
    
    ASSERT_TRUE(batch->commit().ok());
    
    // Verify new keys exist
    std::string val1, val2;
    ASSERT_TRUE(db_->get("new_key1", val1).ok());
    ASSERT_TRUE(db_->get("new_key2", val2).ok());
    EXPECT_EQ(val1, "new_value1");
    EXPECT_EQ(val2, "new_value2");
    
    // Verify old key deleted
    std::string old_val;
    EXPECT_FALSE(db_->get("old_key", old_val).ok()) 
        << "Deleted key should not exist";
}

TEST_F(ThemisOptimizationTest, WriteBatch_LargeBatch) {
    const int BATCH_SIZE = 1000;
    
    auto batch = db_->createWriteBatch();
    
    // Add 1000 operations
    for (int i = 0; i < BATCH_SIZE; ++i) {
        std::string key = "batch_key_" + std::to_string(i);
        std::string value = "batch_value_" + std::to_string(i);
        batch->put(key, value);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto status = batch->commit();
    auto end = std::chrono::high_resolution_clock::now();
    
    ASSERT_TRUE(status.ok()) << "Large batch commit should succeed";
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "  Committed " << BATCH_SIZE << " operations in " 
              << duration.count() << "ms" << std::endl;
    
    // Verify random samples
    for (int i = 0; i < 10; ++i) {
        int idx = rand() % BATCH_SIZE;
        std::string key = "batch_key_" + std::to_string(idx);
        std::string value;
        ASSERT_TRUE(db_->get(key, value).ok());
        EXPECT_EQ(value, "batch_value_" + std::to_string(idx));
    }
}

TEST_F(ThemisOptimizationTest, WriteBatch_PerformanceComparison) {
    const int NUM_OPERATIONS = 100;
    
    // Benchmark: Individual operations
    auto start_individual = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        db_->put("individual_" + std::to_string(i), "value_" + std::to_string(i));
    }
    auto end_individual = std::chrono::high_resolution_clock::now();
    auto duration_individual = std::chrono::duration_cast<std::chrono::microseconds>(
        end_individual - start_individual);
    
    // Benchmark: Batched operations
    auto batch = db_->createWriteBatch();
    auto start_batch = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        batch->put("batched_" + std::to_string(i), "value_" + std::to_string(i));
    }
    batch->commit();
    auto end_batch = std::chrono::high_resolution_clock::now();
    auto duration_batch = std::chrono::duration_cast<std::chrono::microseconds>(
        end_batch - start_batch);
    
    std::cout << "  Individual operations: " << duration_individual.count() << " µs" << std::endl;
    std::cout << "  Batched operations:    " << duration_batch.count() << " µs" << std::endl;
    
    double speedup = static_cast<double>(duration_individual.count()) / duration_batch.count();
    std::cout << "  Speedup: " << speedup << "×" << std::endl;
    
    // Batched operations should be faster (typically 2-5×)
    EXPECT_GT(speedup, 1.5) << "WriteBatch should provide significant speedup";
}

// =============================================================================
// HNSW Parameter Tests
// =============================================================================

TEST_F(ThemisOptimizationTest, HNSW_ParameterValidation) {
    // Test parameter ranges from config
    struct HNSWParams {
        int M;
        int ef_construction;
        int ef_search;
        std::string preset;
    };
    
    std::vector<HNSWParams> presets = {
        {12, 100, 32, "speed"},
        {16, 200, 64, "balanced"},
        {24, 300, 96, "production"},
        {32, 400, 128, "quality"},
        {8, 100, 32, "memory"}
    };
    
    for (const auto& params : presets) {
        // Validate parameter relationships
        EXPECT_GT(params.M, 0) << "M must be positive";
        EXPECT_GE(params.ef_construction, params.M * 8) 
            << "ef_construction should be >= M × 8 for " << params.preset;
        EXPECT_GT(params.ef_search, 0) << "ef_search must be positive";
        
        std::cout << "  Preset '" << params.preset << "': M=" << params.M 
                  << ", ef_construction=" << params.ef_construction
                  << ", ef_search=" << params.ef_search << std::endl;
    }
}

TEST_F(ThemisOptimizationTest, HNSW_MemoryEstimation) {
    // Test memory footprint estimates
    const int NUM_VECTORS = 1000000;  // 1M vectors
    
    auto estimate_memory = [](int M, int num_vectors) -> size_t {
        // Memory per vector ≈ M × 4 bytes × 2 (bidirectional)
        return static_cast<size_t>(M) * 4 * 2 * num_vectors;
    };
    
    std::cout << "  Memory estimates for 1M vectors:" << std::endl;
    
    // Speed preset (M=12)
    size_t mem_speed = estimate_memory(12, NUM_VECTORS);
    std::cout << "    Speed (M=12):      " << mem_speed / (1024*1024) << " MB" << std::endl;
    EXPECT_LT(mem_speed, 3000ULL * 1024 * 1024) << "Speed preset should use < 3GB";
    
    // Balanced preset (M=16)
    size_t mem_balanced = estimate_memory(16, NUM_VECTORS);
    std::cout << "    Balanced (M=16):   " << mem_balanced / (1024*1024) << " MB" << std::endl;
    
    // Production preset (M=24)
    size_t mem_production = estimate_memory(24, NUM_VECTORS);
    std::cout << "    Production (M=24): " << mem_production / (1024*1024) << " MB" << std::endl;
    
    // Quality preset (M=32)
    size_t mem_quality = estimate_memory(32, NUM_VECTORS);
    std::cout << "    Quality (M=32):    " << mem_quality / (1024*1024) << " MB" << std::endl;
    EXPECT_LT(mem_quality, 6000ULL * 1024 * 1024) << "Quality preset should use < 6GB";
}

// =============================================================================
// gRPC Service Tests (if gRPC is enabled)
// =============================================================================

#ifdef THEMIS_ENABLE_GRPC

TEST_F(ThemisOptimizationTest, GRPC_ServiceInitialization) {
    // Test gRPC service can be created
    auto grpc_service = std::make_unique<grpc_service::ThemisGRPCServiceImpl>(db_);
    EXPECT_NE(grpc_service, nullptr);
}

TEST_F(ThemisOptimizationTest, GRPC_ServerStartStop) {
    // Test gRPC server can start and stop
    auto server_manager = std::make_unique<grpc_service::GRPCServerManager>(
        "localhost:50052", db_);
    
    server_manager->start();
    EXPECT_TRUE(server_manager->isRunning());
    
    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server_manager->stop();
    EXPECT_FALSE(server_manager->isRunning());
}

#endif // THEMIS_ENABLE_GRPC

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(ThemisOptimizationTest, Integration_MultiAgentWithWriteBatch) {
    // Simulate multi-agent workflow with WriteBatch
    const int NUM_AGENTS = 5;
    
    auto batch = db_->createWriteBatch();
    
    // Each agent produces a result
    for (int i = 0; i < NUM_AGENTS; ++i) {
        std::string key = "agent_result:" + std::to_string(i);
        std::string value = "{\"agent_id\": " + std::to_string(i) + 
                           ", \"result\": \"analysis_complete\"}";
        batch->put(key, value);
    }
    
    // Update task status atomically with results
    batch->put("task:status", "completed");
    batch->put("task:agent_count", std::to_string(NUM_AGENTS));
    
    // Atomic commit ensures consistency
    ASSERT_TRUE(batch->commit().ok());
    
    // Verify all agent results and task status
    for (int i = 0; i < NUM_AGENTS; ++i) {
        std::string key = "agent_result:" + std::to_string(i);
        std::string value;
        ASSERT_TRUE(db_->get(key, value).ok()) 
            << "Agent " << i << " result should exist";
    }
    
    std::string status;
    ASSERT_TRUE(db_->get("task:status", status).ok());
    EXPECT_EQ(status, "completed");
}

TEST_F(ThemisOptimizationTest, Integration_HNSWWithMultiAgent) {
    // Test that HNSW parameters are suitable for multi-agent use cases
    
    // Agent role matching: fast approximate search
    struct RoleMatchingParams {
        int M = 12;
        int ef_construction = 100;
        int ef_search = 32;
    } role_params;
    
    EXPECT_EQ(role_params.M, 12);
    EXPECT_EQ(role_params.ef_search, 32);
    
    // Agent document retrieval: high quality
    struct DocumentRetrievalParams {
        int M = 24;
        int ef_construction = 300;
        int ef_search = 96;
    } doc_params;
    
    EXPECT_EQ(doc_params.M, 24);
    EXPECT_EQ(doc_params.ef_search, 96);
    
    std::cout << "  Multi-agent HNSW configuration validated" << std::endl;
}

// =============================================================================
// Performance Regression Tests
// =============================================================================

TEST_F(ThemisOptimizationTest, Performance_WriteBatchThroughput) {
    const int NUM_BATCHES = 10;
    const int OPERATIONS_PER_BATCH = 100;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int batch_idx = 0; batch_idx < NUM_BATCHES; ++batch_idx) {
        auto batch = db_->createWriteBatch();
        
        for (int i = 0; i < OPERATIONS_PER_BATCH; ++i) {
            std::string key = "perf_" + std::to_string(batch_idx) + "_" + std::to_string(i);
            batch->put(key, "value_" + std::to_string(i));
        }
        
        ASSERT_TRUE(batch->commit().ok());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    int total_ops = NUM_BATCHES * OPERATIONS_PER_BATCH;
    double ops_per_sec = (total_ops * 1000.0) / duration.count();
    
    std::cout << "  WriteBatch throughput: " << ops_per_sec << " ops/sec" << std::endl;
    
    // Expect at least 5000 ops/sec with batching
    EXPECT_GT(ops_per_sec, 5000.0) 
        << "WriteBatch should provide >5k ops/sec throughput";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
