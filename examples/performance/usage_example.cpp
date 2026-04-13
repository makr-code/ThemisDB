/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            usage_example.cpp                                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file usage_example.cpp
 * @brief Example demonstrating ThemisDB cycle metrics usage
 * 
 * This example shows how to use the cycle metrics system to measure
 * and export performance metrics.
 * 
 * Compile with:
 *   g++ -std=c++17 -I../../include usage_example.cpp \
 *       ../../src/performance/cycle_metrics.cpp -o usage_example
 * 
 * Run:
 *   ./usage_example
 */

#include "performance/cycle_metrics.h"
#include "performance/cycle_metrics_config.h"
#include "performance/expected_cycles.h"
#include "performance/runtime_config.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace themis::performance;

/**
 * @brief Simulate HNSW vector search
 */
void simulate_hnsw_search(OperationCycleMetrics& metrics) {
    THEMIS_SCOPED_CYCLE_TIMER(metrics.hnsw_search_cycles);
    
    // Simulate some work
    std::vector<float> data(10000);
    volatile float sum = 0;
    for (auto val : data) {
        sum += val;
    }
}

/**
 * @brief Simulate pointer passing (should be ~150 cycles)
 */
void simulate_pointer_passing(OperationCycleMetrics& metrics, void* ptr) {
    THEMIS_SCOPED_CYCLE_TIMER(metrics.pointer_passing_cycles);
    
    // Just pass the pointer (minimal work)
    volatile void* local_ptr = ptr;
    (void)local_ptr;
}

/**
 * @brief Simulate LLM inference
 */
void simulate_llm_inference(OperationCycleMetrics& metrics) {
    THEMIS_SCOPED_CYCLE_TIMER(metrics.llm_inference_cycles);
    
    // Simulate heavier computation
    std::vector<double> matrix(1000 * 1000);
    volatile double result = 0;
    for (size_t i = 0; i < 1000; ++i) {
        for (size_t j = 0; j < 1000; ++j) {
            result += matrix[i * 1000 + j];
        }
    }
}

/**
 * @brief Example: Basic cycle measurement
 */
void example_basic_measurement() {
    std::cout << "\n=== Example 1: Basic Cycle Measurement ===" << std::endl;
    
    uint64_t cycles = 0;
    
    // Method 1: Manual start/end
    THEMIS_MEASURE_CYCLES_START(cycles);
    volatile int sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += i;
    }
    THEMIS_MEASURE_CYCLES_END(cycles);
    
    std::cout << "Loop cycles: " << cycles << std::endl;
    
    // Method 2: RAII timer
    {
        THEMIS_SCOPED_CYCLE_TIMER(cycles);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    std::cout << "Sleep cycles: " << cycles << std::endl;
    
    // Convert to time
    uint64_t freq = HardwareCycleCounter::cpu_frequency_hz();
    double time_ms = (double)cycles / freq * 1000.0;
    std::cout << "Sleep time: " << time_ms << " ms" << std::endl;
}

/**
 * @brief Example: RAG pipeline measurement
 */
void example_rag_pipeline() {
    std::cout << "\n=== Example 2: RAG Pipeline Measurement ===" << std::endl;
    
    OperationCycleMetrics metrics;
    
    // Simulate HNSW search
    simulate_hnsw_search(metrics);
    
    // Simulate pointer passing (critical measurement!)
    std::vector<int> data(1000);
    simulate_pointer_passing(metrics, data.data());
    
    // Simulate LLM inference
    simulate_llm_inference(metrics);
    
    // Calculate totals
    metrics.calculate_totals();
    
    // Display results
    std::cout << "HNSW search: " << metrics.hnsw_search_cycles << " cycles" << std::endl;
    std::cout << "Pointer passing: " << metrics.pointer_passing_cycles << " cycles" << std::endl;
    std::cout << "LLM inference: " << metrics.llm_inference_cycles << " cycles" << std::endl;
    std::cout << "Total: " << metrics.total_cycles << " cycles" << std::endl;
    
    // Calculate percentages
    if (metrics.total_cycles > 0) {
        double pointer_pct = (double)metrics.pointer_passing_cycles / metrics.total_cycles * 100.0;
        std::cout << "\nPointer passing overhead: " << pointer_pct << "% of total" << std::endl;
        
        // Compare with expected
        double deviation = ExpectedCycles::deviation_percent(
            metrics.pointer_passing_cycles, 
            ExpectedCycles::POINTER_PASSING
        );
        std::cout << "Deviation from expected: " << deviation << "%" << std::endl;
    }
    
    // Record metrics (if export is enabled)
    THEMIS_RECORD_METRICS("rag_pipeline_example", metrics);
}

