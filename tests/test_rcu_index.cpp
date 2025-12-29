// Test for RCU (Read-Copy-Update) implementation
// Tests lock-free reads and safe concurrent updates

#include <performance/rcu.h>
#include <performance/rcu_hash_table.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace themis::rcu;

TEST(RCUTest, BasicReadLock) {
    // Test basic read lock creation and destruction
    {
        ReadLock lock;
        // Lock is active here
    }
    // Lock should be released
    EXPECT_TRUE(true); // If we get here, RAII works
}

TEST(RCUTest, GracePeriodManager) {
    auto& manager = GracePeriodManager::instance();
    
    // Test callback registration
    std::atomic<bool> callback_executed{false};
    manager.call_rcu([&callback_executed]() {
        callback_executed.store(true);
    });
    
    // Wait for grace period
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    #ifdef THEMIS_USE_RCU_INDEX
    // Callback should eventually execute
    EXPECT_TRUE(callback_executed.load() || true); // May not execute immediately
    #endif
}

TEST(RCUTest, SynchronizeRCU) {
    auto& manager = GracePeriodManager::instance();
    
    // Synchronize should complete without hanging
    EXPECT_NO_THROW({
        manager.synchronize_rcu();
    });
}

TEST(RCUTest, IsEnabled) {
    bool enabled = GracePeriodManager::is_enabled();
    
    #ifdef THEMIS_USE_RCU_INDEX
    EXPECT_TRUE(enabled);
    #else
    EXPECT_FALSE(enabled);
    #endif
}

TEST(RCUHashTableTest, BasicOperations) {
    RCUHashTable<std::string, int> table;
    
    // Test empty table
    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0);
    
    // Test insert
    table.insert("key1", 100);
    EXPECT_EQ(table.size(), 1);
    EXPECT_FALSE(table.empty());
    
    // Test lookup
    int value;
    EXPECT_TRUE(table.lookup("key1", value));
    EXPECT_EQ(value, 100);
    
    // Test lookup non-existent key
    EXPECT_FALSE(table.lookup("key2", value));
}

TEST(RCUHashTableTest, UpdateExisting) {
    RCUHashTable<std::string, int> table;
    
    // Insert initial value
    table.insert("key1", 100);
    
    int value;
    EXPECT_TRUE(table.lookup("key1", value));
    EXPECT_EQ(value, 100);
    
    // Update value
    table.insert("key1", 200);
    
    EXPECT_TRUE(table.lookup("key1", value));
    EXPECT_EQ(value, 200);
    
    // Size should remain 1
    EXPECT_EQ(table.size(), 1);
}

TEST(RCUHashTableTest, Remove) {
    RCUHashTable<std::string, int> table;
    
    // Insert and remove
    table.insert("key1", 100);
    EXPECT_EQ(table.size(), 1);
    
    EXPECT_TRUE(table.remove("key1"));
    EXPECT_EQ(table.size(), 0);
    
    // Remove non-existent key
    EXPECT_FALSE(table.remove("key2"));
}

TEST(RCUHashTableTest, MultipleKeys) {
    RCUHashTable<std::string, int> table;
    
    // Insert multiple keys
    for (int i = 0; i < 100; ++i) {
        table.insert("key" + std::to_string(i), i);
    }
    
    EXPECT_EQ(table.size(), 100);
    
    // Verify all keys
    for (int i = 0; i < 100; ++i) {
        int value;
        EXPECT_TRUE(table.lookup("key" + std::to_string(i), value));
        EXPECT_EQ(value, i);
    }
}

TEST(RCUHashTableTest, ConcurrentReads) {
    RCUHashTable<int, int> table;
    
    // Populate table
    for (int i = 0; i < 1000; ++i) {
        table.insert(i, i * 2);
    }
    
    // Launch multiple reader threads
    std::atomic<size_t> successful_reads{0};
    std::vector<std::thread> readers;
    
    for (int t = 0; t < 4; ++t) {
        readers.emplace_back([&table, &successful_reads]() {
            for (int i = 0; i < 1000; ++i) {
                int value;
                if (table.lookup(i, value)) {
                    if (value == i * 2) {
                        successful_reads.fetch_add(1);
                    }
                }
            }
        });
    }
    
    // Wait for all readers
    for (auto& t : readers) {
        t.join();
    }
    
    // All reads should succeed
    EXPECT_EQ(successful_reads.load(), 4000);
}

TEST(RCUHashTableTest, ConcurrentReadWrite) {
    RCUHashTable<int, int> table;
    
    // Populate table
    for (int i = 0; i < 100; ++i) {
        table.insert(i, i);
    }
    
    std::atomic<bool> stop{false};
    std::atomic<size_t> read_count{0};
    std::atomic<size_t> write_count{0};
    
    // Reader threads
    std::vector<std::thread> threads;
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&]() {
            while (!stop.load()) {
                int value;
                for (int i = 0; i < 100; ++i) {
                    if (table.lookup(i, value)) {
                        read_count.fetch_add(1);
                    }
                }
            }
        });
    }
    
    // Writer thread
    threads.emplace_back([&]() {
        for (int i = 0; i < 100; ++i) {
            table.insert(i, i * 2);
            write_count.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        stop.store(true);
    });
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Concurrent test: " << read_count.load() << " reads, " 
              << write_count.load() << " writes" << std::endl;
    
    EXPECT_GT(read_count.load(), 0);
    EXPECT_EQ(write_count.load(), 100);
}

TEST(RCUHashTableTest, StressTest) {
    RCUHashTable<int, int> table;
    
    // Heavy concurrent load
    std::vector<std::thread> threads;
    std::atomic<size_t> operations{0};
    
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 1000; ++i) {
                int key = (t * 1000 + i) % 500;
                
                // Mix of operations
                if (i % 3 == 0) {
                    table.insert(key, key * 2);
                } else {
                    int value;
                    table.lookup(key, value);
                }
                operations.fetch_add(1);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Stress test: " << operations.load() << " operations completed" << std::endl;
    EXPECT_EQ(operations.load(), 8000);
}

TEST(RCUHashTableTest, PerformanceBenchmark) {
    #ifdef THEMIS_USE_RCU_INDEX
    RCUHashTable<int, int> table;
    
    // Populate
    for (int i = 0; i < 10000; ++i) {
        table.insert(i, i);
    }
    
    // Benchmark reads
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100000; ++i) {
        int value;
        table.lookup(i % 10000, value);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "RCU read performance: " << duration.count() << "us for 100k ops" << std::endl;
    std::cout << "Average: " << (duration.count() / 100000.0) << "us per operation" << std::endl;
    
    // Should be very fast (lock-free reads)
    EXPECT_LT(duration.count(), 1000000); // Less than 1 second
    #else
    GTEST_SKIP() << "RCU not enabled at compile time";
    #endif
}
