// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file converged_slo_metrics.h
 * @brief SLO/SLA metrics specific to Converged Storage-Inference topology
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * @note Addresses GAP: SLA/SLO metrics for Converged Storage-Inference
 * @note Part of RAID-Sharding research: THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <mutex>
#include <atomic>
#include <optional>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

/**
 * @brief Error types specific to Converged Storage-Inference topology
 */
enum class ConvergedErrorType {
    NONE = 0,
    // Cross-layer errors
    CROSS_LAYER_VERSION_MISMATCH = 1,     // Storage and cache versions diverged
    CROSS_LAYER_CONSENSUS_FAILED = 2,     // Dual-consensus failed to reach agreement
    CROSS_LAYER_SYNC_TIMEOUT = 3,        // Sync between storage and cache timed out
    
    // Cross-shard errors
    CROSS_SHARD_KV_STALE = 10,           // KV-Cache has stale data across shards
    CROSS_SHARD_INCONSISTENT_READ = 11,   // Different shards returned different data
    CROSS_SHARD_QUORUM_FAILED = 12,      // RAID quorum not achieved
    
    // Inference-specific errors
    INFERENCE_PREMPTION = 20,            // Inference was preempted (resource contention)
    INFERENCE_TIMEOUT = 21,              // Inference request timed out
    INFERENCE_OOM = 22,                  // Inference out of memory
    INFERENCE_GPU_ERROR = 23,            // GPU error during inference
    
    // Grounding errors
    GROUNDING_AUDIT_FAILED = 30,         // Failed to audit grounding across shards
    GROUNDING_VERSION_GAP = 31,          // Gap in version history detected
    GROUNDING Source_MISSING = 32        // Source reference missing
};

/**
 * @brief SLO targets specific to Converged Storage-Inference
 */
struct ConvergedSLOTarget {
    // ========================================================================
    // Availability SLOs
    // ========================================================================
    
    // Overall system availability
    double system_availability_target = 0.999;  // 99.9% (8.77 hours/year downtime)
    
    // Per-layer availability
    double storage_layer_availability = 0.9999;   // 99.99%
    double cache_layer_availability = 0.999;     // 99.9%
    double inference_layer_availability = 0.99; // 99%
    
    // ========================================================================
    // Latency SLOs (milliseconds)
    // ========================================================================
    
    // Cross-layer operation latencies
    double cross_layer_sync_p50_ms = 50.0;
    double cross_layer_sync_p99_ms = 200.0;
    double dual_consensus_commit_p50_ms = 100.0;
    double dual_consensus_commit_p99_ms = 500.0;
    
    // Inference latencies
    double inference_request_p50_ms = 100.0;
    double inference_request_p99_ms = 1000.0;  // 1 second
    double inference_token_generation_ms_per_token = 20.0;  // ms/token
    
    // RAG-specific latencies
    double rag_retrieval_p50_ms = 50.0;
    double rag_retrieval_p99_ms = 200.0;
    double rag_generation_p50_ms = 200.0;
    double rag_generation_p99_ms = 2000.0;  // 2 seconds
    
    // ========================================================================
    // Consistency SLOs
    // ========================================================================
    
    // Maximum acceptable lag between layers
    double max_storage_cache_lag_ms = 100.0;  // Storage can be at most 100ms ahead of cache
    double max_cache_storage_lag_ms = 500.0;  // Cache can be at most 500ms behind storage
    
    // Cross-shard consistency
    double max_cross_shard_version_spread = 1.0;  // Max version difference between shards
    double cross_shard_consistency_target = 0.999; // 99.9% of reads are consistent
    
    // Grounding audit
    double grounding_audit_coverage_target = 1.0;  // 100% of operations should be auditable
    double grounding_version_continuity_target = 1.0; // 100% version continuity
    
    // ========================================================================
    // Durability SLOs
    // ========================================================================
    
    // RAID-specific durability
    double raid0_data_loss_tolerance = 0.0;      // No tolerance (0%)
    double raid1_data_loss_tolerance = 1.0;      // 1 shard failure
    double raid5_data_loss_tolerance = 1.0;      // 1 shard failure
    double raid10_data_loss_tolerance = 1.0;     // 1 shard failure per stripe
    