/**
 * @brief Example: Runtime configuration
 */
void example_runtime_config() {
    std::cout << "\n=== Example 3: Runtime Configuration ===" << std::endl;
    
    auto& config = RuntimeConfig::instance();
    
    // Set sampling rate
    config.setSamplingRate(10);  // Measure 10% of operations
    std::cout << "Sampling rate: 1/" << config.getSamplingRate() << std::endl;
    
    // Check if we should measure
    int measured = 0;
    for (int i = 0; i < 100; ++i) {
        if (config.shouldMeasure()) {
            measured++;
        }
    }
    std::cout << "Measured " << measured << " out of 100 operations" << std::endl;
    
    // Enable specific operations
    config.enableOperation("critical_path");
    config.enableOperation("hot_loop");
    
    std::cout << "critical_path enabled: " << config.isOperationEnabled("critical_path") << std::endl;
    std::cout << "other_op enabled: " << config.isOperationEnabled("other_op") << std::endl;
}

/**
 * @brief Example: Expected values validation
 */
void example_expected_values() {
    std::cout << "\n=== Example 4: Expected Values ===" << std::endl;
    
    std::cout << "Expected cycle counts:" << std::endl;
    std::cout << "  L1 cache hit: " << ExpectedCycles::L1_CACHE_HIT << " cycles" << std::endl;
    std::cout << "  L2 cache hit: " << ExpectedCycles::L2_CACHE_HIT << " cycles" << std::endl;
    std::cout << "  L3 cache hit: " << ExpectedCycles::L3_CACHE_HIT << " cycles" << std::endl;
    std::cout << "  RAM access: " << ExpectedCycles::RAM_ACCESS << " cycles" << std::endl;
    std::cout << "  Pointer passing: " << ExpectedCycles::POINTER_PASSING << " cycles" << std::endl;
    
    std::cout << "\nRAG Pipeline (10K vectors, 7B model, 10 tokens):" << std::endl;
    std::cout << "  HNSW search: " << ExpectedCycles::RAGPipeline::HNSW_10K_SEARCH << " cycles" << std::endl;
    std::cout << "  Pointer passing: " << ExpectedCycles::RAGPipeline::POINTER_PASSING_OVERHEAD << " cycles" << std::endl;
    std::cout << "  LLM inference: " << ExpectedCycles::RAGPipeline::LLM_10_TOKENS_7B << " cycles" << std::endl;
    std::cout << "  Total: " << ExpectedCycles::RAGPipeline::TOTAL << " cycles" << std::endl;
    std::cout << "  Pointer overhead: " << ExpectedCycles::RAGPipeline::POINTER_OVERHEAD_PERCENT << "%" << std::endl;
    
    // Tolerance checks
    uint64_t measured = 175;
    uint64_t expected = ExpectedCycles::POINTER_PASSING;
    
    std::cout << "\nTolerance check (measured=" << measured << ", expected=" << expected << "):" << std::endl;
    std::cout << "  Normal: " << ExpectedCycles::is_normal(measured, expected) << std::endl;
    std::cout << "  Warning: " << ExpectedCycles::is_warning(measured, expected) << std::endl;
    std::cout << "  Critical: " << ExpectedCycles::is_critical(measured, expected) << std::endl;
}

/**
 * @brief Main example entry point
 */
int main() {
    std::cout << "ThemisDB Cycle Metrics - Usage Examples" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // System info
    std::cout << "\nSystem Information:" << std::endl;
    std::cout << "  CPU: " << HardwareCycleCounter::cpu_model() << std::endl;
    std::cout << "  Frequency: " << HardwareCycleCounter::cpu_frequency_hz() / 1'000'000 << " MHz" << std::endl;
    
#ifdef THEMIS_ENABLE_CYCLE_METRICS
    std::cout << "  Cycle metrics: ENABLED" << std::endl;
#else
    std::cout << "  Cycle metrics: DISABLED (zero cost)" << std::endl;
#endif
    
    // Run examples
    example_basic_measurement();
    example_rag_pipeline();
    example_runtime_config();
    example_expected_values();
    
    std::cout << "\n=== Examples Complete ===" << std::endl;
    
    return 0;
}
