/**
 * @file capability_auto_generator.h
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
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>
#include "sharding/shard_topology.h"
#include "sharding/shard_capabilities.h"
#include "storage/rocksdb_wrapper.h"

namespace themis::util {

// Forward declaration
class SelfAwareness;

/**
 * Capability Auto-Generator
 * 
 * Utility for automatically generating and updating shard capability configurations
 * by analyzing RocksDB data. Runs as background thread with configurable
 * update schedules.
 * 
 * Features:
 * - Automatic metadata extraction from RocksDB
 * - Keyword analysis (TF-IDF)
 * - Configurable update frequencies per shard type
 * - Audit trail with signatures
 * - YAML-based configuration
 */
class CapabilityAutoGenerator {
public:
    /**
     * Update schedule configuration for different shard types
     */
    struct UpdateSchedule {
        std::string shard_type;              // e.g., "real-time", "high-frequency", "normal", "static"
        std::chrono::seconds interval;        // Update interval
        bool enabled = true;                  // Enable/disable updates for this type
        
        // Thresholds for triggering updates
        uint64_t min_document_change = 1000;  // Min docs added/changed to trigger update
        double min_keyword_change = 0.05;     // Min 5% keyword change to trigger update
        
        // Review requirements
        bool require_review = false;          // Require human review before activation
        uint32_t auto_approve_threshold = 10; // Auto-approve if < N keywords changed
    };
    
    /**
     * Configuration for capability auto-generation
     */
    struct Config {
        bool enabled = false;                 // Master switch
        
        // Update schedules by shard type
        std::map<std::string, UpdateSchedule> schedules;
        
        // RocksDB sampling
        uint32_t sampling_rate = 100;         // Analyze every Nth document
        uint32_t max_keywords = 100;          // Max keywords to extract
        
        // Audit
        bool audit_logging = true;
        std::string audit_log_path = "/var/log/themisdb/capability-generation.log";
        
        // Security
        bool require_signature = true;
        std::string signing_key_path = "/etc/themisdb/capability-signing.key";
        
        // Output
        std::string output_directory = "config/capabilities";
        bool create_backups = true;
        bool git_commit = false;              // Auto-commit to git
        
        /**
         * Load configuration from YAML file
         */
        static Config loadFromYAML(const std::string& yaml_path);
    };
    
    /**
     * Analysis result from RocksDB scan
     */
    struct AnalysisResult {
        std::string shard_id;
        
        // Extracted metadata
        std::vector<std::string> domains;
        std::vector<std::string> organizations;
        std::vector<std::string> regions;
        std::vector<std::string> data_types;
        std::vector<std::string> keywords;  // Top N by TF-IDF
        
        // Statistics
        uint64_t document_count = 0;
        uint64_t total_size_bytes = 0;
        std::chrono::system_clock::time_point last_update_time;
        
        // Collections/namespaces found
        std::vector<std::string> collections;
    };
    
    /**
     * Construct capability auto-generator
     * 
     * @param config Configuration
     * @param topology Shard topology manager
     * @param self_awareness Optional self-awareness system (for audit trigger)
     * @param state_db Optional RocksDB instance for persisting schedule/count state
     */
    explicit CapabilityAutoGenerator(
        const Config& config,
        std::shared_ptr<sharding::ShardTopology> topology,
        std::shared_ptr<SelfAwareness> self_awareness = nullptr,
        std::shared_ptr<RocksDBWrapper> state_db = nullptr
    );
    
    /**
     * Destructor - stops background thread
     */
    ~CapabilityAutoGenerator();
    
    /**
     * Start background update thread
     */
    void start();
    
    /**
     * Stop background update thread
     */
    void stop();
    
    /**
     * Check if auto-generator is running
     */
    bool isRunning() const { return running_; }
    
    /**
     * Manually trigger capability generation for a specific shard
     * 
     * @param shard_id Shard identifier
     * @param force Force update even if no changes detected
     * @return true if successful
     */
    bool generateCapability(const std::string& shard_id, bool force = false);
    
    /**
     * Analyze RocksDB data for a shard
     * 
     * @param shard_id Shard identifier
     * @param data_path Path to RocksDB data directory
     * @return Analysis result with extracted metadata
     */
    AnalysisResult analyzeShardData(const std::string& shard_id, const std::string& data_path);
    
