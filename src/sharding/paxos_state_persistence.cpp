/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            paxos_state_persistence.cpp                        ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:52:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     310                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • da5848ffb4  2026-03-10  fix: apply all 7 code review recommendations + LSN.toStri... ║
    • a3ec4aa9e9  2026-03-10  refactor: update tenant metrics handling and improve modu... ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/paxos_state_persistence.h"
#include "sharding/paxos_consensus.h"
#include "utils/logger.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_map>

namespace fs   = std::filesystem;
using json     = nlohmann::json;

namespace themis {
namespace sharding {

// ─────────────────────────────────────────────────────────────────────────────
// DurableAcceptorState
// ─────────────────────────────────────────────────────────────────────────────

bool DurableAcceptorState::operator==(const DurableAcceptorState& o) const noexcept {
    return slot           == o.slot
        && promised_round == o.promised_round
        && promised_node  == o.promised_node
        && accepted_round == o.accepted_round
        && accepted_value == o.accepted_value
        && is_committed   == o.is_committed;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

PaxosStatePersistence::PaxosStatePersistence(PaxosWAL*             wal,
                                              PaxosSnapshotManager* snapshot_mgr)
    : PaxosStatePersistence(wal, snapshot_mgr, Config{})
{}

PaxosStatePersistence::PaxosStatePersistence(PaxosWAL*             wal,
                                              PaxosSnapshotManager* snapshot_mgr,
                                              const Config&         config)
    : wal_(wal), snapshot_mgr_(snapshot_mgr), config_(config)
{
    if (!wal_)          throw std::invalid_argument("PaxosStatePersistence: wal cannot be null");
    if (!snapshot_mgr_) throw std::invalid_argument("PaxosStatePersistence: snapshot_mgr cannot be null");
}

// ─────────────────────────────────────────────────────────────────────────────
// open
// ─────────────────────────────────────────────────────────────────────────────

bool PaxosStatePersistence::open(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_open_.load()) return true;

    node_state_.node_id = node_id;
    fs::create_directories(config_.state_dir);

    // Try to load the latest snapshot
    try {
        auto snap = snapshot_mgr_->loadLatestSnapshot();
        if (snap) {
            node_state_.last_committed = snap->last_committed_slot;
            node_state_.current_round  = snap->current_round;
            node_state_.last_lsn       = snap->last_applied_lsn;
            THEMIS_INFO("PaxosStatePersistence: loaded snapshot "
                        "slot={} round={} lsn={}",
                        node_state_.last_committed,
                        node_state_.current_round,
                        node_state_.last_lsn.toString());
        }
    } catch (const std::exception& ex) {
        THEMIS_WARN("PaxosStatePersistence: snapshot load failed: {}", ex.what());
    }

    // Replay WAL tail
    try {
        replayWal(node_state_.last_lsn);
    } catch (const std::exception& ex) {
        THEMIS_WARN("PaxosStatePersistence: WAL replay failed: {}", ex.what());
    }

    is_open_.store(true);
    THEMIS_INFO("PaxosStatePersistence: opened for node '{}' "
                "last_committed={} round={}",
                node_id, node_state_.last_committed, node_state_.current_round);
    return true;
}

void PaxosStatePersistence::close() {
    is_open_.store(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// WAL replay
// ─────────────────────────────────────────────────────────────────────────────

void PaxosStatePersistence::replayWal(LSN from_lsn) {
    // Read entries from the PaxosWAL since the last snapshot LSN
    auto entries = wal_->readEntries(from_lsn);
    size_t replayed = 0;
    for (const auto& entry : entries) {
        uint64_t slot = entry.slot;

        DurableAcceptorState& s = slot_cache_[slot];
        s.slot = slot;

        switch (entry.type) {
            case PaxosWALEntryType::PROMISE:
                s.promised_round = entry.round;
                s.promised_node  = entry.node_id;
                break;
            case PaxosWALEntryType::ACCEPT:
            case PaxosWALEntryType::ACCEPTED:
                s.accepted_round = entry.round;
                s.accepted_value = entry.data.dump();
                break;
            case PaxosWALEntryType::COMMIT:
                s.is_committed = true;
                if (slot > node_state_.last_committed)
                    node_state_.last_committed = slot;
                break;
            default:
                break;
        }

        if (entry.round > node_state_.current_round)
            node_state_.current_round = entry.round;

        node_state_.last_lsn = entry.lsn;
        ++replayed;
    }
    if (replayed > 0)
        THEMIS_INFO("PaxosStatePersistence: replayed {} WAL entries", replayed);
}

// ─────────────────────────────────────────────────────────────────────────────
// persistPromise
// ─────────────────────────────────────────────────────────────────────────────

bool PaxosStatePersistence::persistPromise(uint64_t slot,
                                            uint64_t ballot_round,
                                            const std::string& proposer_node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open_.load()) return false;

    DurableAcceptorState& s = slot_cache_[slot];
    s.slot           = slot;
    s.promised_round = ballot_round;
    s.promised_node  = proposer_node_id;

    if (ballot_round > node_state_.current_round)
        node_state_.current_round = ballot_round;

    LSN lsn = wal_->logPromise(slot, ballot_round, proposer_node_id,
                                s.accepted_round,
                                s.accepted_value.empty() ? json{} : json::parse(s.accepted_value, nullptr, false));
    if (config_.sync_on_write) wal_->flush();
    node_state_.last_lsn = lsn;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// persistAccept
// ─────────────────────────────────────────────────────────────────────────────

bool PaxosStatePersistence::persistAccept(uint64_t slot,
                                           uint64_t ballot_round,
                                           const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open_.load()) return false;

    DurableAcceptorState& s = slot_cache_[slot];
    s.slot          = slot;
    s.accepted_round = ballot_round;
    s.accepted_value = value;

    // Build a ConsensusLogEntry placeholder with the value as command
    ConsensusLogEntry entry;
    entry.operation = value;

    LSN lsn = wal_->logAccept(slot, ballot_round, node_state_.node_id, entry);
    if (config_.sync_on_write) wal_->flush();
    node_state_.last_lsn = lsn;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// persistCommit
// ─────────────────────────────────────────────────────────────────────────────

bool PaxosStatePersistence::persistCommit(uint64_t slot) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open_.load()) return false;

    auto it = slot_cache_.find(slot);
    if (it != slot_cache_.end()) {
        it->second.is_committed = true;
    } else {
        DurableAcceptorState s;
        s.slot = slot;
        s.is_committed = true;
        slot_cache_[slot] = s;
    }

    if (slot > node_state_.last_committed)
        node_state_.last_committed = slot;

    ++commits_since_compact_;

    ConsensusLogEntry entry;
    if (slot_cache_.count(slot) && !slot_cache_[slot].accepted_value.empty()) {
        entry.operation = slot_cache_[slot].accepted_value;
    }
    LSN lsn = wal_->logCommit(slot, entry);
    if (config_.sync_on_write) wal_->flush();
    node_state_.last_lsn = lsn;

    // Inline compaction check (lock already held)
    if (commits_since_compact_ >= config_.compact_interval) {
        mutex_.unlock();
        forceCompact();
        mutex_.lock();
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// getAcceptorState
// ─────────────────────────────────────────────────────────────────────────────

std::optional<DurableAcceptorState>
PaxosStatePersistence::getAcceptorState(uint64_t slot) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = slot_cache_.find(slot);
    if (it == slot_cache_.end()) return std::nullopt;
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
// maybeCompact / forceCompact
// ─────────────────────────────────────────────────────────────────────────────

void PaxosStatePersistence::maybeCompact() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (commits_since_compact_ < config_.compact_interval) return;
    }
    forceCompact();
}

bool PaxosStatePersistence::forceCompact() {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        // Build snapshot data from the in-memory cache
        std::map<uint64_t, PaxosInstance>        instances;
        std::map<uint64_t, ConsensusLogEntry>     committed_log;
        for (const auto& [s, state] : slot_cache_) {
            if (state.is_committed) {
                ConsensusLogEntry e;
                e.operation = state.accepted_value;
                committed_log[s] = e;
            }
        }

        snapshot_mgr_->createSnapshot(
            node_state_.node_id,
            node_state_.last_lsn,
            node_state_.last_committed,
            node_state_.current_round,
            instances,
            committed_log
        );

        commits_since_compact_ = 0;
        THEMIS_INFO("PaxosStatePersistence: compacted at slot={} round={}",
                    node_state_.last_committed, node_state_.current_round);
        return true;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("PaxosStatePersistence::forceCompact: {}", ex.what());
        return false;
    }
}

} // namespace sharding
} // namespace themis
