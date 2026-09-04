/**
 * @file global_transaction_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=22, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Global Transaction Manager – multi-region ACID coordinator

#include "transaction/global_transaction_manager.h"
#include "utils/logger.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

GlobalTransactionManager::GlobalTransactionManager(
    const std::string&                              coordinator_id,
    std::shared_ptr<themis::sharding::TrueTime>     truetime
)
    : GlobalTransactionManager(coordinator_id, std::move(truetime), Config{})
{
}

GlobalTransactionManager::GlobalTransactionManager(
    const std::string&                              coordinator_id,
    std::shared_ptr<themis::sharding::TrueTime>     truetime,
    const Config&                                   config
)
    : coordinator_id_(coordinator_id)
    , truetime_(std::move(truetime))
    , config_(config)
{
    if (!truetime_) {
        throw std::invalid_argument("GlobalTransactionManager: truetime must not be null");
    }

    if (!config_.wal_directory.empty()) {
        themis::sharding::WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = config_.wal_directory;
        wal_cfg.sync_on_write = config_.sync_wal_writes;
        wal_ = std::make_unique<themis::sharding::WALManager>(wal_cfg);
        THEMIS_INFO("GlobalTransactionManager [{}] WAL initialised at {}",
                    coordinator_id_, config_.wal_directory);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Region management
// ─────────────────────────────────────────────────────────────────────────────

void GlobalTransactionManager::registerRegion(
    const std::string&        region_id,
    IGlobalRegionParticipant* participant
) {
    if (!participant) {
        throw std::invalid_argument("participant must not be null");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    regions_[region_id] = participant;
    THEMIS_DEBUG("GlobalTransactionManager [{}] registered region {}",
                 coordinator_id_, region_id);
}

bool GlobalTransactionManager::unregisterRegion(const std::string& region_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return regions_.erase(region_id) > 0;
}

size_t GlobalTransactionManager::regionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return regions_.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Transaction lifecycle
// ─────────────────────────────────────────────────────────────────────────────

std::string GlobalTransactionManager::beginTransaction(
    const std::vector<std::string>& region_ids
) {
    if (region_ids.empty()) {
        throw std::invalid_argument("region_ids must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Validate all regions are registered
    for (const auto& rid : region_ids) {
        if (regions_.find(rid) == regions_.end()) {
            throw std::invalid_argument(
                "GlobalTransactionManager: unknown region: " + rid
            );
        }
    }

    const std::string txn_id = generateTransactionId();

    GlobalTxnRecord rec;
    rec.transaction_id = txn_id;
    rec.state          = GlobalTxnState::ACTIVE;
    rec.started_at     = std::chrono::steady_clock::now();

    for (const auto& rid : region_ids) {
        rec.region_ops[rid]     = nlohmann::json::array();
        RegionTxnRecord& rrec   = rec.region_records[rid];
        rrec.region_id          = rid;
    }

    transactions_[txn_id] = std::move(rec);
    total_transactions_.fetch_add(1, std::memory_order_relaxed);

    logToWAL(themis::sharding::WALEntryType::BEGIN_TX, txn_id, {
        {"transaction_id", txn_id},
        {"coordinator_id", coordinator_id_},
        {"regions", region_ids}
    });

    THEMIS_DEBUG("GlobalTransactionManager [{}] began txn {} across {} region(s)",
                 coordinator_id_, txn_id, region_ids.size());
    return txn_id;
}

bool GlobalTransactionManager::addOperation(
    const std::string&    txn_id,
    const std::string&    region_id,
    const nlohmann::json& op
) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        THEMIS_WARN("GlobalTransactionManager [{}] addOperation: unknown txn {}",
                    coordinator_id_, txn_id);
        return false;
    }

    auto& rec = it->second;
    if (rec.state != GlobalTxnState::ACTIVE) {
        THEMIS_WARN("GlobalTransactionManager [{}] addOperation: txn {} not ACTIVE",
                    coordinator_id_, txn_id);
        return false;
    }

    auto ops_it = rec.region_ops.find(region_id);
    if (ops_it == rec.region_ops.end()) {
        THEMIS_WARN("GlobalTransactionManager [{}] addOperation: region {} not in txn {}",
                    coordinator_id_, region_id, txn_id);
        return false;
    }

    ops_it->second.push_back(op);
    return true;
}

GlobalTxnOutcome GlobalTransactionManager::commit(const std::string& txn_id) {
    GlobalTxnOutcome outcome;
    outcome.transaction_id = txn_id;

    const auto t0 = std::chrono::steady_clock::now();

    bool all_prepared = false;
    int64_t commit_ts = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = transactions_.find(txn_id);
        if (it == transactions_.end()) {
            outcome.result = GlobalTxnResult::ERROR;
            outcome.reason = "transaction not found";
            total_errors_.fetch_add(1, std::memory_order_relaxed);
            return outcome;
        }

        auto& rec = it->second;

        if (rec.state == GlobalTxnState::COMPLETED) {
            // Idempotent: return the original outcome based on whether a commit
            // timestamp was recorded (only set on the COMMIT path).
            if (rec.commit_timestamp_ns != 0) {
                outcome.result              = GlobalTxnResult::COMMITTED;
                outcome.commit_timestamp_ns = rec.commit_timestamp_ns;
            } else {
                outcome.result = GlobalTxnResult::ABORTED;
                outcome.reason = "transaction was previously aborted";
            }
            return outcome;
        }
        if (rec.state != GlobalTxnState::ACTIVE) {
            outcome.result = GlobalTxnResult::ERROR;
            outcome.reason = "transaction already in progress or failed";
            total_errors_.fetch_add(1, std::memory_order_relaxed);
            return outcome;
        }

        // ── Phase 1: PREPARE ────────────────────────────────────────────────
        rec.state    = GlobalTxnState::PREPARING;
        all_prepared = runPhase1(rec);

        rec.state = all_prepared
            ? GlobalTxnState::COMMIT_DECIDED
            : GlobalTxnState::ABORT_DECIDED;

        if (all_prepared) {
            // Assign TrueTime timestamp while holding the lock (snapshot only)
            const auto interval = truetime_->now();
            commit_ts                 = static_cast<int64_t>(interval.latest.count());
            rec.commit_timestamp_ns   = commit_ts;
        }
    }

    // ── TrueTime commit-wait (outside lock to avoid blocking other ops) ──────
    if (all_prepared) {
        // External consistency: wait until commit_ts is definitely in the past
        truetime_->waitUntil(std::chrono::nanoseconds(commit_ts));
    }

    // Log the Phase 2 decision outside the lock (WAL I/O may block)
    logToWAL(
        all_prepared ? themis::sharding::WALEntryType::COMMIT_TX
                     : themis::sharding::WALEntryType::ABORT_TX,
        txn_id,
        {
            {"transaction_id", txn_id},
            {"coordinator_id", coordinator_id_},
            {"decision",       all_prepared ? "commit" : "abort"},
            {"commit_timestamp_ns", commit_ts}
        }
    );

    // ── Phase 2: COMMIT or ABORT ─────────────────────────────────────────
    // Wave 4C T3: Snapshot the record under the lock, then release before
    // Phase-2 delivery. This prevents holding the global mutex while blocking
    // on potentially slow region commit/abort RPCs.
    GlobalTxnRecord rec_snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rec_snapshot = transactions_.at(txn_id);  // copy snapshot
    }
    // Deliver Phase-2 outside the global lock (no mutex held during RPC calls).
    runPhase2(rec_snapshot, all_prepared);
    // Re-acquire to persist the COMPLETED state.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rec = transactions_.at(txn_id);
        // Merge back acked flags from the snapshot (runPhase2 updates the copy).
        for (auto& [region_id, snap_rrec] : rec_snapshot.region_records) {
            if (auto it = rec.region_records.find(region_id); it != rec.region_records.end()) {
                it->second.phase2_acked = snap_rrec.phase2_acked;
            }
        }
        rec.state = GlobalTxnState::COMPLETED;
    }

    logToWAL(
        all_prepared ? themis::sharding::WALEntryType::COMMIT_TX
                     : themis::sharding::WALEntryType::ABORT_TX,
        txn_id,
        {
            {"transaction_id", txn_id},
            {"coordinator_id", coordinator_id_},
            {"phase",          "complete"}
        }
    );

    const double total_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    if (all_prepared) {
        total_commits_.fetch_add(1, std::memory_order_relaxed);
        outcome.result              = GlobalTxnResult::COMMITTED;
        outcome.commit_timestamp_ns = commit_ts;
        THEMIS_INFO("GlobalTransactionManager [{}] txn {} COMMITTED (ts={}ns, elapsed={:.1f}ms)",
                    coordinator_id_, txn_id, commit_ts, total_ms);
    } else {
        total_aborts_.fetch_add(1, std::memory_order_relaxed);
        outcome.result = GlobalTxnResult::ABORTED;
        outcome.reason = "one or more regions voted ABORT";
        THEMIS_INFO("GlobalTransactionManager [{}] txn {} ABORTED (elapsed={:.1f}ms)",
                    coordinator_id_, txn_id, total_ms);
    }

    return outcome;
}

bool GlobalTransactionManager::abort(const std::string& txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        THEMIS_WARN("GlobalTransactionManager [{}] abort: unknown txn {}",
                    coordinator_id_, txn_id);
        return false;
    }

    auto& rec = it->second;

    if (rec.state == GlobalTxnState::COMPLETED) {
        return true; // already done
    }

    // Broadcast ABORT to all participants that may have PREPAREd
    if (rec.state == GlobalTxnState::PREPARING ||
        rec.state == GlobalTxnState::COMMIT_DECIDED ||
        rec.state == GlobalTxnState::ABORT_DECIDED)
    {
        runPhase2(rec, /*do_commit=*/false);
    }

    rec.state = GlobalTxnState::COMPLETED;
    total_aborts_.fetch_add(1, std::memory_order_relaxed);

    logToWAL(themis::sharding::WALEntryType::ABORT_TX, txn_id, {
        {"transaction_id", txn_id},
        {"coordinator_id", coordinator_id_},
        {"phase",          "complete"}
    });

    THEMIS_INFO("GlobalTransactionManager [{}] txn {} explicitly ABORTED",
                coordinator_id_, txn_id);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Recovery
