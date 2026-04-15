/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            two_phase_commit_coordinator.cpp                   ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-04-15 18:10:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     507                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Two-Phase Commit (2PC) Coordinator – cross-shard transaction driver

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
        std::lock_guard<std::mutex> lock(mutex_);
        auto& stored = transactions_.at(transaction_id);
        stored.state = CoordinatorTxnState::PREPARING;
        all_prepared = runPhase1(stored);

        if (all_prepared) {
            stored.state = CoordinatorTxnState::COMMIT_DECIDED;
        } else {
            stored.state = CoordinatorTxnState::ABORT_DECIDED;
        }
    }

    const double prepare_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t1).count();

    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCPreparePhase(coordinator_id_, prepare_ms, all_prepared);
    }

    // Persist the Phase 2 decision
    logToWAL(all_prepared ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
             transaction_id,
             {
                {"transaction_id", transaction_id},
                {"coordinator_id", coordinator_id_},
                {"decision",       all_prepared ? "commit" : "abort"}
             });

    // ── Phase 2: COMMIT or ABORT ──────────────────────────────────────────
    const auto t2 = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& stored = transactions_.at(transaction_id);
        runPhase2(stored, all_prepared);
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
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [txn_id, rec] : recovered) {
            if (rec.state == CoordinatorTxnState::COMPLETED) {
                transactions_[txn_id] = rec;
                continue;
            }

            auto it = decisions.find(txn_id);
            if (it == decisions.end()) {
                // No decision logged → cannot re-drive; mark for human review
                THEMIS_WARN("2PC coordinator [{}] in-doubt txn {} has no decision "
                            "– skipping (manual resolution required)",
                            coordinator_id_, txn_id);
                rec.state         = CoordinatorTxnState::ABORT_DECIDED;
                transactions_[txn_id] = rec;
                continue;
            }

            const bool do_commit = it->second;
            rec.state = do_commit
                ? CoordinatorTxnState::COMMIT_DECIDED
                : CoordinatorTxnState::ABORT_DECIDED;

            THEMIS_WARN("2PC coordinator [{}] re-driving in-doubt txn {} with decision {}",
                        coordinator_id_, txn_id, do_commit ? "COMMIT" : "ABORT");

            runPhase2(rec, do_commit);
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

bool TwoPhaseCommitCoordinator::runPhase1(CoordinatorTxnRecord& rec) {
    // mutex_ is held by caller
    bool all_committed = true;

    for (const auto& [shard_id, payload] : rec.shard_payloads) {
        auto pit = participants_.find(shard_id);
        if (pit == participants_.end()) {
            THEMIS_ERROR("2PC coordinator [{}] Phase 1 – participant {} not found for txn {}",
                         coordinator_id_, shard_id, rec.transaction_id);
            rec.votes[shard_id] = false;
            all_committed       = false;
            continue;
        }

        bool vote = false;
        try {
            vote = pit->second->onPrepare(
                rec.transaction_id,
                coordinator_id_,
                payload
            );
        } catch (const std::exception& e) {
            THEMIS_ERROR("2PC coordinator [{}] Phase 1 – shard {} threw on PREPARE for txn {}: {}",
                         coordinator_id_, shard_id, rec.transaction_id, e.what());
            vote = false;
        }

        rec.votes[shard_id] = vote;
        if (!vote) {
            THEMIS_INFO("2PC coordinator [{}] shard {} voted ABORT for txn {}",
                        coordinator_id_, shard_id, rec.transaction_id);
            all_committed = false;
        }
    }

    return all_committed;
}

void TwoPhaseCommitCoordinator::runPhase2(CoordinatorTxnRecord& rec, bool do_commit) {
    // mutex_ is held by caller
    for (const auto& [shard_id, _] : rec.shard_payloads) {
        auto pit = participants_.find(shard_id);
        if (pit == participants_.end()) {
            THEMIS_WARN("2PC coordinator [{}] Phase 2 – participant {} not found for txn {} "
                        "(shard may have been deregistered)",
                        coordinator_id_, shard_id, rec.transaction_id);
            continue;
        }

        try {
            if (do_commit) {
                pit->second->onCommit(rec.transaction_id);
            } else {
                pit->second->onAbort(rec.transaction_id);
            }
            rec.phase2_acked.push_back(shard_id);
        } catch (const std::exception& e) {
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
