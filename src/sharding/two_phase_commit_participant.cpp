/**
 * @file two_phase_commit_participant.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Two-Phase Commit (2PC) Participant – shard-side handler

#include "sharding/two_phase_commit_participant.h"
#include "sharding/metrics_registry.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/wal_logging_helper.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <vector>

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
        wal_cfg.recovery_callback = [this](const WALEntry& entry) {
            applyRecoveredEntry(entry, false);
            return true;
        };
        recovery_complete_ = false;
        wal_ = std::make_unique<WALManager>(wal_cfg);
        const auto retained_entries = wal_->readRange(
            wal_->getOldestLSN(), wal_->getCurrentLSN());
        for (const auto& entry : retained_entries) {
            applyRecoveredEntry(entry, true);
        }
        recovery_complete_ = std::none_of(
            transactions_.begin(), transactions_.end(), [](const auto& item) {
                return item.second.state == ParticipantTxnState::PREPARED;
            });
        THEMIS_INFO("2PC participant [{}] WAL initialised at {}", shard_id_, config_.wal_directory);
    }
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── ShardRPCServer::RequestHandler interface ─────────────────────────────────────────────────────────────────────────────
 * @param[in] transaction_id Identifier of the transaction.
 * @param[in] coordinator_shard_id Identifier of the coordinator shard.
 * @param[in] transaction_data Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: std::chrono::steady_clock::now(), lock(), find(), end(), THEMIS_DEBUG(), nlohmann::json::parse(), value(), nlohmann::json::array().
 */

