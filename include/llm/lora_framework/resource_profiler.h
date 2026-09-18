/**
 * @file resource_profiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

struct ResourceSnapshot {
    /**
     * @brief Resource Snapshot.
     * @return Return value.
     */
    virtual ~ResourceSnapshot() = default;
    std::chrono::system_clock::time_point timestamp;
    
    // GPU Memory (bytes)
    size_t gpu_memory_allocated = 0;
    size_t gpu_memory_reserved = 0;
    size_t gpu_memory_free = 0;
    size_t gpu_memory_total = 0;
    
    // CPU Memory (bytes)
    size_t cpu_memory_used = 0;
    size_t cpu_memory_available = 0;
    
    // GPU Utilization (0-100%)
    float gpu_utilization = 0.0f;
    float gpu_memory_utilization = 0.0f;
    
    // Training metrics
    int current_epoch = 0;
    int current_step = 0;
    float current_loss = 0.0f;
    float learning_rate = 0.0f;
    
    // Throughput
    float samples_per_second = 0.0f;
    float tokens_per_second = 0.0f;
    
    json toJSON() const {
        auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
        return json{
            {"timestamp", time_t_val},
            {"gpu_memory", {
                {"allocated_mb", gpu_memory_allocated / (1024 * 1024)},
                {"reserved_mb", gpu_memory_reserved / (1024 * 1024)},
                {"free_mb", gpu_memory_free / (1024 * 1024)},
                {"total_mb", gpu_memory_total / (1024 * 1024)},
                {"utilization_pct", gpu_memory_utilization}
            }},
            {"cpu_memory", {
                {"used_mb", cpu_memory_used / (1024 * 1024)},
                {"available_mb", cpu_memory_available / (1024 * 1024)}
            }},
            {"gpu_utilization_pct", gpu_utilization},
            {"training", {
                {"epoch", current_epoch},
                {"step", current_step},
                {"loss", current_loss},
                {"learning_rate", learning_rate}
            }},
            {"throughput", {
                {"samples_per_sec", samples_per_second},
                {"tokens_per_sec", tokens_per_second}
            }}
        };
    }
};

struct ResourceStats {
    /**
     * @brief Resource Stats.
     * @return Return value.
     */
    virtual ~ResourceStats() = default;
    // Memory peaks
    size_t peak_gpu_memory = 0;
    size_t peak_cpu_memory = 0;
    
    // Average utilization
    float avg_gpu_utilization = 0.0f;
    float avg_gpu_memory_utilization = 0.0f;
    
    // Throughput
    float avg_samples_per_second = 0.0f;
    float avg_tokens_per_second = 0.0f;
    
    // Training time
    std::chrono::seconds total_training_time{0};
    
    // Number of snapshots
    size_t num_snapshots = 0;
    
    json toJSON() const {
        return json{
            {"peak_memory", {
                {"gpu_mb", peak_gpu_memory / (1024 * 1024)},
                {"cpu_mb", peak_cpu_memory / (1024 * 1024)}
            }},
            {"average_utilization", {
                {"gpu_pct", avg_gpu_utilization},
                {"gpu_memory_pct", avg_gpu_memory_utilization}
            }},
            {"average_throughput", {
                {"samples_per_sec", avg_samples_per_second},
                {"tokens_per_sec", avg_tokens_per_second}
            }},
            {"total_training_time_sec", total_training_time.count()},
            {"num_snapshots", num_snapshots}
        };
    }
};

using ResourceMonitorCallback = std::function<void(const ResourceSnapshot&)>;

class ResourceProfiler {
public:
    struct Config {
        bool enabled = true;
        int snapshot_interval_steps = 10;  // Take snapshot every N steps
        bool log_to_file = true;
        std::string log_file = "resource_profile.jsonl";
        bool verbose_logging = false;
        
        // Alerts
        bool enable_alerts = true;
        float gpu_memory_alert_threshold = 0.90f;  // Alert at 90% usage
        float gpu_utilization_alert_threshold = 0.95f;  // Alert at 95%
    };
    
    /**
     * @brief Resource Profiler.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ResourceProfiler(const Config& config);
    /**
     * @brief Resource Profiler.
     * @return Return value.
     */
    explicit ResourceProfiler();
    ~ResourceProfiler();
    
    // Disable copy
    ResourceProfiler(const ResourceProfiler&) = delete;
    ResourceProfiler& operator=(const ResourceProfiler&) = delete;
    
    /**
     * @brief Start.
     */
    void start();
    
    /**
     * @brief Stop.
     */
    void stop();
    
    /**
     * @brief Snapshot.
     * @param[in] epoch Input parameter.
     * @param[in] step Input parameter.
     * @param[in] loss Input parameter.
     * @param[in] lr Input parameter.
     */
    void snapshot(int epoch, int step, float loss, float lr);
    
    /**
     * @brief Get current snapshot.
     * @return Return value.
     */
    ResourceSnapshot get_current_snapshot() const;
    
    /**
     * @brief Get snapshots.
     * @return Return value.
     */
    std::vector<ResourceSnapshot> get_snapshots() const;
    
    /**
     * @brief Compute stats.
     * @return Return value.
     */
    ResourceStats compute_stats() const;
    
    /**
     * @brief Register callback.
     * @param[in] callback Input parameter.
     */
    void register_callback(ResourceMonitorCallback callback);
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Is running.
     * @return True when the operation succeeds.
     */
    bool is_running() const;
    
    /**
     * @brief Get config.
     * @return Return value.
     */
    Config get_config() const;
    
    /**
     * @brief Set config.
     * @param[in] config Input parameter.
     */
    void set_config(const Config& config);
    
    /**
     * @brief Export to json.
     * @param[in] filename Input parameter.
     */
    void export_to_json(const std::string& filename) const;
    
    /**
     * @brief Get peak gpu memory.
     * @return Return value.
     */
    size_t get_peak_gpu_memory() const;
    
    /**
     * @brief Get peak cpu memory.
     * @return Return value.
     */
    size_t get_peak_cpu_memory() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Query gpu memory.
     * @param[in,out] snapshot Input/output parameter.
     */
    void query_gpu_memory(ResourceSnapshot& snapshot) const;
    
    /**
     * @brief Query cpu memory.
     * @param[in,out] snapshot Input/output parameter.
     */
    void query_cpu_memory(ResourceSnapshot& snapshot) const;
    
    /**
     * @brief Query gpu utilization.
     * @param[in,out] snapshot Input/output parameter.
     */
    void query_gpu_utilization(ResourceSnapshot& snapshot) const;
    
    /**
     * @brief Check alerts.
     * @param[in] snapshot Input parameter.
     */
    void check_alerts(const ResourceSnapshot& snapshot);
    
    /**
     * @brief Log snapshot.
     * @param[in] snapshot Input parameter.
     */
    void log_snapshot(const ResourceSnapshot& snapshot);
};

} // namespace lora
} // namespace llm
} // namespace themis
