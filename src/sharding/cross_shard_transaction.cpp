/**
 * @file cross_shard_transaction.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=29, M=40, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// CC-5: ThemisDB provides three transaction coordinator classes that share the
// IRecoverableTwoPhaseCoordinator recovery contract and coordinator-specific APIs.
// This coordinator uses TransactionWAL (not WALLoggingHelper), and WAL formats
// remain coordinator-specific; a transaction begun with one coordinator cannot
// be recovered by another.
// Cross-coordinator recovery tooling is planned for v3.0.0.
// → Architecture reference: docs/architecture/transaction_coordinators.md

#include "sharding/cross_shard_transaction.h"
#include <stdexcept>
#include "sharding/shard_rpc_client.h"
#include "sharding/truetime.h"
#include "sharding/transaction_wal.h"
#include "sharding/transaction_snapshot.h"
#include "utils/thread_join_utils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <limits>
#include <random>

// ── Wave 2-D: Canonical Lock-Hierarchy for CrossShardTransactionCoordinator ──
//
// Three mutexes are used in this translation unit.  They MUST always be
// acquired in the order shown below to prevent deadlock:
//
//   LEVEL 1 — transactions_mutex_  (std::timed_mutex)
//     Guards the main transaction table and coordinator lifecycle state.
//     Any code that needs both L1 and a lower-level mutex must acquire L1
//     first.  Use try_lock_for with LOCK_TIMEOUT_MS when entering from a
//     non-critical path.
//
//   LEVEL 2 — callbacks_mutex_  (std::mutex)
//     Guards registered completion/failure callbacks.
//     May be acquired while L1 is held.  Never hold L2 and then attempt
//     to acquire L1 (would invert the hierarchy).
//     See: failClosedAbortAllParticipants (line ~1724) which explicitly
//     requires that callbacks_mutex_ is NOT held on entry.
//
//   LEVEL 3 — deferred_mutex_  (std::mutex)
//     Guards the deferred-execution queue.
//     MUST NOT be acquired while transactions_mutex_ (L1) is held
//     (enforced by usage pattern: deferred_mutex_ is only accessed from
//     paths that have already released L1).
//     May be acquired while callbacks_mutex_ (L2) is held.
//
// Summary table:
//   Hierarchy  | transactions_mutex_ | callbacks_mutex_ | deferred_mutex_
//   Acquire if | (always first)      | after L1 or solo | after L2 or solo; NEVER under L1
//
// Verification: Wave 2-D / LKO-D1 — grep for mutex acquisition sites and
//   confirm no call site inverts this ordering.
// ─────────────────────────────────────────────────────────────────────────────

namespace themisdb {
namespace sharding {

namespace {
// Returns one set of nodes per independent deadlock cycle (Tarjan's SCC,
// only components with size > 1 or a self-loop are considered cycles).
std::vector<std::unordered_set<std::string>> collectCycles(
    const std::map<std::string, std::vector<std::string>>& graph
) {
    std::vector<std::unordered_set<std::string>> cycles;
    std::unordered_map<std::string, int> index;
    std::unordered_map<std::string, int> lowlink;
    std::vector<std::string> stack;
    std::unordered_set<std::string> on_stack;
    int next_index = 0;

    std::function<void(const std::string&)> strong_connect =
        [&](const std::string& node) {
        index[node] = next_index;
        lowlink[node] = next_index;
        ++next_index;
        stack.push_back(node);
        on_stack.insert(node);

        auto it = graph.find(node);
        if (it != graph.end()) {
            for (const auto& neighbor : it->second) {
                if (graph.find(neighbor) == graph.end()) {
                    continue;
                }
                if (index.find(neighbor) == index.end()) {
                    strong_connect(neighbor);
                    lowlink[node] = std::min(lowlink[node], lowlink[neighbor]);
                } else if (on_stack.count(neighbor) > 0) {
                    lowlink[node] = std::min(lowlink[node], index[neighbor]);
                }
            }
        }

        if (lowlink[node] != index[node]) {
            return;
        }

        std::vector<std::string> component = {};

        while (!stack.empty()) {
            const auto current = stack.back();
            stack.pop_back();
            on_stack.erase(current);
            component.push_back(current);
            if (current == node) {
                break;
            }
        }

        bool is_cycle_component = static_cast<int>(component.size()) > 1;
        if (!is_cycle_component && !component.empty()) {
            const auto self_it = graph.find(component.front());
            if (self_it != graph.end()) {
                is_cycle_component = std::find(
                    self_it->second.begin(),
                    self_it->second.end(),
                    component.front()) != self_it->second.end();
            }
        }

        if (is_cycle_component) {
            cycles.push_back(
                std::unordered_set<std::string>(component.begin(), component.end()));
        }
    };

    for (const auto& [node, _] : graph) {
        if (index.find(node) == index.end()) {
            strong_connect(node);
        }
    }

    return cycles;
}

std::unordered_set<std::string> collectCycleNodes(
    const std::map<std::string, std::vector<std::string>>& graph
) {
    std::unordered_set<std::string> cycle_nodes = {};

    for (auto& cycle : collectCycles(graph)) {
        cycle_nodes.insert(cycle.begin(), cycle.end());
    }
    return cycle_nodes;
}
}  // namespace

CrossShardTransactionCoordinator::CrossShardTransactionCoordinator(
    const CrossShardTransactionConfig& config,
    std::shared_ptr<ConsensusModule> consensus,
    std::shared_ptr<themis::sharding::TrueTime> truetime
)
    : config_(config)
    , consensus_(consensus)
    , truetime_(truetime)
    , ssi_manager_(config.ssi_config)
    , running_(false)
    , total_transactions_(0)
    , committed_transactions_(0)
    , aborted_transactions_(0)
    , deadlocked_transactions_(0)
    , transaction_log_path_(config.transaction_log_path)
{
    // If log path is not absolute, use a safe default
    // CST-4 fix: Reject startup if transaction_log_path_ is not an absolute
    // path.  A /tmp fallback could silently lose transaction logs after a
    // reboot and mask misconfiguration in production.
    if (transaction_log_path_.empty() ||
        !std::filesystem::path(transaction_log_path_).is_absolute()) {
        const std::string msg =
            "CrossShardTransactionCoordinator: transaction_log_path is not "
            "configured with an absolute path (got: '" + transaction_log_path_ +
            "'). Set CrossShardTransactionConfig::transaction_log_path to an "
            "absolute filesystem path before constructing the coordinator.";
        spdlog::error("{}", msg);
        throw std::invalid_argument(msg);
    }
    
    // Create TrueTime instance if not provided (for MVCC timestamp guarantees)
    if (!truetime_) {
        themis::sharding::TrueTime::Config tt_config;
        tt_config.base_uncertainty_us = 1000;  // 1ms base uncertainty
        truetime_ = std::make_shared<themis::sharding::TrueTime>(tt_config);
        spdlog::info("Created TrueTime instance for MVCC timestamp management");
    }
    
    // Phase 2.3.3: Initialize WAL and Snapshot if persistence enabled
    if (config_.enable_persistence) {
        if (config_.data_dir.empty()) {
            spdlog::warn("Persistence enabled but data_dir not configured, disabling persistence");
            return;
        }
        
        try {
            // Initialize Transaction WAL
            ::sharding::TransactionWALConfig wal_config;
            wal_config.wal_directory = config_.data_dir + "/wal";
            wal_config.snapshot_directory = config_.data_dir + "/snapshots";
            wal_config.segment_size = 16 * 1024 * 1024;  // 16 MB
            wal_config.snapshot_interval = config_.snapshot_interval;
            wal_config.max_snapshots = config_.max_snapshots;
            wal_config.sync_on_write = true;
            
            transaction_wal_ = std::make_unique<TransactionWAL>(wal_config);
            
            // Initialize Snapshot Manager
            snapshot_manager_ = std::make_unique<TransactionSnapshotManager>(
                wal_config.snapshot_directory,
                config_.max_snapshots
            );
            
            spdlog::info("Transaction WAL and Snapshot initialized at: {}", config_.data_dir);
        } catch (const std::exception& e) {
            spdlog::error("Failed to initialize transaction persistence: {}", e.what());
            transaction_wal_.reset();
            snapshot_manager_.reset();
        }
    }
}

/** @brief Stop coordinator and background workers during destruction. */
CrossShardTransactionCoordinator::~CrossShardTransactionCoordinator() {
    stop();
}

/** @brief Initialize coordinator dependencies and run startup recovery backend. */
bool CrossShardTransactionCoordinator::initialize() {
    if (!consensus_) {
        spdlog::error("Consensus module required for cross-shard transactions");
        return false;
    }
    
    // Phase 2.3.3: Initialize WAL if available
    if (transaction_wal_ && !transaction_wal_->initialize()) {
        spdlog::error("Failed to initialize transaction WAL");
        // Continue with degraded mode (no persistence)
        transaction_wal_.reset();
        snapshot_manager_.reset();
    }
    
    const auto recovery_result = runRecoveryBackend("initialize");
    if (!recovery_result.ok) {
        return false;
    }

    spdlog::info(
        "Cross-shard transaction coordinator initialized "
        "(backend={}, backend_available={}, recovery_ms={}, pending={}, "
        "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
        recovery_result.backend,
        recovery_result.backend_available,
        recovery_result.elapsed_ms,
        recovery_result.details.pending_transactions,
        recovery_result.details.snapshot_transactions_restored,
        recovery_result.details.wal_entries_replayed,
        recovery_result.details.stale_transactions_detected,
        recovery_result.details.resume_candidates,
        recovery_result.details.failed_operations,
        recovery_result.details.in_doubt_transactions);
    return true;
}

/** @brief Record distributed wait edge for global deadlock detection graph. */
void CrossShardTransactionCoordinator::reportDistributedWait(
    const std::string& waiting_transaction_id,
    const std::string& blocking_transaction_id,
    const std::string& shard_id
) {
    if (waiting_transaction_id.empty() || blocking_transaction_id.empty() ||
        waiting_transaction_id == blocking_transaction_id) {
        return;
    }

    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
    auto waiting_it = transactions_.find(waiting_transaction_id);
    auto blocking_it = transactions_.find(blocking_transaction_id);
    if (waiting_it == transactions_.end() || blocking_it == transactions_.end()) {
        return;
    }

    const auto waiting_state = waiting_it->second.state;
    const auto blocking_state = blocking_it->second.state;
    const bool waiting_live = waiting_state == TransactionState::ACTIVE ||
                              waiting_state == TransactionState::PREPARING;
    const bool blocking_live = blocking_state == TransactionState::ACTIVE ||
                               blocking_state == TransactionState::PREPARING ||
                               blocking_state == TransactionState::PREPARED;
    if (!waiting_live || !blocking_live) {
        return;
    }

    // W2-S06: Consensus validation — validate transaction IDs before recording wait edge
    if (waiting_transaction_id.empty()) {
        spdlog::error("recordDistributedWaitEdge: waiting_transaction_id is empty, rejecting edge");
        return;
    }
    
    if (blocking_transaction_id.empty()) {
        spdlog::error("recordDistributedWaitEdge: blocking_transaction_id is empty, rejecting edge");
        return;
    }
    
    distributed_wait_for_edges_[waiting_transaction_id].insert(blocking_transaction_id);
    spdlog::trace("Recorded distributed wait edge: {} -> {} (shard={})",
                  waiting_transaction_id, blocking_transaction_id, shard_id);
}

/** @brief Remove all distributed wait edges owned by transaction ID. */
void CrossShardTransactionCoordinator::clearDistributedWaits(
    const std::string& transaction_id
) {
    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
    clearDistributedWaitEdgesLocked(transaction_id);
}

/** @brief Acquire exclusive terminal decision guard for commit/abort race prevention. */
bool CrossShardTransactionCoordinator::tryStartTerminalDecision(
    const std::string& transaction_id
) {
    std::lock_guard<std::mutex> lock(decision_mutex_);
    return terminal_decisions_in_progress_.insert(transaction_id).second;
}

/** @brief Release terminal decision guard for transaction. */
void CrossShardTransactionCoordinator::finishTerminalDecision(
    const std::string& transaction_id
) {
    std::lock_guard<std::mutex> lock(decision_mutex_);
    terminal_decisions_in_progress_.erase(transaction_id);
}

/** @brief Start coordinator service and optional deadlock detection thread. */
bool CrossShardTransactionCoordinator::start() {
    if (running_.load()) {
        spdlog::warn("Cross-shard transaction coordinator already running");
        return false;
    }
    
    running_.store(true);
    
    // Start deadlock detection thread if enabled
    if (config_.enable_deadlock_detection) {
        deadlock_detection_thread_ = std::thread(
            &CrossShardTransactionCoordinator::deadlockDetectionThread, this
        );
    }
    
    // Start PreCommit retry thread for non-blocking 3PC
    // Only start if deferred PreCommit callback is configured
    {
        std::lock_guard<std::mutex> lk(callbacks_mutex_);
        if (deferred_precommit_callback_) {
            precommit_retry_thread_ = std::thread(
                &CrossShardTransactionCoordinator::preCommitRetryThread, this
            );
        }
    }
    
    spdlog::info("Cross-shard transaction coordinator started");
    return true;
}

/** @brief Run in-doubt recovery backend and return resolved transaction count. */
size_t CrossShardTransactionCoordinator::recoverInDoubtTransactions() {
    const auto count_in_doubt = [this]() -> size_t {
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        return static_cast<size_t>(std::count_if(
            transactions_.begin(),
            transactions_.end(),
            [](const auto& kv) {
                const auto state = kv.second.state;
                return state != TransactionState::COMMITTED &&
                       state != TransactionState::ABORTED;
            }));
    };

    const auto before = count_in_doubt();
    const auto recovery_result = runRecoveryBackend("recoverInDoubtTransactions");
    const auto* backend = recovery_result.backend;
    if (!recovery_result.ok) {
        return 0;
    }

    const auto after = count_in_doubt();
    if (after > before) {
        spdlog::warn(
            "CrossShardTransactionCoordinator: in-doubt count increased during recovery "
            "(backend={}, before={}, after={}, recovery_ms={}, pending={}, "
            "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={}); "
            "returning conservative resolved=0",
            backend,
            before,
            after,
            recovery_result.elapsed_ms,
            recovery_result.details.pending_transactions,
            recovery_result.details.snapshot_transactions_restored,
            recovery_result.details.wal_entries_replayed,
            recovery_result.details.stale_transactions_detected,
            recovery_result.details.resume_candidates,
            recovery_result.details.failed_operations,
            recovery_result.details.in_doubt_transactions);
        return 0;
    }

    const auto resolved = before - after;
    spdlog::info(
        "CrossShardTransactionCoordinator: in-doubt recovery finished "
        "(backend={}, before={}, after={}, resolved={}, recovery_ms={}, pending={}, "
        "snapshot_restored={}, wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
        backend,
        before,
        after,
        resolved,
        recovery_result.elapsed_ms,
        recovery_result.details.pending_transactions,
        recovery_result.details.snapshot_transactions_restored,
        recovery_result.details.wal_entries_replayed,
        recovery_result.details.stale_transactions_detected,
        recovery_result.details.resume_candidates,
        recovery_result.details.failed_operations,
        recovery_result.details.in_doubt_transactions);

    return resolved;
}

std::string CrossShardTransactionCoordinator::recoveryCoordinatorName() const {
    return "CrossShardTransactionCoordinator";
}

