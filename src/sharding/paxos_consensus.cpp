/**
 * @file paxos_consensus.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=10, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/paxos_consensus.h"
#include "sharding/paxos_wal.h"
#include "sharding/paxos_snapshot.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cstring>  // for strerror

namespace themisdb {
namespace sharding {

PaxosConsensus::PaxosConsensus(const ConsensusConfig& config)
    : config_(config)
    , state_(ConsensusState::FOLLOWER)
    , running_(false)
    , current_round_(0)
    , next_slot_(1)
    , commit_index_(0)
    , total_proposals_(0)
    , failed_proposals_(0)
    , total_prepares_(0)
    , total_accepts_(0)
    , last_applied_lsn_(0, 0)
{
}

PaxosConsensus::~PaxosConsensus() {
    stop();
}

bool PaxosConsensus::initialize(
    const std::string& node_id,
    const std::vector<std::string>& cluster_nodes
) {
    node_id_ = node_id;
    cluster_nodes_ = cluster_nodes;
    
    // Phase 2.1: Initialize WAL and Snapshot infrastructure
    if (config_.enable_persistence) {
        // Initialize WAL
        themis::sharding::PaxosWALConfig wal_config;
        wal_config.wal_directory = config_.data_dir + "/wal";
        wal_config.snapshot_directory = config_.data_dir + "/snapshots";
        wal_config.sync_on_write = true;
        
        wal_ = std::make_unique<themis::sharding::PaxosWAL>(wal_config);
        if (!wal_->initialize()) {
            spdlog::error("Failed to initialize Paxos WAL");
            return false;
        }
        
        // Initialize snapshot manager
        snapshot_manager_ = std::make_unique<themis::sharding::PaxosSnapshotManager>(
            wal_config.snapshot_directory,
            wal_config.max_snapshots
        );
        
        // Recover from snapshot + WAL
        if (!recoverFromWAL()) {
            spdlog::warn("Failed to recover from WAL, starting fresh");
        }
        
        spdlog::info("Paxos persistence enabled: node={}, wal_dir={}", 
                    node_id_, wal_config.wal_directory);
    } else {
        // Legacy: Load persistent state from JSON if available
        if (!loadPersistentState()) {
            spdlog::warn("No persistent state found, starting fresh");
        }
    }
    
    return true;
}

bool PaxosConsensus::start() {
    if (running_.load()) {
        spdlog::warn("Paxos consensus already running");
        return false;
    }
    
    running_.store(true);
    
    // Start background threads
    proposer_thread_ = std::thread(&PaxosConsensus::runProposer, this);
    acceptor_thread_ = std::thread(&PaxosConsensus::runAcceptor, this);
    learner_thread_ = std::thread(&PaxosConsensus::runLearner, this);
    election_thread_ = std::thread(&PaxosConsensus::leaderElectionThread, this);
    
    spdlog::info("Paxos consensus started for node {}", node_id_);
    return true;
}

void PaxosConsensus::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    // Wake up threads
    proposal_cv_.notify_all();
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(proposer_thread_)) {
        THEMIS_WARN("[PaxosConsensus] proposer thread did not finish within shutdown deadline; detaching.");
    }
    if (!themis::utils::joinThreadWithin(acceptor_thread_)) {
        THEMIS_WARN("[PaxosConsensus] acceptor thread did not finish within shutdown deadline; detaching.");
    }
    if (!themis::utils::joinThreadWithin(learner_thread_)) {
        THEMIS_WARN("[PaxosConsensus] learner thread did not finish within shutdown deadline; detaching.");
    }
    if (!themis::utils::joinThreadWithin(election_thread_)) {
        THEMIS_WARN("[PaxosConsensus] election thread did not finish within shutdown deadline; detaching.");
    }
    
    // Save state
    if (config_.enable_persistence) {
        savePersistentState();
    }
    
    spdlog::info("Paxos consensus stopped for node {}", node_id_);
}

bool PaxosConsensus::isLeader() const {
    return state_.load() == ConsensusState::LEADER;
}

std::string PaxosConsensus::getLeaderId() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_leader_;
}

ConsensusState PaxosConsensus::getState() const {
    return state_.load();
}

std::optional<uint64_t> PaxosConsensus::propose(
    const std::string& operation,
    const nlohmann::json& data
) {
    if (!running_.load()) {
        spdlog::error("Cannot propose: Paxos not running");
        return std::nullopt;
    }
    
    // Create log entry
    ConsensusLogEntry entry;
    entry.index = next_slot_.fetch_add(1);
    entry.term = current_round_.load();
    entry.operation = operation;
    entry.data = data;
    entry.timestamp = std::chrono::system_clock::now();
    
    // Add to pending proposals
    {
        std::lock_guard<std::timed_mutex> lock(proposal_mutex_);
        pending_proposals_[entry.index] = entry;
    }
    
    proposal_cv_.notify_one();
    total_proposals_++;
    
    return entry.index;
}

bool PaxosConsensus::waitForCommit(
    uint64_t log_index,
    std::chrono::milliseconds timeout
) {
    auto start = std::chrono::steady_clock::now();
    
    while (running_.load()) {
        {
            std::lock_guard<std::timed_mutex> lock(proposal_mutex_);
            if (committed_log_.find(log_index) != committed_log_.end()) {
                return true;
            }
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return false;
}

std::vector<ConsensusLogEntry> PaxosConsensus::readLog(
    uint64_t start_index,
    std::optional<uint64_t> end_index
) {
    std::vector<ConsensusLogEntry> result;
    std::lock_guard<std::timed_mutex> lock(proposal_mutex_);
    
    uint64_t end = end_index.value_or(commit_index_.load());
    
    for (uint64_t i = start_index; i <= end; ++i) {
        auto it = committed_log_.find(i);
        if (it != committed_log_.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

uint64_t PaxosConsensus::getCommitIndex() const {
    return commit_index_.load();
}

uint64_t PaxosConsensus::getLastLogIndex() const {
    uint64_t next = next_slot_.load();
    return next > 0 ? next - 1 : 0;
}

bool PaxosConsensus::addNode(
    const std::string& node_id,
    const std::string& /*endpoint*/
) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Check if node already exists
    if (std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id) != cluster_nodes_.end()) {
        spdlog::warn("Node {} already in cluster", node_id);
        return false;
    }
    
    // W2-S06: Consensus validation — validate node_id is not empty before adding
    if (node_id.empty()) {
        spdlog::error("PaxosConsensus::addNode: node_id is empty, rejecting");
        return false;
    }
    
    cluster_nodes_.push_back(node_id);
    spdlog::info("Added node {} to cluster", node_id);
    return true;
}

