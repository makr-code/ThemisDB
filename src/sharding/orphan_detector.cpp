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

/*
 * ThemisDB | File: orphan_detector.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 168
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * PR History (last 5): #4259 feat(sharding): Wire Orphan... (2026-03-15) | #4212 fix(chimera/percolator): re... (2026-03-15) | #3632 fix(build): register 40+ mi... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    std::vector<themisdb::sharding::CrossShardTransaction> active_txns;
    if (distributed_coordinator_) {
        active_txns = distributed_coordinator_->listInFlightTransactions();
    } else {
        active_txns = coordinator->getActiveTransactions();
    }

    auto now = std::chrono::system_clock::now();
    const auto threshold = std::chrono::seconds(config_.timeout_seconds);
    
    // QW-6a: State-Specific Timeout Configuration
    // Different transaction states may require different timeout thresholds:
    // PREPARING: timeout indicates prepare phase failure (aggressive cleanup)
    // PREPARED: timeout indicates waiting for commit/abort (standard timeout)
    // COMMITTING/ABORTING: timeout indicates protocol stall (extends slightly)
    std::map<int, uint64_t> state_timeouts;
    state_timeouts[static_cast<int>(themisdb::sharding::TransactionState::PREPARING)] = 
        config_.timeout_seconds / 2;  // Faster detection for prepare phase hangs
    state_timeouts[static_cast<int>(themisdb::sharding::TransactionState::PREPARED)] = 
        config_.timeout_seconds;      // Standard timeout for prepared state
    state_timeouts[static_cast<int>(themisdb::sharding::TransactionState::COMMITTING)] = 
        config_.timeout_seconds * 2;  // Extended timeout for commit phase
    state_timeouts[static_cast<int>(themisdb::sharding::TransactionState::ABORTING)] = 
        config_.timeout_seconds * 2;  // Extended timeout for abort phase

    for (const auto& txn : active_txns) {
        const auto age = now - txn.start_time;
        
        // QW-6a: Apply state-specific timeout thresholds
        uint64_t effective_timeout = config_.timeout_seconds;
        auto state_it = state_timeouts.find(static_cast<int>(txn.state));
        if (state_it != state_timeouts.end()) {
            effective_timeout = state_it->second;
        }
        
        const auto effective_threshold = std::chrono::seconds(effective_timeout);
        if (age < effective_threshold) {
            continue;
        }

        if (isOrphanableState(txn, config_)) {
            // QW-6a: Enhanced logging for orphan lifecycle diagnostics
            spdlog::info("QW-6a: OrphanDetector detected orphaned transaction {} "
                        "(age {}s, state {}, threshold {}s)",
                        txn.transaction_id,
                        std::chrono::duration_cast<std::chrono::seconds>(age).count(),
                        static_cast<int>(txn.state),
                        effective_timeout);
            
            // QW-6a: Log blocked state information for debugging
            if (txn.state == themisdb::sharding::TransactionState::PREPARING) {
                spdlog::warn("QW-6a: Transaction {} blocked in PREPARING state (stalled prepare phase)",
                            txn.transaction_id);
            } else if (txn.state == themisdb::sharding::TransactionState::PREPARED) {
                spdlog::warn("QW-6a: Transaction {} blocked in PREPARED state (awaiting commit/abort decision)",
                            txn.transaction_id);
            }
            
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
    std::optional<themisdb::sharding::CrossShardTransaction> txn_opt;
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

         // QW-6a: Safe rollback with explicit error handling
         try {
             if (coordinator->abort(txn.transaction_id)) {
                 ++cleaned;
                 spdlog::info("QW-6a: Orphan cleanup - stale Percolator lock reclaimed for txn {}",
                             txn.transaction_id);
             } else {
                 spdlog::warn("QW-6a: Orphan cleanup - failed to abort stale Percolator txn {} "
                             "(coordinator returned false)",
                             txn.transaction_id);
             }
         } catch (const std::exception& e) {
             // QW-6a: Catch exceptions during rollback to prevent cleanup stalls
             spdlog::error("QW-6a: Orphan cleanup - exception during abort of txn {}: {}",
                          txn.transaction_id, e.what());
         }
     }

     spdlog::info("QW-6a: OrphanDetector::cleanPercolatorLocks: reclaimed {} stale lock(s) from {} candidates",
                  cleaned, active_txns.size());
     return cleaned;
}

} // namespace sharding