std::string CrossShardTransactionCoordinator::recoveryBackendName() const {
    return (transaction_wal_ && snapshot_manager_) ? "WAL/snapshot" : "disabled";
}

std::vector<themis::transaction::RecoverableTwoPhaseTransaction>
CrossShardTransactionCoordinator::getRecoverableTransactions() const {
    std::vector<themis::transaction::RecoverableTwoPhaseTransaction> recoverable;

    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
    recoverable.reserve(transactions_.size());
    for (const auto& [txn_id, txn] : transactions_) {
        if (txn.state == TransactionState::COMMITTED ||
            txn.state == TransactionState::ABORTED) {
            continue;
        }

        themis::transaction::RecoverableTwoPhaseTransaction info;
        info.transaction_id = txn_id;
        switch (txn.state) {
            case TransactionState::ACTIVE:
                info.state = themis::transaction::RecoverableTwoPhaseState::ACTIVE;
                break;
            case TransactionState::PREPARING:
                info.state = themis::transaction::RecoverableTwoPhaseState::PREPARING;
                break;
            case TransactionState::PREPARED:
                info.state = themis::transaction::RecoverableTwoPhaseState::PREPARED;
                break;
            case TransactionState::COMMITTING:
                info.state = themis::transaction::RecoverableTwoPhaseState::COMMITTING;
                info.decision_recorded = true;
                info.decision_commit = true;
                break;
            case TransactionState::ABORTING:
                info.state = themis::transaction::RecoverableTwoPhaseState::ABORTING;
                info.decision_recorded = true;
                info.decision_commit = false;
                break;
            case TransactionState::UNKNOWN:
                info.state = themis::transaction::RecoverableTwoPhaseState::UNKNOWN;
                break;
            case TransactionState::COMMITTED:
            [[fallthrough]];
            case TransactionState::ABORTED:
                info.state = themis::transaction::RecoverableTwoPhaseState::COMPLETED;
                break;
            default: break;
        }
        recoverable.push_back(std::move(info));
    }

    return recoverable;
}

/** @brief Execute configured recovery backend and emit telemetry summary. */
CrossShardTransactionCoordinator::RecoveryRunResult
CrossShardTransactionCoordinator::runRecoveryBackend(const char* context) {
    RecoveryRunResult result;
    result.backend_available = static_cast<bool>(transaction_wal_ && snapshot_manager_);

    const auto start = std::chrono::steady_clock::now();
    if (!result.backend_available) {
        result.ok = true;
        result.elapsed_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count());
        spdlog::warn(
            "Recovery backend unavailable during {} "
            "(backend={}, persistence_enabled={}, wal_initialized={}, snapshot_initialized={}); "
            "skipping recovery",
            context,
            result.backend,
            config_.enable_persistence,
            static_cast<bool>(transaction_wal_),
            static_cast<bool>(snapshot_manager_));
        return result;
    }

    result.ok = recoverFromWAL(&result.details);
    result.elapsed_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
            .count());

    if (!result.ok) {
        spdlog::error(
            "Recovery backend failed (backend={}, context={}, elapsed_ms={})",
            result.backend,
            context,
            result.elapsed_ms);
    } else {
        spdlog::info(
            "Recovery backend completed successfully "
            "(backend={}, context={}, elapsed_ms={}, pending={}, snapshot_restored={}, "
            "wal_replayed={}, stale={}, resume={}, failures={}, in_doubt={})",
            result.backend,
            context,
            result.elapsed_ms,
            result.details.pending_transactions,
            result.details.snapshot_transactions_restored,
            result.details.wal_entries_replayed,
            result.details.stale_transactions_detected,
            result.details.resume_candidates,
            result.details.failed_operations,
            result.details.in_doubt_transactions);
    }

    return result;
}

/** @brief Stop coordinator service and join deadlock detector thread. */
void CrossShardTransactionCoordinator::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (deadlock_detection_thread_.joinable()) {
        const bool deadlock_joined = themis::utils::joinThreadWithin(deadlock_detection_thread_);
        if (!deadlock_joined) {
            spdlog::warn("CrossShardTransactionCoordinator: deadlock detection thread did not join within timeout");
        }
    }
    
    // Stop PreCommit retry thread
    if (precommit_retry_thread_.joinable()) {
        const bool precommit_joined = themis::utils::joinThreadWithin(precommit_retry_thread_);
        if (!precommit_joined) {
            spdlog::warn("CrossShardTransactionCoordinator: precommit retry thread did not join within timeout");
        }
    }
    
    spdlog::info("Cross-shard transaction coordinator stopped");
}

/** @brief Begin new cross-shard transaction and persist initial state. */
bool CrossShardTransactionCoordinator::beginTransaction(
    const std::string& transaction_id,
    TransactionProtocol protocol,
    IsolationLevel isolation_level
) {
    // W2-S04: Fail-closed on empty transaction_id
    if (transaction_id.empty()) {
        spdlog::error("Cannot begin transaction with empty transaction_id");
        return false;
    }
    
    {
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);

        // Check if transaction already exists
        if (transactions_.find(transaction_id) != transactions_.end()) {
            spdlog::warn("Transaction {} already exists", transaction_id);
            return false;
        }

        // Create new transaction
        CrossShardTransaction txn;
        txn.transaction_id = transaction_id;
        txn.protocol = protocol;
        txn.isolation_level = isolation_level;
        txn.state = TransactionState::ACTIVE;
        txn.start_time = std::chrono::system_clock::now();

        // Assign snapshot timestamp for MVCC isolation
        // For snapshot isolation, use TrueTime to get a globally consistent timestamp
        if ((truetime_ && (isolation_level == IsolationLevel::SNAPSHOT_ISOLATION ||
                          isolation_level == IsolationLevel::SERIALIZABLE))) {
            auto tt_now = truetime_->now();
            // Use the latest bound to ensure we read the most recent committed data
            txn.snapshot_timestamp = tt_now.latest.count();

            spdlog::info("Transaction {} assigned snapshot timestamp {} (MVCC enabled)",
                        transaction_id, txn.snapshot_timestamp);
        } else {
            // For other isolation levels, use system time
            txn.snapshot_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
        }

        transactions_[transaction_id] = txn;
        total_transactions_++;
    }
    
    // Phase 2.3.3: Log to WAL if enabled
    if (transaction_wal_) {
        try {
            std::vector<std::string> participants;  // Empty at begin, will add later
            transaction_wal_->logBegin(
                transaction_id,
                static_cast<::sharding::TransactionProtocol>(protocol),
                participants
            );
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log BEGIN to WAL: {}", e.what());
            // Continue without WAL (graceful degradation)
        }
    }
    
    // Persist transaction state
    persistTransactionState(transaction_id, TransactionState::ACTIVE);
    
    // Replicate transaction metadata via consensus
    if (consensus_) {
        nlohmann::json data = {
            {"transaction_id", transaction_id},
            {"protocol", static_cast<int>(protocol)},
            {"isolation_level", static_cast<int>(isolation_level)},
            {"state", static_cast<int>(TransactionState::ACTIVE)}
        };
        
        static_cast<void>(consensus_->propose("BEGIN_TRANSACTION", data));
    }
    
    spdlog::info("Transaction {} started with protocol {}", 
                 transaction_id, static_cast<int>(protocol));
    return true;
}

/** @brief Add participant shard and operations to active transaction. */
bool CrossShardTransactionCoordinator::addParticipant(
    const std::string& transaction_id,
    const std::string& shard_id,
    const std::string& endpoint,
    const std::vector<std::string>& operations
) {
    // W2-S04: Fail-closed on empty inputs
    if (transaction_id.empty()) {
        spdlog::error("Cannot add participant with empty transaction_id");
        return false;
    }
    
    if (shard_id.empty()) {
        spdlog::error("Cannot add participant with empty shard_id to transaction {}", transaction_id);
        return false;
    }
    
    if (endpoint.empty()) {
        spdlog::error("Cannot add participant {} to transaction {} with empty endpoint", 
                      shard_id, transaction_id);
        return false;
    }
    
    if (operations.empty()) {
        spdlog::error("Cannot add participant {} to transaction {} with empty operations list", 
                      shard_id, transaction_id);
        return false;
    }
    
    // W2-S07: Lock with timeout to prevent indefinite blocking on high contention
    std::unique_lock<std::timed_mutex> lock(transactions_mutex_);
    if (!lock.try_lock_for(config_.lock_timeout)) {
        spdlog::error("Lock acquisition timeout adding participant to transaction {}", transaction_id);
        return false;
    }
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        spdlog::error("Transaction {} is not active", transaction_id);
        return false;
    }
    
    // W2-S04: Fail-closed if participant already exists (duplicate shard)
    if (txn.participants.find(shard_id) != txn.participants.end()) {
        spdlog::error("Participant {} already exists in transaction {}", shard_id, transaction_id);
        return false;
    }
    
    // Add participant
    ShardParticipant participant;
    participant.shard_id = shard_id;
    participant.endpoint = endpoint;
    participant.operations = operations;
    
    txn.participants[shard_id] = participant;
    
    spdlog::debug("Added participant {} to transaction {}", shard_id, transaction_id);
    return true;
}

/** @brief Execute prepare stage for active transaction across participants. */
bool CrossShardTransactionCoordinator::prepare(const std::string& transaction_id) {
    // W2-S04: Fail-closed on empty transaction_id
    if (transaction_id.empty()) {
        spdlog::error("Cannot prepare transaction with empty transaction_id");
        return false;
    }
    
    std::unique_lock<std::timed_mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        spdlog::error("Transaction {} is not active", transaction_id);
        return false;
    }
    
    // W2-S04: Fail-closed if no participants added (precondition violation)
    if (txn.participants.empty()) {
        spdlog::error("Cannot prepare transaction {} with no participants", transaction_id);
        return false;
    }

    // Issue #5390: Cross-Shard FK Referential Integrity Check.
    // Validate all registered foreign key constraints BEFORE advancing to
    // PREPARING and before sending any PREPARE RPC to participants.
    // If any FK violation is detected, fail the prepare phase immediately
    // (fail-closed) so that orphaned child records can never be committed.
    {
        std::shared_ptr<CrossShardForeignKeyValidator> fk_val;
        {
            std::lock_guard<std::mutex> cb_lock(callbacks_mutex_);
            fk_val = fk_validator_;
        }
        if (fk_val) {
            // Collect all operations across all participants for FK checking.
            std::map<std::string, std::vector<std::string>> shard_ops;
            for (const auto& [shard_id, participant] : txn.participants) {
                for (const auto& op_str : participant.operations) {
                    shard_ops[shard_id].push_back(op_str);
                }
            }

            // Run validation outside the transaction lock to avoid deadlock
            // while the lookup callback may perform network I/O.
            lock.unlock();
            auto violations = fk_val->validate(transaction_id, shard_ops);
            if (!lock.try_lock_for(config_.lock_timeout)) {
                spdlog::error("Lock acquisition timeout after FK validation for "
                              "transaction {}", transaction_id);
                return false;
            }

            if (!violations.empty()) {
                spdlog::error(
                    "Transaction {} aborted during prepare: {} cross-shard FK "
                    "violation(s) detected",
                    transaction_id,static_cast<int>(violations.size()));
                for (const auto& v : violations) {
                    spdlog::error("  FK violation: {}", v.message);
                }
                // Transaction remains ACTIVE so caller may inspect and abort.
                return false;
            }
        }
    }

    txn.state = TransactionState::PREPARING;
    persistTransactionState(transaction_id, TransactionState::PREPARING);
    lock.unlock();

    // --- Cross-shard FK validation gate (issue #5392) ---
    // Snapshot the validator pointer outside any transaction lock to avoid
    // holding two mutexes simultaneously.
    std::shared_ptr<CrossShardForeignKeyValidator> fk_validator;
    {
        std::lock_guard<std::mutex> cb_lock(callbacks_mutex_);
        fk_validator = fk_validator_;
    }
    if (fk_validator) {
        // Build the per-shard operation map from participant data.
        std::map<std::string, std::vector<std::string>> shard_ops;
        {
            if (!lock.try_lock_for(config_.lock_timeout)) {
                spdlog::error("FK validation: lock timeout reading operations for txn {}",
                              transaction_id);
                return false;
            }
            auto it2 = transactions_.find(transaction_id);
            if (it2 != transactions_.end()) {
                for (const auto& [sid, p] : it2->second.participants) {
                    shard_ops[sid] = p.operations;
                }
            }
            lock.unlock();
        }

        const auto violations = fk_validator->validate(transaction_id, shard_ops);

        // Abort if any non-deferrable violation was found.
        bool has_blocking = false;
        for (const auto& v : violations) {
            if (!v.deferrable) {
                has_blocking = true;
                spdlog::error(
                    "CrossShardFK: non-deferrable violation blocks prepare of txn {}: {}",
                    transaction_id, v.message);
            }
        }
        if (has_blocking) {
            if (lock.try_lock_for(config_.lock_timeout)) {
                auto it2 = transactions_.find(transaction_id);
                if (it2 != transactions_.end()) {
                    it2->second.state = TransactionState::ACTIVE;
                }
                lock.unlock();
            }
            return false;
        }
    }
    // --- End FK validation gate ---

    // Send prepare requests to all participants
    bool all_prepared = true;
    for (auto& [shard_id, participant] : txn.participants) {
        // Phase 2.3.3: Log PREPARE to WAL
        if (transaction_wal_) {
            try {
                nlohmann::json prepare_data = {
                    {"shard_id", shard_id},
                    {"operations", participant.operations}
                };
                transaction_wal_->logPrepare(transaction_id, shard_id, prepare_data);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log PREPARE to WAL: {}", e.what());
            }
        }
        
        // CST-5: sendPrepare() performs network I/O outside the lock.
        // Wrap the call so that any exception still re-acquires the lock,
        // keeping the unique_lock in a defined (locked) state for cleanup.
        bool prepared = false;
        try {
            prepared = sendPrepare(shard_id, transaction_id);
        } catch (...) {
            if (!lock.try_lock_for(config_.lock_timeout)) {
                spdlog::critical("Lock acquisition timeout in prepare exception handler "
                                 "for shard {} in transaction {}", shard_id, transaction_id);
                all_prepared = false;
                throw;
            }
            participant.prepared = false;
            participant.error_message = "sendPrepare threw an exception";
            all_prepared = false;
            spdlog::error("sendPrepare threw for shard {} in transaction {}",
                         shard_id, transaction_id);
            // Leave lock held; the outer loop will break naturally.
            throw;
        }

        if (!lock.try_lock_for(config_.lock_timeout)) {
            spdlog::error("Lock acquisition timeout after sendPrepare for shard {} in "
                          "transaction {}", shard_id, transaction_id);
            all_prepared = false;
            break;
        }
        participant.prepared = prepared;
        
        // Phase 2.3.3: Log PREPARED response to WAL
        if (transaction_wal_) {
            try {
                std::string response = prepared ? "prepared" : "aborted";
                transaction_wal_->logPrepared(transaction_id, shard_id, prepared, response);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log PREPARED to WAL: {}", e.what());
            }
        }
        lock.unlock();
        
        if (!prepared) {
            all_prepared = false;
            participant.error_message = "Prepare failed";
            spdlog::error("Prepare failed for shard {} in transaction {}", 
                         shard_id, transaction_id);
        }
    }
    
    if (!lock.try_lock_for(config_.lock_timeout)) {
        spdlog::error("Lock acquisition timeout consolidating prepare results for "
                      "transaction {}", transaction_id);
        return false;
    }
    // Capture isolation level while the lock is held.
    const auto isolation = transactions_.count(transaction_id) > 0
        ? transactions_.at(transaction_id).isolation_level
        : IsolationLevel::SNAPSHOT_ISOLATION;

    if (all_prepared) {
        txn.state = TransactionState::PREPARED;
        persistTransactionState(transaction_id, TransactionState::PREPARED);
        spdlog::info("Transaction {} prepared successfully", transaction_id);
    } else {
        txn.state = TransactionState::ACTIVE;  // Roll back to active
    }
    lock.unlock();

    // ── Distributed SSI validation (SERIALIZABLE transactions only) ──────────
    // Perform cross-shard conflict detection after all participants have
    // prepared but before we confirm the PREPARED state to the caller.
    // If a serialization conflict is detected the transaction is aborted.
    if (all_prepared && isolation == IsolationLevel::SERIALIZABLE) {
        auto conflicts = ssi_manager_.validateAtPrepare(transaction_id);
        if (!conflicts.empty()) {
            spdlog::warn(
                "Transaction {} aborted due to cross-shard SSI conflict: {}",
                transaction_id, conflicts.front().message);
            // Abort the transaction and clean up SSI bookkeeping.
            abort(transaction_id);
            ssi_manager_.clearTransaction(transaction_id);
            return false;
        }
    }
    
    // Replicate prepare state via consensus
    if (consensus_ && all_prepared) {
        static_cast<void>(consensus_->propose("PREPARE_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(TransactionState::PREPARED)}
        }));
    }
    
    return all_prepared;
}

