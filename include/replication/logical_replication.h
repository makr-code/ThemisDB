/**
 * @file logical_replication.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "replication/replication_manager.h"

#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>
#include <atomic>

#include <nlohmann/json.hpp>

namespace themisdb {
namespace replication {

/**
 * LogicalChange
 *
 * Represents a schema-aware logical replication event.  Used by
 * LogicalReplicationManager to expose filtered, transformed changes to
 * downstream subscribers.
 */
struct LogicalChange {
    enum class Type { INSERT, UPDATE, DELETE, TRUNCATE, DDL, SNAPSHOT, UNKNOWN };

    // Default to UNKNOWN to avoid misclassifying uninitialized changes
    Type type = Type::UNKNOWN;
    std::string collection;
    std::string schema_version;
    std::string source_version;
    std::string target_version;
    nlohmann::json old_data;
    nlohmann::json new_data;
    std::string ddl_statement;
    uint64_t lsn = 0;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * LogicalReplicationManager
 *
 * Provides schema-aware logical replication with per-slot filtering,
 * data transformation, cross-version metadata, and conflict-free
 * initial sync for new replicas.  Implements IReplicationListener so
 * that it can be attached directly to ReplicationManager and receive
 * WAL events without modifying the write path.
 */
class LogicalReplicationManager : public IReplicationListener,
                                  public std::enable_shared_from_this<LogicalReplicationManager> {
public:
    struct ReplicationFilter {
        std::vector<std::string> include_collections;
        std::vector<std::string> exclude_collections;
        std::string row_filter_expression;  ///< Simple AQL-style predicate ("field == 'value'")
        bool replicate_ddl = true;
        bool replicate_dml = true;
    };

    struct LogicalReplicationSlot {
        std::string slot_name;
        uint64_t restart_lsn = 0;
        uint64_t confirmed_flush_lsn = 0;
        std::string plugin_name;
        ReplicationFilter filter;
        bool initial_sync_pending = false;
    };

    struct Config {
        std::string wal_directory;
        std::string source_version = "v1.5";
        std::string target_version = "v1.6";
        bool parallel_decoding = true;
        std::function<void(LogicalChange&)> transform;  ///< Optional per-change transformer
    };

    struct Stats {
        uint64_t changes_enqueued = 0;
        uint64_t ddl_enqueued = 0;
        uint64_t filtered_out = 0;
        uint64_t slots_loaded = 0;
    };

    /**
     * Construct a LogicalReplicationManager with default configuration.
     *
     * @param wal Shared pointer to the WAL manager; may be nullptr (WAL-dependent
     *            operations such as restart_lsn will default to 0).
     * @post Persisted slots are loaded from the configured wal_directory (if set).
     */
    explicit LogicalReplicationManager(std::shared_ptr<WALManager> wal);
    
    /**
     * Construct a LogicalReplicationManager with custom configuration.
     *
     * @param wal Shared pointer to the WAL manager; may be nullptr.
     * @param config Custom configuration (WAL directory, schema versions, decoding mode).
     * @post Persisted slots are loaded from config.wal_directory (if non-empty).
     */
    LogicalReplicationManager(std::shared_ptr<WALManager> wal, Config config);
    ~LogicalReplicationManager() override = default;

    // ------------------------------------------------------------------
    // Slot lifecycle
    // ------------------------------------------------------------------
    
    /**
     * Create a logical replication slot with default filter settings.
     *
     * A slot represents a persistent subscription to WAL changes. Multiple slots
     * can coexist, each with independent restart LSN and filter settings.
     * The slot is persisted to disk for recovery after restart.
     *
     * @param slot_name Unique identifier for the slot.
     * @param output_plugin Plugin name (e.g., "test_decoding", "wal2json").
     * @return LogicalReplicationSlot with restart_lsn set to current WAL position.
     * @throws std::runtime_error if a slot with slot_name already exists.
     *
     * @note Default filter allows all collections and operations.
     */
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin);
    
