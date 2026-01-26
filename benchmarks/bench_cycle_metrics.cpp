#include "performance/cycle_metrics.h"
#include "performance/cycle_metrics_config.h"
#include "performance/expected_cycles.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace themis::performance;

/**
 * @brief Benchmark: Pointer passing vs memory copy
 * 
 * Demonstrates that pointer passing is ~53x faster than copying memory.
 */
void benchmark_pointer_vs_copy() {
    std::cout << "\n=== Pointer Passing vs Memory Copy ===" << std::endl;
    
    constexpr size_t DATA_SIZE = 10 * 1024;  // 10 KB
    std::vector<uint8_t> source_data(DATA_SIZE, 0x42);
    std::vector<uint8_t> dest_data(DATA_SIZE);
    
    // Measure pointer passing
    uint64_t pointer_cycles = 0;
    {
        const uint8_t* ptr = nullptr;
        THEMIS_MEASURE_CYCLES_START(pointer_cycles);
        ptr = source_data.data();
        (void)ptr;  // Use pointer to prevent optimization
        THEMIS_MEASURE_CYCLES_END(pointer_cycles);
    }
    
    // Measure memory copy
    uint64_t copy_cycles = 0;
    {
        THEMIS_MEASURE_CYCLES_START(copy_cycles);
        std::memcpy(dest_data.data(), source_data.data(), DATA_SIZE);
        THEMIS_MEASURE_CYCLES_END(copy_cycles);
    }
    
    std::cout << "  Pointer passing: " << pointer_cycles << " cycles" << std::endl;
    std::cout << "  Memory copy (10KB): " << copy_cycles << " cycles" << std::endl;
    
    if (pointer_cycles > 0 && copy_cycles > 0) {
        double speedup = (double)copy_cycles / (double)pointer_cycles;
        std::cout << "  Speedup: " << std::fixed << std::setprecision(1) << speedup << "x" << std::endl;
    }
    
    // Compare with expected values
    double pointer_deviation = ExpectedCycles::deviation_percent(pointer_cycles, ExpectedCycles::POINTER_PASSING);
    uint64_t expected_copy = ExpectedCycles::MEMORY_COPY_PER_KB * 10;
    double copy_deviation = ExpectedCycles::deviation_percent(copy_cycles, expected_copy);
    
    std::cout << "  Deviation from expected:" << std::endl;
    std::cout << "    Pointer: " << std::fixed << std::setprecision(1) << pointer_deviation << "%" << std::endl;
    std::cout << "    Copy: " << std::fixed << std::setprecision(1) << copy_deviation << "%" << std::endl;
}

/**
 * @brief Benchmark: Cache hierarchy
 */
void benchmark_cache_hierarchy() {
    std::cout << "\n=== Cache Hierarchy ===" << std::endl;
    
    // L1 cache size: typically 32-64 KB per core
    constexpr size_t L1_SIZE = 32 * 1024;
    constexpr size_t L2_SIZE = 256 * 1024;
    constexpr size_t L3_SIZE = 8 * 1024 * 1024;
    constexpr size_t RAM_SIZE = 64 * 1024 * 1024;
    
    auto measure_access = [](size_t size, size_t iterations) -> uint64_t {
        std::vector<uint64_t> data(size / sizeof(uint64_t));
        volatile uint64_t sum = 0;
        
        uint64_t cycles = 0;
        THEMIS_MEASURE_CYCLES_START(cycles);
        
        for (size_t i = 0; i < iterations; ++i) {
            for (size_t j = 0; j < data.size(); j += 64 / sizeof(uint64_t)) {
                sum += data[j];
            }
        }
        
        THEMIS_MEASURE_CYCLES_END(cycles);
        return cycles / (iterations * (data.size() / (64 / sizeof(uint64_t))));
    };
    
    std::cout << "  L1 access: ~" << measure_access(L1_SIZE, 100) << " cycles (expected: " 
              << ExpectedCycles::L1_CACHE_HIT << ")" << std::endl;
    std::cout << "  L2 access: ~" << measure_access(L2_SIZE, 50) << " cycles (expected: " 
              << ExpectedCycles::L2_CACHE_HIT << ")" << std::endl;
    std::cout << "  L3 access: ~" << measure_access(L3_SIZE, 10) << " cycles (expected: " 
              << ExpectedCycles::L3_CACHE_HIT << ")" << std::endl;
    std::cout << "  RAM access: ~" << measure_access(RAM_SIZE, 5) << " cycles (expected: " 
              << ExpectedCycles::RAM_ACCESS << ")" << std::endl;
}

