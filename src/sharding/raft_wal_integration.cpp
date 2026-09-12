/**
 * @file raft_wal_integration.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/raft_wal_integration.h"
#include <chrono>
#include <set>
#include <vector>

namespace themisdb {
namespace sharding {

/** @brief Construct bridge with initial follower mode. */
RaftWALIntegration::RaftWALIntegration(const Config& config)
    : config_(config), is_leader_(false) {
}

/** @brief Stop active replication role during teardown. */
RaftWALIntegration::~RaftWALIntegration() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (is_leader_) {
        stopWALShipper();
    } else {
        stopWALApplier();
    }
}

/**
 * @brief Append local WAL entry and wait for follower quorum acknowledgments.
 * @param entry WAL entry to persist and replicate.
 * @return Write result indicating quorum success, assigned LSN, and error text.
 */
RaftWALIntegration::WriteResult RaftWALIntegration::write(const WALEntry& entry) {
    // FIXED: Use timed lock to prevent indefinite blocking if mutex is held too long
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    if (!lock.try_lock_for(config_.write_timeout)) {
        return {false, LSN{}, "Write timeout - leader unavailable"};
    }
    
    if (!is_leader_) {
        return {false, LSN{}, "Not leader, redirect to " + config_.raft_state->getLeaderId()};
    }
    
    // 1. Append to local WAL
    LSN wal_lsn = config_.wal_manager->append(entry);

    // 2. Append to Raft log (serialize WAL entry as the replicated command)
    WALEntry replicated_entry = entry;
    replicated_entry.lsn = wal_lsn;  // ensure serialized command carries assigned LSN
    const auto serialized = replicated_entry.serialize();

    LogEntry log_entry;
    log_entry.term = config_.raft_state->getCurrentTerm();
    log_entry.index = config_.raft_log->getLastLogIndex() + 1;
    log_entry.command.assign(serialized.begin(), serialized.end());
    
    uint64_t log_index = config_.raft_log->append(log_entry);
    
    // 3. Track pending write
    PendingWrite pending;
    pending.log_index = log_index;
    pending.wal_lsn = wal_lsn;
    pending.acknowledgments.insert(config_.node_id);  // Leader acks itself
    pending.committed = false;
    
    pending_writes_[log_index] = pending;

    // CC-2a: The original audit finding flagged a potential self-deadlock where
    // onAppendEntriesResponse() needed to acquire mutex_ to deliver ACKs while
    // write() was still holding it.  This is resolved by using cv_.wait_for()
    // which atomically releases mutex_ during the wait and re-acquires it before
    // the predicate check or before returning.  onAppendEntriesResponse() can
    // therefore acquire mutex_ to update pending_writes_ while write() is parked
    // in wait_for(), eliminating the deadlock.
    auto timeout = std::chrono::milliseconds(5000);
    bool quorum_reached = cv_.wait_for(lock, timeout, [this, log_index]() {
        auto it = pending_writes_.find(log_index);
        return it != pending_writes_.end() && it->second.committed;
    });

    if (!quorum_reached) {
        pending_writes_.erase(log_index);
        return {false, wal_lsn, "Quorum timeout"};
    }
    
    pending_writes_.erase(log_index);
    return {true, wal_lsn, ""};
}

/** @brief Serve leader-only linearizable WAL read outside integration lock. */
std::optional<WALEntry> RaftWALIntegration::read(const LSN& lsn) {
    // Check leader status under lock, then perform WAL I/O outside the lock.
    // WALManager has its own internal synchronization; holding mutex_ across a
    // blocking disk read would prevent concurrent writes from progressing.
    {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        if (!is_leader_) {
            return std::nullopt;  // Only leader serves reads for linearizability
        }
    }
    return config_.wal_manager->read(lsn);
}

/** @brief Transition integration into leader mode and start shipper. */
void RaftWALIntegration::onBecomeLeader() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    if (is_leader_) {
        return;  // Already leader
    }
    
    is_leader_ = true;
    stopWALApplier();
    startWALShipper();
}

/** @brief Transition integration into follower mode and start applier side. */
void RaftWALIntegration::onBecomeFollower() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    if (!is_leader_) {
        return;  // Already follower
    }
    
    is_leader_ = false;
    stopWALShipper();
    startWALApplier();
}

/** @brief Advance commit-related compaction state to snapshot boundary. */
void RaftWALIntegration::compact(uint64_t snapshot_index) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    // Truncate Raft log entries up to snapshot_index
    // These are now captured in the snapshot
    // In real impl, would create snapshot and truncate log
    
    // For now, just mark commit index
    if (snapshot_index > config_.raft_log->getCommitIndex()) {
        config_.raft_log->setCommitIndex(snapshot_index);
    }
}

/** @brief Return cached leader/follower mode. */
bool RaftWALIntegration::isLeader() const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return is_leader_;
}

/** @brief Return current leader ID from shared Raft state. */
std::string RaftWALIntegration::getLeaderId() const {
    return config_.raft_state->getLeaderId();
}

/** @brief Start configured WAL shipper when present. */
void RaftWALIntegration::startWALShipper() {
    if (config_.wal_shipper) {
        config_.wal_shipper->start();
    }
}

/** @brief Stop configured WAL shipper when present. */
void RaftWALIntegration::stopWALShipper() {
    if (config_.wal_shipper) {
        config_.wal_shipper->stop();
    }
}

/** @brief Prepare follower-side WAL applier; currently no active start step. */
void RaftWALIntegration::startWALApplier() {
    // WAL Applier is passive, just ensure it's ready
    // No explicit start needed
}

/** @brief Stop follower-side WAL applier; currently no active stop step. */
void RaftWALIntegration::stopWALApplier() {
    // No explicit stop needed
}

/** @brief Evaluate whether acknowledgments meet majority quorum. */
bool RaftWALIntegration::hasQuorum(const std::set<std::string>& acks) const {
    // Use actual cluster membership size from RaftState configuration.
    const auto& members = config_.raft_state->getClusterMembers();
    size_t cluster_size = members.empty() ? 1 : members.size();
    size_t quorum = (cluster_size / 2) + 1;
    
    return acks.size() >= quorum;
}

/** @brief Mark follower acknowledgments up to match index and notify writers. */
void RaftWALIntegration::onAppendEntriesResponse(const std::string& follower_id, uint64_t match_index) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    // Mark acknowledgment for all pending writes up to match_index
    bool any_newly_committed = false;
    for (auto& [log_index, pending] : pending_writes_) {
        if (log_index <= match_index) {
            pending.acknowledgments.insert(follower_id);
            if (!pending.committed && hasQuorum(pending.acknowledgments)) {
                pending.committed = true;
                config_.raft_log->setCommitIndex(log_index);
                any_newly_committed = true;
            }
        }
    }

    // Wake up write() waiters whenever a new entry reaches quorum.
    if (any_newly_committed) {
        cv_.notify_all();
    }
}

} // namespace sharding
} // namespace themisdb
