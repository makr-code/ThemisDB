/*
 * ThemisDB | File: two_phase_commit_coordinator.cpp | Version: 0.0.34 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 502
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=107 | delta=104 | status=divergent
 * External Severity (v3): C=10, H=87, M=10
 * PR: #4450 docs(perf): corrected root-cause analysis for PERF-D1â€“D7 issues w... (2026-04-07T08:17:34Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Two-Phase Commit (2PC) Coordinator – cross-shard transaction driver
//
// CC-5 NOTE: ThemisDB contains three independent 2PC implementations with
// different state machines, WAL integration depths, and recovery logic:
//   1. two_phase_commit_coordinator.cpp  (this file) — standalone coordinator
//   2. cross_shard_transaction.cpp       — CrossShardTransactionCoordinator
//   3. distributed_transaction.cpp       — DistributedTransactionCoordinator
// A transaction begun with one coordinator CANNOT be recovered by another.
// Future work: unify under a single 2PC engine (Target: v2.0.0).

#include "sharding/two_phase_commit_coordinator.h"
#include "sharding/shard_rpc_client_adapter.h"
#include "sharding/metrics_registry.h"
#include "sharding/prometheus_metrics.h"
#include "utils/logger.h"
#include <chrono>
#include <stdexcept>

namespace themis::sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

TwoPhaseCommitCoordinator::TwoPhaseCommitCoordinator(
    const std::string& coordinator_id
)
    : TwoPhaseCommitCoordinator(coordinator_id, Config{})
{}

TwoPhaseCommitCoordinator::TwoPhaseCommitCoordinator(
    const std::string& coordinator_id,
    const Config&      config
)
    : coordinator_id_(coordinator_id)
    , config_(config)
{
    if (!config_.wal_directory.empty()) {
        WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = config_.wal_directory;
        wal_cfg.sync_on_write = config_.sync_wal_writes;
        wal_ = std::make_unique<WALManager>(wal_cfg);
        THEMIS_INFO("2PC coordinator [{}] WAL initialised at {}",
                    coordinator_id_, config_.wal_directory);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Participant management
// ─────────────────────────────────────────────────────────────────────────────

void TwoPhaseCommitCoordinator::registerParticipant(
    const std::string&              shard_id,
    ShardRPCServer::RequestHandler* participant
) {
    if (!participant) {
        throw std::invalid_argument("participant must not be null");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    participants_[shard_id] = participant;
    THEMIS_DEBUG("2PC coordinator [{}] registered participant shard {}",
                 coordinator_id_, shard_id);
}

bool TwoPhaseCommitCoordinator::unregisterParticipant(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    owned_adapters_.erase(shard_id); // also remove any owned adapter
    return participants_.erase(shard_id) > 0;
}

void TwoPhaseCommitCoordinator::registerParticipantByEndpoint(
    const std::string&            shard_id,
    const ShardRPCClient::Config& rpc_config
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto adapter = std::make_unique<ShardRPCClientAdapter>(rpc_config);
    participants_[shard_id] = adapter.get();
    owned_adapters_[shard_id] = std::move(adapter);
    THEMIS_DEBUG("2PC coordinator [{}] registered remote participant shard {} at {}",
                 coordinator_id_, shard_id, rpc_config.endpoint);
}

size_t TwoPhaseCommitCoordinator::participantCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return participants_.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Core 2PC protocol
// ─────────────────────────────────────────────────────────────────────────────

CoordinatorTxnOutcome TwoPhaseCommitCoordinator::commit(
    const std::string&                           transaction_id,
    const std::map<std::string, nlohmann::json>& ops_per_shard
) {
    const auto t0 = std::chrono::steady_clock::now();

    CoordinatorTxnOutcome outcome;
    outcome.transaction_id = transaction_id;

    if (ops_per_shard.empty()) {
        outcome.result = CoordinatorTxnResult::ABORTED;
        outcome.reason = "no shards specified";
        THEMIS_WARN("2PC coordinator [{}] txn {} – no shards, aborting",
                    coordinator_id_, transaction_id);
        return outcome;
    }

    // ── Build the record ────────────────────────────────────────────────────
    CoordinatorTxnRecord rec;
    rec.transaction_id = transaction_id;
    rec.state          = CoordinatorTxnState::ACTIVE;
    rec.started_at     = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Detect duplicate transaction IDs
        if (transactions_.count(transaction_id)) {
            auto& existing = transactions_.at(transaction_id);
            if (existing.state == CoordinatorTxnState::COMPLETED) {
                THEMIS_DEBUG("2PC coordinator [{}] duplicate commit for completed txn {} – "
                             "returning idempotent COMMITTED",
                             coordinator_id_, transaction_id);
                outcome.result = CoordinatorTxnResult::COMMITTED;
                return outcome;
            }
            outcome.result = CoordinatorTxnResult::ERROR;
            outcome.reason = "transaction already in progress";
            return outcome;
        }

        // Validate all shards are registered
        for (const auto& [shard_id, _] : ops_per_shard) {
            if (participants_.find(shard_id) == participants_.end()) {
                outcome.result = CoordinatorTxnResult::ERROR;
                outcome.reason = "unknown shard: " + shard_id;
                THEMIS_ERROR("2PC coordinator [{}] txn {} – unknown shard {}",
                             coordinator_id_, transaction_id, shard_id);
                return outcome;
            }
        }

        // Build per-shard payloads
        for (const auto& [shard_id, ops] : ops_per_shard) {
            rec.shard_payloads[shard_id] = buildPayload(ops);
        }

        transactions_[transaction_id] = rec;
    }

    // Persist BEGIN_TX
    logToWAL(WALEntryType::BEGIN_TX, transaction_id, {
        {"transaction_id", transaction_id},
        {"coordinator_id", coordinator_id_},
        {"shards",         [&]() {
            std::vector<std::string> v;
            for (auto& [s, _] : ops_per_shard) v.push_back(s);
            return v;
        }()}
    });

    total_transactions_.fetch_add(1, std::memory_order_relaxed);

    // ── Phase 1: PREPARE ───────────────────────────────────────────────────
    const auto t1 = std::chrono::steady_clock::now();
    bool all_prepared = false;
    {
        // 2PC-1: use unique_lock so runPhase1 can release it around each RPC.
        std::unique_lock<std::mutex> lock(mutex_);
        auto& stored = transactions_.at(transaction_id);
        stored.state = CoordinatorTxnState::PREPARING;
        all_prepared = runPhase1(stored, lock);
        // lock is re-held here after runPhase1 returns
        stored.state = all_prepared
            ? CoordinatorTxnState::COMMIT_DECIDED
            : CoordinatorTxnState::ABORT_DECIDED;
    }

    const double prepare_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t1).count();

    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCPreparePhase(coordinator_id_, prepare_ms, all_prepared);
    }

    // Persist the Phase 2 decision
    // 2PC-3: Tag this entry with phase="decision" so recovery can distinguish
    // it from the completion entry written after Phase 2 finishes.
    logToWAL(all_prepared ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
             transaction_id,
             {
                {"transaction_id", transaction_id},
                {"coordinator_id", coordinator_id_},
                {"phase",          "decision"},
                {"decision",       all_prepared ? "commit" : "abort"}
             });

    // ── Phase 2: COMMIT or ABORT ──────────────────────────────────────────
    const auto t2 = std::chrono::steady_clock::now();
    {
        // 2PC-1: use unique_lock so runPhase2 can release it around each RPC.
        std::unique_lock<std::mutex> lock(mutex_);
        auto& stored = transactions_.at(transaction_id);
        runPhase2(stored, all_prepared, lock);
        // lock is re-held here after runPhase2 returns
        stored.state = CoordinatorTxnState::COMPLETED;
    }

    const double phase2_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t2).count();

    logToWAL(all_prepared ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
             transaction_id, {
                 {"transaction_id", transaction_id},
                 {"coordinator_id", coordinator_id_},
                 {"phase",          "complete"}
             });

    if (all_prepared) {
        total_commits_.fetch_add(1, std::memory_order_relaxed);
        outcome.result = CoordinatorTxnResult::COMMITTED;
        THEMIS_INFO("2PC coordinator [{}] txn {} COMMITTED", coordinator_id_, transaction_id);
    } else {
        total_aborts_.fetch_add(1, std::memory_order_relaxed);
        outcome.result = CoordinatorTxnResult::ABORTED;
        outcome.reason = "one or more participants voted ABORT";
        THEMIS_INFO("2PC coordinator [{}] txn {} ABORTED", coordinator_id_, transaction_id);
    }

    [[maybe_unused]] const double total_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCCommitPhase(coordinator_id_, phase2_ms, all_prepared);
        m->record2PCTransaction(coordinator_id_, all_prepared);
        if (!all_prepared) {
            m->record2PCAbort(coordinator_id_, outcome.reason);
        }
        // available for future histogram
    }

    return outcome;
}

// ─────────────────────────────────────────────────────────────────────────────
// Recovery
// ─────────────────────────────────────────────────────────────────────────────

size_t TwoPhaseCommitCoordinator::recoverInDoubtTransactions() {
    if (!wal_) return 0;

    THEMIS_INFO("2PC coordinator [{}] recovering from WAL…", coordinator_id_);

    size_t resolved = 0;

    try {
        auto entries = wal_->readRange(wal_->getOldestLSN());

        // Replay entries to rebuild coordinator state
        std::map<std::string, CoordinatorTxnRecord> recovered;
        std::map<std::string, bool>                 decisions; // txn_id → commit?

        for (const auto& entry : entries) {
            const std::string& txn_id = entry.transaction_id;
            if (txn_id.empty()) continue;

            if (entry.type == WALEntryType::BEGIN_TX) {
                CoordinatorTxnRecord& rec  = recovered[txn_id];
                rec.transaction_id         = txn_id;
                rec.state                  = CoordinatorTxnState::ACTIVE;
                rec.started_at             = std::chrono::steady_clock::now();
            } else if (entry.type == WALEntryType::COMMIT_TX) {
                auto phase = entry.data.value("phase", "");
                if (phase == "complete") {
                    if (recovered.count(txn_id)) {
                        recovered[txn_id].state = CoordinatorTxnState::COMPLETED;
                    }
                } else {
                    auto decision = entry.data.value("decision", "");
                    if (decision == "commit") {
                        decisions[txn_id] = true;
                        if (recovered.count(txn_id))
                            recovered[txn_id].state = CoordinatorTxnState::COMMIT_DECIDED;
                    }
                }
            } else if (entry.type == WALEntryType::ABORT_TX) {
                auto phase = entry.data.value("phase", "");
                if (phase == "complete") {
                    if (recovered.count(txn_id))
                        recovered[txn_id].state = CoordinatorTxnState::COMPLETED;
                } else {
                    decisions[txn_id] = false;
                    if (recovered.count(txn_id))
                        recovered[txn_id].state = CoordinatorTxnState::ABORT_DECIDED;
                }
            }
        }

        // Re-drive transactions that have a Phase 2 decision but are not COMPLETED
        // 2PC-1: use unique_lock so runPhase2 can release it around each RPC.
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto& [txn_id, rec] : recovered) {
            if (rec.state == CoordinatorTxnState::COMPLETED) {
                transactions_[txn_id] = rec;
                continue;
            }

            auto it = decisions.find(txn_id);
            if (it == decisions.end()) {
                // No decision logged → abort conservatively and broadcast ABORT
                // to release participants from PREPARED state (2PC-2 fix).
                THEMIS_WARN("2PC coordinator [{}] in-doubt txn {} has no decision "
                            "– aborting conservatively and broadcasting ABORT",
                            coordinator_id_, txn_id);
                rec.state = CoordinatorTxnState::ABORT_DECIDED;
                runPhase2(rec, false, lock);
                // lock is re-held here after runPhase2 returns
                rec.state = CoordinatorTxnState::COMPLETED;
                transactions_[txn_id] = rec;
                logToWAL(WALEntryType::ABORT_TX, txn_id, {
                    {"transaction_id", txn_id},
                    {"coordinator_id", coordinator_id_},
                    {"phase",          "complete"},
                    {"recovery",       true},
                    {"reason",         "no_decision_conservative_abort"}
                });
                ++resolved;
                continue;
            }

            const bool do_commit = it->second;
            rec.state = do_commit
                ? CoordinatorTxnState::COMMIT_DECIDED
                : CoordinatorTxnState::ABORT_DECIDED;

            THEMIS_WARN("2PC coordinator [{}] re-driving in-doubt txn {} with decision {}",
                        coordinator_id_, txn_id, do_commit ? "COMMIT" : "ABORT");

            runPhase2(rec, do_commit, lock);
            // lock is re-held here after runPhase2 returns
            rec.state = CoordinatorTxnState::COMPLETED;
            transactions_[txn_id] = rec;

            logToWAL(do_commit ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
                     txn_id, {
                         {"transaction_id", txn_id},
                         {"coordinator_id", coordinator_id_},
                         {"phase",          "complete"},
                         {"recovery",       true}
                     });

            ++resolved;
        }

    } catch (const std::exception& e) {
        THEMIS_ERROR("2PC coordinator [{}] WAL recovery failed: {}", coordinator_id_, e.what());
    }

    THEMIS_INFO("2PC coordinator [{}] recovery complete – {} in-doubt txns resolved",
                coordinator_id_, resolved);
    return resolved;
}

// ─────────────────────────────────────────────────────────────────────────────
// Introspection
// ─────────────────────────────────────────────────────────────────────────────

std::optional<CoordinatorTxnState>
TwoPhaseCommitCoordinator::getTransactionState(const std::string& transaction_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) return std::nullopt;
    return it->second.state;
}

nlohmann::json TwoPhaseCommitCoordinator::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t active    = 0;
    size_t completed = 0;
    for (const auto& [id, rec] : transactions_) {
        if (rec.state == CoordinatorTxnState::COMPLETED) ++completed;
        else                                              ++active;
    }

    const auto uptime = std::chrono::steady_clock::now() - start_time_;
    return {
        {"coordinator_id",      coordinator_id_},
        {"uptime_seconds",      static_cast<uint64_t>(
                                    std::chrono::duration_cast<std::chrono::seconds>(uptime).count())},
        {"registered_shards",   participants_.size()},
        {"total_transactions",  total_transactions_.load()},
        {"total_commits",       total_commits_.load()},
        {"total_aborts",        total_aborts_.load()},
        {"total_errors",        total_errors_.load()},
        {"active_transactions", active},
        {"completed_transactions", completed}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers (called with mutex_ held)
// ─────────────────────────────────────────────────────────────────────────────

bool TwoPhaseCommitCoordinator::runPhase1(CoordinatorTxnRecord& rec,
                                          std::unique_lock<std::mutex>& lock) {
    // 2PC-1: mutex_ must be held by the caller on entry (asserted by contract);
    // we release it around each blocking RPC call and re-acquire before
    // touching shared state, so that concurrent coordinator operations are
    // not serialised behind network I/O.
    bool all_committed = true;

    for (const auto& [shard_id, payload] : rec.shard_payloads) {
        auto pit = participants_.find(shard_id);  // safe: lock held
        if (pit == participants_.end()) {
            THEMIS_ERROR("2PC coordinator [{}] Phase 1 – participant {} not found for txn {}",
                         coordinator_id_, shard_id, rec.transaction_id);
            rec.votes[shard_id] = false;
            all_committed       = false;
            continue;
        }

        // Capture the raw pointer while the lock is held; the object's lifetime
        // is tied to owned_adapters_ (or external caller), which outlives this call.
        auto* participant = pit->second;

        bool vote = false;
        lock.unlock();  // release mutex_ before blocking RPC
        try {
            vote = participant->onPrepare(
                rec.transaction_id,
                coordinator_id_,
                payload
            );
        } catch (const std::exception& e) {
            THEMIS_ERROR("2PC coordinator [{}] Phase 1 – shard {} threw on PREPARE for txn {}: {}",
                         coordinator_id_, shard_id, rec.transaction_id, e.what());
            vote = false;
        }
        lock.lock();  // re-acquire before touching shared state

        rec.votes[shard_id] = vote;
        if (!vote) {
            THEMIS_INFO("2PC coordinator [{}] shard {} voted ABORT for txn {}",
                        coordinator_id_, shard_id, rec.transaction_id);
            all_committed = false;
        }
    }

    return all_committed;
}

void TwoPhaseCommitCoordinator::runPhase2(CoordinatorTxnRecord& rec, bool do_commit,
                                          std::unique_lock<std::mutex>& lock) {
    // 2PC-1: same pattern as runPhase1 — release lock around each blocking RPC.
    for (const auto& [shard_id, _] : rec.shard_payloads) {
        auto pit = participants_.find(shard_id);  // safe: lock held
        if (pit == participants_.end()) {
            THEMIS_WARN("2PC coordinator [{}] Phase 2 – participant {} not found for txn {} "
                        "(shard may have been deregistered)",
                        coordinator_id_, shard_id, rec.transaction_id);
            continue;
        }

        auto* participant = pit->second;  // capture while lock is held

        lock.unlock();  // release mutex_ before blocking RPC
        try {
            if (do_commit) {
                participant->onCommit(rec.transaction_id);
            } else {
                participant->onAbort(rec.transaction_id);
            }
            lock.lock();  // re-acquire before accessing shared state
            rec.phase2_acked.push_back(shard_id);
        } catch (const std::exception& e) {
            lock.lock();  // ensure lock is re-acquired even on error path
            // Log and continue – idempotency allows retrying later
            THEMIS_ERROR("2PC coordinator [{}] Phase 2 – shard {} threw on {} for txn {}: {}",
                         coordinator_id_, shard_id,
                         do_commit ? "COMMIT" : "ABORT",
                         rec.transaction_id, e.what());
        }
    }
}

std::string TwoPhaseCommitCoordinator::buildPayload(const nlohmann::json& ops) {
    nlohmann::json j;
    j["operations"] = ops;
    return j.dump();
}

void TwoPhaseCommitCoordinator::logToWAL(
    WALEntryType          type,
    const std::string&    txn_id,
    const nlohmann::json& data
) {
    if (!wal_) return;

    try {
        WALEntry entry;
        entry.type           = type;
        entry.transaction_id = txn_id;
        entry.timestamp      = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        entry.data = data;

        wal_->append(entry);
        if (config_.sync_wal_writes) {
            wal_->flush();
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("2PC coordinator [{}] WAL write failed for txn {}: {}",
                     coordinator_id_, txn_id, e.what());
    }
}

} // namespace themis::sharding