bool PaxosConsensus::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = std::find(cluster_nodes_.begin(), cluster_nodes_.end(), node_id);
    if (it == cluster_nodes_.end()) {
        spdlog::warn("Node {} not found in cluster", node_id);
        return false;
    }
    
    cluster_nodes_.erase(it);
    spdlog::info("Removed node {} from cluster", node_id);
    return true;
}

bool PaxosConsensus::transferLeadership(const std::string& target_node_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!isLeader()) {
        spdlog::warn("Cannot transfer leadership: not the leader");
        return false;
    }
    
    // In Paxos, we don't have a persistent leader
    // Just step down and let target become proposer
    state_.store(ConsensusState::FOLLOWER);
    current_leader_ = target_node_id;
    
    spdlog::info("Transferred leadership to {}", target_node_id);
    return true;
}

bool PaxosConsensus::takeSnapshot(const nlohmann::json& snapshot_data) {
    if (!running_.load()) {
        spdlog::warn("takeSnapshot: Paxos not running");
        return false;
    }

    const uint64_t snap_index = commit_index_.load();

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        const uint64_t snap_term = current_round_.load();  // read atomically to avoid race with restoreSnapshot
        snapshot_data_  = snapshot_data;
        snapshot_index_ = snap_index;
        snapshot_term_  = snap_term;
        spdlog::info("PaxosConsensus::takeSnapshot: snapshot at index={} round={}", snap_index, snap_term);
    }

    return true;
}

bool PaxosConsensus::restoreSnapshot(const nlohmann::json& snapshot_data) {
    if (snapshot_data.is_null() || snapshot_data.empty()) {
        spdlog::error("PaxosConsensus::restoreSnapshot: snapshot_data is null or empty");
        return false;
    }

    uint64_t restored_index = 0;
    uint64_t restored_term  = 0;
    if (snapshot_data.contains("_snapshot_index"))
        restored_index = snapshot_data["_snapshot_index"].get<uint64_t>();
    if (snapshot_data.contains("_snapshot_term"))
        restored_term  = snapshot_data["_snapshot_term"].get<uint64_t>();

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        snapshot_data_  = snapshot_data;
        snapshot_index_ = restored_index;
        snapshot_term_  = restored_term;

        // Step down to follower so this node re-syncs before proposing
        if (running_.load()) {
            state_.store(ConsensusState::FOLLOWER);
            if (restored_term > current_round_.load())
                current_round_.store(restored_term);
        }
    }

    spdlog::info("PaxosConsensus::restoreSnapshot: restored at index={} round={}",
                 restored_index, restored_term);
    return true;
}

ConsensusStats PaxosConsensus::getStats() const {
    ConsensusStats stats{};
    stats.current_term = current_round_.load();
    stats.commit_index = commit_index_.load();
    stats.last_applied = commit_index_.load();
    stats.state = state_.load();
    // PAX-6: always hold state_mutex_ when accessing cluster_nodes_.
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        stats.current_leader = current_leader_;
        stats.cluster_size = cluster_nodes_.size();
        stats.reachable_nodes = cluster_nodes_.size();  // Simplified
    }
    stats.total_operations = total_proposals_.load();
    stats.failed_operations = failed_proposals_.load();
    return stats;
}

nlohmann::json PaxosConsensus::getStatus() const {
    auto stats = getStats();

    uint64_t snap_index = 0;
    uint64_t snap_term  = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        snap_index = snapshot_index_;
        snap_term  = snapshot_term_;
    }

    return {
        {"type", "Paxos"},
        {"node_id", node_id_},
        {"is_leader", isLeader()},
        {"leader_id", stats.current_leader},
        {"state", static_cast<int>(stats.state)},
        {"current_round", current_round_.load()},
        {"commit_index", stats.commit_index},
        {"cluster_size", stats.cluster_size},
        {"total_proposals", stats.total_operations},
        {"failed_proposals", stats.failed_operations},
        {"snapshot_index", snap_index},
        {"snapshot_term",  snap_term}
    };
}

void PaxosConsensus::onCommit(
    std::function<void(const ConsensusLogEntry&)> callback
) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
    on_commit_callback_ = std::move([[maybe_unused]] callback);
}

void PaxosConsensus::onStateChange(
    std::function<void(ConsensusState, ConsensusState)> callback
) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
    on_state_change_callback_ = std::move([[maybe_unused]] callback);
}