// ─────────────────────────────────────────────────────────────────────────────

size_t GlobalTransactionManager::recoverInDoubtTransactions() {
    if (!wal_) {
      return 0;
    }

    THEMIS_INFO("GlobalTransactionManager [{}] recovering from WAL…", coordinator_id_);

    size_t resolved = 0;

    try {
        auto entries = wal_->readRange(wal_->getOldestLSN());

        std::map<std::string, GlobalTxnRecord> recovered;
        std::map<std::string, bool>            decisions;

        for (const auto& entry : entries) {
            const std::string& tid = entry.transaction_id;
            if (tid.empty()) {
              continue;
            }

            if (entry.type == themis::sharding::WALEntryType::BEGIN_TX) {
                GlobalTxnRecord& r  = recovered[tid];
                r.transaction_id    = tid;
                r.state             = GlobalTxnState::ACTIVE;
                r.started_at        = std::chrono::steady_clock::now();

                // Restore region list
                if (entry.data.contains("regions")) {
                    for (const auto& rid : entry.data["regions"]) {
                        const std::string s = rid.get<std::string>();
                        r.region_ops[s]     = nlohmann::json::array();
                        RegionTxnRecord rr;
                        rr.region_id        = s;
                        r.region_records[s] = rr;
                    }
                }
            } else if (entry.type == themis::sharding::WALEntryType::COMMIT_TX) {
                const auto phase = entry.data.value("phase", "");
                if (phase == "complete") {
                    if (recovered.count(tid))
                        recovered[tid].state = GlobalTxnState::COMPLETED;
                } else {
                    const auto decision = entry.data.value("decision", "");
                    if (decision == "commit") {
                        decisions[tid] = true;
                        recovered[tid].commit_timestamp_ns =
                            entry.data.value("commit_timestamp_ns", int64_t{0});
                        if (recovered.count(tid))
                            recovered[tid].state = GlobalTxnState::COMMIT_DECIDED;
                    }
                }
            } else if (entry.type == themis::sharding::WALEntryType::ABORT_TX) {
                const auto phase = entry.data.value("phase", "");
                if (phase == "complete") {
                    if (recovered.count(tid))
                        recovered[tid].state = GlobalTxnState::COMPLETED;
                } else {
                    decisions[tid] = false;
                    if (recovered.count(tid))
                        recovered[tid].state = GlobalTxnState::ABORT_DECIDED;
                }
            }
        }

        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& [tid, rec] : recovered) {
            if (rec.state == GlobalTxnState::COMPLETED) {
                transactions_[tid] = rec;
                continue;
            }

            const auto dit = decisions.find(tid);
            if (dit == decisions.end()) {
                THEMIS_WARN("GlobalTransactionManager [{}] in-doubt txn {} has no "
                            "decision – marking ABORT (manual resolution may be required)",
                            coordinator_id_, tid);
                rec.state         = GlobalTxnState::ABORT_DECIDED;
                transactions_[tid] = rec;
                continue;
            }

            const bool do_commit = dit->second;
            rec.state = do_commit
                ? GlobalTxnState::COMMIT_DECIDED
                : GlobalTxnState::ABORT_DECIDED;

            THEMIS_WARN("GlobalTransactionManager [{}] re-driving in-doubt txn {} "
                        "with decision {}",
                        coordinator_id_, tid, do_commit ? "COMMIT" : "ABORT");

            runPhase2(rec, do_commit);
            rec.state         = GlobalTxnState::COMPLETED;
            transactions_[tid] = rec;

            logToWAL(
                do_commit ? themis::sharding::WALEntryType::COMMIT_TX
                          : themis::sharding::WALEntryType::ABORT_TX,
                tid,
                {
                    {"transaction_id", tid},
                    {"coordinator_id", coordinator_id_},
                    {"phase",          "complete"},
                    {"recovery",       true}
                }
            );

            ++resolved;
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("GlobalTransactionManager [{}] WAL recovery failed: {}",
                     coordinator_id_, e.what());
    }

    THEMIS_INFO("GlobalTransactionManager [{}] recovery complete – {} in-doubt txn(s) resolved",
                coordinator_id_, resolved);
    return resolved;
}

