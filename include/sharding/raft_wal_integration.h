/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_wal_integration.h                             ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:35:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • aa92e7621  2025-12-08  Update version, add new modules, and fix build issues ║
    • 1f2920a15  2025-12-08  Implement P1.2 Raft Part 4: WAL Integration ✅ - P1.2 COMP... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMISDB_RAFT_WAL_INTEGRATION_H
#define THEMISDB_RAFT_WAL_INTEGRATION_H

#include <memory>
#include <string>
#include <map>
#include <set>
#include <mutex>
#include "raft_state.h"
#include "raft_log.h"
#include "wal_manager.h"
#include "wal_shipper.h"
#include "wal_applier.h"

namespace themisdb {
namespace sharding {

// Bridge WAL types from themis::sharding into the themisdb::sharding namespace.
using themis::sharding::LSN;
using themis::sharding::WALEntry;
using themis::sharding::WALManager;
using themis::sharding::WALShipper;
using themis::sharding::WALApplier;

/**
 * @brief Integrates Raft consensus with WAL replication for automatic failover
 * 
 * Provides:
 * - Quorum-based writes (blocks until majority acknowledgment)
 * - Automatic failover on leader failure
 * - Strong consistency (linearizable reads/writes)
 * - Leadership management (starts/stops WAL Shipper)
 */
class RaftWALIntegration {
public:
    struct Config {
        std::string node_id;
        std::shared_ptr<RaftState> raft_state;
        std::shared_ptr<RaftLog> raft_log;
        std::shared_ptr<WALManager> wal_manager;
        std::shared_ptr<WALShipper> wal_shipper;
        std::shared_ptr<WALApplier> wal_applier;
    };
    
    struct WriteResult {
        bool success;
        LSN lsn;
        std::string error_message;
    };
    
    explicit RaftWALIntegration(const Config& config);
    ~RaftWALIntegration();
    
    // Write with quorum (blocks until majority replicated)
    WriteResult write(const WALEntry& entry);
    
    // Linearizable read from leader
    std::optional<WALEntry> read(const LSN& lsn);
    
    // Leadership management
    void onBecomeLeader();
    void onBecomeFollower();
    
    // Log compaction
    void compact(uint64_t snapshot_index);
    
    // Status queries
    bool isLeader() const;
    std::string getLeaderId() const;
    
private:
    Config config_;
    bool is_leader_;
    std::mutex mutex_;
    
    // Track pending writes for quorum
    struct PendingWrite {
        uint64_t log_index;
        LSN wal_lsn;
        std::set<std::string> acknowledgments;
        bool committed;
    };
    
    std::map<uint64_t, PendingWrite> pending_writes_;
    
    // Helper methods
    void startWALShipper();
    void stopWALShipper();
    void startWALApplier();
    void stopWALApplier();
    
    bool hasQuorum(const std::set<std::string>& acks) const;
    void onAppendEntriesResponse(const std::string& follower_id, uint64_t match_index);
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_RAFT_WAL_INTEGRATION_H
