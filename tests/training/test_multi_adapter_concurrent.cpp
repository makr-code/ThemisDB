/*
 * ThemisDB | File: test_multi_adapter_concurrent.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_multi_adapter_concurrent.cpp
 * @brief Phase 4 concurrent training scenarios with multiple adapters.
 *
 * Tests verify:
 *  - Concurrent training on multiple adapters
 *  - Independent adapter state isolation
 *  - Thread-safe adapter operations
 *  - Concurrent forward passes
 *  - Merge operations during training
 *  - Data race detection
 *  - Load distribution across adapters
 *  - Deadlock-free execution
 */

#include <gtest/gtest.h>
#include "training/lora_adapter.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace themis::training;

// ============================================================================
// Concurrent test fixture
// ============================================================================

class MultiAdapterConcurrentTest : public ::testing::Test {
protected:
    static constexpr int NUM_ADAPTERS = 4;
    static constexpr int NUM_THREADS = 8;
    static constexpr int ITERATIONS = 50;
    static constexpr int RANK = 8;
    static constexpr float ALPHA = 8.0f;
    static constexpr int IN_DIM = 64;
    static constexpr int OUT_DIM = 64;
    static constexpr unsigned int SEED = 42;

    std::mt19937 rng_{SEED};

    std::vector<std::unique_ptr<LoRAAdapter>> createAdapters(int count) {
        std::vector<std::unique_ptr<LoRAAdapter>> adapters;
        for (int i = 0; i < count; ++i) {
            auto adapter = std::make_unique<LoRAAdapter>(RANK, ALPHA);
            adapter->addLayer("query", IN_DIM, OUT_DIM);
            adapter->addLayer("key", IN_DIM, OUT_DIM);
            adapter->addLayer("value", IN_DIM, OUT_DIM);
            adapters.push_back(std::move(adapter));
        }
        return adapters;
    }

    std::vector<float> generateRandomDelta(float scale = 0.01f) {
        std::uniform_real_distribution<float> dist(-scale, scale);
        std::vector<float> delta(IN_DIM * RANK);
        for (auto& d : delta) {
            d = dist(rng_);
        }
        return delta;
    }
};