    /**
     * Create a logical replication slot with a custom filter.
     *
     * @param slot_name Unique identifier for the slot.
     * @param output_plugin Plugin name for change decoding.
     * @param filter Filtering rules (collections, DDL/DML, row predicates).
     * @return LogicalReplicationSlot configured with the given filter.
     */
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin,
        const ReplicationFilter& filter);
    
    /**
     * Create a logical replication slot with initial sync option.
     *
     * @param slot_name Unique identifier for the slot.
     * @param output_plugin Plugin name for change decoding.
     * @param filter Filtering rules.
     * @param perform_initial_sync If true, initial SNAPSHOT changes are provided
     *                             at slot creation; if false, slot starts at
     *                             current WAL position.
     * @return LogicalReplicationSlot with initial sync pending if requested.
     */
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin,
        const ReplicationFilter& filter,
        bool perform_initial_sync);
    
    /**
     * Create a logical replication slot with custom initial snapshot.
     *
     * Used when the initial sync should include only specific data or a
     * subset of the current snapshot.
     *
     * @param slot_name Unique identifier for the slot.
     * @param output_plugin Plugin name for change decoding.
     * @param filter Filtering rules.
     * @param perform_initial_sync If true, initial_snapshot changes are delivered first.
     * @param initial_snapshot LogicalChange list to deliver before starting incremental.
     * @return LogicalReplicationSlot with custom snapshot enqueued.
     */
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin,
        const ReplicationFilter& filter,
        bool perform_initial_sync,
        std::vector<LogicalChange> initial_snapshot);

    /**
     * Advance the confirmed flush LSN for a slot after processing changes.
     *
     * Call this after successfully processing changes from readChanges() to
     * prevent redelivery on restart. LSN must monotonically increase.
     *
     * @param slot_name Identifier of the slot to advance.
     * @param lsn New confirmed flush LSN.
     *
     * @note Silently returns if slot does not exist or if lsn is less than
     *       the current confirmed_flush_lsn (backwards LSN is ignored).
     */
    void advanceSlot(const std::string& slot_name, uint64_t lsn);
    
    /**
     * List all existing logical replication slots.
     *
     * @return Vector of LogicalReplicationSlot structures for all slots
     *         in the manager.
     */
    std::vector<LogicalReplicationSlot> listSlots() const;
    
    /**
     * Check if a slot exists by name.
     *
     * @param slot_name Slot identifier.
     * @return true if slot exists; false otherwise.
     */
    bool hasSlot(const std::string& slot_name) const;

    // ------------------------------------------------------------------
    // Change streaming
    // ------------------------------------------------------------------
    
    /**
     * Read logical changes from a specific slot.
     *
     * Retrieves up to max_changes changes that have not yet been confirmed
     * flushed (i.e., changes with LSN >= confirmed_flush_lsn). Changes are
     * filtered according to the slot's ReplicationFilter (collection, DDL/DML,
     * row predicates).
     *
     * @param slot_name Identifier of the slot.
     * @param max_changes Maximum number of changes to return (default: 1000).
     * @return Vector of LogicalChange entries; empty if slot does not exist or
     *         has no buffered changes.
     *
     * @note Changes are removed from the in-memory buffer immediately upon return.
     *       Call advanceSlot() after processing to persist the confirmed_flush_lsn
     *       so that changes are not re-delivered after a restart.
     */
    std::vector<LogicalChange> readChanges(const std::string& slot_name, uint32_t max_changes = 1000);
    
    /**
     * Record a DDL change (schema modification) into the logical stream.
     *
     * DDL changes are captured as DDLCHANGE_TYPE logical changes and delivered
     * to all slots with replicate_ddl=true.
     *
     * @param ddl_statement The DDL SQL or DDL description (e.g., "ALTER TABLE ...").
     * @param schema_version Optional schema version label (e.g., "v1.2.3").
     * @param lsn Optional specific WAL LSN to associate; if 0, uses current WAL position.
     */
    void recordDDLChange(const std::string& ddl_statement,
                         const std::string& schema_version = "",
                         uint64_t lsn = 0);

    // ------------------------------------------------------------------
    // Listener callback (invoked by ReplicationManager)
    // ------------------------------------------------------------------
    
    /**
     * Callback: invoked when this node changes replication roles.
     * (Inherited from IReplicationListener)
     *
     * @param old_role Previous role.
     * @param new_role New role (LEADER, FOLLOWER, CANDIDATE, etc.).
     */
    void onRoleChange(ReplicationRole old_role, ReplicationRole new_role) override;
    
    /**
     * Callback: invoked when a new leader is elected.
     * (Inherited from IReplicationListener)
     *
     * @param leader_id Node ID of the newly elected leader.
     */
    void onLeaderElected(const std::string& leader_id) override;
    
    /**
     * Callback: invoked when a replica is added to the replication group.
     * (Inherited from IReplicationListener)
     *
     * @param replica Information about the added replica.
     */
    void onReplicaAdded(const ReplicaInfo& replica) override;
    
    /**
     * Callback: invoked when a replica is removed from the replication group.
     * (Inherited from IReplicationListener)
     *
     * @param node_id Node ID of the removed replica.
     */
    void onReplicaRemoved(const std::string& node_id) override;
    
    /**
     * Callback: invoked when a write conflict is detected.
     * (Inherited from IReplicationListener)
     *
     * @param document_id Identifier of the document involved in the conflict.
     */
    void onConflictDetected(const std::string& document_id) override;
    
    /**
     * Callback: invoked when replication lag exceeds a threshold.
     * (Inherited from IReplicationListener)
     *
     * @param lag_ms Replication lag in milliseconds.
     */
    void onReplicationLagWarning(int64_t lag_ms) override;
    
    /**
     * Callback: invoked when a replica's health status changes.
     * (Inherited from IReplicationListener)
     *
     * @param node_id Node ID of the replica.
     * @param old_status Previous health status.
     * @param new_status New health status (HEALTHY, DEGRADED, FAILED, UNKNOWN).
     */
    void onReplicaHealthChanged(const std::string& node_id,
                                HealthStatus old_status,
                                HealthStatus new_status) override;
    
    /**
     * Callback: invoked when failover is initiated.
     * (Inherited from IReplicationListener)
     *
     * @param failed_leader_id Node ID of the current leader being failed over.
     * @param new_leader_id Node ID of the candidate new leader.
     */
    void onFailoverStarted(const std::string& failed_leader_id,
                           const std::string& new_leader_id) override;
    
    /**
     * Callback: invoked when failover completes.
     * (Inherited from IReplicationListener)
     *
     * @param new_leader_id Node ID of the newly elected leader.
     * @param success true if failover succeeded; false if aborted.
     */
    void onFailoverCompleted(const std::string& new_leader_id, bool success) override;
    void onNetworkPartitionDetected(const std::vector<std::string>& unreachable_nodes) override;
    void onWALEntryApplied(const WALEntry& entry) override;

    Stats getStats() const;