    // Inference state persistence
    double kv_cache_persistence_target = 0.999;  // 99.9% of cache states persisted
    double inference_state_checkpoint_interval_s = 60.0;  // Checkpoint every 60s
    
    // ========================================================================
    // Resource SLOs
    // ========================================================================
    
    // GPU resource limits
    double gpu_memory_utilization_target = 0.85;  // 85% max GPU memory
    double gpu_compute_utilization_target = 0.90; // 90% max GPU compute
    int max_concurrent_inference_requests = 100;
    
    // Inference preemption
    double max_inference_preemption_rate = 0.01;  // 1% of requests can be preempted
    double preemption_recovery_time_target_s = 2.0; // Recover within 2 seconds
};

/**
 * @brief Metrics for a single Converged Storage-Inference operation
 */
struct ConvergedOperationMetrics {
    std::string operation_id;
    std::string operation_type;  // "read", "write", "inference", "rag", "sync"
    
    // Timing
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::milliseconds duration;
    
    // Layer-specific timing
    std::chrono::milliseconds storage_layer_time;
    std::chrono::milliseconds cache_layer_time;
    std::chrono::milliseconds inference_layer_time;
    std::chrono::milliseconds cross_layer_sync_time;
    
    // Version tracking
    uint64_t storage_version = 0;
    uint64_t cache_version = 0;
    uint64_t merged_version = 0;
    double version_spread = 0.0;  // Difference between max and min versions
    
    // Consistency
    bool cross_layer_consistent = true;
    bool cross_shard_consistent = true;
    
    // Resource usage
    uint64_t bytes_read = 0;
    uint64_t bytes_written = 0;
    uint64_t tokens_generated = 0;
    uint64_t gpu_memory_used_bytes = 0;
    
    // Error tracking
    ConvergedErrorType error_type = ConvergedErrorType::NONE;
    std::string error_message;
    bool success = true;
    
    // Grounding audit
    bool grounding_auditable = true;
    std::vector<std::string> source_references;
    
    // Convert to JSON for serialization
    nlohmann::json toJson() const {
        return {
            {"operation_id", operation_id},
            {"operation_type", operation_type},
            {"duration_ms", duration.count()},
            {"storage_layer_time_ms", storage_layer_time.count()},
            {"cache_layer_time_ms", cache_layer_time.count()},
            {"inference_layer_time_ms", inference_layer_time.count()},
            {"cross_layer_sync_time_ms", cross_layer_sync_time.count()},
            {"storage_version", storage_version},
            {"cache_version", cache_version},
            {"merged_version", merged_version},
            {"version_spread", version_spread},
            {"cross_layer_consistent", cross_layer_consistent},
            {"cross_shard_consistent", cross_shard_consistent},
            {"bytes_read", bytes_read},
            {"bytes_written", bytes_written},
            {"tokens_generated", tokens_generated},
            {"gpu_memory_used_bytes", gpu_memory_used_bytes},
            {"error_type", static_cast<int>(error_type)},
            {"error_message", error_message},
            {"success", success},
            {"grounding_auditable", grounding_auditable},
            {"source_references", source_references}
        };
    }
    