/** @brief Execute protocol-specific commit and finalize terminal state. */
bool CrossShardTransactionCoordinator::commit(const std::string& transaction_id) {
    if (!tryStartTerminalDecision(transaction_id)) {
        spdlog::warn("Transaction {} already has a terminal decision in progress",
                     transaction_id);
        return false;
    }
    const auto decision_guard =
        std::unique_ptr<void, std::function<void(void*)>>(
            reinterpret_cast<void*>(1),
            [this, &transaction_id](void*) {
                finishTerminalDecision(transaction_id);
            });

    std::unique_lock<std::timed_mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }

    if (it->second.state == TransactionState::COMMITTED) {
        spdlog::info("Transaction {} already committed", transaction_id);
        return true;
    }
    if (it->second.state == TransactionState::ABORTED) {
        spdlog::error("Transaction {} has already been aborted", transaction_id);
        return false;
    }
    if (it->second.state != TransactionState::ACTIVE &&
        it->second.state != TransactionState::PREPARED) {
        spdlog::error("Transaction {} is not committable from state {}",
                      transaction_id,
                      static_cast<int>(it->second.state));
        return false;
    }
    
    // Copy the transaction by value before releasing the lock to avoid a dangling
    // reference: a concurrent abort() could erase the map entry while we are
    // executing the protocol outside the lock (CST-1).
    auto txn = it->second;
    
    // Execute protocol-specific commit
    bool success = false;
    lock.unlock();
    
    switch (txn.protocol) {
        case TransactionProtocol::TWO_PHASE_COMMIT:
            success = execute2PC(txn);
            break;
        case TransactionProtocol::THREE_PHASE_COMMIT:
            success = execute3PC(txn);
            break;
        case TransactionProtocol::PERCOLATOR:
            success = executePercolator(txn);
            break;
        case TransactionProtocol::CALVIN:
            success = executeCalvin(txn);
            break;
        case TransactionProtocol::SAGA:
            // SAGA commit handled by executeSaga
            spdlog::error("SAGA transactions should use executeSaga method");
            return false;
        default:
            spdlog::error("Unknown transaction protocol");
            return false;
    }
    
    if (!lock.try_lock_for(config_.lock_timeout)) {
        spdlog::error("Lock acquisition timeout updating commit state for transaction {}",
                      transaction_id);
        return false;
    }
    // Re-look-up the entry after re-acquiring the lock; the entry may have been
    // erased by a concurrent operation while the lock was released.
    auto it2 = transactions_.find(transaction_id);
    if (it2 != transactions_.end()) {
        auto& live_txn = it2->second;
        if (success) {
            live_txn.state = TransactionState::COMMITTED;
            live_txn.end_time = std::chrono::system_clock::now();
            committed_transactions_++;
            persistTransactionState(transaction_id, TransactionState::COMMITTED);
            spdlog::info("Transaction {} committed successfully", transaction_id);
        } else {
            live_txn.state = TransactionState::ABORTED;
            live_txn.end_time = std::chrono::system_clock::now();
            aborted_transactions_++;
            persistTransactionState(transaction_id, TransactionState::ABORTED);
            spdlog::error("Transaction {} commit failed, aborted", transaction_id);
        }
        clearDistributedWaitEdgesLocked(transaction_id);
    }
    lock.unlock();
    
    // Replicate final state via consensus
    if (consensus_) {
        auto final_state = success ? TransactionState::COMMITTED : TransactionState::ABORTED;
        static_cast<void>(consensus_->propose("FINALIZE_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(final_state)}
        }));
    }

    // Release SSI bookkeeping regardless of commit/abort outcome.
    ssi_manager_.clearTransaction(transaction_id);

    return success;
}

/** @brief Abort transaction and propagate abort decision to participants. */
bool CrossShardTransactionCoordinator::abort(const std::string& transaction_id) {
    if (!tryStartTerminalDecision(transaction_id)) {
        spdlog::warn("Transaction {} already has a terminal decision in progress",
                     transaction_id);
        return false;
    }
    const auto decision_guard =
        std::unique_ptr<void, std::function<void(void*)>>(
            reinterpret_cast<void*>(1),
            [this, &transaction_id](void*) {
                finishTerminalDecision(transaction_id);
            });

    // W2-S07: Enforce lock timeout during abort decision phase
    std::unique_lock<std::timed_mutex> lock(transactions_mutex_);
    if (!lock.try_lock_for(config_.lock_timeout)) {
        spdlog::error("Lock acquisition timeout acquiring abort decision for transaction {}",
                      transaction_id);
        return false;
    }
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }

    if (it->second.state == TransactionState::ABORTED) {
        spdlog::info("Transaction {} already aborted", transaction_id);
        return true;
    }
    if (it->second.state == TransactionState::COMMITTED) {
        spdlog::error("Transaction {} has already been committed", transaction_id);
        return false;
    }
    if (it->second.state != TransactionState::ACTIVE &&
        it->second.state != TransactionState::PREPARING &&
        it->second.state != TransactionState::PREPARED) {
        spdlog::error("Transaction {} is not abortable from state {}",
                      transaction_id,
                      static_cast<int>(it->second.state));
        return false;
    }
    
    // Copy the transaction by value before releasing the lock to avoid a dangling
    // reference: a concurrent commit() could erase the map entry while we are
    // sending abort RPCs outside the lock (CST-2).
    auto txn = it->second;
    txn.state = TransactionState::ABORTING;
    it->second.state = TransactionState::ABORTING;
    lock.unlock();
    persistTransactionState(transaction_id, TransactionState::ABORTING);
    
    // Phase 2.3.4: Log ABORT decision to WAL
    if (transaction_wal_) {
        try {
            transaction_wal_->logAbort(txn.transaction_id, "coordinator_decision");
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log ABORT to WAL: {}", e.what());
        }
    }
    
    // Send abort requests to all participants using the local copy.
    for (auto& [shard_id, participant] : txn.participants) {
        static_cast<void>(sendAbort(shard_id, transaction_id));
        participant.aborted = true;
        
        // Phase 2.3.4: Log ABORTED confirmation to WAL
        if (transaction_wal_) {
            try {
                transaction_wal_->logAborted(transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log ABORTED to WAL: {}", e.what());
            }
        }
    }
    
    if (!lock.try_lock_for(config_.lock_timeout)) {
        spdlog::error("Lock acquisition timeout updating abort state for transaction {}",
                      transaction_id);
        return false;
    }
    bool should_persist_aborted = false;
    // Re-look-up after re-acquiring the lock so we update the live entry, not the copy.
    auto it2 = transactions_.find(transaction_id);
    if (it2 != transactions_.end()) {
        it2->second.state = TransactionState::ABORTED;
        it2->second.end_time = std::chrono::system_clock::now();
        aborted_transactions_++;
        clearDistributedWaitEdgesLocked(transaction_id);
        should_persist_aborted = true;
    }
    lock.unlock();

    if (should_persist_aborted) {
        persistTransactionState(transaction_id, TransactionState::ABORTED);
    }
    
    // Phase 2.3.4: Check if snapshot needed
    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        createPeriodicSnapshot();
    }
    
    // Replicate abort state via consensus
    if (consensus_) {
        static_cast<void>(consensus_->propose("ABORT_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(TransactionState::ABORTED)}
        }));
    }
    
    spdlog::info("Transaction {} aborted", transaction_id);

    // Release SSI bookkeeping for this transaction.
    ssi_manager_.clearTransaction(transaction_id);

    return true;
}

bool CrossShardTransactionCoordinator::executeSaga(
    const std::string& transaction_id,
    const std::vector<nlohmann::json>& steps,
    const std::vector<nlohmann::json>& compensations
) {
    // Validate input early before acquiring locks
    if (static_cast<int>(steps.size()) != static_cast<int>(compensations.size())) {
        spdlog::error("SAGA transaction {} has mismatched steps ({}) and compensations ({})", 
                     transaction_id,static_cast<int>(steps.size()),static_cast<int>(compensations.size()));
        return false;
    }
    
    std::unique_lock<std::timed_mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    // Validate the protocol on the live entry while locked, then release.
    if (it->second.protocol != TransactionProtocol::SAGA) {
        spdlog::error("Transaction {} is not a SAGA transaction", transaction_id);
        return false;
    }
    
    // Do NOT hold a reference across the lock release (CST-3): after unlock a
    // concurrent call could erase the map entry.  The only field needed during
    // the unlocked section is the transaction_id (already a local string).
    lock.unlock();
    
    spdlog::info("Executing SAGA transaction {} with {} steps", 
                transaction_id,static_cast<int>(steps.size()));
    
    // Execute steps sequentially
    size_t completed_steps = 0;
    std::vector<nlohmann::json> executed_steps;
    
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        
        // Extract shard_id and operation from step
        if (!step.contains("shard_id") || !step.contains("operation")) {
            spdlog::error("SAGA step {} missing shard_id or operation", i);
            
            // Execute compensations for completed steps
            executeCompensations(transaction_id, executed_steps, compensations);
            
            return false;
        }
        
        std::string shard_id = step["shard_id"];
        nlohmann::json operation = step["operation"];
        
        spdlog::info("Executing SAGA step {} on shard {} for transaction {}", 
                    i, shard_id, transaction_id);
        
        // Execute step - send operation to shard via RPC
        try {
            if (!lock.try_lock_for(config_.lock_timeout)) {
                spdlog::error("Lock acquisition timeout during SAGA step {} for transaction {}",
                              i, transaction_id);
                executeCompensations(transaction_id, executed_steps, compensations);
                return false;
            }
            // Re-look-up the live transaction (txn reference was not kept across the lock
            // release to avoid the CST-3 dangling reference).
            auto saga_participant_it = transactions_.find(transaction_id);
            if (saga_participant_it == transactions_.end()) {
                lock.unlock();
                spdlog::error("Transaction {} disappeared while executing SAGA step {}",
                             transaction_id, i);
                executeCompensations(transaction_id, executed_steps, compensations);
                return false;
            }
            auto participant_it = saga_participant_it->second.participants.find(shard_id);
            if (participant_it == saga_participant_it->second.participants.end()) {
                lock.unlock();
                spdlog::error("Shard {} not found in transaction {} participants", 
                            shard_id, transaction_id);
                
                // Execute compensations
                executeCompensations(transaction_id, executed_steps, compensations);
                
                return false;
            }
            
            std::string endpoint = participant_it->second.endpoint;
            lock.unlock();
            
            // Create RPC client for this shard
            themis::sharding::ShardRPCClient::Config rpc_config;
            rpc_config.endpoint = endpoint;
            rpc_config.timeout_ms = static_cast<int>(config_.saga_step_timeout.count());
            rpc_config.max_retries = 2;  // SAGA steps should be idempotent
            rpc_config.retry_delay_ms = 100;
            
            themis::sharding::ShardRPCClient rpc_client(rpc_config);
            
            // Execute the step with timeout
            auto step_start = std::chrono::steady_clock::now();
            bool success = false;
            
            // For SAGA, we use a simplified execution model
            // In production, this would be a specific SAGA operation RPC
            nlohmann::json operations = nlohmann::json::array();
            operations.push_back(operation);
            
            // Try to execute the step
            int retries = 0;
            while (retries <= rpc_config.max_retries) {
                try {
                    // Check timeout
                    auto elapsed = std::chrono::steady_clock::now() - step_start;
                    if (elapsed > config_.saga_step_timeout) {
                        spdlog::error("SAGA step {} timed out after {}ms", 
                                    i, 
                                    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                        break;
                    }
                    
                    // Execute step (using prepare as proxy for step execution)
                    success = rpc_client.prepare(transaction_id + "_step_" + std::to_string(i), 
                                                operations);
                    
                    if (success) {
                        break;
                    }
                    
                } catch (const std::exception& e) {
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("SAGA step {} execution failed (attempt {}/{}): {}. Retrying", 
                                   i, retries + 1, rpc_config.max_retries + 1, e.what());
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                        retries++;
                    } else {
                        spdlog::error("SAGA step {} execution failed after {} retries: {}", 
                                    i, rpc_config.max_retries, e.what());
                        break;
                    }
                }
            }
            
            if (!success) {
                spdlog::error("SAGA step {} failed, executing compensations", i);
                
                // Phase 2.3.4: Log COMPENSATE decision before compensation
                if (transaction_wal_) {
                    try {
                        nlohmann::json compensate_data = {
                            {"step", i},
                            {"reason", "step_failed"}
                        };
                        transaction_wal_->logCompensate(transaction_id, shard_id, compensate_data);
                        operations_since_snapshot_++;
                    } catch (const std::exception& e) {
                        spdlog::warn("Failed to log COMPENSATE to WAL: {}", e.what());
                    }
                }
                
                // Execute compensations for completed steps in reverse order
                executeCompensations(transaction_id, executed_steps, compensations);
                
                abort(transaction_id);
                return false;
            }
            
            executed_steps.push_back(step);
            completed_steps++;
            
            // Phase 2.3.4: Log successful step execution to WAL
            if (transaction_wal_) {
                try {
                    nlohmann::json step_data = {
                        {"step", i},
                        {"shard_id", shard_id},
                        {"operation", operation}
                    };
                    transaction_wal_->logPrepared(transaction_id, shard_id, true, "step_completed");
                    operations_since_snapshot_++;
                } catch (const std::exception& e) {
                    spdlog::warn("Failed to log SAGA step to WAL: {}", e.what());
                }
            }
            
            spdlog::info("SAGA step {} completed successfully", i);
            
        } catch (const std::exception& e) {
            spdlog::error("SAGA step {} failed with exception: {}", i, e.what());
            
            // Execute compensations
            executeCompensations(transaction_id, executed_steps, compensations);
            
            abort(transaction_id);
            return false;
        }
    }
    
    // All steps completed successfully
    if (!lock.try_lock_for(config_.lock_timeout)) {
        spdlog::error("Lock acquisition timeout finalizing SAGA transaction {}",
                      transaction_id);
        return false;
    }
    // Re-look-up the live entry; it may have been erased during the unlocked
    // execution of the SAGA steps (CST-3 fix).
    auto saga_it = transactions_.find(transaction_id);
    if (saga_it != transactions_.end()) {
        saga_it->second.state = TransactionState::COMMITTED;
        saga_it->second.end_time = std::chrono::system_clock::now();
        committed_transactions_++;
    }
    lock.unlock();
    
    spdlog::info("SAGA transaction {} completed successfully with {} steps", 
                transaction_id, completed_steps);
    return true;
}

