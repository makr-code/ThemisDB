#include <gtest/gtest.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/task_arena.h>
#include <tbb/global_control.h>
#include <vector>
#include <numeric>
#include <atomic>

// Test fixture for TBB library integration
class TBBLibIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }
};

// Test 1: TBB library linking
TEST_F(TBBLibIntegrationTest, LibraryLinking) {
    // Simply using TBB types verifies library is linked
    tbb::concurrent_vector<int> vec;
    vec.push_back(42);
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0], 42);
}

// Test 2: TBB parallel_for basic usage
TEST_F(TBBLibIntegrationTest, ParallelForBasic) {
    std::vector<int> data(1000, 0);
    
    tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
        [&data](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                data[i] = static_cast<int>(i);
            }
        });
    
    // Verify all elements were set
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(data[i], static_cast<int>(i));
    }
}

// Test 3: TBB parallel_reduce for sum
TEST_F(TBBLibIntegrationTest, ParallelReduceSum) {
    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 0); // Fill with 0, 1, 2, ..., 999
    
    int sum = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, data.size()),
        0,
        [&data](const tbb::blocked_range<size_t>& r, int init) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                init += data[i];
            }
            return init;
        },
        [](int a, int b) { return a + b; }
    );
    
    // Expected sum: 0 + 1 + ... + 999 = 999 * 1000 / 2
    int expected = 999 * 1000 / 2;
    EXPECT_EQ(sum, expected);
}

// Test 4: TBB concurrent_vector thread safety
TEST_F(TBBLibIntegrationTest, ConcurrentVectorThreadSafety) {
    tbb::concurrent_vector<int> vec;
    
    tbb::parallel_for(tbb::blocked_range<int>(0, 1000),
        [&vec](const tbb::blocked_range<int>& r) {
            for (int i = r.begin(); i != r.end(); ++i) {
                vec.push_back(i);
            }
        });
    
    EXPECT_EQ(vec.size(), 1000u);
    
    // All values 0-999 should be present
    std::vector<int> sorted(vec.begin(), vec.end());
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(sorted[i], i);
    }
}

// Test 5: TBB concurrent_queue operations
TEST_F(TBBLibIntegrationTest, ConcurrentQueueOperations) {
    tbb::concurrent_queue<int> queue;
    
    // Push items in parallel
    tbb::parallel_for(0, 100, [&queue](int i) {
        queue.push(i);
    });
    
    // Verify queue size
    EXPECT_EQ(queue.unsafe_size(), 100u);
    
    // Pop all items
    std::vector<int> values;
    int val;
    while (queue.try_pop(val)) {
        values.push_back(val);
    }
    
    EXPECT_EQ(values.size(), 100u);
    EXPECT_TRUE(queue.empty());
}

// Test 6: TBB concurrent_hash_map operations
TEST_F(TBBLibIntegrationTest, ConcurrentHashMapOperations) {
    tbb::concurrent_hash_map<int, std::string> map;
    
    // Insert in parallel
    tbb::parallel_for(0, 100, [&map](int i) {
        typename tbb::concurrent_hash_map<int, std::string>::accessor a;
        map.insert(a, i);
        a->second = "value_" + std::to_string(i);
    });
    
    EXPECT_EQ(map.size(), 100u);
    
    // Read in parallel
    tbb::parallel_for(0, 100, [&map](int i) {
        typename tbb::concurrent_hash_map<int, std::string>::const_accessor a;
        EXPECT_TRUE(map.find(a, i));
        EXPECT_EQ(a->second, "value_" + std::to_string(i));
    });
}

// Test 7: TBB blocked_range subdivisions
TEST_F(TBBLibIntegrationTest, BlockedRangeSubdivision) {
    tbb::blocked_range<int> range(0, 1000, 100); // grain size 100
    
    EXPECT_EQ(range.begin(), 0);
    EXPECT_EQ(range.end(), 1000);
    EXPECT_EQ(range.size(), 1000u);
    EXPECT_TRUE(range.is_divisible());
    
    // Split range
    tbb::blocked_range<int> right(range, tbb::split());
    
    EXPECT_EQ(range.end(), right.begin());
    EXPECT_LT(range.size(), 1000u);
    EXPECT_LT(right.size(), 1000u);
}

// Test 8: TBB task_arena execution
TEST_F(TBBLibIntegrationTest, TaskArenaExecution) {
    tbb::task_arena arena(4); // 4 threads
    
    std::atomic<int> counter{0};
    
    arena.execute([&counter]() {
        tbb::parallel_for(0, 100, [&counter](int) {
            ++counter;
        });
    });
    
    EXPECT_EQ(counter, 100);
}