    static ConvergedOperationMetrics fromJson(const nlohmann::json& j) {
        ConvergedOperationMetrics metrics;
        metrics.operation_id = j.value("operation_id", "");
        metrics.operation_type = j.value("operation_type", "");
        metrics.duration = std::chrono::milliseconds(j.value("duration_ms", 0));
        metrics.storage_layer_time = std::chrono::milliseconds(j.value("storage_layer_time_ms", 0));
        metrics.cache_layer_time = std::chrono::milliseconds(j.value("cache_layer_time_ms", 0));
        metrics.inference_layer_time = std::chrono::milliseconds(j.value("inference_layer_time_ms", 0));
        metrics.cross_layer_sync_time = std::chrono::milliseconds(j.value("cross_layer_sync_time_ms", 0));
        metrics.storage_version = j.value("storage_version", 0);
        metrics.cache_version = j.value("cache_version", 0);
        metrics.merged_version = j.value("merged_version", 0);
        metrics.version_spread = j.value("version_spread", 0.0);
        metrics.cross_layer_consistent = j.value("cross_layer_consistent", true);
        metrics.cross_shard_consistent = j.value("cross_shard_consistent", true);
        metrics.bytes_read = j.value("bytes_read", 0);
        metrics.bytes_written = j.value("bytes_written", 0);
        metrics.tokens_generated = j.value("tokens_generated", 0);
        metrics.gpu_memory_used_bytes = j.value("gpu_memory_used_bytes", 0);
        metrics.error_type = static_cast<ConvergedErrorType>(j.value("error_type", 0));
        metrics.error_message = j.value("error_message", "");
        metrics.success = j.value("success", true);
        metrics.grounding_auditable = j.value("grounding_auditable", true);
        metrics.source_references = j.value("source_references", std::vector<std::string>{});
        return metrics;
    }
};

/**
 * @brief Aggregated SLO metrics for Converged Storage-Inference
 */
struct ConvergedSLOMetrics {
    // ========================================================================
    // Operation Counters
    // ========================================================================
    
    std::atomic<uint64_t> total_operations = 0;
    std::atomic<uint64_t> successful_operations = 0;
    std::atomic<uint64_t> failed_operations = 0;
    
    // Per-operation-type counters
    std::atomic<uint64_t> read_operations = 0;
    std::atomic<uint64_t> write_operations = 0;
    std::atomic<uint64_t> inference_operations = 0;
    std::atomic<uint64_t> rag_operations = 0;
    std::atomic<uint64_t> sync_operations = 0;
    
    // ========================================================================
    // Error Counters
    // ========================================================================
    
    std::atomic<uint64_t> cross_layer_version_mismatch = 0;
    std::atomic<uint64_t> cross_layer_consensus_failed = 0;
    std::atomic<uint64_t> cross_layer_sync_timeout = 0;
    std::atomic<uint64_t> cross_shard_kv_stale = 0;
    std::atomic<uint64_t> cross_shard_inconsistent_read = 0;
    std::atomic<uint64_t> cross_shard_quorum_failed = 0;
    std::atomic<uint64_t> inference_preemption = 0;
    std::atomic<uint64_t> inference_timeout = 0;
    std::atomic<uint64_t> inference_oom = 0;
    std::atomic<uint64_t> inference_gpu_error = 0;
    std::atomic<uint64_t> grounding_audit_failed = 0;
    std::atomic<uint64_t> grounding_version_gap = 0;
    
    // ========================================================================
    // Latency Aggregation
    // ========================================================================
    
    std::mutex latency_mutex;
    std::vector<double> cross_layer_sync_latencies;
    std::vector<double> dual_consensus_latencies;
    std::vector<double> inference_latencies;
    std::vector<double> rag_latencies;
    
    size_t max_latency_samples = 10000;
    
    // ========================================================================
    // Consistency Metrics
    // ========================================================================
    
    std::atomic<uint64_t> consistent_operations = 0;
    std::atomic<uint64_t> inconsistent_operations = 0;
    std::atomic<double> avg_version_spread = 0.0;
    std::atomic<double> max_version_spread = 0.0;
    
    // ========================================================================
    // Grounding Audit Metrics
    // ========================================================================
    
    std::atomic<uint64_t> auditable_operations = 0;
    std::atomic<uint64_t> non_auditable_operations = 0;
    
    // ========================================================================
    // Resource Metrics
    // ========================================================================
    
    std::atomic<uint64_t> total_tokens_generated = 0;
    std::atomic<uint64_t> total_bytes_read = 0;
    std::atomic<uint64_t> total_bytes_written = 0;
    std::atomic<uint64_t> total_gpu_memory_used = 0;
    
    // ========================================================================
    // Methods
    // ========================================================================
    