/**
 * @brief Return stable coordinator type name for global recovery reports.
 * @return "GlobalTransactionManager".
 */
std::string GlobalTransactionManager::recoveryCoordinatorName() const {
    return "GlobalTransactionManager";
}

/**
 * @brief Return name of the durable backend used by this coordinator.
 * @return "WAL" when a WAL directory is configured, "disabled" otherwise.
 */
std::string GlobalTransactionManager::recoveryBackendName() const {
    return wal_ ? "WAL" : "disabled";
}

/**
 * @brief Return normalized snapshot of non-final transactions for global recovery.
 *
 * Maps each GlobalTxnState to the canonical RecoverableTwoPhaseState so the
 * GlobalTwoPhaseCommitRecoveryManager can compute aggregated in-doubt counts.
 *
 * @return List of non-final (not COMPLETED) transactions.
 */
std::vector<RecoverableTwoPhaseTransaction>
GlobalTransactionManager::getRecoverableTransactions() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<RecoverableTwoPhaseTransaction> result = {};

    for (const auto& [txn_id, rec] : transactions_) {
        if (rec.state == GlobalTxnState::COMPLETED) {
            continue;
        }

        RecoverableTwoPhaseTransaction rt;
        rt.transaction_id = txn_id;

        switch (rec.state) {
        case GlobalTxnState::ACTIVE:
            rt.state = RecoverableTwoPhaseState::ACTIVE;
            break;
        case GlobalTxnState::PREPARING:
            rt.state = RecoverableTwoPhaseState::PREPARING;
            break;
        case GlobalTxnState::COMMIT_DECIDED:
            rt.state             = RecoverableTwoPhaseState::COMMITTING;
            rt.decision_recorded = true;
            rt.decision_commit   = true;
            break;
        case GlobalTxnState::ABORT_DECIDED:
            rt.state             = RecoverableTwoPhaseState::ABORTING;
            rt.decision_recorded = true;
            rt.decision_commit   = false;
            break;
        case GlobalTxnState::FAILED:
            rt.state = RecoverableTwoPhaseState::FAILED;
            break;
        default:
            rt.state = RecoverableTwoPhaseState::UNKNOWN;
            break;
        }

        result.push_back(std::move(rt));
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Introspection
// ─────────────────────────────────────────────────────────────────────────────

std::optional<GlobalTxnState>
GlobalTransactionManager::getTransactionState(const std::string& txn_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
      return std::nullopt;
    }
    return it->second.state;
}

