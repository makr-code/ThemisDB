/**
 * @file resource_monitor.h
 * @brief Memory and resource exhaustion detection for ingestion pipelines.
 *
 * Phase 2.9 (Bounded Resources + Stress Tests) — ING-IMPL-002, ING-IMPL-004
 *
 * Provides:
 * - System memory monitoring and exhaustion alerts
 * - Per-component resource tracking
 * - Quality check timeout mechanisms
 * - Distributed resource coordination hints
 *
 * @see src/ingestion/ROADMAP.md — Phase 2.9 item
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace ingestion {

// ============================================================================
// Resource exhaustion alert
// ============================================================================

/**
 * @brief Types of resource exhaustion events that can be detected.
 */
enum class ResourceExhaustionType {
    MEMORY_PRESSURE,      ///< System memory pressure high
    QUEUE_SATURATION,     ///< Ingestion queue saturated
    QUALITY_CHECK_TIMEOUT, ///< Quality check timed out
    CONNECTION_POOL_EXHAUSTED, ///< Database/API connection pool empty
    DISK_SPACE_LOW,       ///< Disk space below threshold
    THREAD_POOL_EXHAUSTED ///< Thread pool at capacity
};

/**
 * @brief Alert generated when a resource is exhausted.
 */
struct ResourceExhaustionAlert {
    ResourceExhaustionType type;
    std::string component;            ///< Component reporting exhaustion (e.g., "quality_judge")
    std::string description;          ///< Human-readable description
    double current_usage = 0.0;       ///< Current utilization (0.0-1.0 scale)
    double threshold = 0.0;           ///< Threshold that was exceeded
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Callback type for resource exhaustion alerts.
 *
 * Called whenever a resource exhaustion event is detected.
 */
using ResourceExhaustionCallback = std::function<void(const ResourceExhaustionAlert&)>;

// ============================================================================
// Quality check timeout configuration and results
// ============================================================================

/**
 * @brief Configuration for quality check operations with timeout.
 *
 * Prevents quality checks from blocking indefinitely and provides
 * escape valve behavior when quality checks time out.
 */
struct QualityCheckTimeoutConfig {
    /**
     * @brief Timeout for individual quality checks.
     *
     * If a quality check does not complete within this time,
     * it is canceled and the escape valve strategy is invoked.
     * Default: 5 seconds.
     */
    std::chrono::milliseconds timeout_ms = std::chrono::seconds(5);

    /**
     * @brief Escape valve strategy when quality check times out.
     *
     * Determines what action to take when a quality check times out:
     * - "fail_closed": Reject the document (fail-safe behavior)
     * - "fail_open": Accept the document anyway
     * - "defer": Defer quality check to a later batch (accumulate and retry)
     */
    std::string escape_valve = "fail_closed";

    /**
     * @brief Maximum number of deferred quality checks to accumulate.
     *
     * If escape_valve is "defer" and deferred checks exceed this count,
     * fall back to "fail_closed". Default: 1000.
     */
    std::size_t max_deferred_checks = 1000;

    /**
     * @brief Enable parallel quality checks for throughput.
     *
     * If true, multiple documents can be quality-checked in parallel
     * (up to parallel_check_limit). Default: true.
     */
    bool enable_parallel_checks = true;

    /**
     * @brief Maximum number of parallel quality check tasks.
     *
     * If enable_parallel_checks is true, at most this many documents
     * can be checked concurrently. Default: 4.
     */
    int parallel_check_limit = 4;
};

/**
 * @brief Result of a quality check operation.
 */
enum class QualityCheckResult {
    PASS,            ///< Document passed quality threshold
    FAIL,            ///< Document failed quality threshold
    TIMEOUT,         ///< Quality check timed out
    DEFERRED,        ///< Check deferred to next batch
    ESCAPE_VALVE     ///< Escape valve invoked (fail_open)
};

// ============================================================================
// Resource monitor for tracking system and component resource usage
// ============================================================================

/**
 * @brief Monitors system and component resource usage.
 *
 * Tracks memory, CPU, disk, and component-specific resources.
 * Generates alerts when thresholds are exceeded.
 *
 * Example usage:
 * @code
 * ResourceMonitor monitor;
 *
 * // Register alert callback
 * monitor.onResourceExhaustion([](const ResourceExhaustionAlert& alert) {
 *     std::cerr << "Resource alert: " << alert.description << std::endl;
 * });
 *
 * // Start monitoring
 * monitor.startMonitoring();
 *
 * // Register component resource usage
 * monitor.registerComponentMemory("api_connector", 1024 * 1024);  // 1 MB
 *
 * // Update component usage
 * monitor.updateComponentMemory("api_connector", 2048 * 1024);  // 2 MB
 *
 * // Get current system memory usage
 * auto system_mem = monitor.getSystemMemoryPercent();
 * if (system_mem > 85.0) {
 *     // Take action
 * }
 *
 * // Stop monitoring
 * monitor.stopMonitoring();
 * @endcode
 */
class ResourceMonitor {
public:
    /**
     * @brief Construct a resource monitor with default thresholds.
     */
    ResourceMonitor() = default;

    ~ResourceMonitor() { stopMonitoring(); }

    // Delete copy/move to prevent multiple monitor instances
    ResourceMonitor(const ResourceMonitor&) = delete;
    ResourceMonitor& operator=(const ResourceMonitor&) = delete;

    // ── Lifecycle ───────────────────────────────────────────────────────────

    /**
     * @brief Start background monitoring thread.
     *
     * The monitor thread polls system resources and checks component
     * usage at a regular interval. Default interval: 100 ms.
     */
    void startMonitoring() {
        if (is_running_.exchange(true)) {
            return;  // Already running
        }
        // Monitoring implementation details (polling thread, etc.)
        // to be implemented in .cpp file
    }