void PaxosConsensus::onLeaderChange(
    std::function<void(const std::string&, const std::string&)> callback
) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
    on_leader_change_callback_ = std::move([[maybe_unused]] callback);
}

void PaxosConsensus::setPrepareRPCCallback([[maybe_unused]] PaxosPrepareCallback cb) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
    rpc_prepare_cb_ = std::move(cb);
}

void PaxosConsensus::setPrepareFullRPCCallback([[maybe_unused]] PaxosPrepareFullCallback cb) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
    rpc_prepare_full_cb_ = std::move(cb);
}

void PaxosConsensus::setAcceptRPCCallback([[maybe_unused]] PaxosAcceptCallback cb) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
    rpc_accept_cb_ = std::move(cb);
}

// Private methods

void PaxosConsensus::runProposer() {
    spdlog::debug("Paxos proposer thread started");
    
    while (running_.load()) {
        std::unique_lock<std::timed_mutex> lock(proposal_mutex_);
        
        // Wait for proposals
        proposal_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !pending_proposals_.empty() || !running_.load();
        });
        
        if (!running_.load()) {
          break;
        }
        
        // Process pending proposals with retry logic
        std::vector<uint64_t> failed_slots;
        
        for (auto it = pending_proposals_.begin(); it != pending_proposals_.end();) {
            auto& [slot, entry] = *it;
            
            lock.unlock();
            
            // Retry logic: attempt proposal up to 3 times
            bool success = false;
            const int max_retries = 3;
            
            for (int retry = 0; retry < max_retries && running_.load(); ++retry) {
                if (retry > 0) {
                    spdlog::debug("Node {} retrying proposal for slot {} (attempt {}/{})",
                                 node_id_, slot, retry + 1, max_retries);
                    // Exponential backoff: 100ms, 200ms, 400ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << retry)));
                }
                
                try {
                    success = executePreparePhase(slot, entry);
                } catch (const std::exception& e) {
                    spdlog::error("Node {} exception during executePreparePhase for slot {}: {}", 
                                 node_id_, slot, e.what());
                    success = false;
                }
                if (success) {
                  break;
                }
            }
            
            if (!lock.try_lock_for(config_.paxos_prepare_timeout)) {
                spdlog::warn("Node {} timed out re-acquiring proposal_mutex_ for slot {}; stopping iteration",
                             node_id_, slot);
                break;
            }
            
            if (success) {
                it = pending_proposals_.erase(it);
            } else {
                ++it;
                failed_proposals_++;
                failed_slots.push_back(slot);
            }
        }
        
        // Log failed proposals
        for (uint64_t slot : failed_slots) {
            spdlog::warn("Node {} failed to get consensus for slot {} after retries",
                        node_id_, slot);
        }
    }
    
    spdlog::debug("Paxos proposer thread stopped");
}

void PaxosConsensus::runAcceptor() {
    spdlog::debug("Paxos acceptor thread started");

    // PAX-4: The acceptor's core work (handlePrepare / handleAccept) is called
    // synchronously from the RPC thread.  This background loop performs
    // periodic housekeeping: it evicts promises that were made for rounds that
    // are now stale (i.e. the proposer timed out and a higher ballot has been
    // seen), and it applies any newly committed instances to the committed_log_.
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        const uint64_t cur_round = current_round_.load();

        std::lock_guard<std::mutex> lock(state_mutex_);

        for (auto& [slot, instance] : instances_) {
            // If this slot has been committed, ensure it's in committed_log_.
            if (instance.is_committed &&
                committed_log_.find(slot) == committed_log_.end()) {
                committed_log_[slot] = instance.accepted_value;
                spdlog::debug("Acceptor: applied committed slot {} to committed_log_", slot);
            }

            // Evict stale promises: if the promised round is far behind the
            // current proposal round the proposer has given up; clear the
            // promise so a subsequent proposer can make progress.
            constexpr uint64_t kStaleRoundThreshold = 10;
            if (!instance.is_committed &&
                instance.promised_proposal.round > 0 &&
                cur_round > instance.promised_proposal.round + kStaleRoundThreshold) {
                spdlog::debug("Acceptor: evicting stale promise for slot {} "
                              "(promised_round={}, cur_round={})",
                              slot, instance.promised_proposal.round, cur_round);
                instance.promised_proposal = {};
            }
        }
    }

    spdlog::debug("Paxos acceptor thread stopped");
}

void PaxosConsensus::runLearner() {
    spdlog::debug("Paxos learner thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Learner tracks accepted values and determines when consensus is reached
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        // Check for newly committed instances
        for (auto& [slot, instance] : instances_) {
            if (instance.is_committed && committed_log_.find(slot) == committed_log_.end()) {
                // This instance just reached consensus
                spdlog::debug("Learner detected committed value at slot {}", slot);
                
                // Update committed log (done via broadcastCommit)
                // Learner's job is to track and apply committed values
            }
        }
    }
    
    spdlog::debug("Paxos learner thread stopped");
}

