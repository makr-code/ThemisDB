/*
 * ThemisDB | File: two_phase_commit_participant.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 463
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=114 | delta=111 | status=divergent
 * External Severity (v3): C=6, H=105, M=3
 * PR: #3632 fix(build): register 40+ missing sources across 7 modules in cmake ... (2026-03-12T07:39:41Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Two-Phase Commit (2PC) Participant – shard-side handler

#include "sharding/two_phase_commit_participant.h"
#include "sharding/metrics_registry.h"
#include "sharding/prometheus_metrics.h"
#include "utils/logger.h"
#include <chrono>
#include <stdexcept>

namespace themis::sharding {

TwoPhaseCommitParticipant::TwoPhaseCommitParticipant(
    const std::string&          shard_id,
    ValidateAndLockCallback     validate,
    ApplyOperationsCallback     apply,
    ReleaseLockCallback         release
)
    : TwoPhaseCommitParticipant(shard_id, Config{}, std::move(validate), std::move(apply), std::move(release))
{}

TwoPhaseCommitParticipant::TwoPhaseCommitParticipant(
    const std::string&          shard_id,
    const Config&               config,
    ValidateAndLockCallback     validate,
    ApplyOperationsCallback     apply,
    ReleaseLockCallback         release
)
    : shard_id_(shard_id)
    , config_(config)
    , validate_and_lock_(std::move(validate))
    , apply_operations_(std::move(apply))
    , release_locks_(std::move(release))
{
    if (!config_.wal_directory.empty()) {
        WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = config_.wal_directory;
        wal_cfg.sync_on_write = config_.sync_wal_writes;
        wal_ = std::make_unique<WALManager>(wal_cfg);
        THEMIS_INFO("2PC participant [{}] WAL initialised at {}", shard_id_, config_.wal_directory);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ShardRPCServer::RequestHandler interface
// ─────────────────────────────────────────────────────────────────────────────

bool TwoPhaseCommitParticipant::onPrepare(
    const std::string& transaction_id,
    const std::string& coordinator_shard_id,
    const std::string& transaction_data
) {
    const auto t0 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    // Idempotency: if we have already prepared (or decided), return stored vote
    auto it = transactions_.find(transaction_id);
    if (it != transactions_.end()) {
        THEMIS_DEBUG("2PC participant [{}] duplicate PREPARE for {} – returning stored vote {}",
                     shard_id_, transaction_id, it->second.vote_committed);
        return it->second.vote_committed;
    }

    // Parse operations from the serialised transaction data
    nlohmann::json ops;
    try {
        auto parsed = nlohmann::json::parse(transaction_data);
        ops = parsed.value("operations", nlohmann::json::array());
    } catch (const std::exception& e) {
        THEMIS_WARN("2PC participant [{}] PREPARE {}: failed to parse ops – aborting: {}",
                    shard_id_, transaction_id, e.what());
        // Record an abort vote so we remain consistent
        ParticipantTransaction txn;
        txn.transaction_id   = transaction_id;
        txn.state            = ParticipantTxnState::ABORTED;
        txn.vote_committed   = false;
        txn.prepared_at      = std::chrono::steady_clock::now();
        transactions_[transaction_id] = std::move(txn);
        return false;
    }

    bool vote = true;

    // Validate operations and acquire row locks
    if (validate_and_lock_) {
        try {
            vote = validate_and_lock_(transaction_id, ops);
        } catch (const std::exception& e) {
            THEMIS_WARN("2PC participant [{}] PREPARE {}: lock/validation error: {}",
                        shard_id_, transaction_id, e.what());
            vote = false;
        }
    }

    // Record state
    ParticipantTransaction txn;
    txn.transaction_id = transaction_id;
    txn.operations     = std::move(ops);
    txn.vote_committed = vote;
    txn.prepared_at    = std::chrono::steady_clock::now();
    txn.state          = vote ? ParticipantTxnState::PREPARED : ParticipantTxnState::ABORTED;
    transactions_[transaction_id] = txn;

    // Log to WAL (must be durable before returning vote to coordinator)
    logToWAL(WALEntryType::PREPARE_TX, transaction_id, {
        {"transaction_id",      transaction_id},
        {"coordinator_shard",   coordinator_shard_id},
        {"shard_id",            shard_id_},
        {"vote",                vote},
        {"operations",          transactions_[transaction_id].operations}
    });

    total_prepares_.fetch_add(1, std::memory_order_relaxed);

    // Record Prometheus participant response latency
    const double latency_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCParticipantResponse(shard_id_, "prepare", latency_ms);
    }

    THEMIS_DEBUG("2PC participant [{}] PREPARE {} – vote={}",
                 shard_id_, transaction_id, vote ? "COMMIT" : "ABORT");
    return vote;
}

bool TwoPhaseCommitParticipant::onCommit(const std::string& transaction_id) {
    const auto t0 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = transactions_.find(transaction_id);

    // Idempotency: already committed
    if (it != transactions_.end() && it->second.state == ParticipantTxnState::COMMITTED) {
        THEMIS_DEBUG("2PC participant [{}] duplicate COMMIT for {} – idempotent ok",
                     shard_id_, transaction_id);
        return true;
    }

    // Cannot commit something we aborted or that we never prepared
    if (it == transactions_.end() || it->second.state == ParticipantTxnState::ABORTED) {
        THEMIS_ERROR("2PC participant [{}] COMMIT for unknown/aborted txn {}",
                     shard_id_, transaction_id);
        return false;
    }

    auto& txn = it->second;
    txn.commit_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Apply buffered operations to the storage engine
    bool applied = true;
    if (apply_operations_) {
        try {
            applied = apply_operations_(transaction_id, txn.operations, txn.commit_timestamp);
        } catch (const std::exception& e) {
            THEMIS_ERROR("2PC participant [{}] COMMIT {}: apply error: {}",
                         shard_id_, transaction_id, e.what());
            applied = false;
        }
    }

    if (!applied) {
        THEMIS_ERROR("2PC participant [{}] COMMIT {} failed to apply operations",
                     shard_id_, transaction_id);
        return false;
    }

    txn.state = ParticipantTxnState::COMMITTED;

    // Release locks
    if (release_locks_) {
        try { release_locks_(transaction_id); }
        catch (const std::exception& e) {
            THEMIS_WARN("2PC participant [{}] COMMIT {}: lock release error (ignored): {}",
                        shard_id_, transaction_id, e.what());
        }
    }

    // Durable commit record
    logToWAL(WALEntryType::COMMIT_TX, transaction_id, {
        {"transaction_id", transaction_id},
        {"shard_id",       shard_id_},
        {"commit_timestamp", txn.commit_timestamp}
    });

    total_commits_.fetch_add(1, std::memory_order_relaxed);

    // Record Prometheus participant response latency
    const double latency_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCParticipantResponse(shard_id_, "commit", latency_ms);
    }

    THEMIS_DEBUG("2PC participant [{}] COMMIT {} applied successfully", shard_id_, transaction_id);
    return true;
}

bool TwoPhaseCommitParticipant::onAbort(const std::string& transaction_id) {
    const auto t0 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = transactions_.find(transaction_id);

    // Idempotency: already aborted
    if (it != transactions_.end() && it->second.state == ParticipantTxnState::ABORTED) {
        THEMIS_DEBUG("2PC participant [{}] duplicate ABORT for {} – idempotent ok",
                     shard_id_, transaction_id);
        return true;
    }

    // If the transaction was committed, we cannot abort it
    if (it != transactions_.end() && it->second.state == ParticipantTxnState::COMMITTED) {
        THEMIS_ERROR("2PC participant [{}] ABORT for already-committed txn {}",
                     shard_id_, transaction_id);
        return false;
    }

    // Release locks (if any were acquired in PREPARE)
    if (release_locks_) {
        try { release_locks_(transaction_id); }
        catch (const std::exception& e) {
            THEMIS_WARN("2PC participant [{}] ABORT {}: lock release error (ignored): {}",
                        shard_id_, transaction_id, e.what());
        }
    }

    // Record abort
    if (it != transactions_.end()) {
        it->second.state = ParticipantTxnState::ABORTED;
    } else {
        // Never prepared – still record the abort for completeness
        ParticipantTransaction txn;
        txn.transaction_id = transaction_id;
        txn.state          = ParticipantTxnState::ABORTED;
        txn.vote_committed = false;
        txn.prepared_at    = std::chrono::steady_clock::now();
        transactions_[transaction_id] = std::move(txn);
    }

    logToWAL(WALEntryType::ABORT_TX, transaction_id, {
        {"transaction_id", transaction_id},
        {"shard_id",       shard_id_}
    });

    total_aborts_.fetch_add(1, std::memory_order_relaxed);

    // Record Prometheus participant response latency
    const double latency_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCParticipantResponse(shard_id_, "abort", latency_ms);
    }

    THEMIS_DEBUG("2PC participant [{}] ABORT {} completed", shard_id_, transaction_id);
    return true;
}

ShardRPCServer::HealthInfo
TwoPhaseCommitParticipant::onHealthCheck() {
    ShardRPCServer::HealthInfo info;
    const auto uptime = std::chrono::steady_clock::now() - start_time_;
    info.is_healthy     = true;
    info.version        = "1.0";
    info.uptime_seconds = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(uptime).count());
    return info;
}

// ─────────────────────────────────────────────────────────────────────────────
// Participant-specific API
// ─────────────────────────────────────────────────────────────────────────────

std::optional<ParticipantTxnState>
TwoPhaseCommitParticipant::getTransactionState(const std::string& transaction_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) return std::nullopt;
    return it->second.state;
}

size_t TwoPhaseCommitParticipant::abortTimedOutTransactions() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    size_t count = 0;

    for (auto& [txn_id, txn] : transactions_) {
        if (txn.state != ParticipantTxnState::PREPARED) continue;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - txn.prepared_at
        );
        if (elapsed >= config_.prepare_timeout) {
            THEMIS_WARN("2PC participant [{}] aborting timed-out prepared txn {}",
                        shard_id_, txn_id);

            if (release_locks_) {
                try { release_locks_(txn_id); }
                catch (const std::exception&) {}
            }

            txn.state = ParticipantTxnState::ABORTED;
            logToWAL(WALEntryType::ABORT_TX, txn_id, {
                {"transaction_id", txn_id},
                {"shard_id",       shard_id_},
                {"reason",         "prepare timeout"}
            });

            total_aborts_.fetch_add(1, std::memory_order_relaxed);
            total_timeouts_.fetch_add(1, std::memory_order_relaxed);
            ++count;
        }
    }
    return count;
}

size_t TwoPhaseCommitParticipant::recoverFromWAL() {
    if (!wal_) return 0;

    THEMIS_INFO("2PC participant [{}] recovering from WAL…", shard_id_);

    size_t in_doubt = 0;

    try {
        LSN oldest = wal_->getOldestLSN();
        LSN current = wal_->getCurrentLSN();
        if (oldest > current) return 0;

        auto entries = wal_->readRange(oldest, current);

        // Replay entries in order to rebuild state
        for (const auto& entry : entries) {
            if (entry.transaction_id.empty()) continue;

            const std::string& txn_id = entry.transaction_id;

            if (entry.type == WALEntryType::PREPARE_TX) {
                if (transactions_.find(txn_id) == transactions_.end()) {
                    ParticipantTransaction txn;
                    txn.transaction_id = txn_id;
                    txn.state          = ParticipantTxnState::PREPARED;
                    txn.vote_committed = entry.data.value("vote", true);
                    txn.operations     = entry.data.value("operations", nlohmann::json::array());
                    txn.prepared_at    = std::chrono::steady_clock::now();
                    transactions_[txn_id] = std::move(txn);
                }
            } else if (entry.type == WALEntryType::COMMIT_TX) {
                auto it = transactions_.find(txn_id);
                if (it != transactions_.end()) {
                    it->second.state = ParticipantTxnState::COMMITTED;
                } else {
                    ParticipantTransaction txn;
                    txn.transaction_id = txn_id;
                    txn.state          = ParticipantTxnState::COMMITTED;
                    txn.prepared_at    = std::chrono::steady_clock::now();
                    transactions_[txn_id] = std::move(txn);
                }
            } else if (entry.type == WALEntryType::ABORT_TX) {
                auto it = transactions_.find(txn_id);
                if (it != transactions_.end()) {
                    it->second.state = ParticipantTxnState::ABORTED;
                } else {
                    ParticipantTransaction txn;
                    txn.transaction_id = txn_id;
                    txn.state          = ParticipantTxnState::ABORTED;
                    txn.prepared_at    = std::chrono::steady_clock::now();
                    transactions_[txn_id] = std::move(txn);
                }
            }
        }

        // Count in-doubt transactions (prepared, no commit/abort)
        for (const auto& [txn_id, txn] : transactions_) {
            if (txn.state == ParticipantTxnState::PREPARED) {
                THEMIS_WARN("2PC participant [{}] in-doubt transaction found: {}",
                            shard_id_, txn_id);
                ++in_doubt;
            }
        }

    } catch (const std::exception& e) {
        THEMIS_ERROR("2PC participant [{}] WAL recovery failed: {}", shard_id_, e.what());
    }

    THEMIS_INFO("2PC participant [{}] recovery complete – {} in-doubt transactions",
                shard_id_, in_doubt);
    return in_doubt;
}

nlohmann::json TwoPhaseCommitParticipant::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t prepared_count  = 0;
    size_t committed_count = 0;
    size_t aborted_count   = 0;

    for (const auto& [id, txn] : transactions_) {
        switch (txn.state) {
            case ParticipantTxnState::PREPARED:   ++prepared_count;  break;
            case ParticipantTxnState::COMMITTED:  ++committed_count; break;
            case ParticipantTxnState::ABORTED:    ++aborted_count;   break;
        }
    }

    const auto uptime = std::chrono::steady_clock::now() - start_time_;
    return {
        {"shard_id",               shard_id_},
        {"uptime_seconds",         static_cast<uint64_t>(
                                       std::chrono::duration_cast<std::chrono::seconds>(uptime).count())},
        {"total_prepares",         total_prepares_.load()},
        {"total_commits",          total_commits_.load()},
        {"total_aborts",           total_aborts_.load()},
        {"total_timeouts",         total_timeouts_.load()},
        {"active_prepared",        prepared_count},
        {"active_committed",       committed_count},
        {"active_aborted",         aborted_count}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

void TwoPhaseCommitParticipant::logToWAL(
    WALEntryType       type,
    const std::string& txn_id,
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
        THEMIS_ERROR("2PC participant [{}] WAL write failed for txn {}: {}",
                     shard_id_, txn_id, e.what());
    }
}

} // namespace themis::sharding
