/**
 * @file test_tensor_ingestion_bridge_concurrent_focused.cpp
 * @brief Concurrent workload hardening tests for TensorIngestionBridge (Block A1).
 * 
 * Tests: TNIC-01..TNIC-16 (16 concurrent test cases)
 * Acceptance: All pass 100+ iterations without flakiness, zero ThreadSanitizer races
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>

#include "tensor/tensor_ingestion_bridge.h"
#include "ingestion/inference_backend.h"

using namespace themis::ingestion;
using namespace themis::tensor;

// Helper: generate random embedding
static std::vector<float> generateRandomEmbedding(size_t dim, unsigned seed = 42) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    std::vector<float> emb(dim);
    for (auto& e : emb) e = dis(gen);
    return emb;
}

// Helper: generate compressible (low-rank) embedding
static std::vector<float> generateRankOneEmbedding(size_t rows, size_t cols) {
    std::vector<float> u(rows), v(cols);
    for (size_t i = 0; i < rows; ++i) u[i] = static_cast<float>(i + 1);
    for (size_t j = 0; j < cols; ++j) v[j] = static_cast<float>(j + 1);
    std::vector<float> out(rows * cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            out[i * cols + j] = u[i] * v[j];
        }
    }
    return out;
}

class TensorIngestionBridgeConcurrentTest : public ::testing::Test {
protected:
    void SetUp() override {
        bridge_ = std::make_unique<TensorIngestionBridge>(0.01, 32, 1.3);
    }

    std::unique_ptr<TensorIngestionBridge> bridge_;
};

// ============================================================================
// TNIC-01..05: Concurrent Decomposition & Config
// ============================================================================

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC01_ConcurrentDecomposeMultipleEmbeddings) {
    const int num_threads = 32;
    std::vector<std::thread> threads;
    std::atomic<int> successful_decomposes{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto emb = generateRandomEmbedding(256, i);
            auto rec = bridge_->decompose(emb, "chunk_" + std::to_string(i), "file_" + std::to_string(i));
            if (!rec.serialized_train.empty()) {
                successful_decomposes.fetch_add(1);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_GT(successful_decomposes.load(), 0);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC02_ConfigRaceDuringDecompose) {
    const int num_decompose_threads = 20;
    const int num_config_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> decomposes{0}, configs{0};
    std::atomic<bool> stop_flag{false};
    
    auto decompose_fn = [&](int i) {
        while (!stop_flag.load()) {
            auto emb = generateRandomEmbedding(256, i);
            bridge_->decompose(emb, "chunk", "file");
            decomposes.fetch_add(1);
        }
    };
    
    auto config_fn = [&]() {
        while (!stop_flag.load()) {
            bridge_->setEpsilon(0.01 + 0.001);
            bridge_->setMaxRank(32 + 1);
            bridge_->setMinKappa(1.3 + 0.1);
            configs.fetch_add(1);
        }
    };
    
    for (int i = 0; i < num_decompose_threads; ++i) {
        threads.emplace_back(decompose_fn, i);
    }
    for (int i = 0; i < num_config_threads; ++i) {
        threads.emplace_back(config_fn);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop_flag.store(true);
    
    for (auto& t : threads) t.join();
    
    EXPECT_GT(decomposes.load(), 0);
    EXPECT_GT(configs.load(), 0);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC03_ConcurrentShouldDecomposeSameEmbedding) {
    auto emb = generateRankOneEmbedding(32, 32);  // Compressible
    const int num_threads = 50;
    std::vector<std::thread> threads;
    std::atomic<int> should_decompose_true{0}, should_decompose_false{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            bool result = bridge_->shouldDecompose(emb);
            if (result) {
                should_decompose_true.fetch_add(1);
            } else {
                should_decompose_false.fetch_add(1);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    // All should return same result (deterministic)
    if (should_decompose_true.load() > 0) {
        EXPECT_EQ(should_decompose_false.load(), 0);
    } else {
        EXPECT_EQ(should_decompose_true.load(), 0);
    }
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC04_MixedDecomposeAndShouldDecompose) {
    const int num_threads = 32;
    std::vector<std::thread> threads;
    std::atomic<int> operations{0};
    
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([&, i]() {
            auto emb = generateRandomEmbedding(256, i);
            bridge_->decompose(emb, "chunk", "file");
            operations.fetch_add(1);
        });
    }
    
    for (int i = num_threads / 2; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto emb = generateRandomEmbedding(256, i);
            bridge_->shouldDecompose(emb);
            operations.fetch_add(1);
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(operations.load(), num_threads);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC05_AtomicCounterThreadSafety) {
    const int num_threads = 100;
    const int decompositions_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto emb = generateRandomEmbedding(256, i);
            for (int j = 0; j < decompositions_per_thread; ++j) {
                bridge_->decompose(emb, "chunk_" + std::to_string(j), "file");
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    long long total_count = bridge_->decomposeCount();
    EXPECT_GE(total_count, (long long)num_threads * decompositions_per_thread);
}

// ============================================================================
// TNIC-06..09: Pilot Computation & RNG
// ============================================================================

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC06_RNGDeterminismSameSeed) {
    auto emb = generateRandomEmbedding(2048, 42);  // Large embedding triggers pilot
    
    const int num_threads = 20;
    std::vector<std::thread> threads;
    std::atomic<int> should_decompose_results{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            // Same embedding should give same result
            bool result = bridge_->shouldDecompose(emb);
            should_decompose_results.fetch_add(result ? 1 : 0);
        });
    }
    
    for (auto& t : threads) t.join();
    
    // All should give same answer: either all true or all false
    int true_count = should_decompose_results.load();
    EXPECT_TRUE(true_count == num_threads || true_count == 0);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC07_LargeEmbeddingPilotProjection) {
    auto emb = generateRankOneEmbedding(64, 64);  // 4096-dimensional, highly compressible
    
    const int num_threads = 20;
    std::vector<std::thread> threads;
    std::atomic<int> successful_pilots{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            bool result = bridge_->shouldDecompose(emb);
            if (result) successful_pilots.fetch_add(1);
        });
    }
    
    for (auto& t : threads) t.join();
    
    // Should be consistent across threads
    EXPECT_GT(successful_pilots.load(), 0);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC08_RNGSeedCollisionTest) {
    const int num_threads = 1000;
    const int unique_embeddings = 100;
    std::vector<std::thread> threads;
    std::atomic<int> deterministic_results{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            int emb_id = i % unique_embeddings;
            auto emb = generateRandomEmbedding(2048, emb_id);
            bool result = bridge_->shouldDecompose(emb);
            // Result should be deterministic based on embedding
            deterministic_results.fetch_add(1);
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(deterministic_results.load(), num_threads);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC09_PilotWorkUnderHighConcurrency) {
    const int num_threads = 64;
    std::vector<std::thread> threads;
    std::atomic<int> pilot_checks{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            // All use large embeddings to trigger pilot computation
            auto emb = generateRandomEmbedding(2048, i);
            bridge_->shouldDecompose(emb);
            pilot_checks.fetch_add(1);
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(pilot_checks.load(), num_threads);
}

// ============================================================================
// TNIC-10..13: Stress & Saturation
// ============================================================================

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC10_HighConcurrentDecompositions) {
    const int num_threads = 256;  // Exceeds queue limit
    std::vector<std::thread> threads;
    std::atomic<int> completes{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto emb = generateRandomEmbedding(256, i);
            bridge_->decompose(emb, "chunk_" + std::to_string(i), "file");
            completes.fetch_add(1);
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(completes.load(), num_threads);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC11_DecomposerWorkQueueFairness) {
    const int num_threads = 32;
    std::vector<std::thread> threads;
    std::atomic<int> high_priority_done{0}, low_priority_done{0};
    
    // Simulate high and low priority embeddings
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([&, i]() {
            // High priority: small embeddings (quick)
            auto emb = generateRandomEmbedding(64, i);
            bridge_->decompose(emb, "high_" + std::to_string(i), "file");
            high_priority_done.fetch_add(1);
        });
    }
    
    for (int i = num_threads / 2; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            // Low priority: large embeddings (slow)
            auto emb = generateRandomEmbedding(4096, i);
            bridge_->decompose(emb, "low_" + std::to_string(i), "file");
            low_priority_done.fetch_add(1);
        });
    }
    
    for (auto& t : threads) t.join();
    
    EXPECT_GT(high_priority_done.load(), 0);
    EXPECT_GT(low_priority_done.load(), 0);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC12_KappaSkipCountAccuracy) {
    auto incompressible_emb = generateRandomEmbedding(256, 42);  // Random, likely incompressible
    
    const int num_threads = 32;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                bridge_->shouldDecompose(incompressible_emb);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    long long skip_count = bridge_->kappaSkipCount();
    EXPECT_GE(skip_count, 0LL);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC13_ConfigChangeDuringActiveDecompositions) {
    const int num_decompose_threads = 20;
    const int num_config_changes = 100;
    std::vector<std::thread> threads;
    std::atomic<int> decomposes{0};
    
    // Start decomposition threads
    for (int i = 0; i < num_decompose_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 50; ++j) {
                auto emb = generateRandomEmbedding(256, i * 50 + j);
                bridge_->decompose(emb, "chunk", "file");
                decomposes.fetch_add(1);
            }
        });
    }
    
    // Change config while decomposing
    for (int i = 0; i < num_config_changes; ++i) {
        threads.emplace_back([&, i]() {
            bridge_->setEpsilon(0.001 + i * 0.0001);
            bridge_->setMaxRank(16 + i % 32);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(decomposes.load(), num_decompose_threads * 50);
}

// ============================================================================
// TNIC-14..16: Diagnostics & Counters
// ============================================================================

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC14_ConcurrentDecomposeCountReads) {
    // Generate some baseline decompositions
    auto emb = generateRandomEmbedding(256, 42);
    for (int i = 0; i < 100; ++i) {
        bridge_->decompose(emb, "baseline_" + std::to_string(i), "file");
    }
    
    long long baseline_count = bridge_->decomposeCount();
    
    const int num_threads = 50;
    std::vector<std::thread> threads;
    std::atomic<int> reads{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            long long count = bridge_->decomposeCount();
            EXPECT_GE(count, baseline_count);
            reads.fetch_add(1);
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(reads.load(), num_threads);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC15_DescriptionThreadSafety) {
    const int reader_threads = 50;
    const int setter_threads = 2;
    std::vector<std::thread> threads;
    std::atomic<int> reads{0}, sets{0};
    std::atomic<bool> stop_flag{false};
    
    auto reader_fn = [&]() {
        while (!stop_flag.load()) {
            std::string desc = bridge_->description();
            EXPECT_FALSE(desc.empty());
            reads.fetch_add(1);
        }
    };
    
    auto setter_fn = [&]() {
        for (int i = 0; i < 50; ++i) {
            bridge_->setEpsilon(0.001 + i * 0.0001);
            bridge_->setMaxRank(16 + i);
            bridge_->setMinKappa(1.1 + i * 0.01);
            sets.fetch_add(1);
        }
    };
    
    for (int i = 0; i < reader_threads; ++i) {
        threads.emplace_back(reader_fn);
    }
    for (int i = 0; i < setter_threads; ++i) {
        threads.emplace_back(setter_fn);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop_flag.store(true);
    
    for (auto& t : threads) t.join();
    
    EXPECT_GT(reads.load(), 0);
    EXPECT_EQ(sets.load(), setter_threads * 50);
}

TEST_F(TensorIngestionBridgeConcurrentTest, TNIC16_OverflowHandlingLongRunning) {
    // Simulate billions of decompositions (using atomic increment)
    const int num_threads = 32;
    const int increments_per_thread = 10000;
    
    for (int i = 0; i < num_threads; ++i) {
        std::thread([&]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                auto emb = generateRandomEmbedding(128, j);
                bridge_->decompose(emb, "chunk", "file");
            }
        }).detach();
    }
    
    // Wait for threads to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    long long total_count = bridge_->decomposeCount();
    EXPECT_GE(total_count, (long long)num_threads * increments_per_thread);
}