    /** @brief Record a Converged operation */
    void recordOperation(const ConvergedOperationMetrics& metrics) {
        total_operations++;
        
        if (metrics.success) {
            successful_operations++;
        } else {
            failed_operations++;
        }
        
        // Operation type counters
        if (metrics.operation_type == "read") {
          read_operations++;
        }
        else if (metrics.operation_type == "write") write_operations++;
        else if (metrics.operation_type == "inference") inference_operations++;
        else if (metrics.operation_type == "rag") rag_operations++;
        else if (metrics.operation_type == "sync") sync_operations++;
        
        // Error type counters
        switch (metrics.error_type) {
            case ConvergedErrorType::CROSS_LAYER_VERSION_MISMATCH: cross_layer_version_mismatch++; break;
            case ConvergedErrorType::CROSS_LAYER_CONSENSUS_FAILED: cross_layer_consensus_failed++; break;
            case ConvergedErrorType::CROSS_LAYER_SYNC_TIMEOUT: cross_layer_sync_timeout++; break;
            case ConvergedErrorType::CROSS_SHARD_KV_STALE: cross_shard_kv_stale++; break;
            case ConvergedErrorType::CROSS_SHARD_INCONSISTENT_READ: cross_shard_inconsistent_read++; break;
            case ConvergedErrorType::CROSS_SHARD_QUORUM_FAILED: cross_shard_quorum_failed++; break;
            case ConvergedErrorType::INFERENCE_PREMPTION: inference_preemption++; break;
            case ConvergedErrorType::INFERENCE_TIMEOUT: inference_timeout++; break;
            case ConvergedErrorType::INFERENCE_OOM: inference_oom++; break;
            case ConvergedErrorType::INFERENCE_GPU_ERROR: inference_gpu_error++; break;
            case ConvergedErrorType::GROUNDING_AUDIT_FAILED: grounding_audit_failed++; break;
            case ConvergedErrorType::GROUNDING_VERSION_GAP: grounding_version_gap++; break;
            default: break;
        }
        
        // Consistency metrics
        if (metrics.cross_layer_consistent && metrics.cross_shard_consistent) {
            consistent_operations++;
        } else {
            inconsistent_operations++;
        }
        
        // Version spread tracking
        if (metrics.version_spread > max_version_spread.load()) {
            max_version_spread = metrics.version_spread;
        }
        
        // Grounding audit
        if (metrics.grounding_auditable) {
            auditable_operations++;
        } else {
            non_auditable_operations++;
        }
        
        // Resource metrics
        total_tokens_generated += metrics.tokens_generated;
        total_bytes_read += metrics.bytes_read;
        total_bytes_written += metrics.bytes_written;
        total_gpu_memory_used += metrics.gpu_memory_used_bytes;
        
        // Latency recording
        std::lock_guard<std::mutex> lock(latency_mutex);
        if (metrics.cross_layer_sync_time.count() > 0) {
            cross_layer_sync_latencies.push_back(metrics.cross_layer_sync_time.count());
            if (cross_layer_sync_latencies.size() > max_latency_samples) {
                cross_layer_sync_latencies.erase(cross_layer_sync_latencies.begin());
            }
        }
        if (metrics.inference_layer_time.count() > 0) {
            inference_latencies.push_back(metrics.inference_layer_time.count());
            if (inference_latencies.size() > max_latency_samples) {
                inference_latencies.erase(inference_latencies.begin());
            }
        }
    }
    