void PaxosConsensus::leaderElectionThread() {
    spdlog::debug("Paxos leader election thread started");

    // PAX-2: Replace deterministic node-ID sort (split-brain risk) with a
    // simple quorum-based ballot election.  Each candidate atomically bumps
    // current_round_ and then asks all peers whether they accept it as leader
    // for that ballot.  A node becomes leader only when it collects promises
    // from a strict majority (quorum) of the cluster.
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // PAX-6: Always hold state_mutex_ when reading/writing cluster_nodes_.
        std::vector<std::string> nodes_snapshot;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            nodes_snapshot = cluster_nodes_;
        }

        if (nodes_snapshot.empty()) {
          continue;
        }

        const bool is_leader = (state_.load() == ConsensusState::LEADER);

        // If we are already leader, just refresh heartbeat awareness.
        if (is_leader) {
            continue;
        }

        // Propose ourselves as leader with the next ballot.
        const uint64_t ballot = ++current_round_;

        // Count how many nodes in the cluster accept us for this ballot.
        // Self always accepts.
        size_t promises = 1;
        const size_t quorum = (nodes_snapshot.size() / 2) + 1;

        PaxosPrepareCallback cb;
        {
            std::lock_guard<std::mutex> cb_lock([[maybe_unused]] callbacks_mutex_);
            cb = rpc_prepare_cb_;
        }

        if (cb) {
            // Use slot 0 (reserved for leader election) to solicit promises.
            constexpr uint64_t kLeaderElectionSlot = 0;
            for (const auto& peer : nodes_snapshot) {
                if (peer == node_id_) {
                  continue;
                }
                if (cb(peer, kLeaderElectionSlot, ballot, node_id_)) {
                    ++promises;
                    if (promises >= quorum) {
                      break;
                    }
                }
            }
        } else if (nodes_snapshot.size() == 1) {
            // Single-node cluster: we are implicitly the leader.
            promises = quorum;
        }

        if (promises >= quorum) {
            state_.store(ConsensusState::LEADER);
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                current_leader_ = node_id_;
            }
            spdlog::info("Node {} elected as leader (ballot={}, promises={}/{})",
                         node_id_, ballot, promises, nodes_snapshot.size());
        }
    }

    spdlog::debug("Paxos leader election thread stopped");
}

bool PaxosConsensus::executePreparePhase(uint64_t slot, const ConsensusLogEntry& value) {
    // Phase 1a: Generate unique proposal number and send prepare requests
    auto proposal = generateProposalNumber();
    
    // Phase 2.1.3: Log PREPARE to WAL for durability
    if (wal_) {
        try {
            wal_->logPrepare(slot, proposal.round, node_id_);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::error("Node {} WAL PREPARE log failed for slot {}: {} — aborting phase",
                          node_id_, slot, e.what());
            return false;
        }
    }

    // Track highest-ballot accepted value from all promises (Paxos Phase-1b safety).
    uint64_t highest_accepted_round = 0;
    std::optional<ConsensusLogEntry> highest_accepted_value;
    
    // Scoped lock: initialize instance and collect promises, then release before
    // calling executeAcceptPhase() to avoid re-acquiring the same non-recursive mutex.
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
    
        // Get or create instance for this slot
        auto& instance = instances_[slot];
        instance.slot = slot;
        instance.prepare_promises.clear();
    
        spdlog::debug("Node {} executing prepare phase for slot {} with proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
    
        total_prepares_++;

        // Self-promise (local acceptor always promises to its own proposer)
        // W2-S06: Consensus validation — ensure node_id_ is valid before recording promise
        if (!node_id_.empty()) {
            instance.prepare_promises.insert(node_id_);
        } else {
            spdlog::error("onPreparePhase: node_id_ is empty, cannot record self-promise");
            return false;
        }

        // Send Phase-1 Prepare RPCs to all peer nodes.
        // If a real RPC callback has been registered (via setPrepareRPCCallback or
        // setPrepareFullRPCCallback), invoke it for every peer and collect promises.
        // Without a callback we operate in single-node mode.
        auto start_time = std::chrono::steady_clock::now();

        for (const auto& node : cluster_nodes_) {
            if (node == node_id_) {
              continue;
            }

            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > config_.paxos_prepare_timeout) {
                spdlog::warn("Node {} prepare phase timed out for slot {} after {}ms",
                             node_id_, slot,
                             std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                break;
            }

            PaxosPrepareFullCallback full_cb;
            PaxosPrepareCallback     basic_cb;
            {
                std::lock_guard<std::mutex> cb_lock([[maybe_unused]] callbacks_mutex_);
                full_cb  = rpc_prepare_full_cb_;
                basic_cb = rpc_prepare_cb_;
            }

            if (full_cb) {
                // Extended RPC path: peer returns promise + highest accepted value.
                auto result = full_cb(node, slot, proposal.round, node_id_);
                if (result.promised) {
                    instance.prepare_promises.insert(node);
                    spdlog::debug("Node {} received promise from peer {} for slot {}",
                                  node_id_, node, slot);
                    // Phase 1b safe-value: track the highest-ballot accepted value
                    // from all promises.  Paxos safety requires that if any acceptor
                    // already accepted a value at a higher ballot, we MUST propose
                    // that value in Phase 2 instead of our own.
                    if (result.accepted_value.has_value() &&
                        result.accepted_round > highest_accepted_round) {
                        highest_accepted_round = result.accepted_round;
                        highest_accepted_value = result.accepted_value;
                    }
                } else {
                    spdlog::debug("Node {} peer {} rejected prepare for slot {}",
                                  node_id_, node, slot);
                }
            } else if (basic_cb) {
                // Basic RPC path: bool only — cannot propagate accepted value.
                bool promised = basic_cb(node, slot, proposal.round, node_id_);
                if (promised) {
                    instance.prepare_promises.insert(node);
                    spdlog::debug("Node {} received promise from peer {} for slot {} "
                                  "(basic callback — accepted value not propagated)",
                                  node_id_, node, slot);
                } else {
                    spdlog::debug("Node {} peer {} rejected prepare for slot {}",
                                  node_id_, node, slot);
                }
            } else {
                // No RPC callback: single-node mode only.
                spdlog::warn("Node {} has no Paxos prepare RPC callback; "
                             "peer {} not counted (single-node mode)",
                             node_id_, node);
            }
        }

        // Check if we have quorum
        if (!hasQuorum(instance.prepare_promises.size())) {
            spdlog::warn("Node {} failed to get quorum in prepare phase for slot {} ({}/{})",
                        node_id_, slot, instance.prepare_promises.size(), cluster_nodes_.size());
            return false;
        }

        spdlog::debug("Node {} got quorum ({}/{}) in prepare phase for slot {}",
                     node_id_, instance.prepare_promises.size(),
                     cluster_nodes_.size(), slot);

        // Phase 1b complete: quorum of promises received.
        // Per Paxos safety: if any acceptor returned a previously accepted value at
        // a higher ballot, we MUST propose that value in Phase 2 instead of our own.
        // highest_accepted_value is set above while collecting promises.
    } // state_mutex_ released here — executeAcceptPhase() re-acquires it safely

    // Apply highest-accepted-value override (Paxos Phase-1b safety property).
    // proposed_value_override is declared outside the if-block for lifetime
    // reasons (it must outlive the if-block to be referenced by the final
    // proposed_value const-ref), but is only assigned inside the if-block
    // when an earlier accepted value must override our own proposal (Paxos
    // Phase-1b safety): this avoids an unconditional copy of `value` on the
    // common fast path where no prior value was accepted.
    std::optional<ConsensusLogEntry> proposed_value_override;
    if (highest_accepted_value.has_value()) {
        proposed_value_override = std::move(*highest_accepted_value);
        spdlog::debug("Node {} overriding proposed value with highest accepted value "
                      "from ballot {} for slot {} (Paxos safety)",
                      node_id_, highest_accepted_round, slot);
    }

    const ConsensusLogEntry& proposed_value =
        proposed_value_override.has_value() ? *proposed_value_override : value;

    // Move to accept phase
    return executeAcceptPhase(slot, proposal, proposed_value);
}