    /**
     * @brief Stop background monitoring thread.
     */
    void stopMonitoring() {
        is_running_.store(false);
    }

    /**
     * @brief Check if monitoring is active.
     */
    bool isMonitoring() const { return is_running_.load(); }

    // ── Alert registration ──────────────────────────────────────────────────

    /**
     * @brief Register a callback to be invoked on resource exhaustion events.
     * @param callback Function to invoke when a resource exhaustion event occurs
     */
    void onResourceExhaustion(ResourceExhaustionCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        exhaustion_callbacks_.push_back(callback);
    }

    // ── System memory monitoring ────────────────────────────────────────────

    /**
     * @brief Get current system memory usage as a percentage (0-100).
     * @return Percentage of total system memory currently in use
     */
    double getSystemMemoryPercent() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return system_memory_percent_;
    }

    /**
     * @brief Set the memory usage threshold that triggers exhaustion alerts.
     * @param percent_threshold Percentage (0-100) above which to alert
     */
    void setMemoryExhaustionThreshold(double percent_threshold) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        memory_exhaustion_threshold_ = percent_threshold;
    }

    /**
     * @brief Get the current memory exhaustion threshold.
     */
    double getMemoryExhaustionThreshold() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return memory_exhaustion_threshold_;
    }

    // ── Component memory tracking ───────────────────────────────────────────

    /**
     * @brief Register a component and its initial memory usage.
     * @param component_name Logical name of the component (e.g., "api_connector")
     * @param memory_bytes    Initial memory allocation in bytes
     */
    void registerComponentMemory(const std::string& component_name,
                                  std::size_t memory_bytes) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        component_memory_[component_name] = memory_bytes;
    }

    /**
     * @brief Update the memory usage reported for a component.
     * @param component_name Component identifier
     * @param memory_bytes   Updated memory usage
     */
    void updateComponentMemory(const std::string& component_name,
                                std::size_t memory_bytes) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        component_memory_[component_name] = memory_bytes;
    }

    /**
     * @brief Get the current memory usage for a component.
     * @param component_name Component identifier
     * @return Memory usage in bytes, or std::nullopt if component not found
     */
    std::optional<std::size_t> getComponentMemory(
        const std::string& component_name) const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = component_memory_.find(component_name);
        if (it != component_memory_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get total memory usage across all registered components.
     */
    std::size_t getTotalComponentMemory() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        std::size_t total = 0;
        for (const auto& kv : component_memory_) {
            total += kv.second;
        }
        return total;
    }

    /**
     * @brief Get memory usage for all registered components.
     * @return Map of component name -> memory bytes
     */
    std::map<std::string, std::size_t> getAllComponentMemory() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return component_memory_;
    }

    /**
     * @brief Remove a component from memory tracking.
     */
    void unregisterComponent(const std::string& component_name) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        component_memory_.erase(component_name);
    }

    // ── Quality check timeout support ───────────────────────────────────────

    /**
     * @brief Set the quality check timeout configuration.
     */
    void setQualityCheckTimeoutConfig(
        const QualityCheckTimeoutConfig& config) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        qc_timeout_config_ = config;
    }

    /**
     * @brief Get the current quality check timeout configuration.
     */
    QualityCheckTimeoutConfig getQualityCheckTimeoutConfig() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return qc_timeout_config_;
    }

    // ── Distributed resource coordination ───────────────────────────────────

    /**
     * @brief Notify the monitor of the current distributed cluster size.
     *
     * Used for coordinating resource limits across multiple nodes.
     * For example, if cluster size is 3, each node's local limit
     * might be adjusted to 1/3 of the cluster-wide limit.
     *
     * @param node_count Number of nodes in the cluster
     */
    void setDistributedClusterSize(std::size_t node_count) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        cluster_size_ = node_count;
    }

    /**
     * @brief Get the distributed cluster size.
     */
    std::size_t getDistributedClusterSize() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return cluster_size_;
    }

    /**
     * @brief Calculate per-node resource quota given a cluster-wide limit.
     *
     * Useful for distributing a global limit across cluster nodes
     * to prevent starvation.
     *
     * Example: if cluster-wide limit is 1000 items and there are 5 nodes,
     * per-node quota is 200 items.
     *
     * @param cluster_wide_limit Total limit across all nodes
     * @return Per-node quota (cluster_wide_limit / cluster_size)
     */
    std::size_t calculatePerNodeQuota(std::size_t cluster_wide_limit) const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (cluster_size_ == 0) return cluster_wide_limit;
        return cluster_wide_limit / cluster_size_;
    }

private:
    mutable std::mutex callback_mutex_;
    mutable std::mutex state_mutex_;
    std::atomic<bool> is_running_{false};

    // System state
    double system_memory_percent_ = 0.0;
    double memory_exhaustion_threshold_ = 85.0;  // Alert at 85%

    // Component tracking
    std::map<std::string, std::size_t> component_memory_;

    // Quality check config
    QualityCheckTimeoutConfig qc_timeout_config_;

    // Distributed coordination
    std::size_t cluster_size_ = 1;

    // Callbacks
    std::vector<ResourceExhaustionCallback> exhaustion_callbacks_;

    // Helper to generate and dispatch exhaustion alert
    void alertExhaustion(const ResourceExhaustionAlert& alert) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& cb : exhaustion_callbacks_) {
            cb(alert);
        }
    }
};

}  // namespace ingestion
}  // namespace themis

#endif  // THEMISDB_INCLUDE_INGESTION_RESOURCE_MONITOR_H
