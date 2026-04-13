/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            logical_replication.h                              ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:25:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     208                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 16aed6bb00  2026-03-15  refactor: enhance modular build configuration and improve... ║
    • d2968d4872  2026-03-14  tighten logical replication diagnostics and randomness ║
    • 5ec6ad7cd3  2026-03-14  add diagnostics and validation for logical replication slots ║
    • 902333c151  2026-03-14  harden logical replication persistence and filters ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

    explicit LogicalReplicationManager(std::shared_ptr<WALManager> wal);
    LogicalReplicationManager(std::shared_ptr<WALManager> wal, Config config);
    ~LogicalReplicationManager() override = default;

    // ------------------------------------------------------------------
    // Slot lifecycle
    // ------------------------------------------------------------------
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin);
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin,
        const ReplicationFilter& filter);
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin,
        const ReplicationFilter& filter,
        bool perform_initial_sync);
    LogicalReplicationSlot createSlot(
        const std::string& slot_name,
        const std::string& output_plugin,
        const ReplicationFilter& filter,
        bool perform_initial_sync,
        std::vector<LogicalChange> initial_snapshot);

    void advanceSlot(const std::string& slot_name, uint64_t lsn);
    std::vector<LogicalReplicationSlot> listSlots() const;
    bool hasSlot(const std::string& slot_name) const;

    // ------------------------------------------------------------------
    // Change streaming
    // ------------------------------------------------------------------
    std::vector<LogicalChange> readChanges(const std::string& slot_name, uint32_t max_changes = 1000);
    void recordDDLChange(const std::string& ddl_statement,
                         const std::string& schema_version = "",
                         uint64_t lsn = 0);

    // ------------------------------------------------------------------
    // Listener callback (invoked by ReplicationManager)
    // ------------------------------------------------------------------
    void onRoleChange(ReplicationRole old_role, ReplicationRole new_role) override;
    void onLeaderElected(const std::string& leader_id) override;
    void onReplicaAdded(const ReplicaInfo& replica) override;
    void onReplicaRemoved(const std::string& node_id) override;
    void onConflictDetected(const std::string& document_id) override;
    void onReplicationLagWarning(int64_t lag_ms) override;
    void onReplicaHealthChanged(const std::string& node_id,
                                HealthStatus old_status,
                                HealthStatus new_status) override;
    void onFailoverStarted(const std::string& failed_leader_id,
                           const std::string& new_leader_id) override;
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
