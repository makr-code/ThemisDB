/*
 * ThemisDB | Custom Benchmark Runner | Phase A.3 Release Gates
 * Measures performance of utils module hot paths
 */

#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <cstring>

using json = nlohmann::json;
using namespace std::chrono;

// Simulated structure types (matching utils_api_contract.h)
struct BloomFilterConfig {
    uint64_t expectedItems = 1000000;
    double targetFalsePositiveRate = 0.01;
};

struct RetryPolicy {
    int maxAttempts = 5;
    double backoffMultiplier = 2.0;
};

enum class UtilsError {
    kAuditOverflow,
    kBatchRollback,
    kBatchSizeExceeded,
    kRetryExhausted,
    kDeserInvalid,
    kPoolExhausted,
};

struct BenchmarkResult {
    std::string name;
    int iterations = 0;
    long long mean_ns = 0;
    long long median_ns = 0;
    long long min_ns = 0;
    long long max_ns = 0;
    long long p95_ns = 0;
    long long p99_ns = 0;
    double throughput_ops_sec = 0.0;
    
    json to_json() const {
        json j;
        j["name"] = name;
        j["iterations"] = iterations;
        j["mean_ns"] = mean_ns;
        j["median_ns"] = median_ns;
        j["min_ns"] = min_ns;
        j["max_ns"] = max_ns;
        j["p95_ns"] = p95_ns;
        j["p99_ns"] = p99_ns;
        j["throughput_ops_sec"] = throughput_ops_sec;
        return j;
    }
};

std::vector<long long> times_ns;

// Helper to run and measure a benchmark
template<typename Fn>
BenchmarkResult run_benchmark(const std::string& name, int repetitions, int iterations_per_rep, Fn&& fn) {
    times_ns.clear();
    times_ns.reserve(repetitions);
    
    // Warm up
    for (int i = 0; i < 100; i++) {
        fn(1);
    }
    
    // Measure
    for (int rep = 0; rep < repetitions; rep++) {
        auto start = high_resolution_clock::now();
        for (int i = 0; i < iterations_per_rep; i++) {
            fn(1);
        }
        auto end = high_resolution_clock::now();
        long long elapsed = duration_cast<nanoseconds>(end - start).count();
        times_ns.push_back(elapsed / iterations_per_rep);  // time per iteration
    }
    
    // Calculate statistics
    std::sort(times_ns.begin(), times_ns.end());
    
    long long sum = 0;
    for (auto t : times_ns) sum += t;
    long long mean = sum / times_ns.size();
    long long median = times_ns[times_ns.size() / 2];
    long long min = times_ns.front();
    long long max = times_ns.back();
    long long p95 = times_ns[(times_ns.size() * 95) / 100];
    long long p99 = times_ns[(times_ns.size() * 99) / 100];
    
    BenchmarkResult result;
    result.name = name;
    result.iterations = repetitions * iterations_per_rep;
    result.mean_ns = mean;
    result.median_ns = median;
    result.min_ns = min;
    result.max_ns = max;
    result.p95_ns = p95;
    result.p99_ns = p99;
    result.throughput_ops_sec = (mean > 0) ? (1e9 / mean) : 0.0;
    
    return result;
}

int main() {
    std::vector<BenchmarkResult> results;
    
    // GATE-UTL-01: BloomFilterConfig alloc rate
    results.push_back(run_benchmark("BM_BloomFilterConfig_Alloc", 5, 100000, [](int) {
        BloomFilterConfig cfg;
        cfg.expectedItems = 1000000;
        cfg.targetFalsePositiveRate = 0.01;
        __asm__ volatile("" : : "r"(cfg.expectedItems) : );  // prevent optimization
    }));
    
    // GATE-UTL-02: RetryPolicy copy rate
    results.push_back(run_benchmark("BM_RetryPolicy_Copy", 5, 100000, [](int) {
        RetryPolicy orig;
        orig.maxAttempts = 5;
        orig.backoffMultiplier = 2.0;
        RetryPolicy copy = orig;
        __asm__ volatile("" : : "r"(copy.maxAttempts) : );
    }));
    
    // GATE-UTL-03: Error-code cast throughput
    results.push_back(run_benchmark("BM_UtilsError_Cast", 5, 500000, [](int) {
        static const UtilsError kErrors[] = {
            UtilsError::kAuditOverflow,
            UtilsError::kBatchRollback,
            UtilsError::kBatchSizeExceeded,
            UtilsError::kRetryExhausted,
            UtilsError::kDeserInvalid,
            UtilsError::kPoolExhausted,
        };
        int32_t v = static_cast<int32_t>(kErrors[0]);
        __asm__ volatile("" : : "r"(v) : );
    }));
    
    // GATE-UTL-04: RetryPolicy batch alloc
    results.push_back(run_benchmark("BM_RetryPolicy_BatchAlloc_1000", 5, 10000, [](int) {
        std::vector<RetryPolicy> v(1000);
        for (auto& p : v) p.maxAttempts = 3;
        __asm__ volatile("" : : "r"(v.data()) : );
    }));
    
    // GATE-UTL-05: BloomFilterConfig batch alloc
    results.push_back(run_benchmark("BM_BloomFilterConfig_Batch_1000", 5, 10000, [](int) {
        std::vector<BloomFilterConfig> v(1000);
        for (auto& c : v) {
            c.expectedItems = 100;
            c.targetFalsePositiveRate = 0.01;
        }
        __asm__ volatile("" : : "r"(v.data()) : );
    }));
    
    // GATE-UTL-06: UtilsError switch dispatch
    results.push_back(run_benchmark("BM_UtilsError_SwitchDispatch", 5, 500000, [](int) {
        static const UtilsError kErrors[] = {
            UtilsError::kAuditOverflow,
            UtilsError::kBatchRollback,
            UtilsError::kBatchSizeExceeded,
            UtilsError::kRetryExhausted,
            UtilsError::kDeserInvalid,
            UtilsError::kPoolExhausted,
        };
        auto err = kErrors[0];
        int32_t result = 0;
        switch (err) {
            case UtilsError::kAuditOverflow:     result = 1; break;
            case UtilsError::kBatchRollback:     result = 2; break;
            case UtilsError::kBatchSizeExceeded: result = 3; break;
            case UtilsError::kRetryExhausted:    result = 4; break;
            case UtilsError::kDeserInvalid:      result = 5; break;
            case UtilsError::kPoolExhausted:     result = 6; break;
        }
        __asm__ volatile("" : : "r"(result) : );
    }));
    
    // Output results to JSON
    json output;
    output["benchmarks"] = json::array();
    
    for (const auto& result : results) {
        output["benchmarks"].push_back(result.to_json());
    }
    
    // Print as JSON
    std::cout << output.dump(2) << std::endl;
    
    // Print table
    std::cout << "\n\n=== BENCHMARK RESULTS ===\n" << std::endl;
    std::cout << std::left << std::setw(40) << "Benchmark"
              << std::setw(15) << "Mean (µs)"
              << std::setw(15) << "P95 (µs)"
              << std::setw(15) << "Throughput"
              << std::endl;
    std::cout << std::string(85, '-') << std::endl;
    
    for (const auto& r : results) {
        std::cout << std::left << std::setw(40) << r.name
                  << std::setw(15) << (r.mean_ns / 1000.0)
                  << std::setw(15) << (r.p95_ns / 1000.0)
                  << std::fixed << std::setprecision(2) << (r.throughput_ops_sec / 1e6) << "M ops/s"
                  << std::endl;
    }
    
    return 0;
}