    /**
     * Generate capability YAML from analysis result
     * 
     * @param result Analysis result
     * @param previous_capability Previous capability (if exists)
     * @return Generated capability
     */
    sharding::DomainCapability generateFromAnalysis(
        const AnalysisResult& result,
        const sharding::DomainCapability* previous_capability = nullptr
    );
    
    /**
     * Save capability to YAML file with audit trail
     * 
     * @param shard_id Shard identifier
     * @param capability Capability to save
     * @param audit_info Audit information (user, reason, etc.)
     * @return true if successful
     */
    bool saveCapability(
        const std::string& shard_id,
        const sharding::DomainCapability& capability,
        const nlohmann::json& audit_info
    );
    
    /**
     * Get statistics about auto-generation
     * 
     * @return Statistics as JSON
     */
    nlohmann::json getStatistics() const;
    
    /**
     * Update configuration at runtime
     * 
     * @param config New configuration
     */
    void updateConfig(const Config& config);
    
    /**
     * Get current configuration
     */
    Config getConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    /**
     * Persist the last-run timestamp and document count for a shard.
     *
     * Updates both the in-memory maps (used by the schedule gate and
     * shouldUpdate) and the durable RocksDB state key
     * "utils_capgen_state:<shard_id>".  No-op on the RocksDB side when
     * state_db_ is null.  This method is public so that callers can
     * pre-seed or reset persisted state (e.g. during migration, testing,
     * or manual override).
     *
     * @param shard_id   Shard identifier
     * @param timestamp  Unix epoch seconds to record as last run time
     * @param doc_count  Document count to record as last known count
     */
    void persistState(const std::string& shard_id, int64_t timestamp, uint64_t doc_count);

private:
    Config config_;
    std::shared_ptr<sharding::ShardTopology> topology_;
    std::shared_ptr<SelfAwareness> self_awareness_;  // For triggering self-awareness on audit signing
    std::shared_ptr<RocksDBWrapper> state_db_;       // Optional RocksDB for persisting state
    
    // Background thread
    std::unique_ptr<std::thread> worker_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    mutable std::mutex mutex_;
    
    // Persisted state (loaded from state_db_ on construction, protected by mutex_)
    std::map<std::string, int64_t>  last_run_timestamps_;    // shard_id -> unix epoch seconds
    std::map<std::string, uint64_t> last_document_counts_;   // shard_id -> document count
    
    // Statistics
    std::atomic<uint64_t> total_generations_{0};
    std::atomic<uint64_t> successful_generations_{0};
    std::atomic<uint64_t> failed_generations_{0};
    std::atomic<uint64_t> auto_approved_{0};
    std::atomic<uint64_t> manual_review_required_{0};
    
    /**
     * Background worker thread function
     */
    void workerThread();
    
    /**
     * Process a single shard
     */
    void processShard(const sharding::ShardInfo& shard);
    
    /**
     * Check if shard should be updated
     */
    bool shouldUpdate(const sharding::ShardInfo& shard, const AnalysisResult& current);
    
    /**
     * Calculate change significance between old and new
     */
    double calculateChangeSignificance(
        const sharding::DomainCapability& old_cap,
        const AnalysisResult& new_result
    );
    
    /**
     * Extract keywords using TF-IDF
     */
    std::vector<std::string> extractKeywords(
        const std::vector<std::string>& documents,
        uint32_t max_keywords
    );
    
    /**
     * Generate audit trail
     */
    nlohmann::json generateAuditTrail(
        const std::string& shard_id,
        const sharding::DomainCapability* previous,
        const AnalysisResult& current
    );
    
    /**
     * Create cryptographic signature
     */
    std::string generateSignature(
        const sharding::DomainCapability& capability,
        const std::string& private_key_path
    );
    
    /**
     * Log to audit file
     */
    void auditLog(const std::string& shard_id, const nlohmann::json& entry);
    
    /**
     * Get update schedule for shard
     */
    UpdateSchedule getScheduleForShard(const sharding::ShardInfo& shard) const;
    
    /**
     * Determine shard type from metadata
     */
    std::string determineShardType(const sharding::ShardInfo& shard) const;
    
    /**
     * Load persisted schedule/count state from state_db_ into in-memory maps.
     * Called once in the constructor when state_db_ is non-null.
     */
    void loadPersistedState();
};

} // namespace themis::util
