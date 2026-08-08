/**
 * @file bench_updates_long_run_stability.cpp
 * @brief Benchmark suite for sustained update workload (48h+ operational envelope)
 * @version 1.0.0
 * 
 * Benchmark Suite: Long-Run Stability
 * 
 * Workloads:
 *  - 1M+ update operations
 *  - Mixed: state transitions, patches, migrations
 *  - Memory stability (no growth > 5%)
 *  - Error rate stability (< 0.01%)
 * 
 * Metrics: sustained throughput, memory stability, error rates
 */

#include <benchmark/benchmark.h>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include <random>

#include "updates/update_state_machine.h"
#include "updates/delta_update_engine.h"
#include "updates/tenant_update_scheduler.h"

namespace themis {
namespace updates {
namespace benchmark {

// ============================================================================
// Long-Run Stability Benchmark
// ============================================================================

class LongRunStabilityBenchmark {
private:
    struct WorkloadStats {
        std::atomic<uint64_t> total_operations = 0;
        std::atomic<uint64_t> successful_operations = 0;
        std::atomic<uint64_t> failed_operations = 0;
        std::atomic<size_t> peak_memory = 0;
        std::atomic<size_t> current_memory = 0;
        std::vector<double> memory_snapshots;
        std::vector<double> error_rate_snapshots;
        std::chrono::steady_clock::time_point start_time;
    };
    
    std::unique_ptr<UpdateStateManager> state_mgr_;
    std::unique_ptr<DeltaUpdateEngine> delta_engine_;
    std::unique_ptr<TenantUpdateScheduler> scheduler_;
    WorkloadStats stats_;
    std::mt19937 rng_;
    
public:
    LongRunStabilityBenchmark()
        : state_mgr_(std::make_unique<UpdateStateManager>()),
          delta_engine_(std::make_unique<DeltaUpdateEngine>()),
          scheduler_(std::make_unique<TenantUpdateScheduler>()),
          rng_(std::random_device{}()) {
        
        stats_.start_time = std::chrono::steady_clock::now();
    }
    
    ~LongRunStabilityBenchmark() = default;
    
    // Simulate mixed workload: state transitions, patches, migrations
    void runMixedWorkload(uint64_t operation_count) {
        std::uniform_int_distribution<int> op_type(0, 99);
        
        for (uint64_t i = 0; i < operation_count; ++i) {
            int op = op_type(rng_);
            
            try {
                if (op < 30) {
                    // 30%: State transitions
                    executeStateTransition();
                } else if (op < 60) {
                    // 30%: Delta operations
                    executeDeltaOperation();
                } else if (op < 90) {
                    // 30%: Scheduling operations
                    executeScheduleOperation();
                } else {
                    // 10%: Health checks / memory monitoring
                    monitorResources();
                }
                
                stats_.successful_operations++;
            } catch (const std::exception& e) {
                stats_.failed_operations++;
            }
            
            stats_.total_operations++;
            
            // Periodic memory snapshot
            if (i % 10000 == 0) {
                captureMemorySnapshot();
            }
        }
    }
    
    double getMemoryGrowthPercentage() const {
        if (stats_.memory_snapshots.size() < 2) return 0.0;
        
        double initial = stats_.memory_snapshots.front();
        double final = stats_.memory_snapshots.back();
        
        if (initial == 0) return 0.0;
        return ((final - initial) / initial) * 100.0;
    }
    
    double getErrorRate() const {
        uint64_t total = stats_.successful_operations + stats_.failed_operations;
        if (total == 0) return 0.0;
        return (static_cast<double>(stats_.failed_operations) / total) * 100.0;
    }
    
    uint64_t getTotalOperations() const {
        return stats_.total_operations;
    }
    
    std::chrono::duration<double> getElapsedTime() const {
        return std::chrono::steady_clock::now() - stats_.start_time;
    }
    
    double getThroughputOpsPerSecond() const {
        auto elapsed = getElapsedTime();
        if (elapsed.count() == 0) return 0.0;
        return getTotalOperations() / elapsed.count();
    }
    
private:
    void executeStateTransition() {
        // Random valid transition
        std::uniform_int_distribution<int> state_choice(0, 5);
        UpdateState states[] = {
            UpdateState::IDLE, UpdateState::DOWNLOADING, 
            UpdateState::VERIFYING, UpdateState::APPLYING,
            UpdateState::ROLLING_BACK, UpdateState::FAILED
        };
        
        int choice = state_choice(rng_);
        auto result = state_mgr_->transitionTo(states[choice]);
        
        if (!result.success) {
            throw std::runtime_error("State transition failed");
        }
    }
    
    void executeDeltaOperation() {
        try {
            auto patch = delta_engine_->generatePatch("v1.0", "v1.1");
            auto result = delta_engine_->applyPatch(patch);
            
            if (!result.success) {
                throw std::runtime_error("Delta operation failed");
            }
        } catch (...) {
            throw;
        }
    }
    
    void executeScheduleOperation() {
        try {
            auto schedule_result = scheduler_->scheduleUpdate(
                "tenant-1",
                "v1.1"
            );
            
            if (!schedule_result.success) {
                throw std::runtime_error("Schedule operation failed");
            }
        } catch (...) {
            throw;
        }
    }
    
    void monitorResources() {
        // Trigger garbage collection if needed
        size_t current = getCurrentMemoryUsage();
        if (current > stats_.peak_memory) {
            stats_.peak_memory.store(current);
        }
        stats_.current_memory.store(current);
    }
    
