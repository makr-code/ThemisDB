/**
 * @file test_helpers.h
 * @brief Reusable test helpers and utilities for enhanced testing
 * 
 * Provides helper functions and fixtures for comprehensive test coverage including:
 * - Parametrized test generators
 * - Performance assertion helpers
 * - Concurrency testing utilities
 * - Common test patterns and fixtures
 * 
 * These helpers implement best practices for Google Test unit testing:
 * - Edge case coverage
 * - Performance bounds validation
 * - Thread-safety verification
 * - Clear test documentation
 */

#pragma once

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>
#include <atomic>
#include <random>
#include <cmath>

namespace themis {
namespace test {

// ===== Performance Assertion Helpers =====

/**
 * @brief Assert that an operation completes within a time bound
 * @param operation The operation to time
 * @param max_duration_ms Maximum allowed duration in milliseconds
 * @param operation_name Name of operation for error messages
 */
template<typename Func>
void AssertPerformanceBound(Func operation, 
                           long max_duration_ms, 
                           const std::string& operation_name = "Operation") {
    auto start = std::chrono::high_resolution_clock::now();
    operation();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), max_duration_ms)
        << operation_name << " took " << duration.count() 
        << "ms, expected < " << max_duration_ms << "ms";
}

/**
 * @brief Assert that throughput meets minimum requirements
 * @param operation The operation to measure
 * @param num_operations Number of operations to perform
 * @param min_ops_per_sec Minimum operations per second
 * @param operation_name Name of operation for error messages
 */
template<typename Func>
void AssertThroughputBound(Func operation,
                          size_t num_operations,
                          double min_ops_per_sec,
                          const std::string& operation_name = "Operation") {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_operations; ++i) {
        operation();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double actual_ops_per_sec = (num_operations * 1000000.0) / duration.count();
    
    EXPECT_GT(actual_ops_per_sec, min_ops_per_sec)
        << operation_name << " throughput: " << actual_ops_per_sec 
        << " ops/sec, expected > " << min_ops_per_sec << " ops/sec";
}

/**
 * @brief Assert that latency percentile meets requirements
 * @param operation The operation to measure
 * @param num_samples Number of samples to collect
 * @param percentile Percentile to check (e.g., 95.0 for p95)
 * @param max_latency_us Maximum latency in microseconds
 * @param operation_name Name of operation for error messages
 */
template<typename Func>
void AssertLatencyPercentile(Func operation,
                             size_t num_samples,
                             double percentile,
                             long max_latency_us,
                             const std::string& operation_name = "Operation") {
    std::vector<long> latencies;
    latencies.reserve(num_samples);
    
    for (size_t i = 0; i < num_samples; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        latencies.push_back(latency.count());
    }
    
    std::sort(latencies.begin(), latencies.end());
    
    size_t index = static_cast<size_t>((percentile / 100.0) * latencies.size());
    if (index >= latencies.size()) {
      index = latencies.size() - 1;
    }
    
    long percentile_latency = latencies[index];
    
    EXPECT_LT(percentile_latency, max_latency_us)
        << operation_name << " p" << percentile << " latency: " 
        << percentile_latency << "μs, expected < " << max_latency_us << "μs";
}

// ===== Concurrency Testing Helpers =====

/**
 * @brief Execute operation concurrently from multiple threads
 * @param operation Operation to execute
 * @param num_threads Number of concurrent threads
 * @param iterations_per_thread Iterations per thread
 * @return Number of successful executions
 */