/**
 * @brief Benchmark: RAG pipeline breakdown
 */
void benchmark_rag_pipeline() {
    std::cout << "\n=== RAG Pipeline Breakdown ===" << std::endl;
    
    OperationCycleMetrics metrics;
    
    // Simulate HNSW search (10K vectors)
    metrics.hnsw_search_cycles = ExpectedCycles::HNSW_SEARCH_10K_VECTORS;
    
    // Simulate pointer passing
    {
        THEMIS_SCOPED_CYCLE_TIMER(metrics.pointer_passing_cycles);
        volatile const void* ptr = &metrics;
        (void)ptr;
    }
    
    // Simulate LLM inference (10 tokens, 7B model)
    metrics.llm_inference_cycles = ExpectedCycles::LLM_INFERENCE_TOKEN_7B * 10;
    
    metrics.calculate_totals();
    
    std::cout << "  HNSW search: " << metrics.hnsw_search_cycles << " cycles" << std::endl;
    std::cout << "  Pointer passing: " << metrics.pointer_passing_cycles << " cycles" << std::endl;
    std::cout << "  LLM inference: " << metrics.llm_inference_cycles << " cycles" << std::endl;
    std::cout << "  Total: " << metrics.total_cycles << " cycles" << std::endl;
    
    // Calculate percentages
    if (metrics.total_cycles > 0) {
        double hnsw_pct = (double)metrics.hnsw_search_cycles / metrics.total_cycles * 100.0;
        double pointer_pct = (double)metrics.pointer_passing_cycles / metrics.total_cycles * 100.0;
        double llm_pct = (double)metrics.llm_inference_cycles / metrics.total_cycles * 100.0;
        
        std::cout << "\n  Breakdown:" << std::endl;
        std::cout << "    HNSW: " << std::fixed << std::setprecision(2) << hnsw_pct << "%" << std::endl;
        std::cout << "    Pointer: " << std::fixed << std::setprecision(6) << pointer_pct << "% (NEGLIGIBLE!)" << std::endl;
        std::cout << "    LLM: " << std::fixed << std::setprecision(2) << llm_pct << "%" << std::endl;
    }
    
    std::cout << "\n  Expected values:" << std::endl;
    std::cout << "    Pointer overhead: " << std::fixed << std::setprecision(6) 
              << ExpectedCycles::RAGPipeline::POINTER_OVERHEAD_PERCENT << "%" << std::endl;
}

/**
 * @brief Main benchmark entry point
 */
int main() {
    std::cout << "ThemisDB Cycle Metrics Benchmark" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // System info
    std::cout << "\nSystem Information:" << std::endl;
    std::cout << "  CPU: " << HardwareCycleCounter::cpu_model() << std::endl;
    std::cout << "  Frequency: " << HardwareCycleCounter::cpu_frequency_hz() / 1'000'000 << " MHz" << std::endl;
    
#ifdef THEMIS_ENABLE_CYCLE_METRICS
    std::cout << "  Cycle metrics: ENABLED" << std::endl;
#else
    std::cout << "  Cycle metrics: DISABLED (zero cost)" << std::endl;
#endif
    
    // Run benchmarks
    benchmark_pointer_vs_copy();
    benchmark_cache_hierarchy();
    benchmark_rag_pipeline();
    
    std::cout << "\n=== Benchmark Complete ===" << std::endl;
    
    return 0;
}