    /** @brief Get error rate for a specific error type */
    double getErrorRate(ConvergedErrorType error_type) const {
        uint64_t error_count = 0;
        switch (error_type) {
            case ConvergedErrorType::CROSS_LAYER_VERSION_MISMATCH: error_count = cross_layer_version_mismatch; break;
            case ConvergedErrorType::CROSS_LAYER_CONSENSUS_FAILED: error_count = cross_layer_consensus_failed; break;
            case ConvergedErrorType::CROSS_LAYER_SYNC_TIMEOUT: error_count = cross_layer_sync_timeout; break;
            case ConvergedErrorType::CROSS_SHARD_KV_STALE: error_count = cross_shard_kv_stale; break;
            case ConvergedErrorType::CROSS_SHARD_INCONSISTENT_READ: error_count = cross_shard_inconsistent_read; break;
            case ConvergedErrorType::CROSS_SHARD_QUORUM_FAILED: error_count = cross_shard_quorum_failed; break;
            case ConvergedErrorType::INFERENCE_PREMPTION: error_count = inference_preemption; break;
            case ConvergedErrorType::INFERENCE_TIMEOUT: error_count = inference_timeout; break;
            case ConvergedErrorType::INFERENCE_OOM: error_count = inference_oom; break;
            case ConvergedErrorType::INFERENCE_GPU_ERROR: error_count = inference_gpu_error; break;
            case ConvergedErrorType::GROUNDING_AUDIT_FAILED: error_count = grounding_audit_failed; break;
            case ConvergedErrorType::GROUNDING_VERSION_GAP: error_count = grounding_version_gap; break;
            default: return 0.0;
        }
        uint64_t total = total_operations.load();
        return total > 0 ? static_cast<double>(error_count) / total : 0.0;
    }
    
    /** @brief Get availability ratio */
    double getAvailability() const {
        uint64_t total = total_operations.load();
        uint64_t success = successful_operations.load();
        return total > 0 ? static_cast<double>(success) / total : 1.0;
    }
    
    /** @brief Get consistency ratio */
    double getConsistencyRatio() const {
        uint64_t total = consistent_operations.load() + inconsistent_operations.load();
        uint64_t consistent = consistent_operations.load();
        return total > 0 ? static_cast<double>(consistent) / total : 1.0;
    }
    
    /** @brief Get grounding audit coverage */
    double getGroundingAuditCoverage() const {
        uint64_t total = auditable_operations.load() + non_auditable_operations.load();
        uint64_t auditable = auditable_operations.load();
        return total > 0 ? static_cast<double>(auditable) / total : 1.0;
    }
    
    /** @brief Get p50 latency for a specific operation type */
    double getLatencyP50(const std::string& op_type) const;
    
    /** @brief Get p99 latency for a specific operation type */
    double getLatencyP99(const std::string& op_type) const;
    
    /** @brief Reset all metrics */
    void reset();
    
    /** @brief Get all metrics as JSON */
    nlohmann::json toJson() const;
};

/**
 * @brief Converged SLO Monitor for tracking Converged Storage-Inference specific metrics
 */
class ConvergedSLOMonitor {
public:
    /** @brief Configuration for Converged SLO Monitor */
    struct Config {
        ConvergedSLOTarget targets;
        std::chrono::seconds window_duration = std::chrono::hours(1);
        bool enable_alerting = true;
        double alert_threshold = 0.9;  // Alert when 90% of error budget consumed
        
        static Config defaults() { return {}; }
    };
    
    /** @brief Construct Converged SLO Monitor */
    explicit ConvergedSLOMonitor(const Config& config = Config::defaults());
    ~ConvergedSLOMonitor() = default;
    
    /** @brief Record a Converged operation */
    void recordOperation(const ConvergedOperationMetrics& metrics);
    
    /** @brief Record a specific error */
    void recordError(ConvergedErrorType error_type, const std::string& message = "");
    
    /** @brief Record cross-shard KV-stale error */
    void recordCrossShardKVStale(const std::string& shard_id, uint64_t stale_version, uint64_t current_version);
    
    /** @brief Record inference preemption */
    void recordInferencePreemption(uint64_t tokens_generated, uint64_t tokens_lost);
    
    /** @brief Check if SLO is being met */
    bool isSLOMet() const;
    
    /** @brief Get SLO compliance report */
    nlohmann::json getSLOReport() const;
    
    /** @brief Get error budget remaining */
    double getErrorBudgetRemaining() const;
    
    /** @brief Get current metrics */
    ConvergedSLOMetrics getCurrentMetrics() const;
    
    /** @brief Reset all metrics */
    void reset();
    
    /** @brief Get configuration */
    const Config& getConfig() const { return config_; }
    
private:
    Config config_;
    ConvergedSLOMetrics metrics_;
    std::chrono::steady_clock::time_point window_start_;
};

} // namespace sharding
} // namespace themisdb