bool TwoPhaseCommitParticipant::onPrepare(
    const std::string& transaction_id,
    const std::string& coordinator_shard_id,
    const std::string& transaction_data
) {
    const auto t0 = std::chrono::steady_clock::now();
    nlohmann::json parsed;
    nlohmann::json ops;
    EpochToken token;
    bool payload_valid = true;
    try {
        parsed = nlohmann::json::parse(transaction_data);
        ops = parsed.value("operations", nlohmann::json::array());
        if (!ops.is_array() || !parsed.contains("fencing_token") ||
            !parsed["fencing_token"].is_object()) {
            payload_valid = false;
        } else {
            const auto& token_json = parsed["fencing_token"];
            token.epoch = token_json.at("epoch").get<EpochNumber>();
            token.issuer = token_json.at("issuer").get<NodeId>();
            token.shard_id = token_json.at("shard_id").get<std::string>();
            const auto issued_at_ms = token_json.at("issued_at_ms").get<int64_t>();
            token.issued_at = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{issued_at_ms}};
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("2PC participant [{}] PREPARE {}: failed to parse payload – aborting: {}",
                    shard_id_, transaction_id, e.what());
        payload_valid = false;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (durability_failed_ || !recovery_complete_) {
        return false;
    }

    auto it = transactions_.find(transaction_id);
    if (it != transactions_.end()) {
        if (!payload_valid || token.issuer != coordinator_shard_id) {
            return false;
        }
        THEMIS_DEBUG("2PC participant [{}] duplicate PREPARE for {} – returning stored vote {}",
                     shard_id_, transaction_id, it->second.vote_committed);
        return it->second.vote_committed;
    }

    lock.unlock();
    FencingResult fencing_result = FencingResult::INVALID_TOKEN;
    if (payload_valid && config_.fencing_manager &&
        config_.fencing_manager->config().shard_id == shard_id_ &&
        token.issuer == coordinator_shard_id) {
        fencing_result = config_.fencing_manager->checkToken(token, coordinator_shard_id);
    }
    lock.lock();

    if (durability_failed_) {
        return false;
    }

    it = transactions_.find(transaction_id);
    if (it != transactions_.end()) {
        if (!payload_valid || token.issuer != coordinator_shard_id) {
            return false;
        }
        return it->second.vote_committed;
    }

    if (fencing_result != FencingResult::ALLOWED) {
        ParticipantTransaction txn;
        txn.transaction_id = transaction_id;
        txn.state = ParticipantTxnState::ABORTED;
        txn.vote_committed = false;
        txn.prepared_at = std::chrono::steady_clock::now();
        transactions_[transaction_id] = std::move(txn);
        if (wal_ && !logToWAL(WALEntryType::PREPARE_TX, transaction_id, {
            {"transaction_id", transaction_id},
            {"coordinator_shard", coordinator_shard_id},
            {"shard_id", shard_id_},
            {"vote", false},
            {"fencing_result", toString(fencing_result)}
        }, true)) {
            durability_failed_ = true;
        }
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
    txn.fencing_token  = token;
    txn.vote_committed = vote;
    txn.prepared_at    = std::chrono::steady_clock::now();
    txn.state          = vote ? ParticipantTxnState::PREPARED : ParticipantTxnState::ABORTED;
    transactions_[transaction_id] = txn;

    // Log to WAL (must be durable before returning vote to coordinator)
    const bool prepare_wal_written = logToWAL(WALEntryType::PREPARE_TX, transaction_id, {
        {"transaction_id",      transaction_id},
        {"coordinator_shard",   coordinator_shard_id},
        {"shard_id",            shard_id_},
        {"vote",                vote},
        {"operations",          transactions_[transaction_id].operations},
        {"fencing_token", {
            {"epoch", token.epoch},
            {"issuer", token.issuer},
            {"shard_id", token.shard_id},
            {"issued_at_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
                token.issued_at.time_since_epoch()).count()}
        }}
    }, true);
    if (wal_ && !prepare_wal_written) {
        durability_failed_ = true;
        if (release_locks_) {
            try { release_locks_(transaction_id); }
            catch (const std::exception& e) {
                THEMIS_WARN("2PC participant [{}] PREPARE {}: lock release failed after WAL error: {}",
                            shard_id_, transaction_id, e.what());
            }
        }
        auto& failed_txn = transactions_[transaction_id];
        failed_txn.vote_committed = false;
        failed_txn.state = ParticipantTxnState::ABORTED;
        vote = false;
    }

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

/**
 * @brief On Commit.
 * @param[in] transaction_id Identifier of the transaction.
 * @return True when the operation succeeds.
 * @details Calls: std::chrono::steady_clock::now(), lock(), find(), end(), THEMIS_DEBUG(), THEMIS_ERROR(), std::chrono::system_clock::now(), time_since_epoch().
 */
bool TwoPhaseCommitParticipant::onCommit(const std::string& transaction_id) {
    const auto t0 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    if (durability_failed_ || !recovery_complete_) {
        return false;
    }

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
    if (txn.commit_timestamp == 0) {
        txn.commit_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    // Apply buffered operations to the storage engine
    bool applied = true;
    FencingResult fencing_result = FencingResult::INVALID_TOKEN;
    if (config_.fencing_manager &&
        config_.fencing_manager->config().shard_id == shard_id_) {
        try {
            fencing_result = config_.fencing_manager->applyWithToken(
                txn.fencing_token, txn.fencing_token.issuer, [&] {
                    if (apply_operations_) {
                        applied = apply_operations_(transaction_id, txn.operations,
                                                    txn.commit_timestamp);
                    }
                });
        } catch (const std::exception& e) {
            THEMIS_ERROR("2PC participant [{}] COMMIT {}: apply error: {}",
                         shard_id_, transaction_id, e.what());
            applied = false;
        }
    }

    if (fencing_result != FencingResult::ALLOWED || !applied) {
        THEMIS_ERROR("2PC participant [{}] COMMIT {} rejected (fencing={}, applied={})",
                     shard_id_, transaction_id, toString(fencing_result), applied);
        return false;
    }

    // Durable commit record
    if (wal_ && !logToWAL(WALEntryType::COMMIT_TX, transaction_id, {
        {"transaction_id", transaction_id},
        {"shard_id",       shard_id_},
        {"commit_timestamp", txn.commit_timestamp}
    }, true)) {
        THEMIS_ERROR("2PC participant [{}] COMMIT {} could not be durably recorded",
                     shard_id_, transaction_id);
        durability_failed_ = true;
        return false;
    }

    txn.state = ParticipantTxnState::COMMITTED;

    // Release locks only after the commit record is durable.
    if (release_locks_) {
        try { release_locks_(transaction_id); }
        catch (const std::exception& e) {
            THEMIS_WARN("2PC participant [{}] COMMIT {}: lock release error (ignored): {}",
                        shard_id_, transaction_id, e.what());
        }
    }

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

/**
 * @brief On Abort.
 * @param[in] transaction_id Identifier of the transaction.
 * @return True when the operation succeeds.
 * @details Calls: std::chrono::steady_clock::now(), lock(), find(), end(), THEMIS_DEBUG(), THEMIS_ERROR(), release_locks_(), THEMIS_WARN().
 */
bool TwoPhaseCommitParticipant::onAbort(const std::string& transaction_id) {
    const auto t0 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    if (durability_failed_ || !recovery_complete_) {
        return false;
    }

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

    if (wal_ && !logToWAL(WALEntryType::ABORT_TX, transaction_id, {
            {"transaction_id", transaction_id},
            {"shard_id",       shard_id_}
        }, true)) {
        durability_failed_ = true;
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
    std::lock_guard<std::mutex> lock(mutex_);
    ShardRPCServer::HealthInfo info;
    const auto uptime = std::chrono::steady_clock::now() - start_time_;
    info.is_healthy     = !durability_failed_ && recovery_complete_;
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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
      return std::nullopt;
    }
    return it->second.state;
}

/**
 * @brief Abort Timed Out Transactions.
 * @return Return value.
 * @details Calls: lock(), std::chrono::steady_clock::now(), THEMIS_WARN(), release_locks_(), logToWAL(), fetch_add().
 */
size_t TwoPhaseCommitParticipant::abortTimedOutTransactions() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!recovery_complete_ || durability_failed_) {
        return 0;
    }
    auto now = std::chrono::steady_clock::now();
    size_t count = 0;

    for (auto& [txn_id, txn] : transactions_) {
        if (txn.state != ParticipantTxnState::PREPARED) {
          continue;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - txn.prepared_at
        );
        if (elapsed >= config_.prepare_timeout) {
            THEMIS_WARN("2PC participant [{}] aborting timed-out prepared txn {}",
                        shard_id_, txn_id);

            if (wal_ && !logToWAL(WALEntryType::ABORT_TX, txn_id, {
                    {"transaction_id", txn_id},
                    {"shard_id",       shard_id_},
                    {"reason",         "prepare timeout"}
                }, true)) {
                durability_failed_ = true;
                continue;
            }

            if (release_locks_) {
                try { release_locks_(txn_id); }
                catch (...) {}
            }

            txn.state = ParticipantTxnState::ABORTED;

            total_aborts_.fetch_add(1, std::memory_order_relaxed);
            total_timeouts_.fetch_add(1, std::memory_order_relaxed);
            ++count;
        }
    }
    return count;
}

/**
 * @brief Recover From WAL.
 * @return Return value.
 * @details Restores prepared locks after the WAL manager has streamed the full
 *          durable log into the transaction state map.
 */
size_t TwoPhaseCommitParticipant::recoverFromWAL() {
    if (!wal_) {
      return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_);

    if (recovery_complete_) {
        return static_cast<size_t>(std::count_if(
            transactions_.begin(), transactions_.end(), [](const auto& item) {
                return item.second.state == ParticipantTxnState::PREPARED;
            }));
    }

    THEMIS_INFO("2PC participant [{}] recovering from WAL…", shard_id_);

    size_t in_doubt = 0;
    std::vector<std::string> restored_transactions;

    try {
        for (const auto& [txn_id, txn] : transactions_) {
            if (txn.state == ParticipantTxnState::PREPARED) {
                restored_transactions.push_back(txn_id);
            }
        }

        for (const auto& txn_id : restored_transactions) {
            const auto it = transactions_.find(txn_id);
            if (it == transactions_.end() ||
                it->second.state != ParticipantTxnState::PREPARED ||
                !validate_and_lock_) {
                continue;
            }
            if (!validate_and_lock_(txn_id, it->second.operations)) {
                throw std::runtime_error(
                    "could not restore participant locks for prepared transaction " + txn_id);
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

        recovery_complete_ = true;
    } catch (const std::exception& e) {
        durability_failed_ = true;
        THEMIS_ERROR("2PC participant [{}] WAL recovery failed: {}", shard_id_, e.what());
        throw;
    }

    THEMIS_INFO("2PC participant [{}] recovery complete – {} in-doubt transactions",
                shard_id_, in_doubt);
    return in_doubt;
}

void TwoPhaseCommitParticipant::applyRecoveredEntry(
    const WALEntry& entry,
    bool retain_terminal
) {
    if (entry.transaction_id.empty()) {
        return;
    }

    const auto& txn_id = entry.transaction_id;
    if (entry.type == WALEntryType::PREPARE_TX) {
        if (transactions_.find(txn_id) != transactions_.end()) {
            return;
        }

        ParticipantTransaction txn;
        txn.transaction_id = txn_id;
        txn.vote_committed = entry.data.value("vote", true);
        if (!txn.vote_committed && !retain_terminal) {
            transactions_.erase(txn_id);
            return;
        }
        txn.state = txn.vote_committed
            ? ParticipantTxnState::PREPARED
            : ParticipantTxnState::ABORTED;
        txn.operations = entry.data.value("operations", nlohmann::json::array());
        if (entry.data.contains("fencing_token") &&
            entry.data["fencing_token"].is_object()) {
            const auto& token_json = entry.data["fencing_token"];
            txn.fencing_token.epoch = token_json.value("epoch", EpochNumber{0});
            txn.fencing_token.issuer = token_json.value("issuer", NodeId{});
            txn.fencing_token.shard_id = token_json.value("shard_id", std::string{});
            const auto issued_at_ms = token_json.value("issued_at_ms", int64_t{0});
            txn.fencing_token.issued_at = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{issued_at_ms}};
        }
        txn.prepared_at = std::chrono::steady_clock::now();
        transactions_[txn_id] = std::move(txn);
        return;
    }

    if (entry.type != WALEntryType::COMMIT_TX &&
        entry.type != WALEntryType::ABORT_TX) {
        return;
    }

    if (!retain_terminal) {
        transactions_.erase(txn_id);
        return;
    }

    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        ParticipantTransaction txn;
        txn.transaction_id = txn_id;
        txn.prepared_at = std::chrono::steady_clock::now();
        it = transactions_.emplace(txn_id, std::move(txn)).first;
    }

    if (entry.type == WALEntryType::COMMIT_TX) {
        it->second.state = ParticipantTxnState::COMMITTED;
        it->second.commit_timestamp =
            entry.data.value("commit_timestamp", int64_t{0});
    } else {
        it->second.state = ParticipantTxnState::ABORTED;
    }
}

nlohmann::json TwoPhaseCommitParticipant::getStatistics() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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
    std::string wal_recovery_status = "disabled";
    if (wal_) {
        switch (wal_->getRecoveryResult().status) {
            case themis::WALStorage::RecoveryStatus::Complete:
                wal_recovery_status = "complete";
                break;
            case themis::WALStorage::RecoveryStatus::TruncatedTail:
                wal_recovery_status = "truncated_tail";
                break;
            case themis::WALStorage::RecoveryStatus::Corrupted:
                wal_recovery_status = "corrupted";
                break;
            case themis::WALStorage::RecoveryStatus::TimedOut:
                wal_recovery_status = "timed_out";
                break;
            case themis::WALStorage::RecoveryStatus::Failed:
                wal_recovery_status = "failed";
                break;
        }
    }

    return {
        {"shard_id",               shard_id_},
        {"uptime_seconds",         static_cast<uint64_t>(
                                       std::chrono::duration_cast<std::chrono::seconds>(uptime).count())},
        {"recovery_complete",      recovery_complete_},
        {"wal_recovery_status",    wal_recovery_status},
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

/**
 * @brief Log To WAL.
 * @param[in] type Input parameter.
 * @param[in] txn_id Identifier of the txn.
 * @param[in] data Input parameter.
 * @details Calls: WALLoggingHelper::appendEntry(), get().
 */
bool TwoPhaseCommitParticipant::logToWAL(
    WALEntryType       type,
    const std::string& txn_id,
    const nlohmann::json& data,
    bool force_sync
) {
    return WALLoggingHelper::appendEntryWithResult(
        wal_.get(), type, txn_id, data,
        config_.sync_wal_writes || force_sync,
        "participant", shard_id_
    ).has_value();
}

} // namespace themis::sharding