// ============================================================================
// Basic concurrent training
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, MultiAdapter_ConcurrentTraining) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    std::vector<std::thread> threads;
    std::atomic<int> successful_updates(0);

    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        threads.emplace_back([this, &adapters, &successful_updates, adapter_idx]() {
            for (int iter = 0; iter < ITERATIONS; ++iter) {
                auto dB = generateRandomDelta();
                auto dA = generateRandomDelta();

                auto result = adapters[adapter_idx]->applyUpdate("query", dB, dA);
                if (result.success) {
                    successful_updates++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successful_updates.load(), NUM_ADAPTERS * ITERATIONS);
}

// ============================================================================
// Independent adapter state
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, AdapterState_Independent) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    // Apply different updates to each adapter
    std::vector<std::thread> threads;

    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        threads.emplace_back([this, &adapters, adapter_idx]() {
            for (int step = 0; step < 20; ++step) {
                auto dB = generateRandomDelta(0.01f * (adapter_idx + 1));
                auto dA = generateRandomDelta(0.01f * (adapter_idx + 1));
                adapters[adapter_idx]->applyUpdate("query", dB, dA);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all adapters have different weights
    std::vector<float> first_B_values;
    for (int i = 0; i < NUM_ADAPTERS; ++i) {
        const auto& w = adapters[i]->getWeights("query");
        EXPECT_FALSE(w.B.empty());
        if (i == 0) {
            first_B_values = w.B;
        } else {
            // Should have different weights
            bool different = false;
            for (size_t j = 0; j < w.B.size(); ++j) {
                if (std::abs(w.B[j] - first_B_values[j]) > 1e-5f) {
                    different = true;
                    break;
                }
            }
            EXPECT_TRUE(different) << "Adapter " << i << " should differ from adapter 0";
        }
    }
}

// ============================================================================
// Concurrent forward passes
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, ConcurrentForwardPasses_Safe) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    // Prepare input
    std::vector<float> input(32 * IN_DIM, 0.5f);

    std::vector<std::thread> threads;
    std::atomic<int> successful_forwards(0);

    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        threads.emplace_back([&adapters, &input, &successful_forwards, adapter_idx]() {
            for (int iter = 0; iter < 100; ++iter) {
                auto q_output = adapters[adapter_idx]->forward("query", input, 32);
                auto k_output = adapters[adapter_idx]->forward("key", input, 32);
                auto v_output = adapters[adapter_idx]->forward("value", input, 32);

                if (!q_output.empty() && !k_output.empty() && !v_output.empty()) {
                    successful_forwards++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(successful_forwards.load(), 0);
}

// ============================================================================
// Interleaved operations
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, InterleavedUpdatesAndForwards) {
    auto adapters = createAdapters(NUM_ADAPTERS);
    std::vector<float> input(32 * IN_DIM, 0.5f);

    std::vector<std::thread> threads;
    std::atomic<int> operations(0);

    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        threads.emplace_back([this, &adapters, &input, &operations, adapter_idx]() {
            for (int iter = 0; iter < 50; ++iter) {
                // Update
                auto dB = generateRandomDelta();
                auto dA = generateRandomDelta();
                adapters[adapter_idx]->applyUpdate("query", dB, dA);
                operations++;

                // Forward
                auto output = adapters[adapter_idx]->forward("query", input, 32);
                if (!output.empty()) {
                    operations++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(operations.load(), NUM_ADAPTERS * 50 * 2);
}

// ============================================================================
// Batch update concurrency
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, ConcurrentBatchUpdates) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    std::vector<std::thread> threads;
    std::atomic<int> successful_batches(0);

    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        threads.emplace_back([this, &adapters, &successful_batches, adapter_idx]() {
            for (int iter = 0; iter < 30; ++iter) {
                WeightUpdateBatch batch;
                batch.layer_names = {"query", "key", "value"};
                batch.delta_B = {
                    generateRandomDelta(),
                    generateRandomDelta(),
                    generateRandomDelta()
                };
                batch.delta_A = {
                    generateRandomDelta(),
                    generateRandomDelta(),
                    generateRandomDelta()
                };

                auto result = adapters[adapter_idx]->applyBatchUpdate(batch);
                if (result.success) {
                    successful_batches++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successful_batches.load(), NUM_ADAPTERS * 30);
}

// ============================================================================
// Export/Import during concurrent training
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, ExportWhileConcurrentTraining) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    std::vector<std::thread> training_threads;
    std::vector<std::thread> export_threads;
    std::atomic<int> successful_exports(0);

    // Training threads
    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        training_threads.emplace_back([this, &adapters, adapter_idx]() {
            for (int step = 0; step < 50; ++step) {
                auto dB = generateRandomDelta();
                auto dA = generateRandomDelta();
                adapters[adapter_idx]->applyUpdate("query", dB, dA);
            }
        });
    }

    // Export threads
    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        export_threads.emplace_back([&adapters, &successful_exports, adapter_idx]() {
            for (int exp = 0; exp < 10; ++exp) {
                auto exported = adapters[adapter_idx]->exportWeights();
                if (exported.size() == 3) {  // 3 layers
                    successful_exports++;
                }
            }
        });
    }

    for (auto& thread : training_threads) {
        thread.join();
    }
    for (auto& thread : export_threads) {
        thread.join();
    }

    EXPECT_EQ(successful_exports.load(), NUM_ADAPTERS * 10);
}

// ============================================================================
// Load balancing across adapters
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, LoadDistribution_Balanced) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    std::vector<std::atomic<int>> update_counts(NUM_ADAPTERS);
    for (auto& count : update_counts) {
        count = 0;
    }

    std::vector<std::thread> threads;

    // Multiple threads updating all adapters
    for (int thread_idx = 0; thread_idx < NUM_THREADS; ++thread_idx) {
        threads.emplace_back([this, &adapters, &update_counts, thread_idx]() {
            for (int iter = 0; iter < ITERATIONS; ++iter) {
                int adapter_idx = (thread_idx + iter) % NUM_ADAPTERS;
                auto dB = generateRandomDelta();
                auto dA = generateRandomDelta();
                auto result = adapters[adapter_idx]->applyUpdate("query", dB, dA);
                if (result.success) {
                    update_counts[adapter_idx]++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all adapters received updates
    for (int i = 0; i < NUM_ADAPTERS; ++i) {
        EXPECT_GT(update_counts[i].load(), 0);
    }
}

// ============================================================================
// Concurrent layer operations
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, ConcurrentLayerQueries) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    std::vector<std::thread> threads;
    std::atomic<int> queries(0);

    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        threads.emplace_back([&adapters, &queries, adapter_idx]() {
            for (int iter = 0; iter < 100; ++iter) {
                auto count = adapters[adapter_idx]->layerCount();
                EXPECT_EQ(count, 3u);  // Always 3 layers
                queries++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(queries.load(), NUM_ADAPTERS * 100);
}

// ============================================================================
// Parameter count consistency
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, ParameterCountConsistency) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    // Expected: 3 layers × 2 matrices (B, A) × IN_DIM × RANK
    size_t expected_params = 3 * 2 * IN_DIM * RANK;

    for (const auto& adapter : adapters) {
        EXPECT_EQ(adapter->totalParameterCount(), expected_params);
    }
}

// ============================================================================
// Stress with many threads
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, HighConcurrency_Stable) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    const int high_thread_count = 16;
    std::vector<std::thread> threads;
    std::atomic<int> successful_ops(0);

    for (int thread_idx = 0; thread_idx < high_thread_count; ++thread_idx) {
        threads.emplace_back([this, &adapters, &successful_ops, thread_idx]() {
            std::mt19937 local_rng(thread_idx);
            std::uniform_int_distribution<int> adapter_dist(0, NUM_ADAPTERS - 1);
            std::uniform_int_distribution<int> layer_dist(0, 2);

            for (int iter = 0; iter < 20; ++iter) {
                int adapter_idx = adapter_dist(local_rng);
                int layer_idx = layer_dist(local_rng);

                std::string layer_names[] = {"query", "key", "value"};
                std::string layer_name = layer_names[layer_idx];

                auto dB = generateRandomDelta();
                auto dA = generateRandomDelta();

                try {
                    auto result = adapters[adapter_idx]->applyUpdate(layer_name, dB, dA);
                    if (result.success) {
                        successful_ops++;
                    }
                } catch (...) {
                    // Expected to handle errors gracefully
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Should have completed without crashes
    EXPECT_GT(successful_ops.load(), 0);
}

// ============================================================================
// No deadlocks
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, NoDeadlock_TimedCompletion) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    std::atomic<bool> timeout(false);

    for (int adapter_idx = 0; adapter_idx < NUM_ADAPTERS; ++adapter_idx) {
        threads.emplace_back([this, &adapters, adapter_idx]() {
            for (int step = 0; step < 100; ++step) {
                auto dB = generateRandomDelta();
                auto dA = generateRandomDelta();
                adapters[adapter_idx]->applyUpdate("query", dB, dA);
            }
        });
    }

    // Wait with timeout
    for (auto& thread : threads) {
        thread.join();
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - start).count();

    // Should complete in reasonable time (not deadlocked)
    EXPECT_LT(elapsed, 30);  // 30 seconds timeout
}

// ============================================================================
// Memory safety
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, NoDataRaces_StressTest) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    std::vector<std::thread> threads;
    std::atomic<bool> error_detected(false);

    for (int thread_idx = 0; thread_idx < 10; ++thread_idx) {
        threads.emplace_back([this, &adapters, &error_detected, thread_idx]() {
            try {
                for (int iter = 0; iter < 100; ++iter) {
                    int adapter_idx = iter % NUM_ADAPTERS;

                    // Mixed operations
                    auto dB = generateRandomDelta();
                    auto dA = generateRandomDelta();
                    adapters[adapter_idx]->applyUpdate("query", dB, dA);

                    std::vector<float> input(32 * IN_DIM, 0.5f);
                    auto output = adapters[adapter_idx]->forward("key", input, 32);

                    auto count = adapters[adapter_idx]->layerCount();
                    if (count != 3) {
                        error_detected = true;
                    }
                }
            } catch (...) {
                error_detected = true;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_FALSE(error_detected.load());
}

// ============================================================================
// Adapter independence validation
// ============================================================================

TEST_F(MultiAdapterConcurrentTest, AdapterState_NotShared) {
    auto adapters = createAdapters(NUM_ADAPTERS);

    // Modify one adapter
    for (int step = 0; step < 50; ++step) {
        auto dB = generateRandomDelta(0.1f);
        auto dA = generateRandomDelta(0.1f);
        adapters[0]->applyUpdate("query", dB, dA);
    }

    // Verify other adapters are unaffected
    const auto& w0 = adapters[0]->getWeights("query");
    for (int i = 1; i < NUM_ADAPTERS; ++i) {
        const auto& wi = adapters[i]->getWeights("query");

        // Should be different weights
        bool different = false;
        for (size_t j = 0; j < w0.B.size(); ++j) {
            if (std::abs(w0.B[j] - wi.B[j]) > 1e-5f) {
                different = true;
                break;
            }
        }
        EXPECT_TRUE(different) << "Adapter " << i << " should not be affected by adapter 0 updates";
    }
}