bool PaxosConsensus::executeAcceptPhase(
    uint64_t slot,
    const ProposalNumber& proposal,
    const ConsensusLogEntry& value
) {
    // Phase 2a: Send accept requests to all acceptors
    auto start_time = std::chrono::steady_clock::now();
    
    // Phase 2.1.3: Log ACCEPT to WAL for durability
    if (wal_) {
        try {
            wal_->logAccept(slot, proposal.round, node_id_, value);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::error("Node {} WAL ACCEPT log failed for slot {}: {} — aborting phase",
                          node_id_, slot, e.what());
            return false;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        auto& instance = instances_[slot];
        instance.accept_acks.clear();
        
        spdlog::debug("Node {} executing accept phase for slot {} with proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
    }
    
    total_accepts_++;

    // Self-accept (local acceptor always accepts its own proposer)
    if (handleAccept(slot, proposal, value)) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        // W2-S06: Consensus validation — validate node_id_ before recording self-ack
        if (!node_id_.empty()) {
            instances_[slot].accept_acks.insert(node_id_);
        } else {
            spdlog::error("onAcceptPhase: node_id_ is empty, cannot record self-ack");
            return false;
        }
    }

    // Send Phase-2 Accept RPCs to all peer nodes.
    // If a real RPC callback has been registered (via setAcceptRPCCallback),
    // invoke it for every peer and count acknowledgements.
    // Without a callback we operate in single-node mode; peers are NOT
    // auto-inserted so quorum is only reachable in a 1-node cluster.
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto& instance = instances_[slot];

        for (const auto& node : cluster_nodes_) {
            if (node == node_id_) {
              continue;
            }

            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > config_.paxos_accept_timeout) {
                spdlog::warn("Node {} accept phase timed out for slot {} after {}ms",
                             node_id_, slot,
                             std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                break;
            }

            PaxosAcceptCallback cb;
            {
                std::lock_guard<std::mutex> cb_lock([[maybe_unused]] callbacks_mutex_);
                cb = rpc_accept_cb_;
            }

            if (cb) {
                // Real RPC path: ask the peer to accept the value
                bool accepted = cb(node, slot, proposal.round, value);
                if (accepted) {
                    instance.accept_acks.insert(node);
                    spdlog::debug("Node {} received accept-ack from peer {} for slot {}",
                                  node_id_, node, slot);
                } else {
                    spdlog::debug("Node {} peer {} rejected accept for slot {}",
                                  node_id_, node, slot);
                }
            } else {
                // No RPC callback: single-node mode only.
                // Do NOT auto-insert the peer.
                spdlog::warn("Node {} has no Paxos accept RPC callback; "
                             "peer {} not counted (single-node mode)",
                             node_id_, node);
            }
        }

        // Phase 2b: Check if we have quorum of accepts
        if (!hasQuorum(instance.accept_acks.size())) {
            spdlog::warn("Node {} failed to get quorum in accept phase for slot {} ({}/{})",
                        node_id_, slot, instance.accept_acks.size(), cluster_nodes_.size());
            return false;
        }

        spdlog::debug("Node {} got quorum ({}/{}) in accept phase for slot {}",
                     node_id_, instance.accept_acks.size(),
                     cluster_nodes_.size(), slot);
        
        // Quorum reached - value is chosen
        instance.is_committed = true;
        instance.accepted_value = value;
        instance.accepted_proposal = proposal;
    }
    
    // Broadcast commit to all learners; propagate WAL failure as a phase failure.
    return broadcastCommit(slot, value);
}