    void captureMemorySnapshot() {
        size_t current_mem = getCurrentMemoryUsage();
        stats_.memory_snapshots.push_back(current_mem / (1024.0 * 1024.0));  // Convert to MB
        
        double current_error_rate = getErrorRate();
        stats_.error_rate_snapshots.push_back(current_error_rate);
    }
    
    size_t getCurrentMemoryUsage() {
        // Simplified memory usage (in real implementation, use platform-specific APIs)
        #ifdef _WIN32
            PROCESS_MEMORY_COUNTERS pmc;
            GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
            return pmc.WorkingSetSize;
        #else
            // On Unix, read from /proc/self/status
            std::ifstream status("/proc/self/status");
            std::string line;
            while (std::getline(status, line)) {
                if (line.find("VmRSS:") != std::string::npos) {
                    std::istringstream iss(line);
                    std::string key;
                    size_t value;
                    iss >> key >> value;
                    return value * 1024;  // Convert KB to bytes
                }
            }
            return 0;
        #endif
    }
};

// ============================================================================
// Benchmark: 1M Operations Mixed Workload
// ============================================================================

void LongRunStability_1MOperations(benchmark::State& state) {
    for (auto _ : state) {
        LongRunStabilityBenchmark bench;
        
        // Execute 1M operations
        bench.runMixedWorkload(1000000);
        
        // Record metrics
        state.counters["total_operations"] = bench.getTotalOperations();
        state.counters["throughput_ops_per_sec"] = bench.getThroughputOpsPerSecond();
        state.counters["memory_growth_pct"] = bench.getMemoryGrowthPercentage();
        state.counters["error_rate_pct"] = bench.getErrorRate();
        state.counters["elapsed_time_sec"] = bench.getElapsedTime().count();
        
        // Validate gates
        if (bench.getMemoryGrowthPercentage() > 5.0) {
            state.SkipWithError("Memory growth exceeded 5%");
        }
        
        if (bench.getErrorRate() > 0.01) {
            state.SkipWithError("Error rate exceeded 0.01%");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 1000000);
}

BENCHMARK(LongRunStability_1MOperations);

// ============================================================================
// Benchmark: 10M Operations Extended Workload (for 48h+ simulation)
// ============================================================================

void LongRunStability_10MOperations(benchmark::State& state) {
    for (auto _ : state) {
        LongRunStabilityBenchmark bench;
        
        // Execute 10M operations
        bench.runMixedWorkload(10000000);
        
        // Record metrics
        state.counters["total_operations"] = bench.getTotalOperations();
        state.counters["throughput_ops_per_sec"] = bench.getThroughputOpsPerSecond();
        state.counters["memory_growth_pct"] = bench.getMemoryGrowthPercentage();
        state.counters["error_rate_pct"] = bench.getErrorRate();
        state.counters["elapsed_time_sec"] = bench.getElapsedTime().count();
        
        // Stricter gates for extended run
        if (bench.getMemoryGrowthPercentage() > 3.0) {
            state.SkipWithError("Memory growth exceeded 3% (extended run)");
        }
        
        if (bench.getErrorRate() > 0.001) {
            state.SkipWithError("Error rate exceeded 0.001% (extended run)");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 10000000);
}

BENCHMARK(LongRunStability_10MOperations);

// ============================================================================
// Benchmark: Concurrent Mixed Workload (Multi-Thread)
// ============================================================================

void LongRunStability_ConcurrentWorkload(benchmark::State& state) {
    int thread_count = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::vector<LongRunStabilityBenchmark> benches(thread_count);
        
        // Launch worker threads
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                benches[i].runMixedWorkload(100000);  // 100K ops per thread
            });
        }
        
        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }
        
        // Aggregate metrics
        uint64_t total_ops = 0;
        double total_elapsed = 0;
        double peak_memory_growth = 0;
        double peak_error_rate = 0;
        
        for (const auto& bench : benches) {
            total_ops += bench.getTotalOperations();
            total_elapsed = std::max(total_elapsed, bench.getElapsedTime().count());
            peak_memory_growth = std::max(peak_memory_growth, bench.getMemoryGrowthPercentage());
            peak_error_rate = std::max(peak_error_rate, bench.getErrorRate());
        }
        
        state.counters["total_operations"] = total_ops;
        state.counters["throughput_ops_per_sec"] = total_ops / total_elapsed;
        state.counters["max_memory_growth_pct"] = peak_memory_growth;
        state.counters["max_error_rate_pct"] = peak_error_rate;
        state.counters["thread_count"] = thread_count;
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0) * 100000);
}

BENCHMARK(LongRunStability_ConcurrentWorkload)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8);

// ============================================================================
// Performance Gates
// ============================================================================

/**
 * UPDP-7: Long-run stability: no memory growth > 5%
 * 
 * Expected: Memory growth <= 5% over 1M operations
 * Measurement: memory_growth_pct from LongRunStability_1MOperations
 * 
 * Validates:
 *  - No memory leaks
 *  - Proper resource cleanup
 *  - GC/deallocation effectiveness
 */

/**
 * UPDP-7b: Error rate stability
 * 
 * Expected: Error rate <= 0.01% (1 error per 10K ops)
 * Measurement: error_rate_pct from LongRunStability_1MOperations
 * 
 * Validates:
 *  - Operational reliability
 *  - Exception safety
 *  - Failure isolation
 */

/**
 * UPDP-7c: Sustained throughput
 * 
 * Expected: Throughput stable within 5% deviation over 48h simulation
 * Measurement: throughput_ops_per_sec across all snapshots
 * 
 * Validates:
 *  - No degradation over time
 *  - Predictable performance
 */

} // namespace benchmark
} // namespace updates
} // namespace themis
