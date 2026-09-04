/**
 * @file orphan_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/orphan_detector.h"
#include "sharding/cross_shard_transaction.h"
#include "sharding/distributed_coordinator.h"
#include <spdlog/spdlog.h>

namespace sharding {

namespace {

/**
 * @brief Return true when transaction state is configured as orphanable.
 * @param txn Transaction record to evaluate.
 * @param cfg Detector configuration with enabled state filters.
 * @return True when transaction state is selected for orphan checks.
 */
bool isOrphanableState(
    const themisdb::sharding::CrossShardTransaction& txn,
    const OrphanDetector::Config& cfg)
{
    using S = themisdb::sharding::TransactionState;
    return (cfg.check_preparing  && txn.state == S::PREPARING)  ||
           (cfg.check_prepared   && txn.state == S::PREPARED)    ||
           (cfg.check_committing && txn.state == S::COMMITTING)  ||
           (cfg.check_aborting   && txn.state == S::ABORTING);
}

} // anonymous namespace

/** @brief Construct detector with per-call coordinator mode only. */
OrphanDetector::OrphanDetector(const Config& config)
    : config_(config) {
}

/** @brief Construct detector with optional distributed coordinator backend. */
OrphanDetector::OrphanDetector(const Config& config,
                               themis::sharding::DistributedCoordinator* dist_coordinator)
    : config_(config), distributed_coordinator_(dist_coordinator) {
}

/**
 * @brief Scan active transactions and return IDs classified as orphaned.
 * @param coordinator Fallback transaction coordinator for active transaction scan.
 * @return List of orphaned transaction IDs.
 */
std::vector<std::string> OrphanDetector::detectOrphans(
    const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator) {
    
    std::vector<std::string> orphaned_txns;
    
    if (!coordinator && !distributed_coordinator_) {
        spdlog::warn("OrphanDetector: No coordinator available");
        return orphaned_txns;
    }
    
    spdlog::info("OrphanDetector: Scanning for orphaned transactions (timeout: {}s)", 
                 config_.timeout_seconds);

    // Prefer the authoritative in-flight list from DistributedCoordinator when
    // available; fall back to the per-call CrossShardTransactionCoordinator.
    std::vector<themisdb::sharding::CrossShardTransaction> active_txns = {};

    if (distributed_coordinator_) {
        active_txns = distributed_coordinator_->listInFlightTransactions();
    } else {
        active_txns = coordinator->getActiveTransactions();
    }

    auto now = std::chrono::system_clock::now();
    const auto threshold = std::chrono::seconds(config_.timeout_seconds);

    for (const auto& txn : active_txns) {
        const auto age = now - txn.start_time;
        if (age < threshold) {
            continue;
        }

        if (isOrphanableState(txn, config_)) {
            spdlog::info("OrphanDetector: Transaction {} is orphaned (age {}s, state {})",
                         txn.transaction_id,
                         std::chrono::duration_cast<std::chrono::seconds>(age).count(),
                         static_cast<int>(txn.state));
            orphaned_txns.push_back(txn.transaction_id);
        }
    }

    return orphaned_txns;
}

/**
 * @brief Check one transaction for orphan condition.
 * @param transaction_id Transaction identifier.
 * @param coordinator Fallback coordinator used when distributed coordinator is absent.
 * @return True when transaction exceeds timeout and is in configured orphanable state.
 */
bool OrphanDetector::isOrphaned(
    const std::string& transaction_id,
    const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator) {
    
    if (!coordinator && !distributed_coordinator_) {
        return false;
    }

    // Prefer the authoritative getTransaction() from DistributedCoordinator
    // when available; fall back to the per-call coordinator.
    std::optional<themisdb::sharding::CrossShardTransaction> txn_opt = {};

    if (distributed_coordinator_) {
        txn_opt = distributed_coordinator_->getTransaction(transaction_id);
    } else {
        txn_opt = coordinator->getTransaction(transaction_id);
    }

    if (!txn_opt.has_value()) {
        return false;
    }

    const auto& txn = *txn_opt;
    const auto age  = std::chrono::system_clock::now() - txn.start_time;

    if (age < std::chrono::seconds(config_.timeout_seconds)) {
        return false;
    }

    return isOrphanableState(txn, config_);
}

/**
 * @brief Abort stale Percolator transactions to reclaim orphaned locks.
 * @param coordinator Coordinator used to enumerate and abort transactions.
 * @return Number of stale Percolator transactions successfully aborted.
 */
size_t OrphanDetector::cleanPercolatorLocks(
    const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator) {

    if (!coordinator) {
        spdlog::warn("OrphanDetector::cleanPercolatorLocks: coordinator is null");
        return 0;
    }

    auto active_txns = coordinator->getActiveTransactions();
    auto now         = std::chrono::system_clock::now();
    const auto threshold = std::chrono::seconds(config_.timeout_seconds);

    size_t cleaned = 0;
    for (const auto& txn : active_txns) {
        // Only process Percolator transactions.
        if (txn.protocol != themisdb::sharding::TransactionProtocol::PERCOLATOR) {
            continue;
        }

        // Skip transactions that haven't yet hit the stale threshold.
        if ((now - txn.start_time) < threshold) {
            continue;
        }

        // Only clean up locks that are still being held (PREPARING / PREPARED).
        if (txn.state != themisdb::sharding::TransactionState::PREPARING &&
            txn.state != themisdb::sharding::TransactionState::PREPARED) {
            continue;
        }

        spdlog::info("OrphanDetector: Reclaiming stale Percolator lock for txn {} "
                     "(age {}s)",
                     txn.transaction_id,
                     std::chrono::duration_cast<std::chrono::seconds>(
                         now - txn.start_time).count());

        if (coordinator->abort(txn.transaction_id)) {
            ++cleaned;
            spdlog::info("OrphanDetector: Stale Percolator lock reclaimed for txn {}",
                         txn.transaction_id);
        } else {
            spdlog::warn("OrphanDetector: Failed to abort stale Percolator txn {}",
                         txn.transaction_id);
        }
    }

    spdlog::info("OrphanDetector::cleanPercolatorLocks: reclaimed {} stale lock(s)",
                 cleaned);
    return cleaned;
}

} // namespace sharding