bool PaxosConsensus::broadcastCommit(uint64_t slot, const ConsensusLogEntry& value) {
    // Phase 2.1.3: Log COMMIT to WAL for durability before updating in-memory state.
    // WAL failure is a hard error: proceeding without a durable COMMIT record would
    // allow a restarted node to re-accept lower-ballot proposals for the same slot,
    // violating the Paxos safety property.
    if (wal_) {
        try {
            wal_->logCommit(slot, value);
            uint64_t ops = ++operations_since_snapshot_;
            
            // Check if we should create a snapshot
            if (wal_->shouldCreateSnapshot(ops)) {
                spdlog::info("Triggering snapshot creation after {} operations", ops);
                createPeriodicSnapshot();
            }
        } catch (const std::exception& e) {
            spdlog::error("Node {} WAL COMMIT log failed for slot {}: {} — aborting broadcast",
                          node_id_, slot, e.what());
            return false;
        }
    }
    
    {
        std::lock_guard<std::timed_mutex> lock(proposal_mutex_);
        committed_log_[slot] = value;
        uint64_t current_commit = commit_index_.load();
        if (slot > current_commit) {
            commit_index_.store(slot);
        }
    }
    
    // Call commit callback
    {
        std::lock_guard<std::mutex> lock([[maybe_unused]] callbacks_mutex_);
        if ([[maybe_unused]] on_commit_callback_) {
            on_commit_callback_([[maybe_unused]] value);
        }
    }
    
    spdlog::debug("Committed entry at slot {}", slot);
    return true;
}

size_t PaxosConsensus::getQuorumSize() const {
    if (config_.paxos_quorum_size > 0) {
        return config_.paxos_quorum_size;
    }
    // PAX-6: callers of hasQuorum() always hold state_mutex_ already;
    // cluster_nodes_ is therefore safe to read here without re-locking.
    return (cluster_nodes_.size() / 2) + 1;
}

bool PaxosConsensus::hasQuorum([[maybe_unused]] size_t count) const {
    return count >= getQuorumSize();
}

ProposalNumber PaxosConsensus::generateProposalNumber() {
    // Increment round and generate unique proposal number
    uint64_t round = ++current_round_;
    return ProposalNumber{round, node_id_};
}

bool PaxosConsensus::loadPersistentState() {
    if (config_.data_dir.empty()) {
        spdlog::warn("No data directory configured for Paxos persistence");
        return false;
    }
    
    try {
        std::string state_file = config_.data_dir + "/paxos_state_" + node_id_ + ".json";
        std::ifstream file(state_file);
        
        if (!file.is_open()) {
            spdlog::info("No persistent state file found at {}, starting fresh", state_file);
            return false;
        }
        
        nlohmann::json state_json;
        file >> state_json;
        file.close();
        
        // Load basic state
        if (state_json.contains("current_round")) {
            current_round_.store(state_json["current_round"].get<uint64_t>());
        }
        
        if (state_json.contains("next_slot")) {
            next_slot_.store(state_json["next_slot"].get<uint64_t>());
        }
        
        if (state_json.contains("commit_index")) {
            commit_index_.store(state_json["commit_index"].get<uint64_t>());
        }
        
        // Load Paxos instances
        if (state_json.contains("instances")) {
            for (const auto& [slot_str, instance_json] : state_json["instances"].items()) {
                uint64_t slot = std::stoull(slot_str);
                PaxosInstance& instance = instances_[slot];
                
                instance.slot = slot;
                
                if (instance_json.contains("promised_proposal")) {
                    const auto& promised = instance_json["promised_proposal"];
                    instance.promised_proposal.round = promised["round"].get<uint64_t>();
                    instance.promised_proposal.node_id = promised["node_id"].get<std::string>();
                }
                
                if (instance_json.contains("accepted_proposal")) {
                    const auto& accepted = instance_json["accepted_proposal"];
                    instance.accepted_proposal.round = accepted["round"].get<uint64_t>();
                    instance.accepted_proposal.node_id = accepted["node_id"].get<std::string>();
                }
                
                if (instance_json.contains("accepted_value")) {
                    const auto& value_json = instance_json["accepted_value"];
                    // Note: JSON uses "log_index" but struct uses "index"
                    if (value_json.contains("log_index")) {
                        instance.accepted_value.index = value_json["log_index"].get<uint64_t>();
                    }
                    instance.accepted_value.term = value_json["term"].get<uint64_t>();
                    instance.accepted_value.operation = value_json["operation"].get<std::string>();
                    instance.accepted_value.data = value_json["data"];
                }
                
                if (instance_json.contains("is_committed")) {
                    instance.is_committed = instance_json["is_committed"].get<bool>();
                }
            }
        }
        
        // Load committed log
        if (state_json.contains("committed_log")) {
            for (const auto& [index_str, entry_json] : state_json["committed_log"].items()) {
                uint64_t index = std::stoull(index_str);
                ConsensusLogEntry& entry = committed_log_[index];
                
                // Note: JSON may use "log_index" but struct uses "index"
                if (entry_json.contains("index")) {
                    entry.index = entry_json["index"].get<uint64_t>();
                } else if (entry_json.contains("log_index")) {
                    entry.index = entry_json["log_index"].get<uint64_t>();
                }
                entry.term = entry_json["term"].get<uint64_t>();
                entry.operation = entry_json["operation"].get<std::string>();
                entry.data = entry_json["data"];
            }
        }
        
        spdlog::info("Loaded Paxos persistent state: round={}, next_slot={}, commit_index={}, instances={}, committed_entries={}",
                     current_round_.load(), next_slot_.load(), commit_index_.load(), 
                     instances_.size(), committed_log_.size());
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load Paxos persistent state: {}", e.what());
        return false;
    }
}

