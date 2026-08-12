/**
 * @file self_awareness.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis::util {

/**
 * Self-Awareness System for ThemisDB
 * 
 * Monitors and reports on ThemisDB's own state, health, and capabilities.
 * Triggered automatically when audit logs are signed to create introspective
 * snapshots of the system state.
 * 
 * Features:
 * - System health monitoring (CPU, memory, disk)
 * - Capability state awareness (what we can do)
 * - Shard topology awareness (where data lives)
 * - Resource utilization tracking
 * - Historical state comparison
 * - Anomaly detection (self-diagnosis)
 */
class SelfAwareness {
public:
    /**
     * System health metrics
     */
    struct HealthMetrics {
        // CPU metrics
        double cpu_usage_percent = 0.0;
        double cpu_load_1min = 0.0;
        double cpu_load_5min = 0.0;
        double cpu_load_15min = 0.0;
        
        // Memory metrics
        uint64_t memory_total_bytes = 0;
        uint64_t memory_used_bytes = 0;
        uint64_t memory_available_bytes = 0;
        double memory_usage_percent = 0.0;
        
        // Disk metrics
        uint64_t disk_total_bytes = 0;
        uint64_t disk_used_bytes = 0;
        uint64_t disk_available_bytes = 0;
        double disk_usage_percent = 0.0;
        
        // Network metrics
        uint64_t network_bytes_in = 0;
        uint64_t network_bytes_out = 0;
        uint64_t network_packets_in = 0;
        uint64_t network_packets_out = 0;
        
        // Process metrics
        uint32_t thread_count = 0;
        uint32_t open_file_descriptors = 0;
        uint64_t uptime_seconds = 0;
    };
    
    /**
     * Capability state - what ThemisDB knows about itself
     */
    struct CapabilityState {
        // Shards
        uint32_t total_shards = 0;
        uint32_t active_shards = 0;
        uint32_t inactive_shards = 0;
        std::vector<std::string> shard_ids;
        
        // Capabilities
        uint32_t total_capabilities_configured = 0;
        uint32_t auto_generated_capabilities = 0;
        uint32_t manually_configured_capabilities = 0;
        
        // Data volume
        uint64_t total_documents = 0;
        uint64_t total_size_bytes = 0;
        
        // Keywords & domains
        uint32_t total_unique_keywords = 0;
        uint32_t total_unique_domains = 0;
        uint32_t total_unique_organizations = 0;
        uint32_t total_unique_regions = 0;
    };
    
    /**
     * Query performance awareness
     */
    struct QueryPerformance {
        // Adaptive routing stats
        uint64_t total_queries = 0;
        uint64_t adaptive_routed_queries = 0;
        uint64_t scatter_gather_queries = 0;
        double adaptive_routing_ratio = 0.0;
        
        // Performance metrics
        double avg_query_time_ms = 0.0;
        double p50_query_time_ms = 0.0;
        double p95_query_time_ms = 0.0;
        double p99_query_time_ms = 0.0;
        
        // Efficiency metrics
        double avg_shards_queried = 0.0;
        double network_traffic_saved_percent = 0.0;
        uint64_t iterations_saved = 0;
    };
    
    /**
     * Self-awareness snapshot - complete state at a point in time
     */
    struct Snapshot {
        std::chrono::system_clock::time_point timestamp;
        std::string triggered_by;              // "audit_signing", "manual", "scheduled"
        
        HealthMetrics health;
        CapabilityState capabilities;
        QueryPerformance performance;
        
        // Anomalies detected
        std::vector<std::string> anomalies;
        
        // Self-assessment
        std::string overall_health_status;     // "excellent", "good", "degraded", "critical"
        double confidence_score = 1.0;          // 0.0-1.0: How confident are we in our state?
        
        /**
         * Convert snapshot to JSON
         */
        nlohmann::json toJSON() const;
    };
    
    /**
     * Configuration for self-awareness system
     */
    struct Config {
        bool enabled = true;
        
        // When to trigger snapshots
        bool on_audit_signing = true;          // Trigger on every audit log signature
        bool on_schedule = false;              // Periodic snapshots
        std::chrono::seconds schedule_interval{3600};  // Default: hourly
        
        // Anomaly detection thresholds
        double cpu_warning_threshold = 0.80;   // 80% CPU
        double cpu_critical_threshold = 0.95;  // 95% CPU
        double memory_warning_threshold = 0.80;
        double memory_critical_threshold = 0.90;
        double disk_warning_threshold = 0.80;
        double disk_critical_threshold = 0.90;
        
        // Historical snapshots
        uint32_t max_snapshots_retained = 100;
        bool persist_snapshots = true;
        std::string snapshot_directory = "/var/lib/themisdb/self-awareness";
        
        /**
         * Load configuration from YAML
         */
        static Config loadFromYAML(const std::string& yaml_path);
    };
    
    /**
     * Construct self-awareness system
     */
    explicit SelfAwareness(const Config& config);
    
    /**
     * Destructor
     */
    ~SelfAwareness();
    
    /**
     * Take a snapshot of current system state
     * 
     * @param triggered_by What triggered this snapshot
     * @return Snapshot of current state
     */
    Snapshot takeSnapshot(const std::string& triggered_by = "manual");
    
    /**
     * Trigger self-awareness on audit log signing
     * 
     * Called automatically when audit trail is signed.
     * Creates snapshot and analyzes state.
     * 
     * @param audit_entry The audit log entry that was signed
     * @return Snapshot created
     */
    Snapshot onAuditSigning(const nlohmann::json& audit_entry);
    
    /**
     * Get all historical snapshots
     */
    std::vector<Snapshot> getSnapshots() const {
        return snapshots_;
    }
    
    /**
     * Get latest snapshot
     */
    Snapshot getLatestSnapshot() const {
        if (snapshots_.empty()) {
            return Snapshot{};
        }
        return snapshots_.back();
    }
    
    /**
     * Compare current state with previous snapshot
     * 
     * @return JSON describing changes
     */
    nlohmann::json compareWithPrevious() const;
    
    /**
     * Detect anomalies in current state
     * 
     * @param snapshot Current snapshot
     * @return List of detected anomalies
     */
    std::vector<std::string> detectAnomalies(const Snapshot& snapshot) const;
    
    /**
     * Self-assessment: Determine overall health status
     * 
     * @param snapshot Current snapshot
     * @return Health status string
     */
    std::string assessOverallHealth(const Snapshot& snapshot) const;
    
    /**
     * Get statistics about self-awareness system
     */
    nlohmann::json getStatistics() const;
    
    /**
     * Update configuration at runtime
     */
    void updateConfig(const Config& config) {
        config_ = config;
    }
    
    /**
     * Get current configuration
     */
    Config getConfig() const {
        return config_;
    }

private:
    Config config_;
    std::vector<Snapshot> snapshots_;
    
    /**
     * Collect health metrics
     */
    HealthMetrics collectHealthMetrics() const;
    
    /**
     * Collect capability state
     */
    CapabilityState collectCapabilityState() const;
    
    /**
     * Collect query performance metrics
     */
    QueryPerformance collectQueryPerformance() const;
    
    /**
     * Persist snapshot to disk
     */
    void persistSnapshot(const Snapshot& snapshot);
    
    /**
     * Load snapshots from disk
     */
    void loadSnapshots();
    
    /**
     * Prune old snapshots to maintain max_snapshots_retained
     */
    void pruneSnapshots();
};

} // namespace themis::util
