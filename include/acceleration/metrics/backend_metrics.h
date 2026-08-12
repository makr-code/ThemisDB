/**
 * @file backend_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "metrics_collector.h"
#include <string>

namespace themis {
namespace acceleration {
namespace metrics {

/**
 * @brief Standard metrics collection for acceleration backends (CUDA, HIP, OpenCL, Metal, etc.)
 * 
 * Provides a consistent set of metrics for all acceleration backends covering:
 * - Initialization success/failure and duration
 * - Operation metrics (L2 distance, cosine similarity) with duration and vector counts
 * - Resource metrics (device memory, queue depth)
 * - Error tracking (total errors, kernel failures, memory allocation failures)
 * - Device metrics (count, active device index)
 * 
 * All metrics are automatically registered with the central MetricsCollector singleton
 * and can be exported in Prometheus or JSON format.
 */
class BackendMetrics {
public:
    /// @brief Constructor initializing metrics for a specific backend
    /// @param backend_name Name of the backend (e.g., "cuda", "hip", "opencl", "metal")
    explicit BackendMetrics(const std::string& backend_name) 
        : backend_name_(backend_name) {
        registerMetrics();
    }
    
    /// @brief Record successful backend initialization
    void recordInitSuccess() {
        if (init_success_) init_success_->increment();
    }
    
    /// @brief Record failed backend initialization
    void recordInitFailure() {
        if (init_failures_) init_failures_->increment();
    }
    
    /// @brief Record backend initialization duration
    /// @param seconds Time taken for initialization in seconds (floating-point)
    void recordInitDuration(double seconds) {
        if (init_duration_) init_duration_->observe(seconds);
    }
    
    /// @brief Record L2 distance operation completion
    /// @param duration_seconds Time taken for the operation in seconds
    /// @param vector_count Number of vectors processed in this operation
    void recordL2DistanceOperation(double duration_seconds, size_t vector_count) {
        if (l2_distance_duration_) l2_distance_duration_->observe(duration_seconds);
        if (l2_distance_ops_) l2_distance_ops_->increment();
        if (l2_distance_vectors_) l2_distance_vectors_->increment(vector_count);
    }
    
    /// @brief Record cosine similarity operation completion
    /// @param duration_seconds Time taken for the operation in seconds
    /// @param vector_count Number of vectors processed in this operation
    void recordCosineOperation(double duration_seconds, size_t vector_count) {
        if (cosine_duration_) cosine_duration_->observe(duration_seconds);
        if (cosine_ops_) cosine_ops_->increment();
        if (cosine_vectors_) cosine_vectors_->increment(vector_count);
    }
    
    /// @brief Update current device memory usage
    /// @param bytes Number of bytes currently in use on device
    void setDeviceMemoryUsed(double bytes) {
        if (device_memory_used_) device_memory_used_->set(bytes);
    }
    
    /// @brief Update available device memory
    /// @param bytes Number of bytes available on device
    void setDeviceMemoryAvailable(double bytes) {
        if (device_memory_available_) device_memory_available_->set(bytes);
    }
    
    /// @brief Update command queue depth
    /// @param depth Current number of queued operations
    void setQueueDepth(double depth) {
        if (queue_depth_) queue_depth_->set(depth);
    }
    
    /// @brief Record an error event
    /// @param error_code Error category or code (for optional categorization)
    void recordError([[maybe_unused]] const std::string& error_code) {
        if (errors_total_) errors_total_->increment();
    }
    
    /// @brief Record a kernel launch failure
    void recordKernelLaunchFailure() {
        if (kernel_launch_failures_) kernel_launch_failures_->increment();
    }
    
    /// @brief Record a memory allocation failure
    void recordMemoryAllocationFailure() {
        if (memory_alloc_failures_) memory_alloc_failures_->increment();
    }
    
    /// @brief Update the count of available devices
    /// @param count Number of available acceleration devices
    void setDeviceCount(int count) {
        if (device_count_) device_count_->set(count);
    }
    
    /// @brief Update the index of the currently active device
    /// @param index Index (0-based) of the active device
    void setActiveDeviceIndex(int index) {
        if (active_device_) active_device_->set(index);
    }
    
    /// @brief Get total operations per second (L2 + cosine operations)
    /// @return Combined operation count (not normalized by time; raw counter value)
    double getOperationsPerSecond() const {
        if (!l2_distance_ops_ || !cosine_ops_) return 0.0;
        return static_cast<double>(l2_distance_ops_->value()) +
               static_cast<double>(cosine_ops_->value());
    }
    
    /// @brief Get total vectors processed per second (L2 + cosine)
    /// @return Combined vector count (not normalized by time; raw counter value)
    double getVectorsPerSecond() const {
        if (!l2_distance_vectors_ || !cosine_vectors_) return 0.0;
        return static_cast<double>(l2_distance_vectors_->value()) +
               static_cast<double>(cosine_vectors_->value());
    }
    
private:
    /// @brief Register all metrics with the central MetricsCollector
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
    
    std::string backend_name_;  ///< Name of the backend (e.g., "cuda", "hip")
    
    // Initialization metrics (owned by MetricsCollector singleton)
    Counter* init_success_ = nullptr;           ///< Counter for successful initializations
    Counter* init_failures_ = nullptr;          ///< Counter for failed initializations
    Histogram* init_duration_ = nullptr;        ///< Histogram of initialization durations
    
    // Operation metrics (duration histograms and operation counters)
    Histogram* l2_distance_duration_ = nullptr; ///< Histogram of L2 operation durations
    Counter* l2_distance_ops_ = nullptr;        ///< Counter of L2 operations
    Counter* l2_distance_vectors_ = nullptr;    ///< Counter of vectors in L2 operations
    
    Histogram* cosine_duration_ = nullptr;      ///< Histogram of cosine operation durations
    Counter* cosine_ops_ = nullptr;             ///< Counter of cosine operations
    Counter* cosine_vectors_ = nullptr;         ///< Counter of vectors in cosine operations
    
    // Resource metrics (gauges)
    Gauge* device_memory_used_ = nullptr;       ///< Gauge for device memory usage
    Gauge* device_memory_available_ = nullptr;  ///< Gauge for available device memory
    Gauge* queue_depth_ = nullptr;              ///< Gauge for command queue depth
    
    // Error metrics (counters)
    Counter* errors_total_ = nullptr;           ///< Counter for total errors
    Counter* kernel_launch_failures_ = nullptr; ///< Counter for kernel launch failures
    Counter* memory_alloc_failures_ = nullptr;  ///< Counter for memory allocation failures
    
    // Device metrics (gauges)
    Gauge* device_count_ = nullptr;             ///< Gauge for number of available devices
    Gauge* active_device_ = nullptr;            ///< Gauge for active device index
};

} // namespace metrics
} // namespace acceleration
} // namespace themis