nlohmann::json GlobalTransactionManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t active    = 0;
    size_t completed = 0;
    for (const auto& [tid, rec] : transactions_) {
        if (rec.state == GlobalTxnState::COMPLETED) {
          ++completed;
        }
        else                                         ++active;
    }

    const auto uptime = std::chrono::steady_clock::now() - start_time_;
    return {
        {"coordinator_id",         coordinator_id_},
        {"uptime_seconds",         static_cast<uint64_t>(
                                       std::chrono::duration_cast<std::chrono::seconds>(uptime).count())},
        {"registered_regions",     regions_.size()},
        {"total_transactions",     total_transactions_.load()},
        {"total_commits",          total_commits_.load()},
        {"total_aborts",           total_aborts_.load()},
        {"total_errors",           total_errors_.load()},
        {"active_transactions",    active},
        {"completed_transactions", completed}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers (may be called with mutex_ held)
// ─────────────────────────────────────────────────────────────────────────────

bool GlobalTransactionManager::runPhase1(GlobalTxnRecord& rec) {
    bool all_voted_commit = true;

    for (auto& [region_id, rrec] : rec.region_records) {
        auto pit = regions_.find(region_id);
        if (pit == regions_.end()) {
            THEMIS_ERROR("GlobalTransactionManager [{}] Phase 1 – region {} not found for txn {}",
                         coordinator_id_, region_id, rec.transaction_id);
            rrec.voted = false;
            all_voted_commit = false;
            continue;
        }

        const nlohmann::json& ops = rec.region_ops.count(region_id)
            ? rec.region_ops.at(region_id)
            : nlohmann::json::array();

        bool vote = false;
        try {
            vote = pit->second->prepare(rec.transaction_id, ops);
        } catch (const std::exception& e) {
            THEMIS_ERROR("GlobalTransactionManager [{}] Phase 1 – region {} threw on PREPARE "
                         "for txn {}: {}",
                         coordinator_id_, region_id, rec.transaction_id, e.what());
            vote = false;
        }

        rrec.voted = vote;
        if (!vote) {
            THEMIS_INFO("GlobalTransactionManager [{}] region {} voted ABORT for txn {}",
                        coordinator_id_, region_id, rec.transaction_id);
            all_voted_commit = false;
        }
    }

    return all_voted_commit;
}

void GlobalTransactionManager::runPhase2(GlobalTxnRecord& rec, bool do_commit) {
    for (auto& [region_id, rrec] : rec.region_records) {
        auto pit = regions_.find(region_id);
        if (pit == regions_.end()) {
            THEMIS_WARN("GlobalTransactionManager [{}] Phase 2 – region {} not found for txn {} "
                        "(region may have been unregistered)",
                        coordinator_id_, region_id, rec.transaction_id);
            continue;
        }

        try {
            if (do_commit) {
                pit->second->commit(rec.transaction_id, rec.commit_timestamp_ns);
            } else {
                pit->second->abort(rec.transaction_id);
            }
            rrec.phase2_acked = true;
        } catch (const std::exception& e) {
            THEMIS_ERROR("GlobalTransactionManager [{}] Phase 2 – region {} threw on {} "
                         "for txn {}: {}",
                         coordinator_id_, region_id,
                         do_commit ? "COMMIT" : "ABORT",
                         rec.transaction_id, e.what());
        }
    }
}

void GlobalTransactionManager::logToWAL(
    themis::sharding::WALEntryType type,
    const std::string&             txn_id,
    const nlohmann::json&          data
) {
    if (!wal_) {
      return;
    }

    try {
        themis::sharding::WALEntry entry;
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
        THEMIS_ERROR("GlobalTransactionManager [{}] WAL write failed for txn {}: {}",
                     coordinator_id_, txn_id, e.what());
    }
}

std::string GlobalTransactionManager::generateTransactionId() {
    const uint64_t counter = txn_counter_.fetch_add(1, std::memory_order_relaxed);
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::ostringstream oss;
    oss << "gtxn-" << coordinator_id_ << "-" << now_ms << "-" << counter;
    return oss.str();
}

} // namespace themis::transaction
