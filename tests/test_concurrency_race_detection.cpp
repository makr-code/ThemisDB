/**
 * @file test_concurrency_race_detection.cpp
 * @brief Race condition and concurrency tests for ThemisDB
 * 
 * Tests concurrent access patterns and race conditions using ThreadSanitizer-compatible code.
 * Coverage areas:
 * - Multi-threaded transaction conflicts
 * - Concurrent index access
 * - Shared cache contention
 * - Lock-free data structure races
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <random>

using namespace std::chrono_literals;

namespace themis {
namespace test {

/**
 * @brief Test concurrent writes to shared counter
 * Detects race conditions in atomic operations
 */
TEST(ConcurrencyRaceTest, AtomicCounterIncrement) {
    constexpr int NUM_THREADS = 10;
    constexpr int INCREMENTS_PER_THREAD = 1000;
    
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < INCREMENTS_PER_THREAD; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(counter.load(), NUM_THREADS * INCREMENTS_PER_THREAD);
}

/**
 * @brief Test concurrent map access with mutex
 * Validates proper locking patterns
 */
TEST(ConcurrencyRaceTest, ConcurrentMapAccess) {
    constexpr int NUM_THREADS = 8;
    constexpr int OPS_PER_THREAD = 500;
    
    std::map<int, int> shared_map;
    std::mutex map_mutex = {};
    std::atomic<int> insert_count{0};
    std::atomic<int> read_count{0};
    
    std::vector<std::thread> threads;
    
    // Mix of readers and writers
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            std::random_device rd = {};
            std::mt19937 gen(rd() + thread_id);
            std::uniform_int_distribution<> dis(0, 1000);
            
            for (int j = 0; j < OPS_PER_THREAD; ++j) {
                int key = dis(gen);
                
                if (j % 2 == 0) {
                    // Write operation
                    std::lock_guard<std::mutex> lock(map_mutex);
                    shared_map[key] = thread_id;
                    insert_count.fetch_add(1, std::memory_order_relaxed);
                } else {
                    // Read operation
                    std::lock_guard<std::mutex> lock(map_mutex);
                    auto it = shared_map.find(key);
                    if (it != shared_map.end()) {
                        volatile int val = it->second;
                        (void)val;
                    }
                    read_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(insert_count + read_count, NUM_THREADS * OPS_PER_THREAD);
    EXPECT_GT(shared_map.size(), 0);
}

/**
 * @brief Test transaction-like concurrent operations
 * Simulates MVCC concurrent transaction patterns
 */
TEST(ConcurrencyRaceTest, ConcurrentTransactionSimulation) {
    constexpr int NUM_THREADS = 6;
    constexpr int TRANSACTIONS = 100;
    
    struct TransactionLog {
        std::atomic<int> committed{0};
        std::atomic<int> aborted{0};
        std::mutex mutex = {};
        std::vector<int> log;
    };
    
    TransactionLog tx_log;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&tx_log, thread_id = i]() {
            for (int tx_id = 0; tx_id < TRANSACTIONS; ++tx_id) {
                // Simulate transaction work
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                
                // Commit with 90% success rate
                if ((thread_id + tx_id) % 10 != 0) {
                    std::lock_guard<std::mutex> lock(tx_log.mutex);
                    tx_log.log.push_back(thread_id * 1000 + tx_id);
                    tx_log.committed.fetch_add(1, std::memory_order_release);
                } else {
                    tx_log.aborted.fetch_add(1, std::memory_order_release);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int total = tx_log.committed.load() + tx_log.aborted.load();
    EXPECT_EQ(total, NUM_THREADS * TRANSACTIONS);
    EXPECT_EQ(tx_log.log.size(), tx_log.committed.load());
}

/**
 * @brief Test producer-consumer pattern
 * Validates thread-safe queue operations
 */
TEST(ConcurrencyRaceTest, ProducerConsumerPattern) {
    constexpr int NUM_PRODUCERS = 4;
    constexpr int NUM_CONSUMERS = 4;
    constexpr int ITEMS_PER_PRODUCER = 200;
    
    std::vector<int> queue;
    std::mutex queue_mutex = {};
    std::condition_variable cv = {};
    std::atomic<bool> done{false};
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    
    std::vector<std::thread> threads;
    
    // Producers
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        threads.emplace_back([&, producer_id = i]() {
            for (int j = 0; j < ITEMS_PER_PRODUCER; ++j) {
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    queue.push_back(producer_id * 10000 + j);
                    produced.fetch_add(1, std::memory_order_release);
                }
                cv.notify_one();
                std::this_thread::sleep_for(std::chrono::microseconds(5));
            }
        });
    }
    
    // Consumers
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        threads.emplace_back([&]() {
            while (!done.load(std::memory_order_acquire)) {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait_for(lock, 10ms, [&]() { 
                    return !queue.empty() || done.load(std::memory_order_acquire); 
                });
                
                if (!queue.empty()) {
                    volatile int item = queue.back();
                    (void)item;
                    queue.pop_back();
                    consumed.fetch_add(1, std::memory_order_release);
                }
            }
        });
    }
    
    // Wait for all producers
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        threads[i].join();
    }
    
    // Signal consumers to finish
    done.store(true, std::memory_order_release);
    cv.notify_all();
    
    // Wait for all consumers
    for (int i = NUM_PRODUCERS; i < NUM_PRODUCERS + NUM_CONSUMERS; ++i) {
        threads[i].join();
    }
    
    EXPECT_EQ(produced.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    EXPECT_LE(consumed.load(), produced.load());
}

/**
 * @brief Test concurrent read-write lock patterns
 * Multiple readers, single writer scenarios
 */
TEST(ConcurrencyRaceTest, ReaderWriterLock) {
    constexpr int NUM_READERS = 8;
    constexpr int NUM_WRITERS = 2;
    constexpr int OPERATIONS = 100;
    
    std::shared_mutex rw_mutex = {};
    int shared_value = 0;
    std::atomic<int> read_operations{0};
    std::atomic<int> write_operations{0};
    
    std::vector<std::thread> threads;
    
    // Readers
    for (int i = 0; i < NUM_READERS; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < OPERATIONS; ++j) {
                std::shared_lock<std::shared_mutex> lock(rw_mutex);
                volatile int val = shared_value;
                (void)val;
                read_operations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Writer threads
    for (int i = 0; i < NUM_WRITERS; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < OPERATIONS; ++j) {
                std::unique_lock<std::shared_mutex> lock(rw_mutex);
                ++shared_value;
                write_operations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(20));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(shared_value, NUM_WRITERS * OPERATIONS);
    EXPECT_EQ(read_operations.load(), NUM_READERS * OPERATIONS);
    EXPECT_EQ(write_operations.load(), NUM_WRITERS * OPERATIONS);
}

/**
 * @brief Test memory ordering and synchronization
 * Validates acquire-release semantics
 */
TEST(ConcurrencyRaceTest, MemoryOrderingSynchronization) {
    constexpr int NUM_ITERATIONS = 1000;
    
    std::atomic<bool> ready{false};
    std::atomic<int> data{0};
    int result = 0;
    
    std::thread writer([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.store(i, std::memory_order_relaxed);
            ready.store(true, std::memory_order_release);
            
            // Wait for reader
            while (ready.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
        }
    });
    
    std::thread reader([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            // Wait for data
            while (!ready.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            result = data.load(std::memory_order_relaxed);
            ready.store(false, std::memory_order_release);
        }
    });
    
    writer.join();
    reader.join();
    
    EXPECT_EQ(result, NUM_ITERATIONS - 1);
}

/**
 * @brief Test concurrent access to shared cache-like structure
 * Simulates cache contention scenarios
 */
TEST(ConcurrencyRaceTest, ConcurrentCacheAccess) {
    constexpr int NUM_THREADS = 10;
    constexpr int CACHE_SIZE = 100;
    constexpr int OPERATIONS = 500;
    
    struct CacheEntry {
        std::mutex mutex;
        int value = 0;
        int access_count = 0;
    };
    
    std::vector<CacheEntry> cache(CACHE_SIZE);
    std::atomic<int> total_hits{0};
    std::atomic<int> total_updates{0};
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            std::random_device rd = {};
            std::mt19937 gen(rd() + thread_id);
            std::uniform_int_distribution<> cache_dis(0, CACHE_SIZE - 1);
            std::uniform_int_distribution<> op_dis(0, 9);
            
            for (int j = 0; j < OPERATIONS; ++j) {
                int cache_idx = cache_dis(gen);
                int operation = op_dis(gen);
                
                if (operation < 7) {
                    // Read operation (70%)
                    std::lock_guard<std::mutex> lock(cache[cache_idx].mutex);
                    cache[cache_idx].access_count++;
                    volatile int val = cache[cache_idx].value;
                    (void)val;
                    total_hits.fetch_add(1, std::memory_order_relaxed);
                } else {
                    // Write operation (30%)
                    std::lock_guard<std::mutex> lock(cache[cache_idx].mutex);
                    cache[cache_idx].value = thread_id * 1000 + j;
                    cache[cache_idx].access_count++;
                    total_updates.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_hits + total_updates, NUM_THREADS * OPERATIONS);
    
    // Verify cache consistency
    int total_accesses = 0;
    for (const auto& entry : cache) {
        total_accesses += entry.access_count;
    }
    EXPECT_EQ(total_accesses, NUM_THREADS * OPERATIONS);
}

/**
 * @brief Test lock-free stack operations
 * Validates lock-free data structure correctness
 */
TEST(ConcurrencyRaceTest, LockFreeStackOperations) {
    constexpr int NUM_THREADS = 8;
    constexpr int PUSH_OPS = 200;
    
    struct Node {
        int value = 0;
        std::atomic<Node*> next;
        Node(int v) : value(v), next(nullptr) {}
    };
    
    std::atomic<Node*> head{nullptr};
    std::atomic<int> push_count{0};
    std::atomic<int> pop_count{0};
    
    std::vector<std::thread> threads;
    
    // Push threads
    for (int i = 0; i < NUM_THREADS / 2; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < PUSH_OPS; ++j) {
                Node* new_node = new Node(thread_id * 1000 + j);
                Node* old_head = head.load(std::memory_order_acquire);
                
                do {
                    new_node->next.store(old_head, std::memory_order_relaxed);
                } while (!head.compare_exchange_weak(old_head, new_node,
                                                      std::memory_order_release,
                                                      std::memory_order_acquire));
                
                push_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for pushes to complete
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    threads.clear();
    
    // Pop threads
    for (int i = 0; i < NUM_THREADS / 2; ++i) {
        threads.emplace_back([&]() {
            int local_pops = 0;
            while (true) {
                Node* old_head = head.load(std::memory_order_acquire);
                if (!old_head) {
                  break;
                }
                
                Node* new_head = old_head->next.load(std::memory_order_relaxed);
                if (head.compare_exchange_weak(old_head, new_head,
                                               std::memory_order_release,
                                               std::memory_order_acquire)) {
                    delete old_head;
                    local_pops++;
                }
            }
            pop_count.fetch_add(local_pops, std::memory_order_relaxed);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(push_count.load(), (NUM_THREADS / 2) * PUSH_OPS);
    EXPECT_LE(pop_count.load(), push_count.load());
    
    // Cleanup remaining nodes
    Node* current = head.load();
    while (current) {
        Node* next = current->next.load();
        delete current;
        current = next;
    }
}

} // namespace test
} // namespace themis
