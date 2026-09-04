/**
 * @file raft_v2.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "replication/raft_v2.h"

#include <stdexcept>
#include <sstream>

namespace themisdb {
namespace replication {

// ============================================================================
// Lock Hierarchy Documentation (raft_v2.cpp)
// ============================================================================
//
// This module implements a 2-level lock hierarchy to prevent deadlocks
// during cluster configuration changes and membership updates.
//
// LOCK HIERARCHY (ordered from outermost to innermost):
//
//   Level 1: RaftV2ClusterConfig::mutex_
//            - Purpose: Protects membership sets and joint consensus state
//            - Scope: Config mutations and membership queries
//            - Hold time: MINIMAL (set operations only, ~microseconds)
//            - Pattern: Acquire → access sets → release → call external
//
//   Level 2: MembershipChangeManager::mutex_
//            - Purpose: Protects pending change entries
//            - Scope: Change request tracking
//            - Hold time: MINIMAL (state copy, ~microseconds)
//            - Pattern: Acquire → copy entry → release → WAL append outside
//
//   Level 3: Blocking I/O and External Operations
//            - Purpose: WAL append, config persistence
//            - Scope: NEVER held while holding Level 1 or 2 locks
//            - Hold time: VARIABLE (I/O dependent, often 10-100ms)
//            - Pattern: Must complete AFTER all higher-level locks released
//
// CRITICAL VIOLATION BEING FIXED:
//   writeEntry() previously called wal_->append() while holding mutex_
//   This has been corrected: entry created under lock, WAL append outside
//
// ============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// RaftV2ClusterConfig
// ─────────────────────────────────────────────────────────────────────────────

RaftV2ClusterConfig::RaftV2ClusterConfig(const std::set<std::string>& members)
    : new_members_(members) {}

void RaftV2ClusterConfig::beginAddMember(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (in_joint_consensus_) {
        throw std::runtime_error(
            "RaftV2ClusterConfig: membership change already in progress");
    }
    old_members_ = new_members_;
    new_members_.insert(node_id);
    in_joint_consensus_ = true;
}

void RaftV2ClusterConfig::beginRemoveMember(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (in_joint_consensus_) {
        throw std::runtime_error(
            "RaftV2ClusterConfig: membership change already in progress");
    }
    if (new_members_.size() <= 1) {
        throw std::runtime_error(
            "RaftV2ClusterConfig: cannot remove the last member from the cluster");
    }
    old_members_ = new_members_;
    new_members_.erase(node_id);
    in_joint_consensus_ = true;
}

void RaftV2ClusterConfig::commitTransition() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!in_joint_consensus_) {
        throw std::runtime_error(
            "RaftV2ClusterConfig: no transition in progress to commit");
    }
    old_members_.clear();
    in_joint_consensus_ = false;
}

void RaftV2ClusterConfig::rollbackTransition() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!in_joint_consensus_) {
        throw std::runtime_error(
            "RaftV2ClusterConfig: no transition in progress to roll back");
    }
    new_members_ = old_members_;
    old_members_.clear();
    in_joint_consensus_ = false;
}

bool RaftV2ClusterConfig::isInJointConsensus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return in_joint_consensus_;
}

bool RaftV2ClusterConfig::isMember(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (in_joint_consensus_) {
        return old_members_.count(node_id) > 0 ||
               new_members_.count(node_id) > 0;
    }
    return new_members_.count(node_id) > 0;
}

std::set<std::string> RaftV2ClusterConfig::getAllMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (in_joint_consensus_) {
        std::set<std::string> all(old_members_);
        all.insert(new_members_.begin(), new_members_.end());
        return all;
    }
    return new_members_;
}

std::set<std::string> RaftV2ClusterConfig::getNewMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return new_members_;
}

std::set<std::string> RaftV2ClusterConfig::getOldMembers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return old_members_;
}

bool RaftV2ClusterConfig::hasQuorum(const std::set<std::string>& votes) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (in_joint_consensus_) {
        // Joint consensus: need majority in BOTH old and new configurations.
        size_t old_votes = 0;
        for (const auto& v : votes) {
            if (old_members_.count(v) > 0) { ++old_votes; }
        }
        size_t new_votes = 0;
        for (const auto& v : votes) {
            if (new_members_.count(v) > 0) { ++new_votes; }
        }
        return old_votes >= majority(old_members_.size()) &&
               new_votes >= majority(new_members_.size());
    } else {
        size_t cnt = 0;
        for (const auto& v : votes) {
            if (new_members_.count(v) > 0) { ++cnt; }
        }
        return cnt >= majority(new_members_.size());
    }
}

size_t RaftV2ClusterConfig::quorumSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return majority(new_members_.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// MembershipChangeManager
// ─────────────────────────────────────────────────────────────────────────────

MembershipChangeManager::MembershipChangeManager(
    std::shared_ptr<RaftV2ClusterConfig> config,
    const std::string& node_id,
    std::shared_ptr<WALManager> wal)
    : config_(std::move(config))
    , node_id_(node_id)
    , wal_(std::move(wal)) {}

MembershipChangeEntry MembershipChangeManager::proposeAdd(
    const std::string& new_node_id)
{
    MembershipChangeEntry entry;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_) {
            throw std::runtime_error(
                "MembershipChangeManager: membership change already in progress");
        }
        auto old_members = config_->getNewMembers();
        config_->beginAddMember(new_node_id);
        auto new_members = config_->getNewMembers();

        entry = writeEntry(
            MembershipChangeEntry::Phase::JOINT, old_members, new_members);
        pending_ = entry;
    }  // LOCK RELEASED: WAL append happens outside lock
    
    // Persist to WAL outside lock (Level 3 I/O)
    if (wal_) {
        WALEntry wal_entry;
        wal_entry.operation    = "MEMBERSHIP_CHANGE";
        wal_entry.collection   = "__raft_config__";
        wal_entry.document_id  = "joint";
        std::ostringstream oss;
        oss << "{\"phase\":\"joint\",\"old\":[";
        bool first = true;
        for (const auto& m : entry.old_members) {
            if (!first) {
              oss << ",";
            }
            oss << "\"" << m << "\"";
            first = false;
        }
        oss << "],\"new\":[";
        first = true;
        for (const auto& m : entry.new_members) {
            if (!first) {
              oss << ",";
            }
            oss << "\"" << m << "\"";
            first = false;
        }
        oss << "]}";
        wal_entry.data = oss.str();
        wal_->append(wal_entry);
        entry.log_index = wal_->getCurrentSequence();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_ &&
            pending_->phase == MembershipChangeEntry::Phase::JOINT &&
            pending_->old_members == entry.old_members &&
            pending_->new_members == entry.new_members) {
            pending_->log_index = entry.log_index;
        }
    }
    return entry;
}

MembershipChangeEntry MembershipChangeManager::proposeRemove(
    const std::string& target_node_id)
{
    MembershipChangeEntry entry;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_) {
            throw std::runtime_error(
                "MembershipChangeManager: membership change already in progress");
        }
        auto old_members = config_->getNewMembers();
        config_->beginRemoveMember(target_node_id);
        auto new_members = config_->getNewMembers();

        entry = writeEntry(
            MembershipChangeEntry::Phase::JOINT, old_members, new_members);
        pending_ = entry;
    }  // LOCK RELEASED: WAL append happens outside lock
    
    // Persist to WAL outside lock (Level 3 I/O)
    if (wal_) {
        WALEntry wal_entry;
        wal_entry.operation    = "MEMBERSHIP_CHANGE";
        wal_entry.collection   = "__raft_config__";
        wal_entry.document_id  = "joint";
        std::ostringstream oss;
        oss << "{\"phase\":\"joint\",\"old\":[";
        bool first = true;
        for (const auto& m : entry.old_members) {
            if (!first) {
              oss << ",";
            }
            oss << "\"" << m << "\"";
            first = false;
        }
        oss << "],\"new\":[";
        first = true;
        for (const auto& m : entry.new_members) {
            if (!first) {
              oss << ",";
            }
            oss << "\"" << m << "\"";
            first = false;
        }
        oss << "]}";
        wal_entry.data = oss.str();
        wal_->append(wal_entry);
        entry.log_index = wal_->getCurrentSequence();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_ &&
            pending_->phase == MembershipChangeEntry::Phase::JOINT &&
            pending_->old_members == entry.old_members &&
            pending_->new_members == entry.new_members) {
            pending_->log_index = entry.log_index;
        }
    }
    return entry;
}

void MembershipChangeManager::onJointCommitted([[maybe_unused]] uint64_t log_index) {
    MembershipChangeEntry commit;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pending_ || pending_->phase != MembershipChangeEntry::Phase::JOINT) {
            return;  // Stale callback – ignore
        }
        if (pending_->log_index != log_index) {
            return;  // Mismatch – ignore
        }
        // Write the COMMIT entry so followers know to finalise
        commit = writeEntry(
            MembershipChangeEntry::Phase::COMMIT,
            pending_->old_members,
            pending_->new_members);
        pending_ = commit;
    }  // LOCK RELEASED: WAL append happens outside lock

    if (wal_) {
        WALEntry wal_entry;
        wal_entry.operation    = "MEMBERSHIP_CHANGE";
        wal_entry.collection   = "__raft_config__";
        wal_entry.document_id  = "commit";
        std::ostringstream oss;
        oss << "{\"phase\":\"commit\",\"old\":[";
        bool first = true;
        for (const auto& m : commit.old_members) {
            if (!first) {
              oss << ",";
            }
            oss << "\"" << m << "\"";
            first = false;
        }
        oss << "],\"new\":[";
        first = true;
        for (const auto& m : commit.new_members) {
            if (!first) {
              oss << ",";
            }
            oss << "\"" << m << "\"";
            first = false;
        }
        oss << "]}";
        wal_entry.data = oss.str();
        wal_->append(wal_entry);
        commit.log_index = wal_->getCurrentSequence();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_ &&
            pending_->phase == MembershipChangeEntry::Phase::COMMIT &&
            pending_->old_members == commit.old_members &&
            pending_->new_members == commit.new_members) {
            pending_->log_index = commit.log_index;
        }
    }
}

void MembershipChangeManager::onNewConfigCommitted() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!pending_ || pending_->phase != MembershipChangeEntry::Phase::COMMIT) {
        return;  // Stale callback – ignore
    }
    config_->commitTransition();
    pending_.reset();
}

void MembershipChangeManager::applyEntry(const MembershipChangeEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entry.phase == MembershipChangeEntry::Phase::JOINT) {
        // Followers must apply the joint configuration immediately upon
        // writing the entry to the local log (Raft §4.1), even before it is
        // committed.  We detect whether the entry represents an add or a
        // remove by comparing old_members and new_members, then drive
        // config_ into joint-consensus state accordingly.
        if (!config_->isInJointConsensus()) {
            if (!entry.old_members.empty() &&
                entry.new_members != entry.old_members) {
                // Determine added node (in new but not old)
                for (const auto& m : entry.new_members) {
                    if (!entry.old_members.count(m)) {
                        config_->beginAddMember(m);
                        break;
                    }
                }
                // Determine removed node (in old but not new) – only if no
                // add was performed (i.e. config_ is not yet in transition).
                if (!config_->isInJointConsensus()) {
                    for (const auto& m : entry.old_members) {
                        if (!entry.new_members.count(m)) {
                            config_->beginRemoveMember(m);
                            break;
                        }
                    }
                }
            }
        }
        pending_ = entry;
    } else if (entry.phase == MembershipChangeEntry::Phase::COMMIT) {
        if (config_->isInJointConsensus()) {
            config_->commitTransition();
        }
        pending_.reset();
    }
}

bool MembershipChangeManager::isChangeInProgress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.has_value();
}

std::optional<MembershipChangeEntry>
MembershipChangeManager::pendingEntry() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_;
}

std::shared_ptr<const RaftV2ClusterConfig>
MembershipChangeManager::currentConfig() const {
    return config_;
}

MembershipChangeEntry MembershipChangeManager::writeEntry(
    MembershipChangeEntry::Phase phase,
    const std::set<std::string>& old_members,
    const std::set<std::string>& new_members)
{
    // LOCK HIERARCHY NOTE (Level 2 → Level 3):
    // Create and return the entry WITHOUT calling wal_->append() while holding mutex_.
    // The caller is responsible for WAL persistence outside the lock.
    // This prevents circular deadlock if WAL manager holds any locks.
    
    MembershipChangeEntry entry;
    entry.phase       = phase;
    entry.old_members = old_members;
    entry.new_members = new_members;
    entry.term        = 0;  // Filled in by caller when term is known
    entry.log_index   = 0;  // Filled in by WAL on append
    
    // Note: WAL append is now the caller's responsibility (proposeAdd/proposeRemove/applyEntry)
    return entry;
}


}  // namespace replication
}  // namespace themisdb
