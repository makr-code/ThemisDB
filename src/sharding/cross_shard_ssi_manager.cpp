/**
 * @file cross_shard_ssi_manager.cpp
 * @brief Distributed Serializable Snapshot Isolation (SSI) manager — implementation.
 *
 * Implements the optimistic SSI conflict-detection algorithm for cross-shard
 * transactions.  All key comparisons use lexicographic order, which is the
 * same ordering used by the underlying RocksDB storage layer.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/cross_shard_ssi_manager.h"

#include <algorithm>

namespace themisdb::sharding {

// ============================================================================
// Construction
// ============================================================================

CrossShardSSIManager::CrossShardSSIManager()
    : config_() {}

CrossShardSSIManager::CrossShardSSIManager(const Config& config)
    : config_(config) {}

// ============================================================================
// Registration API
// ============================================================================

void CrossShardSSIManager::registerReadSet(
    const std::string& txn_id,
    const std::string& shard_id,
    const std::vector<CrossShardPredicateLock>& predicates)
{
    if (txn_id.empty() || shard_id.empty() || predicates.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lk(mutex_);

    if (!config_.enable_predicate_locking) {
        return;
    }

    auto& shard_map = read_sets_[txn_id];
    auto& target = shard_map[shard_id];

    for (const auto& pred : predicates) {
        // Enforce per-transaction cap — dropping excess predicates degrades
        // coverage but prevents unbounded memory growth.
        size_t total = 0;
        for (const auto& [sid, preds] : shard_map) {
            total += preds.size();
        }
        if (total >= config_.max_predicate_locks_per_txn) {
            break;
        }
        target.push_back(pred);
    }
}

void CrossShardSSIManager::registerWriteSet(
    const std::string& txn_id,
    const std::string& shard_id,
    const std::vector<std::string>& keys)
{
    if (txn_id.empty() || shard_id.empty() || keys.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lk(mutex_);

    auto& shard_map = write_sets_[txn_id];
    auto& target = shard_map[shard_id];

    for (const auto& key : keys) {
        // Enforce per-transaction cap.
        size_t total = 0;
        for (const auto& [sid, ks] : shard_map) {
            total += ks.size();
        }
        if (total >= config_.max_write_keys_per_txn) {
            break;
        }
        target.push_back(key);
    }
}

// ============================================================================
// Validation
// ============================================================================

std::vector<CrossShardSSIConflict>
CrossShardSSIManager::validateAtPrepare(const std::string& txn_id) const
{
    std::vector<CrossShardSSIConflict> conflicts;

    if (txn_id.empty()) {
        return conflicts;
    }

    std::lock_guard<std::mutex> lk(mutex_);

    if (!config_.enable_predicate_locking) {
        return conflicts;
    }

    // Retrieve this transaction's read predicates and write keys.
    const auto my_reads_it  = read_sets_.find(txn_id);
    const auto my_writes_it = write_sets_.find(txn_id);

    const bool has_reads  = (my_reads_it  != read_sets_.end());
    const bool has_writes = (my_writes_it != write_sets_.end());

    if (!has_reads && !has_writes) {
        return conflicts; // Nothing registered — no conflicts possible.
    }

    // ── Forward RW: our writes fall inside another transaction's read predicates ──
    // Pattern: we write K; another transaction read-scanned a range that covers K.
    // This can produce write-skew or phantom anomalies.
    if (has_writes) {
        for (const auto& [other_id, other_reads_by_shard] : read_sets_) {
            if (other_id == txn_id) {
                continue;
            }
            for (const auto& [my_shard, my_keys] : my_writes_it->second) {
                // Check other's predicates — even cross-shard ones matter
                // because a key namespace may span multiple shards.
                for (const auto& [other_shard, other_preds] : other_reads_by_shard) {
                    for (const auto& pred : other_preds) {
                        for (const auto& key : my_keys) {
                            if (keyInRange(key, pred.start_key, pred.end_key)) {
                                CrossShardSSIConflict c;
                                c.transaction_id             = txn_id;
                                c.conflicting_transaction_id = other_id;
                                c.shard_id                   = my_shard;
                                c.key                        = key;
                                c.conflict_type              = "rw";
                                c.message =
                                    "Write key '" + key + "' on shard '" + my_shard +
                                    "' falls within predicate range ['" +
                                    pred.start_key + "', '" + pred.end_key +
                                    "'] held by transaction '" + other_id +
                                    "' on shard '" + other_shard + "' (write-skew risk)";
                                conflicts.push_back(std::move(c));
                                // One conflict per (txn-pair, key) is sufficient.
                                goto next_fwd_write_key;
                            }
                        }
                        next_fwd_write_key:;
                    }
                }
            }
        }
    }

    // ── Reverse RW: another transaction's writes fall inside our read predicates ──
    // Pattern: another transaction writes K; we read-scanned a range covering K.
    // This means our snapshot missed a write that happened in the same epoch —
    // phantom / read-skew risk.
    //
    // NOTE: This pass iterates write_sets_ directly so that write-only
    // transactions (those with no read set registered) are also checked.
    if (has_reads) {
        for (const auto& [other_id, other_writes_by_shard] : write_sets_) {
            if (other_id == txn_id) {
                continue;
            }
            for (const auto& [other_shard, other_keys] : other_writes_by_shard) {
                for (const auto& [my_shard, my_preds] : my_reads_it->second) {
                    for (const auto& pred : my_preds) {
                        for (const auto& key : other_keys) {
                            if (keyInRange(key, pred.start_key, pred.end_key)) {
                                CrossShardSSIConflict c;
                                c.transaction_id             = txn_id;
                                c.conflicting_transaction_id = other_id;
                                c.shard_id                   = other_shard;
                                c.key                        = key;
                                c.conflict_type              = "rw";
                                c.message =
                                    "Concurrent write of key '" + key + "' by transaction '" +
                                    other_id + "' on shard '" + other_shard +
                                    "' falls within our predicate range ['" +
                                    pred.start_key + "', '" + pred.end_key +
                                    "'] on shard '" + my_shard + "' (phantom risk)";
                                conflicts.push_back(std::move(c));
                                goto next_rev_write_key;
                            }
                        }
                        next_rev_write_key:;
                    }
                }
            }
        }
    }

    // ── Write-write conflict detection ───────────────────────────────────────
    // WW: both this transaction and another write the same key on any shard.
    if (has_writes) {
        for (const auto& [other_id, other_writes_by_shard] : write_sets_) {
            if (other_id == txn_id) {
                continue;
            }
            for (const auto& [my_shard, my_keys] : my_writes_it->second) {
                for (const auto& [other_shard, other_keys] : other_writes_by_shard) {
                    // Key namespaces across shards are disjoint when sharding is
                    // key-range based; for hash-based sharding the same logical key
                    // can appear on different shards.  We compare plain key strings
                    // (application-level keys before shard routing) to be safe.
                    for (const auto& mk : my_keys) {
                        for (const auto& ok : other_keys) {
                            if (mk == ok) {
                                CrossShardSSIConflict c;
                                c.transaction_id             = txn_id;
                                c.conflicting_transaction_id = other_id;
                                c.shard_id                   = my_shard;
                                c.key                        = mk;
                                c.conflict_type              = "ww";
                                c.message =
                                    "Write-write conflict on key '" + mk + "': both '" +
                                    txn_id + "' (shard '" + my_shard + "') and '" +
                                    other_id + "' (shard '" + other_shard +
                                    "') wrote this key (lost-update risk)";
                                conflicts.push_back(std::move(c));
                                goto next_ww_key;
                            }
                        }
                        next_ww_key:;
                    }
                }
            }
        }
    }

    return conflicts;
}

// ============================================================================
// Lifecycle
// ============================================================================

void CrossShardSSIManager::clearTransaction(const std::string& txn_id)
{
    if (txn_id.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    read_sets_.erase(txn_id);
    write_sets_.erase(txn_id);
}

// ============================================================================
// Configuration
// ============================================================================

void CrossShardSSIManager::setConfig(const Config& config)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_ = config;
}

CrossShardSSIManager::Config CrossShardSSIManager::getConfig() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return config_;
}

// ============================================================================
// Diagnostics
// ============================================================================

size_t CrossShardSSIManager::trackedTransactionCount() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    // Union of txn IDs that appear in either read_sets_ or write_sets_.
    size_t count = read_sets_.size();
    for (const auto& [id, _] : write_sets_) {
        if (read_sets_.find(id) == read_sets_.end()) {
            ++count;
        }
    }
    return count;
}

// ============================================================================
// Internal helpers
// ============================================================================

bool CrossShardSSIManager::keyInRange(const std::string& key,
                                       const std::string& start,
                                       const std::string& end) noexcept
{
    if (key < start) {
        return false;
    }
    // Empty end_key = single-key predicate.
    if (end.empty()) {
        return key == start;
    }
    return key <= end;
}

} // namespace themisdb::sharding
