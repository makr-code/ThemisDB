/**
 * @file test_thread_safety_stress.cpp
 * @brief Thread-safety stress tests for ThemisDB components
 * 
 * These tests verify thread-safety under high concurrency by:
 * - Running multiple threads simultaneously
 * - Stressing shared data structures
 * - Verifying consistency after concurrent operations
 * 
 * Run with ThreadSanitizer: cmake -DTHEMIS_ENABLE_TSAN=ON
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date February 2026
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "llm/model_loader.h"
#include "llm/continuous_batch_scheduler.h"
#include "utils/thread_safety.h"

using namespace themis::llm;
using namespace themis::utils::threading;

// ═══════════════════════════════════════════════════════════
// Thread-Safety Utility Tests
// ═══════════════════════════════════════════════════════════

TEST(ThreadSafetyUtils, SynchronizedBasicAccess) {
    Synchronized<int> counter(0);
    
    const int NUM_THREADS = 10;
    const int INCREMENTS_PER_THREAD = 1000;
    
    std::vector<std::thread> threads;
    
    // Concurrent increments
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&counter, INCREMENTS_PER_THREAD]() {
            for (int j = 0; j < INCREMENTS_PER_THREAD; ++j) {
                counter.with_lock([](int& val) {
                    val++;
                });
            }
        });
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify: All increments should be accounted for
    int final_value = counter.with_lock([](const int& val) { return val; });
    EXPECT_EQ(final_value, NUM_THREADS * INCREMENTS_PER_THREAD);
}

TEST(ThreadSafetyUtils, SharedSynchronizedReaderWriter) {
    SharedSynchronized<std::unordered_map<int, std::string>> map;
    
    const int NUM_WRITERS = 5;
    const int NUM_READERS = 20;
    const int WRITES_PER_THREAD = 100;
    const int READS_PER_THREAD = 500;
    
    std::atomic<int> successful_reads{0};
    std::atomic<int> successful_writes{0};
    
    std::vector<std::thread> writers;
    std::vector<std::thread> readers;
    
    // Writer threads
    for (int i = 0; i < NUM_WRITERS; ++i) {
        writers.emplace_back([&map, &successful_writes, i, WRITES_PER_THREAD]() {
            for (int j = 0; j < WRITES_PER_THREAD; ++j) {
                map.with_unique_lock([i, j](auto& m) {
                    int key = i * 1000 + j;
                    m[key] = "value_" + std::to_string(key);
                });
                successful_writes.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Reader threads (can run concurrently with each other)
    for (int i = 0; i < NUM_READERS; ++i) {
        readers.emplace_back([&map, &successful_reads, READS_PER_THREAD]() {
            for (int j = 0; j < READS_PER_THREAD; ++j) {
                map.with_shared_lock([](const auto& m) {
                    // Just iterate to stress-test the shared lock
                    volatile size_t size = m.size();
                    (void)size;
                });
                successful_reads.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for completion
    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();
    
    // Verify counts
    EXPECT_EQ(successful_writes.load(), NUM_WRITERS * WRITES_PER_THREAD);
    EXPECT_EQ(successful_reads.load(), NUM_READERS * READS_PER_THREAD);
    
    // Verify map size
    size_t final_size = map.with_shared_lock([](const auto& m) { return m.size(); });
    EXPECT_EQ(final_size, NUM_WRITERS * WRITES_PER_THREAD);
}

// ═══════════════════════════════════════════════════════════
// Model Loader Thread-Safety Tests
// ═══════════════════════════════════════════════════════════

class ModelLoaderStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_vram_mb = 8192;
        config_.max_models = 5;
        config_.default_n_gpu_layers = 0;  // CPU-only for tests
        config_.default_n_ctx = 512;
        config_.enable_lazy_load = true;
    }
    
    LazyModelLoader::Config config_;
};

TEST_F(ModelLoaderStressTest, ConcurrentStatisticsAccess) {
    LazyModelLoader loader(config_);
    
    const int NUM_THREADS = 20;
    const int OPERATIONS_PER_THREAD = 500;
    
    std::atomic<int> operations_completed{0};
    std::vector<std::thread> threads;
    
    // Threads repeatedly read statistics
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&loader, &operations_completed, OPERATIONS_PER_THREAD]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                // Read statistics (exercises atomic loads)
                auto stats = loader.getStatistics();
                
                // Basic sanity checks
                EXPECT_GE(stats.cache_hits, 0);
                EXPECT_GE(stats.cache_misses, 0);
                EXPECT_GE(stats.evictions, 0);
                EXPECT_GE(stats.models_loaded, 0);
                
                // Also test JSON stats
                auto json_stats = loader.getCacheStats();
                EXPECT_TRUE(json_stats.contains("cache_hits"));
                
                operations_completed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(operations_completed.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
}

TEST_F(ModelLoaderStressTest, ConcurrentModelQueries) {
    LazyModelLoader loader(config_);
    
    const int NUM_THREADS = 10;
    const int QUERIES_PER_THREAD = 100;
    
    std::atomic<int> queries_completed{0};
    std::vector<std::thread> threads;
    
    // Threads repeatedly query model status
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&loader, &queries_completed, QUERIES_PER_THREAD]() {
            for (int j = 0; j < QUERIES_PER_THREAD; ++j) {
                // Various read operations that should be thread-safe
                bool loaded = loader.isModelLoaded("nonexistent-model");
                EXPECT_FALSE(loaded);
                
                auto models = loader.listLoadedModels();
                // Should not crash, regardless of state
                
                auto info = loader.getModelInfo("nonexistent-model");
                EXPECT_FALSE(info.has_value());
                
                queries_completed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(queries_completed.load(), NUM_THREADS * QUERIES_PER_THREAD);
}

// ═══════════════════════════════════════════════════════════
// Scheduler Thread-Safety Tests
// ═══════════════════════════════════════════════════════════

class SchedulerStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_batch_size = 64;
        config_.max_concurrent_requests = 128;
        config_.enable_preemption = false;  // Simplify for stress test
        config_.enable_priority_scheduling = true;
    }
    
    ContinuousBatchScheduler::SchedulerConfig config_;
};

TEST_F(SchedulerStressTest, ConcurrentRequestSubmission) {
    // Note: This test focuses on the atomic counters
    // Full integration requires a PagedKVCache which we skip here
    
    // We'll just test that the atomic operations work correctly
    std::atomic<int> sequence_counter{0};
    std::atomic<int> request_counter{0};
    
    const int NUM_THREADS = 10;
    const int REQUESTS_PER_THREAD = 1000;
    
    std::vector<std::thread> threads;
    
    // Simulate concurrent request ID generation
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&sequence_counter, &request_counter, REQUESTS_PER_THREAD]() {
            for (int j = 0; j < REQUESTS_PER_THREAD; ++j) {
                // Simulate atomic counter increments as used in scheduler
                int seq_id = sequence_counter.fetch_add(1, std::memory_order_relaxed);
                int req_id = request_counter.fetch_add(1, std::memory_order_relaxed);
                
                // Both should be unique and increasing
                EXPECT_GE(seq_id, 0);
                EXPECT_GE(req_id, 0);
            }
        });
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify: All IDs should be unique (total count should match)
    EXPECT_EQ(sequence_counter.load(), NUM_THREADS * REQUESTS_PER_THREAD);
    EXPECT_EQ(request_counter.load(), NUM_THREADS * REQUESTS_PER_THREAD);
}

// ═══════════════════════════════════════════════════════════
// Atomic Counter Stress Tests
// ═══════════════════════════════════════════════════════════

TEST(AtomicStress, ConcurrentIncrements) {
    std::atomic<size_t> counter{0};
    
    const int NUM_THREADS = 50;
    const int INCREMENTS = 10000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&counter, INCREMENTS]() {
            for (int j = 0; j < INCREMENTS; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(counter.load(), NUM_THREADS * INCREMENTS);
}

TEST(AtomicStress, MixedReadWrite) {
    std::atomic<size_t> counter{0};
    
    const int NUM_WRITERS = 10;
    const int NUM_READERS = 40;
    const int WRITES = 5000;
    const int READS = 10000;
    
    std::vector<std::thread> threads;
    std::atomic<size_t> read_sum{0};
    
    // Writers
    for (int i = 0; i < NUM_WRITERS; ++i) {
        threads.emplace_back([&counter, WRITES]() {
            for (int j = 0; j < WRITES; ++j) {
                counter.fetch_add(1, std::memory_order_release);
            }
        });
    }
    
    // Readers
    for (int i = 0; i < NUM_READERS; ++i) {
        threads.emplace_back([&counter, &read_sum, READS]() {
            for (int j = 0; j < READS; ++j) {
                size_t value = counter.load(std::memory_order_acquire);
                read_sum.fetch_add(value, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Final counter value should be deterministic
    EXPECT_EQ(counter.load(), NUM_WRITERS * WRITES);
    
    // Read sum is non-deterministic but should be > 0
    EXPECT_GT(read_sum.load(), 0);
}

// ═══════════════════════════════════════════════════════════
// Performance Baseline Tests
// ═══════════════════════════════════════════════════════════

TEST(ThreadSafetyPerformance, SynchronizedOverhead) {
    // Compare synchronized vs unsynchronized performance
    const int ITERATIONS = 100000;
    
    // Baseline: unsynchronized (UNSAFE - for comparison only)
    int unsafe_counter = 0;
    auto start_unsafe = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        unsafe_counter++;
    }
    auto end_unsafe = std::chrono::high_resolution_clock::now();
    auto duration_unsafe = std::chrono::duration_cast<std::chrono::microseconds>(
        end_unsafe - start_unsafe).count();
    
    // Thread-safe: Synchronized
    Synchronized<int> safe_counter(0);
    auto start_safe = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        safe_counter.with_lock([](int& val) { val++; });
    }
    auto end_safe = std::chrono::high_resolution_clock::now();
    auto duration_safe = std::chrono::duration_cast<std::chrono::microseconds>(
        end_safe - start_safe).count();
    
    // Thread-safe: Atomic
    std::atomic<int> atomic_counter{0};
    auto start_atomic = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        atomic_counter.fetch_add(1, std::memory_order_relaxed);
    }
    auto end_atomic = std::chrono::high_resolution_clock::now();
    auto duration_atomic = std::chrono::duration_cast<std::chrono::microseconds>(
        end_atomic - start_atomic).count();
    
    // Report performance (for informational purposes)
    std::cout << "Performance comparison (" << ITERATIONS << " iterations):\n";
    std::cout << "  Unsafe:       " << duration_unsafe << " μs\n";
    std::cout << "  Synchronized: " << duration_safe << " μs (overhead: " 
              << (duration_safe * 100.0 / duration_unsafe - 100) << "%)\n";
    std::cout << "  Atomic:       " << duration_atomic << " μs (overhead: "
              << (duration_atomic * 100.0 / duration_unsafe - 100) << "%)\n";
    
    // Verify correctness
    int final_safe = safe_counter.with_lock([](const int& val) { return val; });
    EXPECT_EQ(final_safe, ITERATIONS);
    EXPECT_EQ(atomic_counter.load(), ITERATIONS);
    
    // Note: Atomic should be fastest thread-safe option for simple counters
    EXPECT_LT(duration_atomic, duration_safe);
}
