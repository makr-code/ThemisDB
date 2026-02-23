/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cycle_metrics_config.h                             ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file cycle_metrics_config.h
 * @brief Zero-cost abstraction macros for cycle metrics
 * 
 * When disabled, all macros compile to nothing (zero overhead).
 * When enabled, provides efficient cycle measurement.
 */

#include "cycle_metrics.h"

namespace themis {
namespace performance {

// Forward declaration
class CycleMetricsCollector;

} // namespace performance
} // namespace themis

// ============================================================================
// CYCLE MEASUREMENT MACROS
// ============================================================================

#ifdef THEMIS_ENABLE_CYCLE_METRICS
    /**
     * @brief Start cycle measurement
     * @param var Variable to store start cycle count
     */
    #define THEMIS_MEASURE_CYCLES_START(var) \
        var = ::themis::performance::HardwareCycleCounter::cpu_cycles()
    
    /**
     * @brief End cycle measurement
     * @param var Variable containing start cycle count (will be overwritten with elapsed cycles)
     */
    #define THEMIS_MEASURE_CYCLES_END(var) \
        var = ::themis::performance::HardwareCycleCounter::rdtscp() - var
    
    /**
     * @brief RAII cycle timer
     * @param var Variable to store elapsed cycles
     */
    #define THEMIS_SCOPED_CYCLE_TIMER(var) \
        ::themis::performance::ScopedCycleTimer _scoped_timer_##__LINE__(&var)
#else
    // Zero cost: compiles to nothing
    #define THEMIS_MEASURE_CYCLES_START(var) ((void)0)
    #define THEMIS_MEASURE_CYCLES_END(var) ((void)0)
    #define THEMIS_SCOPED_CYCLE_TIMER(var) ((void)0)
#endif

// ============================================================================
// METRICS RECORDING MACROS
// ============================================================================

#ifdef THEMIS_ENABLE_METRICS_EXPORT
    /**
     * @brief Record operation metrics
     * @param operation Operation name (string literal)
     * @param metrics OperationCycleMetrics struct
     */
    #define THEMIS_RECORD_METRICS(operation, metrics) \
        ::themis::performance::CycleMetricsCollector::instance() \
            .recordOperation(operation, metrics)
#else
    // Zero cost: compiles to nothing
    #define THEMIS_RECORD_METRICS(operation, metrics) ((void)0)
#endif

// ============================================================================
// DETAILED METRICS MACROS (higher overhead)
// ============================================================================

#ifdef THEMIS_ENABLE_DETAILED_METRICS
    /**
     * @brief Record detailed per-function metrics
     * @param function_name Function name
     * @param cycles Cycle count
     */
    #define THEMIS_RECORD_FUNCTION_CYCLES(function_name, cycles) \
        ::themis::performance::CycleMetricsCollector::instance() \
            .recordFunctionCycles(function_name, cycles)
#else
    #define THEMIS_RECORD_FUNCTION_CYCLES(function_name, cycles) ((void)0)
#endif

// ============================================================================
// GPU METRICS MACROS
// ============================================================================

#ifdef THEMIS_ENABLE_GPU_CYCLE_METRICS
    /**
     * @brief Start GPU cycle measurement
     */
    #define THEMIS_GPU_CYCLES_START() \
        ::themis::performance::HardwareCycleCounter::gpu_cycles_start()
    
    /**
     * @brief End GPU cycle measurement
     * @param event GPU event handle
     */
    #define THEMIS_GPU_CYCLES_END(event) \
        ::themis::performance::HardwareCycleCounter::gpu_cycles_end(event)
#else
    #define THEMIS_GPU_CYCLES_START() nullptr
    #define THEMIS_GPU_CYCLES_END(event) 0
#endif
