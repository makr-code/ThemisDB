/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            backend_metrics.h                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     235                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • dbc9bfed9f  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • dd319b9918  2026-04-13  Add CI/CD workflows and scripts for release management ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "metrics_collector.h"
#include <string>

namespace themis {
namespace acceleration {
namespace metrics {

/**
 * @brief Standard metrics for acceleration backends
 * 
 * This class provides a consistent set of metrics for all acceleration
 * backends (CUDA, HIP, OpenCL, Metal, etc.)
 */
class BackendMetrics {
public:
    explicit BackendMetrics(const std::string& backend_name) 
        : backend_name_(backend_name) {
        registerMetrics();
    }
    
    // Initialization metrics
    void recordInitSuccess() {
        if (init_success_) init_success_->increment();
    }
    
    void recordInitFailure() {
        if (init_failures_) init_failures_->increment();
    }
    
    void recordInitDuration(double seconds) {
        if (init_duration_) init_duration_->observe(seconds);
    }
    
    // Operation metrics
    void recordL2DistanceOperation(double duration_seconds, size_t vector_count) {
        if (l2_distance_duration_) l2_distance_duration_->observe(duration_seconds);
        if (l2_distance_ops_) l2_distance_ops_->increment();
        if (l2_distance_vectors_) l2_distance_vectors_->increment(vector_count);
    }
    
    void recordCosineOperation(double duration_seconds, size_t vector_count) {
        if (cosine_duration_) cosine_duration_->observe(duration_seconds);
        if (cosine_ops_) cosine_ops_->increment();
        if (cosine_vectors_) cosine_vectors_->increment(vector_count);
    }
    
    // Resource metrics
    void setDeviceMemoryUsed(double bytes) {
        if (device_memory_used_) device_memory_used_->set(bytes);
    }
    
    void setDeviceMemoryAvailable(double bytes) {
        if (device_memory_available_) device_memory_available_->set(bytes);
    }
    
    void setQueueDepth(double depth) {
        if (queue_depth_) queue_depth_->set(depth);
    }
    
    // Error metrics
    void recordError([[maybe_unused]] const std::string& error_code) {
        if (errors_total_) errors_total_->increment();
        // Could add per-error-code counters here
    }
    
    void recordKernelLaunchFailure() {
        if (kernel_launch_failures_) kernel_launch_failures_->increment();
    }
    
    void recordMemoryAllocationFailure() {
        if (memory_alloc_failures_) memory_alloc_failures_->increment();
    }
    
    // Device metrics
    void setDeviceCount(int count) {
        if (device_count_) device_count_->set(count);
    }
    
    void setActiveDeviceIndex(int index) {
        if (active_device_) active_device_->set(index);
    }
    
    // Throughput metrics
    double getOperationsPerSecond() const {
        if (!l2_distance_ops_ || !cosine_ops_) return 0.0;
        return static_cast<double>(l2_distance_ops_->value()) +
               static_cast<double>(cosine_ops_->value());
    }
    
    double getVectorsPerSecond() const {
        if (!l2_distance_vectors_ || !cosine_vectors_) return 0.0;
        return static_cast<double>(l2_distance_vectors_->value()) +
               static_cast<double>(cosine_vectors_->value());
    }
    
private:
    void registerMetrics() {
        auto& collector = MetricsCollector::instance();
        std::string prefix = "themis_acceleration_" + backend_name_ + "_";
        
        // Initialization metrics
        init_success_ = collector.registerCounter(
            prefix + "init_success_total",
            "Number of successful backend initializations");
        
        init_failures_ = collector.registerCounter(
            prefix + "init_failures_total",
            "Number of failed backend initializations");
        
        init_duration_ = collector.registerHistogram(
            prefix + "init_duration_seconds",
            "Duration of backend initialization in seconds",
            {0.001, 0.01, 0.1, 1.0, 5.0});
        
        // Operation metrics
        l2_distance_duration_ = collector.registerHistogram(
            prefix + "l2_distance_duration_seconds",
            "Duration of L2 distance operations in seconds",
            {0.0001, 0.001, 0.01, 0.1, 1.0});
        
        l2_distance_ops_ = collector.registerCounter(
            prefix + "l2_distance_operations_total",
            "Total number of L2 distance operations");
        
        l2_distance_vectors_ = collector.registerCounter(
            prefix + "l2_distance_vectors_total",
            "Total number of vectors processed in L2 distance operations");
        
        cosine_duration_ = collector.registerHistogram(
            prefix + "cosine_duration_seconds",
            "Duration of cosine similarity operations in seconds",
            {0.0001, 0.001, 0.01, 0.1, 1.0});
        
        cosine_ops_ = collector.registerCounter(
            prefix + "cosine_operations_total",
            "Total number of cosine similarity operations");
        
        cosine_vectors_ = collector.registerCounter(
            prefix + "cosine_vectors_total",
            "Total number of vectors processed in cosine operations");
        
        // Resource metrics
        device_memory_used_ = collector.registerGauge(
            prefix + "device_memory_used_bytes",
            "Current device memory usage in bytes");
        
        device_memory_available_ = collector.registerGauge(
            prefix + "device_memory_available_bytes",
            "Available device memory in bytes");
        
        queue_depth_ = collector.registerGauge(
            prefix + "queue_depth",
            "Current command queue depth");
        
        // Error metrics
        errors_total_ = collector.registerCounter(
            prefix + "errors_total",
            "Total number of errors encountered");
        
        kernel_launch_failures_ = collector.registerCounter(
            prefix + "kernel_launch_failures_total",
            "Number of kernel launch failures");
        
        memory_alloc_failures_ = collector.registerCounter(
            prefix + "memory_allocation_failures_total",
            "Number of memory allocation failures");
        
        // Device metrics
        device_count_ = collector.registerGauge(
            prefix + "device_count",
            "Number of available devices");
        
        active_device_ = collector.registerGauge(
            prefix + "active_device_index",
            "Index of the currently active device");
    }
    
    std::string backend_name_;
    
    // Metrics pointers (owned by MetricsCollector)
    Counter* init_success_ = nullptr;
    Counter* init_failures_ = nullptr;
    Histogram* init_duration_ = nullptr;
    
    Histogram* l2_distance_duration_ = nullptr;
    Counter* l2_distance_ops_ = nullptr;
    Counter* l2_distance_vectors_ = nullptr;
    
    Histogram* cosine_duration_ = nullptr;
    Counter* cosine_ops_ = nullptr;
    Counter* cosine_vectors_ = nullptr;
    
    Gauge* device_memory_used_ = nullptr;
    Gauge* device_memory_available_ = nullptr;
    Gauge* queue_depth_ = nullptr;
    
    Counter* errors_total_ = nullptr;
    Counter* kernel_launch_failures_ = nullptr;
    Counter* memory_alloc_failures_ = nullptr;
    
    Gauge* device_count_ = nullptr;
    Gauge* active_device_ = nullptr;
};

} // namespace metrics
} // namespace acceleration
} // namespace themis