private:
    struct SlotRuntime {
        LogicalReplicationSlot meta;
        std::deque<LogicalChange> buffer;
        std::unordered_set<std::string> snapshot_keys;
        bool initial_sync_pending = false;
        std::mutex mutex;
    };

    std::shared_ptr<WALManager> wal_;
    Config config_;

    mutable std::shared_mutex slots_mutex_;
    std::map<std::string, std::shared_ptr<SlotRuntime>> slots_;

    mutable std::mutex stats_mutex_;
    Stats stats_;
    mutable std::atomic<bool> missing_seq_warned_{false};

    // Persistence helpers
    void loadPersistedSlots();
    void persistSlot(const SlotRuntime& slot) const;
    std::string slotStatePath(const std::string& slot_name) const;

    // Filtering and transformation helpers
    bool matchesFilter(const LogicalChange& change, const ReplicationFilter& filter) const;
    bool evaluateRowFilter(const std::string& expression, const nlohmann::json& payload) const;
    LogicalChange makeLogicalChange(const WALEntry& entry) const;
    void applyTransform(LogicalChange& change) const;
    std::string documentIdFromChange(const LogicalChange& change) const;

    static std::string collectionKey(const std::string& collection, const std::string& document_id);
};

}  // namespace replication
}  // namespace themisdb