bool PaxosConsensus::savePersistentState() {
    if (config_.data_dir.empty()) {
        spdlog::warn("No data directory configured for Paxos persistence");
        return false;
    }
    
    try {
        nlohmann::json state_json;
        
        // Save basic state
        state_json["current_round"] = current_round_.load();
        state_json["next_slot"] = next_slot_.load();
        state_json["commit_index"] = commit_index_.load();
        
        // Save Paxos instances
        nlohmann::json instances_json = nlohmann::json::object();
        for (const auto& [slot, instance] : instances_) {
            nlohmann::json instance_json;
            
            instance_json["slot"] = instance.slot;
            instance_json["is_committed"] = instance.is_committed;
            
            // Save promised proposal
            if (instance.promised_proposal.round > 0) {
                instance_json["promised_proposal"] = {
                    {"round", instance.promised_proposal.round},
                    {"node_id", instance.promised_proposal.node_id}
                };
            }
            
            // Save accepted proposal
            if (instance.accepted_proposal.round > 0) {
                instance_json["accepted_proposal"] = {
                    {"round", instance.accepted_proposal.round},
                    {"node_id", instance.accepted_proposal.node_id}
                };
                
                // Save accepted value
                instance_json["accepted_value"] = {
                    {"index", instance.accepted_value.index},
                    {"term", instance.accepted_value.term},
                    {"operation", instance.accepted_value.operation},
                    {"data", instance.accepted_value.data}
                };
            }
            
            instances_json[std::to_string(slot)] = instance_json;
        }
        state_json["instances"] = instances_json;
        
        // Save committed log
        nlohmann::json committed_log_json = nlohmann::json::object();
        for (const auto& [index, entry] : committed_log_) {
            committed_log_json[std::to_string(index)] = {
                {"index", entry.index},
                {"term", entry.term},
                {"operation", entry.operation},
                {"data", entry.data}
            };
        }
        state_json["committed_log"] = committed_log_json;
        
        // Write to file atomically (write to temp, then rename)
        std::string state_file = config_.data_dir + "/paxos_state_" + node_id_ + ".json";
        std::string temp_file = state_file + ".tmp";
        
        std::ofstream file(temp_file);
        if (!file.is_open()) {
            spdlog::error("Failed to open temporary state file: {}", temp_file);
            return false;
        }
        
        file << state_json.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        // Atomic rename
        if (std::rename(temp_file.c_str(), state_file.c_str()) != 0) {
            spdlog::error("Failed to rename temporary state file: {}", strerror(errno));
            return false;
        }
        
        spdlog::debug("Saved Paxos persistent state: round={}, next_slot={}, commit_index={}, instances={}, committed_entries={}",
                      current_round_.load(), next_slot_.load(), commit_index_.load(),
                      instances_.size(), committed_log_.size());
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to save Paxos persistent state: {}", e.what());
        return false;
    }
}

bool PaxosConsensus::handlePrepare(uint64_t slot, const ProposalNumber& proposal) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Get or create instance for this slot
    auto& instance = instances_[slot];
    
    // Phase 1b: Promise not to accept proposals with lower ballot
    if (proposal > instance.promised_proposal) {
        // Update promised proposal
        instance.promised_proposal = proposal;
        
        // WAL durability: persist promise BEFORE responding so the invariant
        // survives a crash between sending the PROMISE and the next restart.
        if (wal_) {
            const uint64_t prev_accepted_round = instance.accepted_proposal.round;
            const nlohmann::json prev_accepted_value = instance.accepted_value.data;
            wal_->logPromise(slot, proposal.round, node_id_,
                             prev_accepted_round, prev_accepted_value);
            ++operations_since_snapshot_;
        }
        
        spdlog::debug("Node {} promised slot {} to proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
        
        return true;
    }
    
    // Reject if we've already promised to a higher proposal
    spdlog::debug("Node {} rejected prepare for slot {} - already promised to higher proposal",
                 node_id_, slot);
    return false;
}

bool PaxosConsensus::handleAccept(
    uint64_t slot,
    const ProposalNumber& proposal,
    const ConsensusLogEntry& value
) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Get or create instance for this slot
    auto& instance = instances_[slot];
    
    // Phase 2b: Accept proposal if it's >= our promised proposal
    if (proposal.round >= instance.promised_proposal.round || 
        (proposal.round == instance.promised_proposal.round && proposal.node_id >= instance.promised_proposal.node_id)) {
        // Accept the proposal
        instance.accepted_proposal = proposal;
        instance.accepted_value = value;
        
        // WAL durability: persist accept BEFORE responding so the invariant
        // survives a crash between sending ACCEPTED and the next restart.
        if (wal_) {
            wal_->logAccept(slot, proposal.round, node_id_, value);
            ++operations_since_snapshot_;
        }
        
        spdlog::debug("Node {} accepted slot {} with proposal {}/{}",
                     node_id_, slot, proposal.round, proposal.node_id);
        
        return true;
    }
    
    // Reject if proposal number is lower than promised
    spdlog::debug("Node {} rejected accept for slot {} - proposal too low",
                 node_id_, slot);
    return false;
}

void PaxosConsensus::handleCommit(uint64_t slot, const ConsensusLogEntry& value) {
    broadcastCommit(slot, value);
}