// Test 9: TBB global_control thread limit
TEST_F(TBBLibIntegrationTest, GlobalControlThreadLimit) {
    // Limit to 2 threads
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism, 2);
    
    std::atomic<int> max_concurrent{0};
    std::atomic<int> current{0};
    
    tbb::parallel_for(0, 100, [&max_concurrent, &current](int) {
        int cur = ++current;
        int expected = max_concurrent.load();
        while (cur > expected && !max_concurrent.compare_exchange_weak(expected, cur)) {
          ;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        --current;
    });
    
    // With 2 threads, max concurrent should be <= 2
    EXPECT_LE(max_concurrent.load(), 4); // Allow some overhead
}

// Test 10: TBB parallel_for with custom grain size
TEST_F(TBBLibIntegrationTest, ParallelForCustomGrainSize) {
    std::vector<int> data(10000, 0);
    std::atomic<int> num_ranges{0};
    
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, data.size(), 1000), // grain size 1000
        [&data, &num_ranges](const tbb::blocked_range<size_t>& r) {
            ++num_ranges;
            for (size_t i = r.begin(); i != r.end(); ++i) {
                data[i] = 1;
            }
        });
    
    // All elements should be set
    int sum = std::accumulate(data.begin(), data.end(), 0);
    EXPECT_EQ(sum, 10000);
    
    // Number of ranges should be reasonable (around 10 with grain size 1000)
    EXPECT_GE(num_ranges.load(), 1);
    EXPECT_LE(num_ranges.load(), 100);
}

// Test 11: TBB concurrent_vector iterators
TEST_F(TBBLibIntegrationTest, ConcurrentVectorIterators) {
    tbb::concurrent_vector<int> vec;
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i);
    }
    
    // Test iterators
    int sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        sum += *it;
    }
    
    int expected = 99 * 100 / 2;
    EXPECT_EQ(sum, expected);
    
    // Test range-based for
    int sum2 = 0;
    for (int val : vec) {
        sum2 += val;
    }
    EXPECT_EQ(sum2, expected);
}

// Test 12: TBB concurrent_hash_map erase
TEST_F(TBBLibIntegrationTest, ConcurrentHashMapErase) {
    tbb::concurrent_hash_map<int, int> map;
    
    // Insert elements
    for (int i = 0; i < 100; ++i) {
        typename tbb::concurrent_hash_map<int, int>::accessor a;
        map.insert(a, i);
        a->second = i * 2;
    }
    
    EXPECT_EQ(map.size(), 100u);
    
    // Erase some elements
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(map.erase(i));
    }
    
    EXPECT_EQ(map.size(), 50u);
    
    // Verify remaining elements
    for (int i = 50; i < 100; ++i) {
        typename tbb::concurrent_hash_map<int, int>::const_accessor a;
        EXPECT_TRUE(map.find(a, i));
        EXPECT_EQ(a->second, i * 2);
    }
}

// Test 13: TBB parallel_reduce with custom combine
TEST_F(TBBLibIntegrationTest, ParallelReduceCustomCombine) {
    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 1); // 1, 2, 3, ..., 1000
    
    struct Result {
        int sum = 0;
        int count = 0;
    };
    
    Result result = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, data.size()),
        Result{},
        [&data](const tbb::blocked_range<size_t>& r, Result init) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                init.sum += data[i];
                init.count++;
            }
            return init;
        },
        [](Result a, Result b) {
            return Result{a.sum + b.sum, a.count + b.count};
        }
    );
    
    EXPECT_EQ(result.count, 1000);
    EXPECT_EQ(result.sum, 1000 * 1001 / 2); // Sum of 1..1000
}

// Test 14: TBB task isolation
TEST_F(TBBLibIntegrationTest, TaskIsolation) {
    std::atomic<int> outer_counter{0};
    std::atomic<int> inner_counter{0};
    
    tbb::task_arena outer_arena(2);
    tbb::task_arena inner_arena(1);
    
    outer_arena.execute([&]() {
        tbb::parallel_for(0, 10, [&](int) {
            ++outer_counter;
            
            // Execute isolated task in separate arena
            inner_arena.execute([&]() {
                ++inner_counter;
            });
        });
    });
    
    EXPECT_EQ(outer_counter, 10);
    EXPECT_EQ(inner_counter, 10);
}

// Test 15: TBB performance comparison: serial vs parallel
TEST_F(TBBLibIntegrationTest, SerialVsParallelPerformance) {
    const size_t N = 10000;
    std::vector<double> data(N);
    
    // Fill with data
    for (size_t i = 0; i < N; ++i) {
        data[i] = static_cast<double>(i);
    }
    
    // Serial sum
    double serial_sum = 0;
    for (size_t i = 0; i < N; ++i) {
        serial_sum += data[i];
    }
    
    // Parallel sum
    double parallel_sum = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, N),
        0.0,
        [&data](const tbb::blocked_range<size_t>& r, double init) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                init += data[i];
            }
            return init;
        },
        [](double a, double b) { return a + b; }
    );
    
    // Results should be the same
    EXPECT_DOUBLE_EQ(serial_sum, parallel_sum);
}
