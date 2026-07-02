/**
 * @file adapter_version_manager.h
 * @brief Manages adapter versioning, snapshots, and rollback capability
 * 
 * This component provides:
 * - Version metadata and history tracking
 * - Snapshot persistence for rollback
 * - Version comparison and validation
 * - Automatic version cleanup
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Metadata for a single adapter version
 */
struct AdapterVersionInfo {
    virtual ~AdapterVersionInfo() = default;
    
    // Version identification
    std::string version_id;                 // e.g., "v1.0", "v1.1"
    std::string adapter_id;
    std::chrono::system_clock::time_point created_at;
    
    // Training metadata
    std::string training_source;            // "user_feedback", "telemetry", "manual", etc.
    int training_samples = 0;
    float final_loss = 0.0f;
    float validation_accuracy = 0.0f;
    std::chrono::seconds training_duration;
    
    // Performance metrics
    float avg_accuracy = 0.0f;
    float avg_latency_ms = 0.0f;
    int total_inferences = 0;
    int errors = 0;
    
    // Status
    bool is_active = false;
    bool is_stable = true;
    std::string deployment_status;          // "staging", "production", "disabled"
    
    // Hash for integrity
    std::string weights_hash;
    std::string config_hash;
    
    // Rollback eligibility
    bool can_rollback_to = true;
    std::string rollback_reason_if_not;
    
    json toJSON() const;
    static AdapterVersionInfo fromJSON(const json& j);
};

/**
 * @brief Adapter version snapshot for efficient rollback
 */
struct AdapterVersionSnapshot {
    virtual ~AdapterVersionSnapshot() = default;
    
    std::string version_id;
    std::string adapter_id;
    
    // Snapshot metadata
    std::chrono::system_clock::time_point snapshot_time;
    size_t snapshot_size_bytes = 0;
    std::string storage_path;               // Where snapshot is stored
    bool is_compressed = true;
    
    // Integrity
    std::string checksum;                   // SHA256 checksum
    
    // Composition
    json adapter_config;
    json training_config;
    std::vector<std::string> dependency_versions;
    
    json toJSON() const;
    static AdapterVersionSnapshot fromJSON(const json& j);
};

/**
 * @brief Version comparison result
 */
struct VersionComparison {
    virtual ~VersionComparison() = default;
    
    std::string version_a;
    std::string version_b;
    
    // Improvements from A to B
    float accuracy_delta = 0.0f;            // positive = improvement
    float latency_delta_ms = 0.0f;          // negative = improvement
    float error_rate_delta = 0.0f;          // negative = improvement
    
    // Overall improvement
    bool is_b_better = false;
    float improvement_confidence = 0.0f;
    
    json toJSON() const;
};

/**
 * @brief Manages adapter versioning and rollback capability
 * 
 * Key responsibilities:
 * 1. Track and persist version metadata
 * 2. Create and manage snapshots for rollback
 * 3. Compare versions for quality assessment
 * 4. Support atomic rollback operations
 * 5. Maintain version audit trail
 * 
 * Thread-safe for concurrent operations.
 */
class AdapterVersionManager {
public:
    struct Config {
        // Version retention
        size_t max_versions_kept = 10;
        std::chrono::days version_retention_period{30};
        
        // Snapshot configuration
        std::string snapshot_storage_path = "adapters/snapshots";
        bool compress_snapshots = true;
        size_t max_snapshot_size_mb = 500;
        
        // Rollback safety
        bool verify_checksum_on_rollback = true;
        bool require_approval_for_rollback = false;
    };
    
    explicit AdapterVersionManager(
        const std::string& adapter_id,
        const Config& config = Config{}
    );
    
    ~AdapterVersionManager() = default;
    
    /**
     * @brief Create a new version record
     * 
     * @param version_id Version identifier (e.g., "v1.1")
     * @param training_source Source of training
     * @param metrics Performance metrics
     * @return true if version created successfully
     */
    bool createVersion(
        const std::string& version_id,
        const std::string& training_source,
        const json& metrics
    );
    
    /**
     * @brief Create and store a snapshot for rollback
     * 
     * @param version_id Version to snapshot
     * @param adapter_data Adapter weights/config data
     * @return Snapshot metadata if successful
     */
    std::optional<AdapterVersionSnapshot> createSnapshot(
        const std::string& version_id,
        const json& adapter_data
    );
    
    /**
     * @brief Get version information
     * 
     * @param version_id The version to retrieve
     * @return Version info if found
     */
    std::optional<AdapterVersionInfo> getVersionInfo(const std::string& version_id) const;
    
    /**
     * @brief Get all versions in chronological order
     * 
     * @return Vector of version IDs
     */
    std::vector<std::string> getAllVersions() const;
    
    /**
     * @brief Get the current active version
     */
    std::string getActiveVersion() const;
    
    /**
     * @brief Set a version as active
     * 
     * @param version_id Version to activate
     * @return true if successful
     */
    bool setActiveVersion(const std::string& version_id);
    
    /**
     * @brief Get snapshot for a version
     * 
     * @param version_id The version
     * @return Snapshot metadata if available
     */
    std::optional<AdapterVersionSnapshot> getSnapshot(const std::string& version_id) const;
    
    /**
     * @brief Restore adapter from a snapshot (rollback)
     * 
     * @param version_id Version to restore
     * @return true if rollback successful
     */
    bool rollback(const std::string& version_id);
    
    /**
     * @brief Compare two versions
     * 
     * @param version_a First version
     * @param version_b Second version
     * @return Comparison result
     */
    std::optional<VersionComparison> compareVersions(
        const std::string& version_a,
        const std::string& version_b
    ) const;
    
    /**
     * @brief Mark version as stable/production-ready
     */
    bool markVersionAsStable(const std::string& version_id, bool is_stable);
    
    /**
     * @brief Delete a version (if safe to do so)
     */
    bool deleteVersion(const std::string& version_id);
    
    /**
     * @brief Clean up old versions and snapshots
     * 
     * @return Number of versions/snapshots deleted
     */
    int cleanup();
    
    /**
     * @brief Export version metadata
     */
    json exportVersionMetadata() const;
    
    /**
     * @brief Import version metadata
     */
    bool importVersionMetadata(const json& metadata);
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const Config& config);

private:
    std::string adapter_id_;
    Config config_;
    mutable std::mutex mutex_;
    
    // Version tracking
    std::vector<AdapterVersionInfo> versions_;
    std::string active_version_;
    
    // Snapshots
    std::unordered_map<std::string, AdapterVersionSnapshot> snapshots_;
    
    /**
     * @brief Compute hash of adapter data for integrity
     */
    std::string computeHash(const json& data) const;
    
    /**
     * @brief Validate snapshot integrity
     */
    bool validateSnapshot(const AdapterVersionSnapshot& snapshot) const;
    
    /**
     * @brief Load snapshot from storage
     */
    std::optional<json> loadSnapshot(const std::string& storage_path) const;
    
    /**
     * @brief Save snapshot to storage
     */
    bool saveSnapshot(const std::string& version_id, const json& data);
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