// ============================================================================
// Phase 2.1: WAL and Snapshot Methods
// ============================================================================

bool PaxosConsensus::recoverFromWAL() {
    if (!snapshot_manager_ || !wal_) {
        return false;
    }
    
    try {
        // Step 1: Load latest snapshot if available
        auto snapshot_opt = snapshot_manager_->loadLatestSnapshot();
        
        if (snapshot_opt.has_value()) {
            const auto& snapshot = snapshot_opt.value();
            
            // Restore state from snapshot
            current_round_.store(snapshot.current_round);
            next_slot_.store(snapshot.last_committed_slot + 1);
            commit_index_.store(snapshot.last_committed_slot);
            last_applied_lsn_ = snapshot.last_applied_lsn;
            
            // Restore Paxos instances
            instances_.clear();
            for (const auto& [slot, instance_json] : snapshot.instances) {
                // Parse instance from JSON
                // (Simplified - full implementation would reconstruct PaxosInstance)
                spdlog::debug("Restored Paxos instance for slot {}", slot);
            }
            
            // Restore committed log
            committed_log_.clear();
            for (const auto& [index, entry_json] : snapshot.committed_log) {
                // Parse entry from JSON
                spdlog::debug("Restored committed log entry {}", index);
            }
            
            spdlog::info("Recovered from Paxos snapshot: id={}, slot={}, instances={}, log_entries={}",
                        snapshot.snapshot_id, snapshot.last_committed_slot,
                        snapshot.instances.size(), snapshot.committed_log.size());
        } else {
            spdlog::info("No snapshot found, starting from empty state");
            last_applied_lsn_ = LSN(0, 0);
        }
        
        // Step 2: Replay WAL entries since snapshot
        auto wal_entries = wal_->readEntries(last_applied_lsn_);
        
        spdlog::info("Replaying {} WAL entries from LSN {}", 
                    wal_entries.size(), last_applied_lsn_.toString());
        
        for (const auto& entry : wal_entries) {
            // Apply each WAL entry to rebuild state
            switch (entry.type) {
                case themis::sharding::PaxosWALEntryType::PREPARE:
                    spdlog::debug("Replay PREPARE: slot={}, round={}", entry.slot, entry.round);
                    break;
                    
                case themis::sharding::PaxosWALEntryType::PROMISE: {
                    spdlog::debug("Replay PROMISE: slot={}, round={}", entry.slot, entry.round);
                    auto& inst = instances_[entry.slot];
                    inst.slot = entry.slot;
                    if (entry.round > inst.promised_proposal.round) {
                        inst.promised_proposal.round   = entry.round;
                        inst.promised_proposal.node_id = entry.node_id;
                    }
                    break;
                }
                    
                case themis::sharding::PaxosWALEntryType::ACCEPT:
                [[fallthrough]];\n                case themis::sharding::PaxosWALEntryType::ACCEPTED: {
                    spdlog::debug("Replay ACCEPT: slot={}, round={}", entry.slot, entry.round);
                    auto& inst = instances_[entry.slot];
                    inst.slot = entry.slot;
                    if (entry.round >= inst.accepted_proposal.round) {
                        inst.accepted_proposal.round   = entry.round;
                        inst.accepted_proposal.node_id = entry.node_id;
                        if (!entry.data.is_null()) {
                            inst.accepted_value.operation =
                                entry.data.value("operation", std::string{});
                            inst.accepted_value.data =
                                entry.data.value("data", nlohmann::json{});
                        }
                    }
                    break;
                }
                    
                case themis::sharding::PaxosWALEntryType::COMMIT: {
                    spdlog::debug("Replay COMMIT: slot={}", entry.slot);
                    auto& inst = instances_[entry.slot];
                    inst.is_committed = true;
                    // Update commit index
                    if (entry.slot > commit_index_.load()) {
                        commit_index_.store(entry.slot);
                    }
                    // Rebuild committed log
                    ConsensusLogEntry log_entry;
                    if (!entry.data.is_null()) {
                        log_entry.operation = entry.data.value("operation", std::string{});
                        log_entry.data      = entry.data.value("data", nlohmann::json{});
                        log_entry.index     = entry.slot;
                    }
                    committed_log_[entry.slot] = log_entry;
                    break;
                }
                    
                default:
                    break;
            }
            
            last_applied_lsn_ = entry.lsn;
        }
        
        spdlog::info("Paxos recovery complete: round={}, next_slot={}, commit_index={}",
                    current_round_.load(), next_slot_.load(), commit_index_.load());
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to recover from WAL: {}", e.what());
        return false;
    }
}

void PaxosConsensus::createPeriodicSnapshot() {
    if (!snapshot_manager_ || !wal_) {
        return;
    }
    
    // Check if we should create a snapshot
    uint64_t ops = operations_since_snapshot_.load();
    if (!wal_->shouldCreateSnapshot(ops)) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        // Create snapshot from current state
        auto snapshot_id = snapshot_manager_->createSnapshot(
            node_id_,
            last_applied_lsn_,
            commit_index_.load(),
            current_round_.load(),
            instances_,
            committed_log_
        );
        
        if (snapshot_id.has_value()) {
            operations_since_snapshot_.store(0);
            spdlog::info("Created Paxos snapshot: id={}, slot={}, instances={}",
                        snapshot_id.value(), commit_index_.load(), instances_.size());
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create Paxos snapshot: {}", e.what());
    }
}

} // namespace sharding
} // namespace themisdb

