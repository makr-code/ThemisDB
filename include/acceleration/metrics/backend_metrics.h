/**
 * @file backend_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "metrics_collector.h"
#include <string>

namespace themis {
namespace acceleration {
namespace metrics {

class BackendMetrics {
public:
    /**
     * @brief Backend Metrics.
     * @param[in] backend_name Name of the backend.
     * @return Return value.
     */
    explicit BackendMetrics(const std::string& backend_name) 
        : backend_name_(backend_name) {
        registerMetrics();
    }
    
    /**
     * @brief Record Init Success.
     * @details Calls: increment().
     */
    void recordInitSuccess() {
        if (init_success_) {
          init_success_->increment();
        }
    }
    
    /**
     * @brief Record Init Failure.
     * @details Calls: increment().
     */
    void recordInitFailure() {
        if (init_failures_) {
          init_failures_->increment();
        }
    }
    
    /**
     * @brief Record Init Duration.
     * @param[in] seconds Input parameter.
     * @details Calls: observe().
     */
    void recordInitDuration(double seconds) {
        if (init_duration_) {
          init_duration_->observe(seconds);
        }
    }
    
    /**
     * @brief Record L2 Distance Operation.
     * @param[in] duration_seconds Input parameter.
     * @param[in] vector_count Input parameter.
     * @details Calls: observe(), increment().
     */
    void recordL2DistanceOperation(double duration_seconds, size_t vector_count) {
        if (l2_distance_duration_) {
          l2_distance_duration_->observe(duration_seconds);
        }
        if (l2_distance_ops_) {
          l2_distance_ops_->increment();
        }
        if (l2_distance_vectors_) {
          l2_distance_vectors_->increment(vector_count);
        }
    }
    
    /**
     * @brief Record Cosine Operation.
     * @param[in] duration_seconds Input parameter.
     * @param[in] vector_count Input parameter.
     * @details Calls: observe(), increment().
     */
    void recordCosineOperation(double duration_seconds, size_t vector_count) {
        if (cosine_duration_) {
          cosine_duration_->observe(duration_seconds);
        }
        if (cosine_ops_) {
          cosine_ops_->increment();
        }
        if (cosine_vectors_) {
          cosine_vectors_->increment(vector_count);
        }
    }
    
    /**
     * @brief Set Device Memory Used.
     * @param[in] bytes Input parameter.
     * @details Calls: set().
     */
    void setDeviceMemoryUsed(double bytes) {
        if (device_memory_used_) {
          device_memory_used_->set(bytes);
        }
    }
    
    /**
     * @brief Set Device Memory Available.
     * @param[in] bytes Input parameter.
     * @details Calls: set().
     */
    void setDeviceMemoryAvailable(double bytes) {
        if (device_memory_available_) {
          device_memory_available_->set(bytes);
        }
    }
    
    /**
     * @brief Set Queue Depth.
     * @param[in] depth Input parameter.
     * @details Calls: set().
     */
    void setQueueDepth(double depth) {
        if (queue_depth_) {
          queue_depth_->set(depth);
        }
    }
    
    void recordError([[maybe_unused]] const std::string& error_code) {
        if (errors_total_) {
          errors_total_->increment();
        }
    }
    
    /**
     * @brief Record Kernel Launch Failure.
     * @details Calls: increment().
     */
    void recordKernelLaunchFailure() {
        if (kernel_launch_failures_) {
          kernel_launch_failures_->increment();
        }
    }
    
    /**
     * @brief Record Memory Allocation Failure.
     * @details Calls: increment().
     */
    void recordMemoryAllocationFailure() {
        if (memory_alloc_failures_) {
          memory_alloc_failures_->increment();
        }
    }
    
    /**
     * @brief Set Device Count.
     * @param[in] count Input parameter.
     * @details Calls: set().
     */
    void setDeviceCount(int count) {
        if (device_count_) {
          device_count_->set(count);
        }
    }
    
    /**
     * @brief Set Active Device Index.
     * @param[in] index Input parameter.
     * @details Calls: set().
     */
    void setActiveDeviceIndex(int index) {
        if (active_device_) {
          active_device_->set(index);
        }
    }
    
    double getOperationsPerSecond() const {
        if (!l2_distance_ops_ || !cosine_ops_) {
          return 0.0;
        }
        return static_cast<double>(l2_distance_ops_->value()) +
               static_cast<double>(cosine_ops_->value());
    }
    
    double getVectorsPerSecond() const {
        if (!l2_distance_vectors_ || !cosine_vectors_) {
          return 0.0;
        }
        return static_cast<double>(l2_distance_vectors_->value()) +
               static_cast<double>(cosine_vectors_->value());
    }
    
private:
    /**
     * @brief Register Metrics.
     * @details Calls: MetricsCollector::instance(), registerCounter(), registerHistogram(), registerGauge().
     */
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
