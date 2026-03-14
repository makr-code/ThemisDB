/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            orphan_detector.cpp                                ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:00:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     71                                             ║
    • Open Issues:     TODOs: 2, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/orphan_detector.h"
#include "sharding/cross_shard_transaction.h"
#include <spdlog/spdlog.h>

namespace sharding {

OrphanDetector::OrphanDetector(const Config& config)
    : config_(config) {
}

std::vector<std::string> OrphanDetector::detectOrphans(
    const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator) {
    
    std::vector<std::string> orphaned_txns;
    
    if (!coordinator) {
        spdlog::warn("OrphanDetector: Coordinator is null");
        return orphaned_txns;
    }
    
    spdlog::info("OrphanDetector: Scanning for orphaned transactions (timeout: {}s)", 
                 config_.timeout_seconds);

    auto active_txns = coordinator->getActiveTransactions();
    auto now = std::chrono::system_clock::now();
    const auto threshold = std::chrono::seconds(config_.timeout_seconds);

    for (const auto& txn : active_txns) {
        const auto age = now - txn.start_time;
        if (age < threshold) {
            continue;
        }

        const bool orphanable =
            (config_.check_preparing  && txn.state == themisdb::sharding::TransactionState::PREPARING)  ||
            (config_.check_prepared   && txn.state == themisdb::sharding::TransactionState::PREPARED)    ||
            (config_.check_committing && txn.state == themisdb::sharding::TransactionState::COMMITTING)  ||
            (config_.check_aborting   && txn.state == themisdb::sharding::TransactionState::ABORTING);

        if (orphanable) {
            spdlog::info("OrphanDetector: Transaction {} is orphaned (age {}s, state {})",
                         txn.transaction_id,
                         std::chrono::duration_cast<std::chrono::seconds>(age).count(),
                         static_cast<int>(txn.state));
            orphaned_txns.push_back(txn.transaction_id);
        }
    }

    return orphaned_txns;
}

bool OrphanDetector::isOrphaned(
    const std::string& transaction_id,
    const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator) {
    
    if (!coordinator) {
        return false;
    }

    auto txn_opt = coordinator->getTransaction(transaction_id);
    if (!txn_opt.has_value()) {
        return false;
    }

    const auto& txn = *txn_opt;
    const auto age  = std::chrono::system_clock::now() - txn.start_time;

    if (age < std::chrono::seconds(config_.timeout_seconds)) {
        return false;
    }

    return
        (config_.check_preparing  && txn.state == themisdb::sharding::TransactionState::PREPARING)  ||
        (config_.check_prepared   && txn.state == themisdb::sharding::TransactionState::PREPARED)    ||
        (config_.check_committing && txn.state == themisdb::sharding::TransactionState::COMMITTING)  ||
        (config_.check_aborting   && txn.state == themisdb::sharding::TransactionState::ABORTING);
}

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