std::optional<TransactionState> CrossShardTransactionCoordinator::getTransactionState(
    const std::string& transaction_id
) const {
    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second.state;
}

std::optional<CrossShardTransaction> CrossShardTransactionCoordinator::getTransaction(
    const std::string& transaction_id
) const {
    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

bool CrossShardTransactionCoordinator::isDeadlocked(
    const std::string& transaction_id
) const {
    // Build wait-for graph
    auto graph = buildWaitForGraph();
    const auto cycle_nodes = collectCycleNodes(graph);
    return cycle_nodes.count(transaction_id) > 0;
}

std::vector<CrossShardTransaction> CrossShardTransactionCoordinator::getActiveTransactions() const {
    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
    
    std::vector<CrossShardTransaction> active = {};

    for (const auto& [id, txn] : transactions_) {
        if (txn.state == TransactionState::ACTIVE ||
            txn.state == TransactionState::PREPARING ||
            txn.state == TransactionState::PREPARED) {
            active.push_back(txn);
        }
    }
    
    return active;
}

nlohmann::json CrossShardTransactionCoordinator::getStatistics() const {
    return {
        {"total_transactions", total_transactions_.load()},
        {"committed_transactions", committed_transactions_.load()},
        {"aborted_transactions", aborted_transactions_.load()},
        {"deadlocked_transactions", deadlocked_transactions_.load()},
        {"active_transactions", getActiveTransactions().size()}
    };
}

void CrossShardTransactionCoordinator::onTransactionStateChange(
    std::function<void(const std::string&, TransactionState, TransactionState)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_state_change_callback_ = std::move(callback);
}

void CrossShardTransactionCoordinator::setPreCommitCallback(PreCommitRpcFn fn) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    precommit_callback_ = std::move(fn);
}

void CrossShardTransactionCoordinator::setDeferredPreCommitCallback(DeferredPreCommitFn fn) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    deferred_precommit_callback_ = std::move(fn);
}

void CrossShardTransactionCoordinator::setForeignKeyValidator(
    std::shared_ptr<CrossShardForeignKeyValidator> validator)
{
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    fk_validator_ = std::move(validator);
}

// Private methods