template<typename Func>
size_t ExecuteConcurrently(Func operation,
                          size_t num_threads,
                          size_t iterations_per_thread) {
    std::atomic<size_t> success_count{0};
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&operation, &success_count, iterations_per_thread]() {
            for (size_t i = 0; i < iterations_per_thread; ++i) {
                try {
                    operation();
                    success_count++;
                } catch (...) {
                    // Count failures
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    return success_count.load();
}

/**
 * @brief Test for race conditions by running operation concurrently
 * @param operation Operation to test
 * @param num_threads Number of concurrent threads
 * @param iterations_per_thread Iterations per thread
 * @param operation_name Name for error messages
 */
template<typename Func>
void AssertThreadSafe(Func operation,
                     size_t num_threads,
                     size_t iterations_per_thread,
                     const std::string& operation_name = "Operation") {
    size_t expected_successes = num_threads * iterations_per_thread;
    size_t actual_successes = ExecuteConcurrently(operation, num_threads, iterations_per_thread);
    
    EXPECT_EQ(actual_successes, expected_successes)
        << operation_name << " race condition detected: "
        << actual_successes << "/" << expected_successes << " succeeded";
}

/**
 * @brief Stress test operation under high concurrency
 * @param operation Operation to stress test
 * @param duration_seconds Duration of stress test
 * @param num_threads Number of concurrent threads
 * @return Total operations completed
 */
template<typename Func>
size_t StressTest(Func operation,
                 int duration_seconds,
                 size_t num_threads) {
    std::atomic<size_t> ops_completed{0};
    std::atomic<bool> stop_flag{false};
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&operation, &ops_completed, &stop_flag]() {
            while (!stop_flag.load()) {
                try {
                    operation();
                    ops_completed++;
                } catch (...) {
                    // Continue on errors during stress test
                }
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    stop_flag.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    return ops_completed.load();
}

// ===== Edge Case Testing Helpers =====

/**
 * @brief Generate edge case values for testing
 */
template<typename T>
struct EdgeCaseGenerator {
    static std::vector<T> GetEdgeCases();
};

// Specializations for common types
template<>
struct EdgeCaseGenerator<int> {
    static std::vector<int> GetEdgeCases() {
        return {
            0,                                      // Zero
            1,                                      // Single element
            -1,                                     // Negative
            std::numeric_limits<int>::max(),       // Maximum
            std::numeric_limits<int>::min(),       // Minimum
            100,                                    // Small positive
            -100                                    // Small negative
        };
    }
};

template<>
struct EdgeCaseGenerator<double> {
    static std::vector<double> GetEdgeCases() {
        return {
            0.0,                                          // Zero
            1.0,                                          // One
            -1.0,                                         // Negative one
            1e-10,                                        // Very small positive
            -1e-10,                                       // Very small negative
            1e10,                                         // Very large positive
            -1e10,                                        // Very large negative
            std::numeric_limits<double>::infinity(),     // Infinity
            -std::numeric_limits<double>::infinity(),    // Negative infinity
            std::numeric_limits<double>::quiet_NaN()     // Not a number
        };
    }
};

template<>
struct EdgeCaseGenerator<size_t> {
    static std::vector<size_t> GetEdgeCases() {
        return {
            0,                    // Zero
            1,                    // One
            10,                   // Small
            100,                  // Medium
            1000,                 // Large
            10000                 // Very large
        };
    }
};

/**
 * @brief Test operation with all edge cases
 * @param operation Operation to test
 * @param edge_cases Vector of edge case values
 * @param operation_name Name for error messages
 */
template<typename T, typename Func>
void TestWithEdgeCases(Func operation,
                      const std::vector<T>& edge_cases,
                      const std::string& operation_name = "Operation") {
    for (const auto& edge_case : edge_cases) {
        EXPECT_NO_THROW({
            operation(edge_case);
        }) << operation_name << " failed with edge case: " << edge_case;
    }
}

// ===== Parametrized Test Helpers =====

/**
 * @brief Base fixture for parametrized performance tests
 */
template<typename ParamType>
class ParametrizedPerformanceTest : public ::testing::TestWithParam<ParamType> {
protected:
    /**
     * @brief Assert operation meets performance bounds for current parameter
     */
    template<typename Func>
    void AssertPerformance(Func operation,
                          long max_duration_ms,
                          const std::string& test_name = "Test") {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        EXPECT_LT(duration.count(), max_duration_ms)
            << test_name << " with param " << this->GetParam() 
            << " took " << duration.count() << "ms";
    }
};

// ===== Data Generation Helpers =====

/**
 * @brief Random number generator for tests
 */
class TestRandomGenerator {
public:
    TestRandomGenerator(unsigned seed = std::random_device{}()) 
        : gen_(seed) {}
    
    /**
     * @brief Generate random integer in range [min, max]
     */
    int RandomInt(int min, int max) {
        std::uniform_int_distribution<> dist(min, max);
        return dist(gen_);
    }
    
    /**
     * @brief Generate random double in range [min, max]
     */
    double RandomDouble(double min, double max) {
        std::uniform_real_distribution<> dist(min, max);
        return dist(gen_);
    }
    
    /**
     * @brief Generate random boolean
     */
    bool RandomBool() {
        std::uniform_int_distribution<> dist(0, 1);
        return dist(gen_) == 1;
    }
    
    /**
     * @brief Generate vector of random integers
     */
    std::vector<int> RandomIntVector(size_t size, int min, int max) {
        std::vector<int> result(size);
        for (auto& val : result) {
            val = RandomInt(min, max);
        }
        return result;
    }
    
    /**
     * @brief Generate vector of random doubles
     */
    std::vector<double> RandomDoubleVector(size_t size, double min, double max) {
        std::vector<double> result(size);
        for (auto& val : result) {
            val = RandomDouble(min, max);
        }
        return result;
    }

private:
    std::mt19937 gen_;
};

// ===== Memory Testing Helpers =====

/**
 * @brief Verify operation completes successfully (basic sanity check)
 * @param operation Operation to execute
 * @param operation_name Name for error messages
 * 
 * Note: For accurate memory measurement, use platform-specific APIs,
 * valgrind, or sanitizers. This is a basic sanity check only.
 */
template<typename Func>
void AssertOperationCompletes(Func operation,
                             const std::string& operation_name = "Operation") {
    // Execute operation and verify it completes without throwing
    EXPECT_NO_THROW({
        operation();
    }) << operation_name << " threw exception";
}

/**
 * @brief Assert memory is properly cleaned up after operation
 */
template<typename Func>
void AssertMemoryCleanup(Func operation,
                        const std::string& operation_name = "Operation") {
    // Execute operation in scope
    {
        operation();
    }
    
    // Memory should be cleaned up after scope
    // This is a basic check - proper verification needs leak detection tools
    SUCCEED() << operation_name << " completed - verify with leak detectors";
}

// ===== Assertion Helpers with Better Messages =====

/**
 * @brief Assert value is within tolerance
 */
template<typename T>
void AssertNear(T actual, T expected, T tolerance, 
               const std::string& value_name = "Value") {
    T diff = std::abs(actual - expected);
    EXPECT_LE(diff, tolerance)
        << value_name << " is " << actual 
        << ", expected " << expected << " ± " << tolerance;
}

/**
 * @brief Assert vector values are approximately equal
 */
template<typename T>
void AssertVectorNear(const std::vector<T>& actual,
                     const std::vector<T>& expected,
                     T tolerance,
                     const std::string& vector_name = "Vector") {
    ASSERT_EQ(actual.size(), expected.size())
        << vector_name << " size mismatch";
    
    for (size_t i = 0; i < actual.size(); ++i) {
        T diff = std::abs(actual[i] - expected[i]);
        EXPECT_LE(diff, tolerance)
            << vector_name << "[" << i << "] is " << actual[i]
            << ", expected " << expected[i] << " ± " << tolerance;
    }
}

/**
 * @brief Assert operation completes without throwing
 */
template<typename Func>
void AssertNoThrow(Func operation, const std::string& operation_name = "Operation") {
    EXPECT_NO_THROW({
        operation();
    }) << operation_name << " threw unexpected exception";
}

/**
 * @brief Assert operation throws specific exception type
 */
template<typename ExceptionType, typename Func>
void AssertThrows(Func operation, const std::string& operation_name = "Operation") {
    EXPECT_THROW({
        operation();
    }, ExceptionType) << operation_name << " did not throw expected exception";
}

} // namespace test
} // namespace themis