bool CrossShardTransactionCoordinator::execute2PC(CrossShardTransaction& txn) {
    // Phase 1: Prepare (already done)
    if (txn.state != TransactionState::PREPARED) {
        if (!prepare(txn.transaction_id)) {
            return false;
        }
    }
    
    // Phase 2: Commit
    txn.state = TransactionState::COMMITTING;
    
    // Phase 2.3.3: Log COMMIT decision to WAL
    // CC-3: WAL write must succeed (or WAL be absent) before we send commit RPCs
    // to participants; otherwise a coordinator crash could leave participants
    // committed with no recovery record.
    if (transaction_wal_) {
        try {
            nlohmann::json commit_data = {
                {"protocol", "2PC"},
                {"participants", nlohmann::json::array()}
            };
            for (const auto& [shard_id, participant] : txn.participants) {
                commit_data["participants"].push_back(shard_id);
            }
            transaction_wal_->logCommit(txn.transaction_id, commit_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::error("execute2PC [{}]: WAL COMMIT log failed: {} — aborting to preserve durability",
                          txn.transaction_id, e.what());
            return false;
        }
    }
    
    const auto failClosedAbortRemaining = [this, &txn](std::string_view reason) {
        txn.state = TransactionState::ABORTING;

        if (transaction_wal_) {
            try {
                transaction_wal_->logAbort(txn.transaction_id, std::string(reason));
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::error("execute2PC [{}]: WAL ABORT log failed during fail-closed handling: {}",
                             txn.transaction_id,
                             e.what());
            }
        }

        for (auto& [remaining_shard_id, remaining_participant] : txn.participants) {
            if (remaining_participant.committed) {
                continue;
            }

            const bool aborted = sendAbort(remaining_shard_id, txn.transaction_id);
            remaining_participant.aborted = aborted;
            if (!aborted) {
                spdlog::error("execute2PC [{}]: fail-closed abort RPC failed for shard {}",
                             txn.transaction_id,
                             remaining_shard_id);
            }

            if (transaction_wal_ && aborted) {
                try {
                    transaction_wal_->logAborted(txn.transaction_id, remaining_shard_id);
                    operations_since_snapshot_++;
                } catch (const std::exception& e) {
                    spdlog::error("execute2PC [{}]: WAL ABORTED log failed for shard {}: {}",
                                 txn.transaction_id,
                                 remaining_shard_id,
                                 e.what());
                }
            }
        }
    };

    for (auto& [shard_id, participant] : txn.participants) {
        bool committed = sendCommit(shard_id, txn.transaction_id);
        participant.committed = committed;

        if (!committed) {
            spdlog::error("execute2PC [{}]: Commit failed for shard {} - failing closed",
                         txn.transaction_id,
                         shard_id);
            failClosedAbortRemaining("phase2_commit_failed");
            return false;
        }

        // Phase 2.3.3: Log COMMITTED response to WAL.
        // Fail-closed: if the coordinator cannot durably record a participant
        // commit acknowledgement, do not continue committing additional shards.
        if (transaction_wal_) {
            try {
                transaction_wal_->logCommitted(txn.transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::error("execute2PC [{}]: WAL COMMITTED log failed for shard {}: {} - failing closed",
                             txn.transaction_id,
                             shard_id,
                             e.what());
                failClosedAbortRemaining("phase2_wal_ack_log_failed");
                return false;
            }
        }

        spdlog::info("Shard {} committed transaction {}",
                         shard_id, txn.transaction_id);
    }
    
    // Phase 2.3.3: Check if snapshot needed
    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        createPeriodicSnapshot();
    }
    
    return true;
}

bool CrossShardTransactionCoordinator::execute3PC(CrossShardTransaction& txn) {
    const auto failClosedAbortAllParticipants = [this, &txn](const char* abort_reason) {
        txn.state = TransactionState::ABORTING;
        for (auto& [shard_id, participant] : txn.participants) {
            const bool aborted = sendAbort(shard_id, txn.transaction_id);
            participant.aborted = aborted;
            if (!aborted) {
                spdlog::error("execute3PC [{}]: fail-closed abort RPC failed for shard {}",
                              txn.transaction_id, shard_id);
            }
        }

        if (transaction_wal_) {
            try {
                transaction_wal_->logAbort(txn.transaction_id, std::string(abort_reason));
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::error("execute3PC [{}]: failed to log fail-closed ABORT to WAL: {}",
                              txn.transaction_id, e.what());
            }
        }
    };

    // Snapshot the PreCommit callback under the lock so the lock is not held
    // during the (potentially blocking) RPC fan-out.
    PreCommitRpcFn precommit_cb;
    {
        std::lock_guard<std::mutex> lk(callbacks_mutex_);
        precommit_cb = precommit_callback_;
    }

    // Phase 1: Prepare (CanCommit)
    if (txn.state != TransactionState::PREPARED) {
        if (!prepare(txn.transaction_id)) {
            spdlog::error("3PC Phase 1 (Prepare) failed for transaction {}",
                         txn.transaction_id);
            return false;
        }
    }

    // Phase 2: PreCommit
    //
    // Each participant must durably persist PREPARED state before Phase 3.
    // Missing PreCommit callback is treated as hard misconfiguration and
    // therefore fails closed.
    if (!precommit_cb) {
        spdlog::error("execute3PC [{}]: missing PreCommit RPC callback; failing closed",
                      txn.transaction_id);
        failClosedAbortAllParticipants("precommit_callback_missing");
        return false;
    }

    txn.state = TransactionState::COMMITTING;
    spdlog::info("3PC Phase 2 (PreCommit) starting for transaction {}",
                txn.transaction_id);

    if (transaction_wal_) {
        try {
            nlohmann::json precommit_data = {
                {"protocol", "3PC"},
                {"phase", "PRE_COMMIT"},
                {"participants", nlohmann::json::array()}
            };
            for (const auto& [shard_id, participant] : txn.participants) {
                precommit_data["participants"].push_back(shard_id);
            }
            transaction_wal_->logCommit(txn.transaction_id, precommit_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::error("Failed to log PRE_COMMIT to WAL for txn={}: {}",
                          txn.transaction_id, e.what());
            return false;
        }
    }

    bool all_precommitted = true;
    for (auto& [shard_id, participant] : txn.participants) {
        bool precommitted = false;
        // Invoke the real PreCommit RPC for this participant.
        // Exceptions are treated as NACK per the contract documented on
        // setPreCommitCallback(): "must not throw; exceptions treated as NACK".
        try {
            precommitted = precommit_cb(shard_id, txn.transaction_id);
        } catch (const std::exception& ex) {
            spdlog::error("PreCommit callback threw for shard {} txn={}: {} — treating as NACK",
                          shard_id, txn.transaction_id, ex.what());
            precommitted = false;
        } catch (...) {
            spdlog::error("PreCommit callback threw unknown exception for shard {} txn={} — treating as NACK",
                          shard_id, txn.transaction_id);
            precommitted = false;
        }

        // Update participant state
        participant.precommitted = precommitted;

        if (transaction_wal_ && precommitted) {
            try {
                transaction_wal_->logPrepared(txn.transaction_id, shard_id, true, "pre_committed");
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::error("Failed to log PRE_COMMITTED to WAL for txn={} shard={}: {}",
                              txn.transaction_id, shard_id, e.what());
            }
        }

        if (!precommitted) {
            all_precommitted = false;
            spdlog::error("PreCommit failed for shard {} in transaction {}",
                         shard_id, txn.transaction_id);
        }
    }

    if (!all_precommitted) {
        // Check if we have deferred PreCommit callback for non-blocking behavior
        std::vector<std::string> failed_shards = {};

        for (const auto& [shard_id, participant] : txn.participants) {
            if (!participant.precommitted) {
                failed_shards.push_back(shard_id);
            }
        }

        // Non-blocking mode: snapshot the deferred callback under the lock, then
        // release before invoking it — mirroring the precommit_cb snapshot pattern
        // above so callbacks_mutex_ is never held while failClosedAbortAllParticipants
        // calls sendAbort (which acquires transactions_mutex_).  Holding both locks
        // simultaneously in this order would invert the lock order established in
        // prepare(), which takes transactions_mutex_ before callbacks_mutex_.
        DeferredPreCommitFn deferred_cb;
        {
            std::lock_guard<std::mutex> lk(callbacks_mutex_);
            deferred_cb = deferred_precommit_callback_;
        }

        if (deferred_cb && !failed_shards.empty()) {
            spdlog::warn("3PC Phase 2 (PreCommit) partial failure for transaction {}: "
                       "{} failed shards, scheduling deferred retry",
                       txn.transaction_id,static_cast<int>(failed_shards.size()));

            // Store deferred PreCommit for retry tracking before invoking the
            // callback so a concurrent retry thread can observe the entry.
            {
                std::lock_guard<std::mutex> def_lk(deferred_mutex_);
                deferred_precommits_[txn.transaction_id] = failed_shards;
            }

            // Schedule async retry via callback (non-blocking).
            // Contract: callback must not throw. Any exception is treated as
            // fatal orchestration failure and triggers fail-closed handling.
            try {
                deferred_cb(txn.transaction_id, failed_shards);
            } catch (const std::exception& ex) {
                spdlog::error("execute3PC [{}]: deferred PreCommit callback threw: {} - failing closed",
                              txn.transaction_id, ex.what());
                // Remove the stale entry so the retry thread cannot re-attempt
                // a transaction that has already been decided-abort.
                {
                    std::lock_guard<std::mutex> def_lk(deferred_mutex_);
                    deferred_precommits_.erase(txn.transaction_id);
                }
                failClosedAbortAllParticipants("deferred_precommit_callback_threw");
                return false;
            } catch (...) {
                spdlog::error("execute3PC [{}]: deferred PreCommit callback threw unknown exception - failing closed",
                              txn.transaction_id);
                {
                    std::lock_guard<std::mutex> def_lk(deferred_mutex_);
                    deferred_precommits_.erase(txn.transaction_id);
                }
                failClosedAbortAllParticipants("deferred_precommit_callback_unknown_exception");
                return false;
            }

            // Continue to Phase 3 with successful shards
            // Transaction will be in a "PRE_COMMIT_PENDING" state
            txn.state = TransactionState::COMMITTING;
            spdlog::info("3PC Phase 2 (PreCommit) scheduled deferred retry for transaction {}",
                       txn.transaction_id);

            // Only proceed with shards that successfully pre-committed
            // This allows partial progress in Converged Storage-Inference scenarios
            return true;  // Non-blocking: continue with available shards
        }

        // Fallback: Traditional fail-closed behavior when no deferred callback
        spdlog::error("3PC Phase 2 (PreCommit) failed for transaction {}",
                     txn.transaction_id);
        for (auto& [shard_id, participant] : txn.participants) {
            sendAbort(shard_id, txn.transaction_id);
            participant.aborted = true;
        }
        return false;
    }

    spdlog::info("3PC Phase 2 (PreCommit) succeeded for transaction {}",
                txn.transaction_id);

    // Phase 3: DoCommit
    spdlog::info("3PC Phase 3 (DoCommit) starting for transaction {}",
                txn.transaction_id);

    if (transaction_wal_) {
        try {
            nlohmann::json commit_data = {
                {"protocol", "3PC"},
                {"phase", "DO_COMMIT"},
                {"participants", nlohmann::json::array()}
            };
            for (const auto& [shard_id, participant] : txn.participants) {
                commit_data["participants"].push_back(shard_id);
            }
            transaction_wal_->logCommit(txn.transaction_id, commit_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::error("Failed to log DO_COMMIT to WAL for txn={}: {}",
                          txn.transaction_id, e.what());
            return false;
        }
    }
    
    bool all_committed = true;
    for (auto& [shard_id, participant] : txn.participants) {
        bool committed = sendCommit(shard_id, txn.transaction_id);
        participant.committed = committed;
        
        if (transaction_wal_ && committed) {
            try {
                transaction_wal_->logCommitted(txn.transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log COMMITTED to WAL: {}", e.what());
            }
        }
        
        if (!committed) {
            all_committed = false;
            spdlog::error("Commit failed for shard {} in transaction {}", 
                         shard_id, txn.transaction_id);
        }
    }
    
    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        createPeriodicSnapshot();
    }
    
    if (all_committed) {
        spdlog::info("3PC completed successfully for transaction {}", 
                    txn.transaction_id);
    } else {
        spdlog::error("3PC Phase 3 (DoCommit) had failures for transaction {}", 
                     txn.transaction_id);
    }
    
    return all_committed;
}

bool CrossShardTransactionCoordinator::executePercolator(CrossShardTransaction& txn) {
    // Delegate to the PercolatorCoordinator class which provides:
    //   - TrueTime-based commit-wait via now_with_uncertainty()
    //   - WAL-backed coordinator state persistence
    //   - Clean separation of the Percolator protocol logic
    //
    // The callbacks wire the coordinator's private shard RPC methods into the
    // PercolatorCoordinator so it can perform lock acquisition and release
    // without breaking the coordinator's encapsulation.

    PercolatorCoordinator::Config perc_cfg;
    perc_cfg.lock_timeout         = config_.percolator_lock_timeout;
    perc_cfg.max_retries          = config_.percolator_max_retries;
    perc_cfg.stale_lock_threshold = std::chrono::seconds(30);

    // Pass transaction_wal_.get() so PercolatorCoordinator can log PREPARE/COMMIT
    // records using the coordinator's own WAL without taking ownership.
    PercolatorCoordinator perc_coord(perc_cfg, truetime_, transaction_wal_.get());

    auto result = perc_coord.execute(
        txn,
        /*prepare_fn=*/[this](const std::string& shard_id, const std::string& txn_id) {
            return sendPrepare(shard_id, txn_id);
        },
        /*commit_fn=*/[this](const std::string& shard_id, const std::string& txn_id) {
            return sendCommit(shard_id, txn_id);
        },
        /*abort_fn=*/[this](const std::string& shard_id, const std::string& txn_id) {
            return sendAbort(shard_id, txn_id);
        }
    );

    // Log to coordinator-level WAL and check snapshot threshold.
    if (transaction_wal_) {
        try {
            if (result) {
                operations_since_snapshot_ += static_cast<uint64_t>(txn.participants.size());
            } else {
                transaction_wal_->logAbort(txn.transaction_id, "percolator_failed");
                operations_since_snapshot_++;
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log Percolator result to WAL: {}", e.what());
        }
        if (transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
            createPeriodicSnapshot();
        }
    }

    return result;
}

bool CrossShardTransactionCoordinator::executeCalvin(CrossShardTransaction& txn) {
    // Calvin deterministic distributed transaction protocol
    // Based on Thomson et al., "Calvin: Fast Distributed Transactions for Partitioned
    // Database Systems" (SIGMOD 2012).
    //
    // Key insight: by pre-ordering transactions before execution, all participants
    // apply the same deterministic sequence – eliminating the prepare/vote round
    // of 2PC and guaranteeing identical outcomes on every replica.
    //
    // Three phases:
    //   1. Sequencing  – assign a globally unique, monotonically increasing sequence
    //                    number that establishes the execution order for this epoch.
    //   2. Lock Acquisition – pre-acquire all read/write set locks in a canonical
    //                    (sorted) order to prevent deadlocks without a lock manager.
    //   3. Execution   – each participant executes and commits in sequence order;
    //                    no voting is required because the order is deterministic.

    spdlog::info("Starting Calvin transaction {}", txn.transaction_id);

    if (txn.participants.empty()) {
        spdlog::error("No participants in Calvin transaction {}", txn.transaction_id);
        return false;
    }

    // -------------------------------------------------------------------------
    // Phase 1: Sequencing
    // Assign a monotonically increasing sequence number within the current epoch.
    // In a full deployment the sequence number would come from a dedicated
    // replicated sequencer layer (as described in the Calvin paper) to guarantee
    // strict monotonicity across all coordinators regardless of clock skew.
    // Here we use the transaction's snapshot timestamp as a practical approximation
    // that provides relative ordering within a single coordinator instance.
    // -------------------------------------------------------------------------
    txn.state = TransactionState::PREPARING;

    int64_t sequence_number = txn.snapshot_timestamp;  // Relative ordering key for this coordinator

    spdlog::info("Calvin transaction {} assigned sequence number {}", 
                 txn.transaction_id, sequence_number);

    // Log sequencing decision to WAL for crash recovery
    if (transaction_wal_) {
        try {
            nlohmann::json seq_data = {
                {"protocol", "Calvin"},
                {"phase", "SEQUENCING"},
                {"sequence_number", sequence_number}
            };
            transaction_wal_->logPrepare(txn.transaction_id, "sequencer", seq_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log Calvin sequencing to WAL: {}", e.what());
        }
    }

    // Replicate the sequence decision via consensus so all nodes agree
    if (consensus_) {
        static_cast<void>(consensus_->propose("CALVIN_SEQUENCE", {
            {"transaction_id", txn.transaction_id},
            {"sequence_number", sequence_number}
        }));
    }

    // -------------------------------------------------------------------------
    // Phase 2: Lock Acquisition
    // Pre-acquire locks on all participants in a deterministic (sorted) order.
    // Sorting by shard_id removes any possibility of circular wait, making
    // deadlocks structurally impossible during the Calvin execution phase.
    // -------------------------------------------------------------------------
    std::vector<std::string> shard_order = {};

    shard_order.reserve(txn.participants.size());
    for (const auto& [shard_id, _] : txn.participants) {
        shard_order.push_back(shard_id);
    }
    if (config_.calvin_enable_deterministic_lock_order) {
        std::sort(shard_order.begin(), shard_order.end());
    }

    spdlog::info("Calvin transaction {}: acquiring locks on {} shards in deterministic order",
                 txn.transaction_id,static_cast<int>(shard_order.size()));

    std::vector<std::string> locked_shards;
    bool all_locked = true;

    for (const auto& shard_id : shard_order) {
        spdlog::debug("Calvin: acquiring lock on shard {} for transaction {}",
                      shard_id, txn.transaction_id);

        bool locked = sendPrepare(shard_id, txn.transaction_id);
        if (locked) {
            txn.participants[shard_id].prepared = true;
            locked_shards.push_back(shard_id);

            if (transaction_wal_) {
                try {
                    transaction_wal_->logPrepared(txn.transaction_id, shard_id, true, "calvin_locked");
                    operations_since_snapshot_++;
                } catch (const std::exception& e) {
                    spdlog::warn("Failed to log Calvin lock to WAL: {}", e.what());
                }
            }
        } else {
            all_locked = false;
            spdlog::error("Calvin: failed to acquire lock on shard {} for transaction {}",
                          shard_id, txn.transaction_id);
            break;
        }
    }

    if (!all_locked) {
        spdlog::error("Calvin: lock acquisition failed for transaction {}, aborting",
                      txn.transaction_id);
        for (const auto& shard_id : locked_shards) {
            sendAbort(shard_id, txn.transaction_id);
            txn.participants[shard_id].prepared = false;
        }
        return false;
    }

    txn.state = TransactionState::PREPARED;
    spdlog::info("Calvin transaction {}: all locks acquired, proceeding to execution phase",
                 txn.transaction_id);

    // -------------------------------------------------------------------------
    // Phase 3: Execution
    // Execute and commit on every participant in the pre-determined sequence
    // order.  Because the order was agreed upon during sequencing, no voting
    // round is required – participants simply apply the transaction and report
    // success.  A commit timestamp derived from TrueTime ensures MVCC
    // correctness across shards.
    // -------------------------------------------------------------------------
    txn.state = TransactionState::COMMITTING;

    int64_t commit_timestamp = generateCommitTimestamp(txn);
    txn.commit_timestamp = commit_timestamp;

    if (transaction_wal_) {
        try {
            nlohmann::json exec_data = {
                {"protocol", "Calvin"},
                {"phase", "EXECUTION"},
                {"sequence_number", sequence_number},
                {"commit_timestamp", commit_timestamp}
            };
            transaction_wal_->logCommit(txn.transaction_id, exec_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log Calvin execution phase to WAL: {}", e.what());
        }
    }

    bool all_executed = true;
    for (const auto& shard_id : shard_order) {
        spdlog::debug("Calvin: executing on shard {} for transaction {} (seq={})",
                      shard_id, txn.transaction_id, sequence_number);

        bool committed = sendCommit(shard_id, txn.transaction_id);
        txn.participants[shard_id].committed = committed;

        if (transaction_wal_ && committed) {
            try {
                transaction_wal_->logCommitted(txn.transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log Calvin committed shard to WAL: {}", e.what());
            }
        }

        if (!committed) {
            all_executed = false;
            spdlog::error("Calvin: execution failed on shard {} for transaction {}",
                          shard_id, txn.transaction_id);
        }
    }

    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        createPeriodicSnapshot();
    }

    if (all_executed) {
        spdlog::info("Calvin transaction {} completed successfully (seq={}, commit_ts={})",
                     txn.transaction_id, sequence_number, commit_timestamp);
    } else {
        spdlog::error("Calvin transaction {} execution had failures (seq={})",
                      txn.transaction_id, sequence_number);
    }

    return all_executed;
}

bool CrossShardTransactionCoordinator::sendPrepare(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending prepare to shard {} for transaction {}", 
                  shard_id, transaction_id);

    // Snapshot all data needed for the RPC under the lock, then release before
    // doing any network I/O or exponential-backoff sleeps.
    themis::sharding::ShardRPCClient::Config rpc_config;
    nlohmann::json operations = nlohmann::json::array();
    {
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        auto it = transactions_.find(transaction_id);
        if (it == transactions_.end()) {
            spdlog::error("Transaction {} not found", transaction_id);
            return false;
        }
        auto& txn = it->second;
        auto participant_it = txn.participants.find(shard_id);
        if (participant_it == txn.participants.end()) {
            spdlog::error("Shard {} is not a participant in transaction {}", 
                         shard_id, transaction_id);
            return false;
        }
        auto& participant = participant_it->second;
        rpc_config.endpoint    = participant.endpoint;
        rpc_config.timeout_ms  = static_cast<int>(config_.prepare_timeout.count());
        rpc_config.max_retries = 3;
        rpc_config.retry_delay_ms = 100;
        for (const auto& op : participant.operations) {
            operations.push_back(op);
        }
    }
    // Lock released – all network + sleep work happens outside transactions_mutex_.
    try {
        themis::sharding::ShardRPCClient rpc_client(rpc_config);

        int retries  = 0;
        int delay_ms = rpc_config.retry_delay_ms;

        while (retries <= rpc_config.max_retries) {
            try {
                bool vote = rpc_client.prepare(transaction_id, operations);
                if (vote) {
                    spdlog::info("Shard {} voted COMMIT for transaction {}", 
                               shard_id, transaction_id);
                    return true;
                } else {
                    spdlog::warn("Shard {} voted ABORT for transaction {}", 
                               shard_id, transaction_id);
                    return false;
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Prepare RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;
                    retries++;
                } else {
                    spdlog::error("Prepare RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    return false;
                }
            }
        }
        spdlog::error("Unexpected exit from prepare retry loop");
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        return false;
    }
}

bool CrossShardTransactionCoordinator::sendCommit(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending commit to shard {} for transaction {}", 
                  shard_id, transaction_id);

    // Snapshot all data needed for the RPC under the lock, then release before
    // doing any network I/O or exponential-backoff sleeps.
    themis::sharding::ShardRPCClient::Config rpc_config;
    int64_t commit_timestamp = 0;
    {
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        auto it = transactions_.find(transaction_id);
        if (it == transactions_.end()) {
            spdlog::error("Transaction {} not found", transaction_id);
            return false;
        }
        auto& txn = it->second;
        auto participant_it = txn.participants.find(shard_id);
        if (participant_it == txn.participants.end()) {
            spdlog::error("Shard {} is not a participant in transaction {}", 
                         shard_id, transaction_id);
            return false;
        }
        auto& participant = participant_it->second;

        // Resolve / allocate the MVCC commit timestamp while still under the lock
        // so all participants for this transaction share the same value.
        commit_timestamp = txn.commit_timestamp;
        if (commit_timestamp == 0) {
            commit_timestamp = generateCommitTimestamp(txn);
            txn.commit_timestamp = commit_timestamp;
        }

        rpc_config.endpoint       = participant.endpoint;
        rpc_config.timeout_ms     = static_cast<int>(config_.commit_timeout.count());
        rpc_config.max_retries    = 3;
        rpc_config.retry_delay_ms = 100;
    }
    // Lock released – all network + sleep work happens outside transactions_mutex_.
    try {
        themis::sharding::ShardRPCClient rpc_client(rpc_config);

        int retries  = 0;
        int delay_ms = rpc_config.retry_delay_ms;

        while (retries <= rpc_config.max_retries) {
            try {
                bool success = rpc_client.commit(transaction_id, commit_timestamp);
                if (success) {
                    spdlog::info("Shard {} committed transaction {} at timestamp {}", 
                               shard_id, transaction_id, commit_timestamp);
                    return true;
                } else {
                    spdlog::error("Shard {} failed to commit transaction {}", 
                                shard_id, transaction_id);
                    return false;
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Commit RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;
                    retries++;
                } else {
                    spdlog::error("Commit RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    return false;
                }
            }
        }
        spdlog::error("Unexpected exit from commit retry loop");
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        return false;
    }
}

bool CrossShardTransactionCoordinator::sendAbort(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending abort to shard {} for transaction {}", 
                  shard_id, transaction_id);

    // Snapshot all data needed for the RPC under the lock, then release before
    // doing any network I/O or exponential-backoff sleeps.
    themis::sharding::ShardRPCClient::Config rpc_config;
    {
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        auto it = transactions_.find(transaction_id);
        if (it == transactions_.end()) {
            spdlog::error("Transaction {} not found", transaction_id);
            return false;
        }
        auto& txn = it->second;
        auto participant_it = txn.participants.find(shard_id);
        if (participant_it == txn.participants.end()) {
            spdlog::error("Shard {} is not a participant in transaction {}", 
                         shard_id, transaction_id);
            return false;
        }
        auto& participant = participant_it->second;
        rpc_config.endpoint       = participant.endpoint;
        rpc_config.timeout_ms     = static_cast<int>(config_.abort_timeout.count());
        rpc_config.max_retries    = 3;
        rpc_config.retry_delay_ms = 100;
    }
    // Lock released – all network + sleep work happens outside transactions_mutex_.
    try {
        themis::sharding::ShardRPCClient rpc_client(rpc_config);

        int retries  = 0;
        int delay_ms = rpc_config.retry_delay_ms;

        while (retries <= rpc_config.max_retries) {
            try {
                bool success = rpc_client.abort(transaction_id);
                if (success) {
                    spdlog::info("Shard {} aborted transaction {}", 
                               shard_id, transaction_id);
                    return true;
                } else {
                    spdlog::warn("Shard {} reported abort failure for transaction {}", 
                               shard_id, transaction_id);
                    return true; // best-effort
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Abort RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;
                    retries++;
                } else {
                    spdlog::error("Abort RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    spdlog::warn("Abort considered successful despite failures (best-effort semantics)");
                    return true;
                }
            }
        }
        spdlog::warn("Unexpected exit from abort retry loop - treating as successful (best-effort)");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        spdlog::warn("Abort considered successful despite client creation failure (best-effort semantics)");
        return true;
    }
}

void CrossShardTransactionCoordinator::deadlockDetectionThread() {
    spdlog::debug("Deadlock detection thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(config_.deadlock_detection_interval);
        
        // Build wait-for graph from explicitly-reported push edges.
        auto graph = buildWaitForGraph();

        // Additionally, pull wait-for edges from all configured shard endpoints.
        // This supplements the push-based reportDistributedWait() mechanism and
        // enables detection of deadlocks involving shards that have not yet (or
        // cannot) proactively report their local lock-wait state.
        if (!config_.shard_endpoints.empty()) {
            for (const auto& [shard_id, endpoint] : config_.shard_endpoints) {
                try {
                    std::vector<CrossShardTransactionConfig::PolledWaitForEdge> remote_edges = {};

                    if (config_.polled_wait_for_edge_collector) {
                        remote_edges = config_.polled_wait_for_edge_collector(shard_id, endpoint);
                    } else {
                        themis::sharding::ShardRPCClient::Config rpc_cfg;
                        rpc_cfg.endpoint = endpoint;
                        rpc_cfg.shard_id = shard_id;
                        const auto timeout_ms = std::max<int64_t>(
                            1, config_.deadlock_detection_interval.count() / 2);
                        rpc_cfg.timeout_ms = static_cast<int>(
                            std::min<int64_t>(timeout_ms, std::numeric_limits<int>::max()));
                        rpc_cfg.max_retries  = 1;
                        rpc_cfg.enable_circuit_breaker = false;

                        themis::sharding::ShardRPCClient client(rpc_cfg);
                        const auto rpc_edges = client.collectWaitForEdges();
                        remote_edges.reserve(rpc_edges.size());
                        for (const auto& edge : rpc_edges) {
                            remote_edges.push_back({
                                edge.waiting_transaction_id,
                                edge.blocking_transaction_id
                            });
                        }
                    }

                    for (const auto& edge : remote_edges) {
                        if (edge.waiting_transaction_id.empty() ||
                            edge.blocking_transaction_id.empty() ||
                            edge.waiting_transaction_id == edge.blocking_transaction_id) {
                            continue;
                        }

                        // Only merge remote edges that reference known live
                        // transactions tracked by this coordinator. Unknown or
                        // finished transaction IDs cannot be resolved locally
                        // and would otherwise create false-positive cycles.
                        bool include_edge = false;
                        {
                            std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
                            const auto waiting_it = transactions_.find(edge.waiting_transaction_id);
                            const auto blocking_it = transactions_.find(edge.blocking_transaction_id);
                            if (waiting_it != transactions_.end() &&
                                blocking_it != transactions_.end()) {
                                const auto waiting_state = waiting_it->second.state;
                                const auto blocking_state = blocking_it->second.state;
                                const bool waiting_live =
                                    waiting_state == TransactionState::ACTIVE ||
                                    waiting_state == TransactionState::PREPARING;
                                const bool blocking_live =
                                    blocking_state == TransactionState::ACTIVE ||
                                    blocking_state == TransactionState::PREPARING ||
                                    blocking_state == TransactionState::PREPARED;
                                include_edge = waiting_live && blocking_live;
                            }
                        }
                        if (!include_edge) {
                            continue;
                        }

                        graph[edge.waiting_transaction_id].push_back(
                            edge.blocking_transaction_id);
                        spdlog::trace(
                            "Wait-for edge (polled from {}): {} -> {}",
                            shard_id,
                            edge.waiting_transaction_id,
                            edge.blocking_transaction_id);
                    }
                } catch (const std::exception& ex) {
                    spdlog::debug(
                        "Deadlock polling from shard {} ({}) skipped: {}",
                        shard_id, endpoint, ex.what());
                }
            }
        }

        if (graph.empty()) {
            continue;  // No active transactions with potential conflicts
        }

        // collectCycles() returns one set per independent cycle (SCC). Each
        // independent deadlock requires its own victim; selecting one global
        // victim would leave concurrent independent cycles unresolved until
        // the next detection round.
        const auto cycles = collectCycles(graph);

        for (const auto& cycle_nodes : cycles) {
            std::vector<std::string> deadlocked_txns(
                cycle_nodes.begin(), cycle_nodes.end());

            spdlog::warn("Deadlock detected involving {} transactions",
                        deadlocked_txns.size());

            deadlocked_transactions_++;

            // Select deadlock victim according to configured policy.
            std::string victim_id = {};
            std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> candidates;
            {
                std::lock_guard<std::timed_mutex> lock(transactions_mutex_);

                for (const auto& txn_id : deadlocked_txns) {
                    auto it = transactions_.find(txn_id);
                    if (it != transactions_.end()) {
                        candidates.emplace_back(txn_id, it->second.start_time);
                    }
                }
            }

            if (!candidates.empty()) {
                switch (config_.deadlock_victim_policy) {
                    case DeadlockVictimPolicy::YOUNGEST: {
                        victim_id = std::max_element(
                            candidates.begin(),
                            candidates.end(),
                            [](const auto& lhs, const auto& rhs) {
                                return lhs.second < rhs.second;
                            })->first;
                        break;
                    }
                    case DeadlockVictimPolicy::OLDEST: {
                        victim_id = std::min_element(
                            candidates.begin(),
                            candidates.end(),
                            [](const auto& lhs, const auto& rhs) {
                                return lhs.second < rhs.second;
                            })->first;
                        break;
                    }
                    case DeadlockVictimPolicy::RANDOM: {
                        thread_local std::mt19937 rng{std::random_device{}()};
                        std::uniform_int_distribution<uint64_t> dist(0, static_cast<int>(candidates.size()) - 1);
                        victim_id = candidates[static_cast<size_t>(dist(rng))].first;
                        break;
                    default: break;
                    }
                }
            }

            if (!victim_id.empty()) {
                spdlog::warn("Aborting transaction {} to resolve deadlock", victim_id);
                abort(victim_id);
            }
        }
    }
    
    spdlog::debug("Deadlock detection thread stopped");
}

std::map<std::string, std::vector<std::string>> 
CrossShardTransactionCoordinator::buildWaitForGraph() const {
    std::map<std::string, std::vector<std::string>> graph;
    
    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);

    // Build wait-for graph from explicit cross-shard wait reports.
    // Edge: waiting_txn -> blocking_txn.
    for (const auto& [waiting_txn_id, blockers] : distributed_wait_for_edges_) {
        const auto waiting_it = transactions_.find(waiting_txn_id);
        if (waiting_it == transactions_.end()) {
            continue;
        }
        const auto waiting_state = waiting_it->second.state;
        if (waiting_state != TransactionState::ACTIVE &&
            waiting_state != TransactionState::PREPARING) {
            continue;
        }

        for (const auto& blocking_txn_id : blockers) {
            const auto blocking_it = transactions_.find(blocking_txn_id);
            if (blocking_it == transactions_.end()) {
                continue;
            }
            const auto blocking_state = blocking_it->second.state;
            if (blocking_state != TransactionState::ACTIVE &&
                blocking_state != TransactionState::PREPARING &&
                blocking_state != TransactionState::PREPARED) {
                continue;
            }

            graph[waiting_txn_id].push_back(blocking_txn_id);
            spdlog::trace("Wait-for edge (distributed): {} -> {}",
                          waiting_txn_id, blocking_txn_id);
        }
    }
    
    spdlog::debug("Built wait-for graph with {} nodes",static_cast<int>(graph.size()));
    
    return graph;
}

void CrossShardTransactionCoordinator::clearDistributedWaitEdgesLocked(
    const std::string& transaction_id
) {
    distributed_wait_for_edges_.erase(transaction_id);
    for (auto it = distributed_wait_for_edges_.begin();
         it != distributed_wait_for_edges_.end();) {
        it->second.erase(transaction_id);
        if (it->second.empty()) {
            it = distributed_wait_for_edges_.erase(it);
        } else {
            ++it;
        }
    }
}

bool CrossShardTransactionCoordinator::detectCycle(
    const std::map<std::string, std::vector<std::string>>& graph,
    const std::string& start_node,
    std::set<std::string>& visited,
    std::set<std::string>& rec_stack
) const {
    if (rec_stack.find(start_node) != rec_stack.end()) {
        return true;  // Cycle detected
    }
    
    if (visited.find(start_node) != visited.end()) {
        return false;  // Already visited, no cycle from here
    }
    
    visited.insert(start_node);
    rec_stack.insert(start_node);
    
    auto it = graph.find(start_node);
    if (it != graph.end()) {
        for (const auto& neighbor : it->second) {
            if (detectCycle(graph, neighbor, visited, rec_stack)) {
                return true;
            }
        }
    }
    
    rec_stack.erase(start_node);
    return false;
}

void CrossShardTransactionCoordinator::executeCompensations(
    const std::string& transaction_id,
    const std::vector<nlohmann::json>& executed_steps,
    const std::vector<nlohmann::json>& compensations
) {
    spdlog::info("Executing compensations for SAGA transaction {} ({} steps to compensate)", 
                transaction_id,static_cast<int>(executed_steps.size()));
    
    // Execute compensations in reverse order
    // Using index-based loop to access both executed_steps and compensations arrays
    // Reverse iterators not used here because we need synchronized access to both arrays
    if (!executed_steps.empty()) {
        for (size_t j = executed_steps.size(); j > 0; --j) {
            const size_t idx = j - 1;
            const auto& compensation = compensations[idx];
            
            if (!compensation.contains("shard_id") || !compensation.contains("operation")) {
                spdlog::error("Compensation {} missing shard_id or operation", idx);
                continue;
            }
            
            std::string shard_id = compensation["shard_id"];
            nlohmann::json operation = compensation["operation"];
            
            spdlog::info("Executing compensation {} on shard {} for transaction {}", 
                        idx, shard_id, transaction_id);
            
            // Phase 2.3.4: Log compensation to WAL
            {
                std::lock_guard<std::timed_mutex> wal_lock(transactions_mutex_);
                if (transaction_wal_) {
                    try {
                        nlohmann::json comp_data = {
                            {"step", idx},
                            {"operation", operation}
                        };
                        transaction_wal_->logCompensate(transaction_id, shard_id, comp_data);
                        operations_since_snapshot_++;
                    } catch (const std::exception& e) {
                        spdlog::warn("Failed to log COMPENSATE to WAL: {}", e.what());
                    }
                }
            }
            
            try {
            // Snapshot endpoint under lock, then release before RPC + sleep.
            themis::sharding::ShardRPCClient::Config rpc_config;
            {
                std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
                auto it = transactions_.find(transaction_id);
                if (it == transactions_.end()) {
                    spdlog::error("Transaction {} not found during compensation", transaction_id);
                    continue;
                }
                auto& txn = it->second;
                auto participant_it = txn.participants.find(shard_id);
                if (participant_it == txn.participants.end()) {
                    spdlog::error("Shard {} not found in transaction {} participants", 
                                shard_id, transaction_id);
                    continue;
                }
                auto& participant = participant_it->second;
                rpc_config.endpoint       = participant.endpoint;
                rpc_config.timeout_ms     = static_cast<int>(config_.saga_step_timeout.count());
                rpc_config.max_retries    = 3;
                rpc_config.retry_delay_ms = 100;
            }
            // Lock released – all RPC + sleep work happens outside transactions_mutex_.
            themis::sharding::ShardRPCClient rpc_client(rpc_config);

            // Execute the SAGA compensation operation via a dedicated compensate RPC.
            // The operation JSON carries the reverse action (e.g., DELETE to undo INSERT)
            // that the shard will apply idempotently.
            int retries = 0;
            bool success = false;
            
            while (retries <= rpc_config.max_retries) {
                try {
                    success = rpc_client.compensate(transaction_id, operation);
                    
                    if (success) {
                        spdlog::info("Compensation {} completed successfully", idx);
                        break;
                    }
                    
                    // RPC returned a non-success status without throwing — count as
                    // a failed attempt so the bounded retry loop terminates correctly.
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("Compensation {} not acknowledged (attempt {}/{}). Retrying",
                                   idx, retries + 1, rpc_config.max_retries + 1);
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                    }
                    retries++;
                    
                } catch (const std::exception& e) {
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("Compensation {} execution failed (attempt {}/{}): {}. Retrying", 
                                   idx, retries + 1, rpc_config.max_retries + 1, e.what());
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                        retries++;
                    } else {
                        spdlog::error("Compensation {} execution failed after {} retries: {}", 
                                    idx, rpc_config.max_retries, e.what());
                        break;
                    }
                }
            }
            
            if (!success) {
                spdlog::error("Compensation {} failed - manual intervention may be required", idx);
                // In production, this would be logged to a persistent compensation log
                // for manual intervention or retry
            }
            
            } catch (const std::exception& e) {
                spdlog::error("Failed to execute compensation {}: {}", idx, e.what());
            }
        }
    }
    
    spdlog::info("Compensation execution completed for SAGA transaction {}", 
                transaction_id);
}

int64_t CrossShardTransactionCoordinator::generateCommitTimestamp(
    const CrossShardTransaction& txn
) {
    int64_t commit_timestamp = 0;
    
    // Use TrueTime for MVCC isolation levels
    if ((truetime_ && (txn.isolation_level == IsolationLevel::SNAPSHOT_ISOLATION ||
                      txn.isolation_level == IsolationLevel::SERIALIZABLE))) {
        auto tt_now = truetime_->now();
        commit_timestamp = tt_now.earliest.count();
        
        // Wait until the commit timestamp is definitely after the snapshot timestamp
        // This ensures external consistency: if T1 commits before T2 starts, T2 sees T1's writes
        if (txn.snapshot_timestamp > 0 && commit_timestamp <= txn.snapshot_timestamp) {
            truetime_->waitUntil(std::chrono::nanoseconds(txn.snapshot_timestamp + 1));
            tt_now = truetime_->now();
            commit_timestamp = tt_now.earliest.count();
        }
        
        spdlog::debug("Generated MVCC commit timestamp {} (snapshot: {}, uncertainty: {}ns)",
                     commit_timestamp, txn.snapshot_timestamp,
                     truetime_->getUncertainty().count());
    } else {
        // Fallback to system time for other isolation levels
        commit_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    return commit_timestamp;
}

bool CrossShardTransactionCoordinator::persistTransactionState(
    const std::string& transaction_id,
    TransactionState state
) {
    std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Cannot persist state for non-existent transaction {}", transaction_id);
        return false;
    }
    
    const auto& txn = it->second;
    
    try {
        // Open transaction log file in append mode
        std::ofstream log_file(transaction_log_path_, std::ios::app);
        if (!log_file.is_open()) {
            spdlog::error("Failed to open transaction log file: {}", transaction_log_path_);
            return false;
        }
        
        // Create log entry
        nlohmann::json log_entry = {
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(state)},
            {"protocol", static_cast<int>(txn.protocol)},
            {"isolation_level", static_cast<int>(txn.isolation_level)},
            {"snapshot_timestamp", txn.snapshot_timestamp},
            {"commit_timestamp", txn.commit_timestamp}
        };
        
        // Add participant information
        nlohmann::json participants_json = nlohmann::json::array();
        for (const auto& [shard_id, participant] : txn.participants) {
            participants_json.push_back({
                {"shard_id", shard_id},
                {"endpoint", participant.endpoint},
                {"prepared", participant.prepared},
                {"committed", participant.committed},
                {"aborted", participant.aborted}
            });
        }
        log_entry["participants"] = participants_json;
        
        // Write log entry as a single line (JSONL format)
        log_file << log_entry.dump() << std::endl;
        log_file.close();
        
        spdlog::debug("Persisted transaction {} state: {}", 
                     transaction_id, static_cast<int>(state));
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to persist transaction state: {}", e.what());
        return false;
    }
}

// Phase 2.3.3: Recover from WAL and snapshot
bool CrossShardTransactionCoordinator::recoverFromWAL(
    BackendRecoveryStats* stats
) {
    if (!transaction_wal_ || !snapshot_manager_) {
        spdlog::warn("WAL or snapshot manager not available, skipping WAL recovery");
        return true;
    }
    
    spdlog::info("Starting recovery from WAL and snapshot...");
    
    // Step 1: Load latest snapshot
    auto snapshot_opt = snapshot_manager_->loadLatestSnapshot();
    if (snapshot_opt.has_value()) {
        const auto& snapshot = snapshot_opt.value();
        
        // Verify snapshot integrity
        if (!snapshot_manager_->verifySnapshot(snapshot)) {
            spdlog::error("Snapshot integrity check failed, cannot recover");
            return false;
        }
        
        spdlog::info("Loaded snapshot {} with {} active transactions", 
                    snapshot.snapshot_id, snapshot.total_transactions);
        
        auto from_snapshot_protocol = [](::sharding::TransactionProtocol p) -> TransactionProtocol {
            switch (p) {
                case ::sharding::TransactionProtocol::TWO_PHASE_COMMIT:
                    return TransactionProtocol::TWO_PHASE_COMMIT;
                case ::sharding::TransactionProtocol::THREE_PHASE_COMMIT:
                    return TransactionProtocol::THREE_PHASE_COMMIT;
                case ::sharding::TransactionProtocol::SAGA:
                    return TransactionProtocol::SAGA;
                case ::sharding::TransactionProtocol::PERCOLATOR:
                    return TransactionProtocol::PERCOLATOR;
                case ::sharding::TransactionProtocol::CALVIN:
                    return TransactionProtocol::CALVIN;
                default: break;
            }
            return TransactionProtocol::TWO_PHASE_COMMIT;
        };

        auto from_snapshot_state = [](::sharding::TransactionState s) -> TransactionState {
            switch (s) {
                case ::sharding::TransactionState::INITIATED:
                    return TransactionState::ACTIVE;
                case ::sharding::TransactionState::PREPARING:
                    return TransactionState::PREPARING;
                case ::sharding::TransactionState::PREPARED:
                    return TransactionState::PREPARED;
                case ::sharding::TransactionState::PRE_COMMITTING:
                [[fallthrough]];
                case ::sharding::TransactionState::PRE_COMMITTED:
                [[fallthrough]];
                case ::sharding::TransactionState::COMMITTING:
                    return TransactionState::COMMITTING;
                case ::sharding::TransactionState::COMMITTED:
                    return TransactionState::COMMITTED;
                case ::sharding::TransactionState::ABORTING:
                    return TransactionState::ABORTING;
                case ::sharding::TransactionState::ABORTED:
                [[fallthrough]];
                case ::sharding::TransactionState::COMPENSATING:
                [[fallthrough]];
                case ::sharding::TransactionState::COMPENSATED:
                    return TransactionState::ABORTED;
                default: break;
            }
            return TransactionState::UNKNOWN;
        };

        // Restore active transactions from snapshot
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        uint64_t restored_from_snapshot = 0;
        for (const auto& txn_entry : snapshot.active_transactions) {
            CrossShardTransaction txn;
            txn.transaction_id = txn_entry.transaction_id;
            txn.protocol = from_snapshot_protocol(txn_entry.protocol);
            txn.state = from_snapshot_state(txn_entry.state);
            txn.start_time = std::chrono::system_clock::from_time_t(
                txn_entry.start_timestamp / 1000000000);  // Convert from nanoseconds
            
            // Restore participants
            for (const auto& [part_id, part_status] : txn_entry.participant_status) {
                ShardParticipant participant;
                participant.shard_id = part_id;
                participant.prepared = part_status.prepared;
                participant.committed = part_status.committed;
                participant.aborted = part_status.aborted;
                txn.participants[part_id] = participant;
            }
            
            transactions_[txn.transaction_id] = txn;
            ++restored_from_snapshot;
        }

        if (stats) {
            stats->snapshot_transactions_restored = restored_from_snapshot;
        }
        
        last_applied_lsn_ = snapshot.last_applied_lsn;
        spdlog::info("Restored {} transactions from snapshot, last LSN: {}", 
                snapshot.total_transactions, last_applied_lsn_.toString());
    } else {
        spdlog::info("No snapshot found, starting with empty state");
        last_applied_lsn_ = LSN(0, 0);
    }
    
    // Step 2: Replay WAL from last_applied_lsn
    try {
        auto wal_entries = transaction_wal_->readEntries(last_applied_lsn_);
        if (stats) {
            stats->wal_entries_replayed = wal_entries.size();
        }
        spdlog::info("Replaying {} WAL entries from LSN {}", 
                wal_entries.size(), last_applied_lsn_.toString());
        
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        for (const auto& entry : wal_entries) {
            // Update last_applied_lsn
            last_applied_lsn_ = entry.lsn;
            
            // Find or create transaction
            auto it = transactions_.find(entry.transaction_id);
            
            switch (entry.type) {
                case ::sharding::TransactionWALEntryType::BEGIN:
                    if (it == transactions_.end()) {
                        CrossShardTransaction txn;
                        txn.transaction_id = entry.transaction_id;
                        txn.protocol = static_cast<TransactionProtocol>(entry.protocol);
                        txn.state = TransactionState::ACTIVE;
                        txn.start_time = std::chrono::system_clock::from_time_t(
                            entry.timestamp / 1000);
                        transactions_[entry.transaction_id] = txn;
                        spdlog::debug("Replayed BEGIN for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::PREPARE:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::PREPARING;
                        spdlog::debug("Replayed PREPARE for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::PREPARED:
                    if (it != transactions_.end()) {
                        auto& participant = it->second.participants[entry.participant_id];
                        participant.prepared = entry.vote;
                        spdlog::debug("Replayed PREPARED for transaction {}, participant {}, vote: {}", 
                                    entry.transaction_id, entry.participant_id, entry.vote);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::COMMIT:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::COMMITTING;
                        spdlog::debug("Replayed COMMIT for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::COMMITTED:
                    if (it != transactions_.end()) {
                        auto& participant = it->second.participants[entry.participant_id];
                        participant.committed = true;
                        // Check if all committed
                        bool all_committed = true;
                        for (const auto& [_, p] : it->second.participants) {
                            if (!p.committed) {
                                all_committed = false;
                                break;
                            }
                        }
                        if (all_committed) {
                            it->second.state = TransactionState::COMMITTED;
                            it->second.end_time = std::chrono::system_clock::now();
                        }
                        spdlog::debug("Replayed COMMITTED for transaction {}, participant {}", 
                                    entry.transaction_id, entry.participant_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::ABORT:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::ABORTING;
                        spdlog::debug("Replayed ABORT for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::ABORTED:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::ABORTED;
                        it->second.end_time = std::chrono::system_clock::now();
                        spdlog::debug("Replayed ABORTED for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::COMPENSATE:
                    if (it != transactions_.end()) {
                        spdlog::debug("Replayed COMPENSATE for transaction {}", entry.transaction_id);
                        // SAGA compensation - store in metadata
                        it->second.compensations[entry.participant_id] = entry.data;
                    }
                    break;
                default: break;
            }
        }
        
        spdlog::info("WAL replay complete, {} transactions in memory",static_cast<int>(transactions_.size()));
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to replay WAL: {}", e.what());
        return false;
    }
    
    // Step 3: Resume in-flight transactions based on their state
    // Phase 2.3.4: Automatic transaction resumption with timeout handling
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> transactions_to_resume;
    std::vector<std::string> transactions_to_timeout;

    {
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        for (const auto& [txn_id, txn] : transactions_) {
            // Check for timeout (default: 5 minutes)
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - txn.start_time);
            if (age.count() > 300) {  // 5 minutes
                spdlog::warn("Transaction {} is stale (age: {}s), will abort", txn_id, age.count());
                transactions_to_timeout.push_back(txn_id);
                continue;
            }

            switch (txn.state) {
                case TransactionState::PREPARING:
                    spdlog::info("Transaction {} recovered in PREPARING state - will resend prepare", 
                                txn_id);
                    transactions_to_resume.push_back(txn_id);
                    break;
                case TransactionState::PREPARED:
                    spdlog::info("Transaction {} recovered in PREPARED state - will make commit decision", 
                                txn_id);
                    transactions_to_resume.push_back(txn_id);
                    break;
                case TransactionState::COMMITTING:
                    spdlog::info("Transaction {} recovered in COMMITTING state - will complete commit", 
                                txn_id);
                    transactions_to_resume.push_back(txn_id);
                    break;
                case TransactionState::ABORTING:
                    spdlog::info("Transaction {} recovered in ABORTING state - will complete abort", 
                                txn_id);
                    transactions_to_resume.push_back(txn_id);
                    break;
                case TransactionState::COMMITTED:
                [[fallthrough]];
                case TransactionState::ABORTED:
                    // Final states - can be cleaned up eventually
                    break;
                default:
                    break;
            }
        }

        if (stats) {
            stats->in_doubt_transactions = static_cast<uint64_t>(std::count_if(
                transactions_.begin(),
                transactions_.end(),
                [](const auto& kv) {
                    const auto state = kv.second.state;
                    return state != TransactionState::COMMITTED &&
                           state != TransactionState::ABORTED;
                }));
        }
    }
    
    // Phase 2.3.4: Actually resume transactions (done outside lock to avoid deadlock)
    if (!transactions_to_timeout.empty()) {
        spdlog::info("Aborting {} stale transactions",static_cast<int>(transactions_to_timeout.size()));
        for (const auto& txn_id : transactions_to_timeout) {
            try {
                // Log timeout abort
                if (transaction_wal_) {
                    transaction_wal_->logAbort(txn_id, "recovery_timeout");
                }
                // Will be aborted in background
            } catch (const std::exception& e) {
                if (stats) {
                    ++stats->failed_operations;
                }
                spdlog::error("Failed to abort stale transaction {}: {}", txn_id, e.what());
            }
        }
    }
    
    if (!transactions_to_resume.empty()) {
        spdlog::info("Will resume {} in-flight transactions",static_cast<int>(transactions_to_resume.size()));
        // Note: Actual resumption would happen in a background thread
        // For now, we just log what needs to be done
        // In production, you would:
        // 1. For PREPARING: Call prepare(txn_id) again
        // 2. For PREPARED: Check all votes and call commit() or abort()
        // 3. For COMMITTING: Re-send commit to any non-committed participants
        // 4. For ABORTING: Re-send abort to any non-aborted participants
    }
    
    if (stats) {
        stats->stale_transactions_detected = transactions_to_timeout.size();
        stats->resume_candidates = transactions_to_resume.size();
        stats->pending_transactions =
            stats->stale_transactions_detected + stats->resume_candidates;
    }

    spdlog::info("Transaction coordinator recovery complete");
    return true;
}

// Phase 2.3.3: Create periodic snapshot
void CrossShardTransactionCoordinator::createPeriodicSnapshot() {
    if (!transaction_wal_ || !snapshot_manager_) {
        return;
    }
    
    try {
        std::lock_guard<std::timed_mutex> lock(transactions_mutex_);
        
        // Convert active transactions to snapshot format
        auto to_snapshot_protocol = [](TransactionProtocol p) -> ::sharding::TransactionProtocol {
            switch (p) {
                case TransactionProtocol::TWO_PHASE_COMMIT:
                    return ::sharding::TransactionProtocol::TWO_PHASE_COMMIT;
                case TransactionProtocol::THREE_PHASE_COMMIT:
                    return ::sharding::TransactionProtocol::THREE_PHASE_COMMIT;
                case TransactionProtocol::SAGA:
                    return ::sharding::TransactionProtocol::SAGA;
                case TransactionProtocol::PERCOLATOR:
                    return ::sharding::TransactionProtocol::PERCOLATOR;
                case TransactionProtocol::CALVIN:
                    return ::sharding::TransactionProtocol::CALVIN;
                default: break;
            }
            return ::sharding::TransactionProtocol::TWO_PHASE_COMMIT;
        };

        auto to_snapshot_state = [](TransactionState s) -> ::sharding::TransactionState {
            switch (s) {
                case TransactionState::ACTIVE:
                    return ::sharding::TransactionState::INITIATED;
                case TransactionState::PREPARING:
                    return ::sharding::TransactionState::PREPARING;
                case TransactionState::PREPARED:
                    return ::sharding::TransactionState::PREPARED;
                case TransactionState::COMMITTING:
                    return ::sharding::TransactionState::COMMITTING;
                case TransactionState::COMMITTED:
                    return ::sharding::TransactionState::COMMITTED;
                case TransactionState::ABORTING:
                    return ::sharding::TransactionState::ABORTING;
                case TransactionState::ABORTED:
                    return ::sharding::TransactionState::ABORTED;
                case TransactionState::UNKNOWN:
                    return ::sharding::TransactionState::ABORTING;
                default: break;
            }
            return ::sharding::TransactionState::INITIATED;
        };

        std::vector<::sharding::TransactionSnapshotEntry> active_txns = {};

        for (const auto& [txn_id, txn] : transactions_) {
            // Only snapshot non-final transactions
            if (txn.state != TransactionState::COMMITTED && 
                txn.state != TransactionState::ABORTED) {
                
                ::sharding::TransactionSnapshotEntry entry;
                entry.transaction_id = txn.transaction_id;
                entry.protocol = to_snapshot_protocol(txn.protocol);
                entry.state = to_snapshot_state(txn.state);
                entry.start_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    txn.start_time.time_since_epoch()).count();
                entry.coordinator_id = config_.coordinator_id.empty()
                    ? "unknown-coordinator"
                    : config_.coordinator_id;
                
                // Add participants
                for (const auto& [shard_id, participant] : txn.participants) {
                    entry.participants.push_back(shard_id);
                    
                    ::sharding::ParticipantStatus status;
                    status.participant_id = shard_id;
                    status.prepared = participant.prepared;
                    status.committed = participant.committed;
                    status.aborted = participant.aborted;
                    status.response_data = participant.error_message;
                    status.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    
                    entry.participant_status[shard_id] = status;
                }
                
                active_txns.push_back(entry);
            }
        }
        
        // Create snapshot
        auto snapshot_id = snapshot_manager_->createSnapshot(
            config_.coordinator_id.empty() ? "unknown-coordinator" : config_.coordinator_id,
            last_applied_lsn_,
            active_txns
        );
        
        if (snapshot_id.has_value()) {
            operations_since_snapshot_ = 0;
            spdlog::info("Created transaction snapshot {} with {} active transactions", 
                        snapshot_id.value(),static_cast<int>(active_txns.size()));
        } else {
            spdlog::error("Failed to create transaction snapshot");
        }
         
    } catch (const std::exception& e) {
        spdlog::error("Failed to create periodic snapshot: {}", e.what());
    }
 }

// ============================================================================
// PercolatorCoordinator implementation
// ============================================================================

PercolatorCoordinator::PercolatorCoordinator(
    const Config& config,
    std::shared_ptr<themis::sharding::TrueTime> truetime,
    TransactionWAL* wal
)
    : config_(config)
    , truetime_(std::move(truetime))
    , wal_(wal)
{
}

int64_t PercolatorCoordinator::computeCommitTimestamp() const {
    if (truetime_) {
        // Use the *latest* bound so that the commit timestamp is definitely
        // after any concurrent read that obtained a snapshot.
        return truetime_->now_with_uncertainty().latest.count();
    }
    // Fallback: wall clock in nanoseconds.
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

void PercolatorCoordinator::commitWait(int64_t commit_ts_ns) const {
    if (!truetime_) {
        return; // No TrueTime → no commit-wait required.
    }

    // Percolator commit-wait: wait until commit_ts is definitely in the past.
    // commit_ts_ns was drawn from now_with_uncertainty().latest, which already
    // incorporates the uncertainty bound.  Calling waitUntil(commit_ts) is
    // therefore sufficient to guarantee TT.now().earliest > commit_ts.
    truetime_->waitUntil(std::chrono::nanoseconds(commit_ts_ns));
}

bool PercolatorCoordinator::execute(
    CrossShardTransaction& txn,
    SendPrepareFn prepare_fn,
    SendCommitFn  commit_fn,
    SendAbortFn   abort_fn
) {
    spdlog::info("[Percolator] Starting Percolator commit for transaction {}",
                 txn.transaction_id);

    if (txn.participants.empty()) {
        spdlog::error("[Percolator] No participants in transaction {}",
                      txn.transaction_id);
        return false;
    }

    // Base retry delay and maximum shift for exponential backoff.
    // Backoff = BASE_RETRY_DELAY_MS * 2^(attempt-1), capped at lock_timeout.
    static constexpr uint32_t BASE_RETRY_DELAY_MS  = 10;
    static constexpr uint32_t MAX_BACKOFF_SHIFT     = 20; // caps at ~10 s before lock_timeout clip

    // Helper: acquire a lock on one shard with retry/backoff honoring
    // config_.max_retries and config_.lock_timeout.
    auto acquire_lock = [&](const std::string& shard_id) -> bool {
        for (uint32_t attempt = 0; attempt <= config_.max_retries; ++attempt) {
            if (attempt > 0) {
                // Exponential backoff capped at lock_timeout.
                // Guard the left-shift against overflow by capping the exponent.
                const uint32_t shift = std::min(attempt - 1, MAX_BACKOFF_SHIFT);
                auto delay = std::min(
                    config_.lock_timeout,
                    std::chrono::milliseconds(BASE_RETRY_DELAY_MS) * (1 << shift)
                );
                spdlog::debug("[Percolator] Retry {} for lock on shard {} (delay {}ms)",
                              attempt, shard_id, delay.count());
                std::this_thread::sleep_for(delay);
            }
            if (prepare_fn(shard_id, txn.transaction_id)) {
                return true;
            }
        }
        return false;
    };

    // ------------------------------------------------------------------
    // Phase 1 – PreWrite: acquire locks (secondary shards first, primary last).
    // ------------------------------------------------------------------
    const std::string primary_shard_id = txn.participants.begin()->first;

    std::vector<std::string> locked_shards;

    // Lock secondary shards first.
    for (auto& [shard_id, participant] : txn.participants) {
        if (shard_id == primary_shard_id) {
            continue;
        }

        spdlog::debug("[Percolator] Acquiring lock on secondary shard {} for txn {}",
                      shard_id, txn.transaction_id);

        bool locked = acquire_lock(shard_id);
        if (!locked) {
            spdlog::error("[Percolator] Lock acquisition failed on shard {} for txn {} "
                          "after {} retries",
                          shard_id, txn.transaction_id, config_.max_retries);
            for (const auto& ls : locked_shards) {
                abort_fn(ls, txn.transaction_id);
            }
            return false;
        }

        locked_shards.push_back(shard_id);
        participant.prepared = true;
    }

    // Lock the primary shard.
    spdlog::debug("[Percolator] Acquiring lock on primary shard {} for txn {}",
                  primary_shard_id, txn.transaction_id);

    bool primary_locked = acquire_lock(primary_shard_id);
    if (!primary_locked) {
        spdlog::error("[Percolator] Primary shard lock failed for txn {} after {} retries",
                      txn.transaction_id, config_.max_retries);
        for (const auto& ls : locked_shards) {
            abort_fn(ls, txn.transaction_id);
        }
        return false;
    }

    locked_shards.push_back(primary_shard_id);
    txn.participants.begin()->second.prepared = true;
    txn.state = TransactionState::PREPARED;

    // Log PREPARED state to WAL for crash-recovery.
    if (wal_) {
        try {
            nlohmann::json lock_meta = {
                {"protocol", "Percolator"},
                {"primary", primary_shard_id},
                {"locks_acquired", locked_shards}
            };
            wal_->logPrepare(txn.transaction_id, primary_shard_id, lock_meta);
        } catch (const std::exception& e) {
            spdlog::warn("[Percolator] WAL logPrepare failed: {}", e.what());
        }
    }

    spdlog::info("[Percolator] All locks acquired for txn {}", txn.transaction_id);

    // ------------------------------------------------------------------
    // Phase 2 – Assign TrueTime commit timestamp and perform commit-wait.
    // ------------------------------------------------------------------
    txn.state = TransactionState::COMMITTING;

    const int64_t commit_ts = computeCommitTimestamp();
    txn.commit_timestamp = commit_ts;

    spdlog::info("[Percolator] Commit timestamp for txn {} = {}",
                 txn.transaction_id, commit_ts);

    // Log commit decision to WAL (primary commit record).
    if (wal_) {
        try {
            nlohmann::json commit_meta = {
                {"protocol", "Percolator"},
                {"primary", primary_shard_id},
                {"commit_timestamp", commit_ts}
            };
            wal_->logCommit(txn.transaction_id, commit_meta);
        } catch (const std::exception& e) {
            spdlog::warn("[Percolator] WAL logCommit failed: {}", e.what());
        }
    }

    // Commit-wait: ensure commit_ts is definitely in the past before revealing it.
    commitWait(commit_ts);

    // ------------------------------------------------------------------
    // Phase 3 – Commit primary, then secondaries (releases locks).
    // ------------------------------------------------------------------
    bool primary_committed = commit_fn(primary_shard_id, txn.transaction_id);
    if (!primary_committed) {
        spdlog::error("[Percolator] Primary shard commit failed for txn {}",
                      txn.transaction_id);
        for (const auto& ls : locked_shards) {
            abort_fn(ls, txn.transaction_id);
        }
        return false;
    }

    txn.participants.begin()->second.committed = true;
    spdlog::info("[Percolator] Primary committed for txn {}", txn.transaction_id);

    // Commit secondaries (lock cleanup; can be retried asynchronously).
    for (auto& [shard_id, participant] : txn.participants) {
        if (shard_id == primary_shard_id) {
            continue;
        }

        bool committed = commit_fn(shard_id, txn.transaction_id);
        participant.committed = committed;

        if (committed) {
            // Log COMMITTED only after the RPC succeeds to preserve WAL integrity.
            if (wal_) {
                try {
                    wal_->logCommitted(txn.transaction_id, shard_id);
                } catch (const std::exception& e) {
                    spdlog::warn("[Percolator] WAL logCommitted failed for shard {}: {}",
                                 shard_id, e.what());
                }
            }
        } else {
            spdlog::warn("[Percolator] Secondary shard {} commit failed for txn {} "
                         "(will be cleaned up by background worker)",
                         shard_id, txn.transaction_id);
        }
    }

    spdlog::info("[Percolator] Transaction {} committed successfully", txn.transaction_id);
    return true;
}

size_t PercolatorCoordinator::cleanStaleLocks(
    const std::vector<std::string>& stale_txn_ids,
    CrossShardTransactionCoordinator& coordinator
) {
    size_t cleaned = 0;

    for (const auto& txn_id : stale_txn_ids) {
        auto txn_opt = coordinator.getTransaction(txn_id);
        if (!txn_opt.has_value()) {
            continue;
        }

        const auto& txn = *txn_opt;

        // Only abort Percolator transactions that are stuck in PREPARING or PREPARED.
        if (txn.protocol != TransactionProtocol::PERCOLATOR) {
            continue;
        }

        if (txn.state != TransactionState::PREPARING &&
            txn.state != TransactionState::PREPARED) {
            continue;
        }

        const auto age = std::chrono::system_clock::now() - txn.start_time;
        if (age < config_.stale_lock_threshold) {
            continue;
        }

        spdlog::info("[Percolator] Cleaning stale lock for txn {} (age {}s)",
                     txn_id,
                     std::chrono::duration_cast<std::chrono::seconds>(age).count());

        // Log the abort to WAL before issuing it.
        if (wal_) {
            try {
                wal_->logAbort(txn_id, "stale_lock_cleanup");
            } catch (const std::exception& e) {
                spdlog::warn("[Percolator] WAL logAbort failed for {}: {}", txn_id, e.what());
            }
        }

        if (coordinator.abort(txn_id)) {
            ++cleaned;
            spdlog::info("[Percolator] Stale lock cleaned for txn {}", txn_id);
        } else {
            spdlog::warn("[Percolator] Failed to clean stale lock for txn {}", txn_id);
        }
    }

    spdlog::info("[Percolator] cleanStaleLocks: cleaned {} / {} stale locks",
                 cleaned,static_cast<int>(stale_txn_ids.size()));
    return cleaned;
}

// 3PC Non-blocking support methods
void CrossShardTransactionCoordinator::preCommitRetryThread() {
    spdlog::info("3PC PreCommit retry thread started");
    
    while (running_.load()) {
        // Check for deferred PreCommits to retry
        std::map<std::string, std::vector<std::string>> retries;
        
        {
            std::lock_guard<std::mutex> lk(deferred_mutex_);
            if (deferred_precommits_.empty()) {
                // Nothing to retry, sleep and continue
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // Move all deferred PreCommits to local copy
            retries = std::move(deferred_precommits_);
        }
        
        // Process each transaction with failed PreCommits
        for (const auto& [txn_id, failed_shards] : retries) {
            if (!running_.load()) {
                break;
            }
            
            spdlog::debug("Attempting PreCommit retry for transaction {} ({} shards)",
                        txn_id,static_cast<int>(failed_shards.size()));
            
            // Get the transaction
            auto txn_opt = getTransaction(txn_id);
            if (!txn_opt) {
                spdlog::warn("Transaction {} not found during PreCommit retry, skipping",
                           txn_id);
                continue;
            }
            
            auto& txn = *txn_opt;
            
            // Only retry if still in PREPARED or COMMITTING state
            if (txn.state != TransactionState::PREPARED && 
                txn.state != TransactionState::COMMITTING) {
                spdlog::debug("Transaction {} no longer in retryable state ({}), skipping",
                           txn_id, static_cast<int>(txn.state));
                continue;
            }
            
            // Snapshot the PreCommit callback under the lock
            PreCommitRpcFn precommit_cb;
            {
                std::lock_guard<std::mutex> lk(callbacks_mutex_);
                precommit_cb = precommit_callback_;
            }
            
            if (!precommit_cb) {
                spdlog::error("PreCommit retry: No callback configured, cannot retry");
                continue;
            }
            
            // Retry PreCommit for failed shards
            bool all_recovered = true;
            for (const auto& shard_id : failed_shards) {
                try {
                    bool success = precommit_cb(shard_id, txn_id);
                    if (!success) {
                        spdlog::warn("PreCommit retry failed for shard {} in txn {}",
                                   shard_id, txn_id);
                        all_recovered = false;
                    } else {
                        spdlog::info("PreCommit retry succeeded for shard {} in txn {}",
                                   shard_id, txn_id);
                        // Mark participant as precommitted
                        std::lock_guard<std::timed_mutex> txn_lk(transactions_mutex_);
                        auto txn_it = transactions_.find(txn_id);
                        if (txn_it != transactions_.end()) {
                            txn_it->second.participants[shard_id].precommitted = true;
                        }
                    }
                } catch (const std::exception& ex) {
                    spdlog::error("PreCommit retry threw for shard {} txn={}: {}",
                                shard_id, txn_id, ex.what());
                    all_recovered = false;
                } catch (...) {
                    spdlog::error("PreCommit retry threw unknown exception for shard {} txn={}",
                                shard_id, txn_id);
                    all_recovered = false;
                }
            }
            
            if (all_recovered) {
                // All shards recovered, transaction can proceed to DoCommit
                spdlog::info("All PreCommits recovered for transaction {}, "
                           "proceeding to DoCommit", txn_id);
                
                // Transition to COMMITTING state if not already
                {
                    std::lock_guard<std::timed_mutex> txn_lk(transactions_mutex_);
                    auto txn_it = transactions_.find(txn_id);
                    if (txn_it != transactions_.end() && 
                        txn_it->second.state == TransactionState::PREPARED) {
                        txn_it->second.state = TransactionState::COMMITTING;
                    }
                }
                
                // Trigger commit
                commit(txn_id);
            } else {
                // Some shards still failing, reschedule for later retry
                spdlog::warn("Partial PreCommit recovery for transaction {}, "
                           "rescheduling remaining shards", txn_id);
                
                std::vector<std::string> still_failed = {};

                for (const auto& shard_id : failed_shards) {
                    auto txn_snapshot = getTransaction(txn_id);
                    if (txn_snapshot) {
                        const auto& txn_state = *txn_snapshot;
                        const auto participant_it = txn_state.participants.find(shard_id);
                        if (participant_it != txn_state.participants.end() &&
                            !participant_it->second.precommitted) {
                            still_failed.push_back(shard_id);
                        }
                    }
                }
                
                if (!still_failed.empty()) {
                    std::lock_guard<std::mutex> def_lk(deferred_mutex_);
                    deferred_precommits_[txn_id] = still_failed;
                }
            }
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    spdlog::info("3PC PreCommit retry thread stopped");
}

// ============================================================================
// Distributed SSI API
// ============================================================================

void CrossShardTransactionCoordinator::registerShardReadSet(
    const std::string& transaction_id,
    const std::string& shard_id,
    const std::vector<CrossShardPredicateLock>& predicates)
{
    // Only track SERIALIZABLE transactions — early-exit for others to avoid
    // overhead on the common non-serializable path.
    {
        std::unique_lock<std::timed_mutex> lk(transactions_mutex_, std::defer_lock);
        if (lk.try_lock_for(config_.lock_timeout)) {
            auto it = transactions_.find(transaction_id);
            if (it == transactions_.end() ||
                it->second.isolation_level != IsolationLevel::SERIALIZABLE) {
                return;
            }
        }
    }
    ssi_manager_.registerReadSet(transaction_id, shard_id, predicates);
}

void CrossShardTransactionCoordinator::registerShardWriteSet(
    const std::string& transaction_id,
    const std::string& shard_id,
    const std::vector<std::string>& keys)
{
    ssi_manager_.registerWriteSet(transaction_id, shard_id, keys);
}

std::vector<CrossShardSSIConflict>
CrossShardTransactionCoordinator::validateCrossShardSSI(
    const std::string& transaction_id) const
{
    return ssi_manager_.validateAtPrepare(transaction_id);
}

void CrossShardTransactionCoordinator::setSSIConfig(
    const CrossShardSSIManager::Config& config)
{
    ssi_manager_.setConfig(config);
}

CrossShardSSIManager::Config
CrossShardTransactionCoordinator::getSSIConfig() const
{
    return ssi_manager_.getConfig();
}

} // namespace sharding
} // namespace themisdb
