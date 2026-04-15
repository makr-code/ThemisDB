/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_transaction_manager.cpp                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 07:14:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     390                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • baf9174be9  2026-03-14  fix(storage): address all code review issues on Distribut... ║
    • 7b51fcb963  2026-03-14  feat(storage): implement DistributedTransactionManager wi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// DistributedTransactionManager — 2PC coordinator for storage-layer
// distributed transactions across multiple shards (v1.7.0).

#include "storage/distributed_transaction_manager.h"
#include "utils/logger.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// DistributedTransaction — implementation
// ─────────────────────────────────────────────────────────────────────────────

DistributedTransaction::DistributedTransaction(
    PrivateTag,
    std::string                         txn_id,
    char                                separator,
    std::shared_ptr<ManagerSharedState> state
)
    : txn_id_(std::move(txn_id))
    , separator_(separator)
    , mgr_state_(std::move(state))
{
}

DistributedTransaction::~DistributedTransaction() {
    // If the transaction is still ACTIVE when destroyed, roll it back to ensure
    // prepared shards are not left in a stuck state.
    if (state_ == DistributedTxnState::ACTIVE || state_ == DistributedTxnState::PREPARING) {
        rollback();
    }
}

// ── Key parsing ───────────────────────────────────────────────────────────────

std::pair<std::string, std::string>
DistributedTransaction::parseKey(std::string_view composite) const {
    const auto pos = composite.find(separator_);
    if (pos == std::string_view::npos) {
        throw std::invalid_argument(
            "DistributedTransaction: key must be in 'shard_id" +
            std::string(1, separator_) + "logical_key' format, got: " +
            std::string(composite)
        );
    }
    return {std::string(composite.substr(0, pos)),
            std::string(composite.substr(pos + 1))};
}

// ── Participant lookup ────────────────────────────────────────────────────────

std::shared_ptr<IDistributedShardParticipant>
DistributedTransaction::requireParticipant(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lk(mgr_state_->shards_mutex);
    auto it = mgr_state_->shards.find(shard_id);
    if (it == mgr_state_->shards.end()) {
        throw std::invalid_argument(
            "DistributedTransaction [" + txn_id_ + "]: unknown shard '" + shard_id + "'"
        );
    }
    // Return a ref-counted copy; safe to use after the lock is released.
    return it->second;
}

// ── Write operations ──────────────────────────────────────────────────────────

void DistributedTransaction::put(std::string_view key, std::string_view value) {
    if (state_ != DistributedTxnState::ACTIVE) {
        throw std::invalid_argument(
            "DistributedTransaction [" + txn_id_ + "]: put() called on non-ACTIVE transaction"
        );
    }

    auto [shard_id, logical_key] = parseKey(key);
    // Validate shard existence (throws if unknown)
    requireParticipant(shard_id);

    DistributedOperation op;
    op.type     = DistributedOperation::Type::PUT;
    op.shard_id = shard_id;
    op.key      = logical_key;
    op.value    = std::string(value);

    pending_ops_[shard_id].push_back(std::move(op));
}

void DistributedTransaction::del(std::string_view key) {
    if (state_ != DistributedTxnState::ACTIVE) {
        throw std::invalid_argument(
            "DistributedTransaction [" + txn_id_ + "]: del() called on non-ACTIVE transaction"
        );
    }

    auto [shard_id, logical_key] = parseKey(key);
    // Validate shard existence (throws if unknown)
    requireParticipant(shard_id);

    DistributedOperation op;
    op.type     = DistributedOperation::Type::DELETE;
    op.shard_id = shard_id;
    op.key      = logical_key;

    pending_ops_[shard_id].push_back(std::move(op));
}

// ── Read operation ────────────────────────────────────────────────────────────

std::optional<std::string> DistributedTransaction::get(std::string_view key) {
    auto [shard_id, logical_key] = parseKey(key);
    // requireParticipant throws std::invalid_argument if shard is not registered.
    auto participant = requireParticipant(shard_id);
    return participant->get(logical_key);
}

// ── Commit (2PC) ──────────────────────────────────────────────────────────────

bool DistributedTransaction::commit() {
    if (state_ == DistributedTxnState::COMMITTED) {
        return true;
    }
    if (state_ == DistributedTxnState::ABORTED) {
        return false;
    }
    if (state_ != DistributedTxnState::ACTIVE) {
        THEMIS_WARN("DistributedTransaction [{}]: commit() called in unexpected state", txn_id_);
        return false;
    }

    state_ = DistributedTxnState::PREPARING;
    THEMIS_DEBUG("DistributedTransaction [{}]: Phase 1 — PREPARE to {} shard(s)",
                 txn_id_, pending_ops_.size());

    // ── Phase 1: PREPARE ─────────────────────────────────────────────────────
    bool all_prepared = true;

    for (auto& [shard_id, ops] : pending_ops_) {
        // Copy the shared_ptr under lock; safe to call prepare() after lock release.
        std::shared_ptr<IDistributedShardParticipant> participant;
        {
            std::lock_guard<std::mutex> lk(mgr_state_->shards_mutex);
            auto it = mgr_state_->shards.find(shard_id);
            if (it == mgr_state_->shards.end()) {
                THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' no longer registered during prepare",
                             txn_id_, shard_id);
                all_prepared = false;
                break;
            }
            participant = it->second;
        }

        bool voted_commit = false;
        try {
            voted_commit = participant->prepare(txn_id_, ops);
        } catch (const std::exception& ex) {
            THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' prepare threw: {}",
                         txn_id_, shard_id, ex.what());
            voted_commit = false;
        }

        if (voted_commit) {
            prepared_shards_.emplace_back(shard_id, participant);  // copy: participant still ref-counted here
            THEMIS_DEBUG("DistributedTransaction [{}]: shard '{}' voted COMMIT", txn_id_, shard_id);
        } else {
            THEMIS_WARN("DistributedTransaction [{}]: shard '{}' voted ABORT", txn_id_, shard_id);
            all_prepared = false;
            break;
        }
    }

    // ── Phase 2: COMMIT or ABORT ──────────────────────────────────────────────
    if (all_prepared) {
        THEMIS_DEBUG("DistributedTransaction [{}]: Phase 2 — COMMIT to {} shard(s)",
                     txn_id_, prepared_shards_.size());

        for (auto& [shard_id, participant] : prepared_shards_) {
            try {
                participant->commit(txn_id_);
            } catch (const std::exception& ex) {
                // The commit decision is irreversible once all Phase-1 votes were YES.
                // Log the failure; the shard must recover via its own WAL replay.
                // Operators should monitor ERROR logs for unacknowledged Phase-2 commits
                // and trigger shard-side WAL recovery to reach a consistent state.
                THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' commit threw: {}",
                             txn_id_, shard_id, ex.what());
            }
        }

        state_ = DistributedTxnState::COMMITTED;
        THEMIS_INFO("DistributedTransaction [{}]: COMMITTED across {} shard(s)",
                    txn_id_, prepared_shards_.size());
        mgr_state_->committed.fetch_add(1, std::memory_order_relaxed);
        mgr_state_->active.fetch_sub(1, std::memory_order_relaxed);
        return true;

    } else {
        // Send ABORT to all shards that already voted YES.
        for (auto& [shard_id, participant] : prepared_shards_) {
            try {
                participant->abort(txn_id_);
            } catch (const std::exception& ex) {
                THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort threw: {}",
                             txn_id_, shard_id, ex.what());
            }
        }

        state_ = DistributedTxnState::ABORTED;
        THEMIS_INFO("DistributedTransaction [{}]: ABORTED (prepare phase failed)", txn_id_);
        mgr_state_->aborted.fetch_add(1, std::memory_order_relaxed);
        mgr_state_->active.fetch_sub(1, std::memory_order_relaxed);
        return false;
    }
}

// ── Rollback ──────────────────────────────────────────────────────────────────

void DistributedTransaction::rollback() {
    if (state_ == DistributedTxnState::COMMITTED || state_ == DistributedTxnState::ABORTED) {
        return;  // Already finished
    }

    state_ = DistributedTxnState::ABORTED;

    // Send ABORT to every shard that received a PREPARE.
    for (auto& [shard_id, participant] : prepared_shards_) {
        try {
            participant->abort(txn_id_);
        } catch (const std::exception& ex) {
            THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
                         txn_id_, shard_id, ex.what());
        }
    }

    THEMIS_INFO("DistributedTransaction [{}]: rolled back", txn_id_);
    mgr_state_->aborted.fetch_add(1, std::memory_order_relaxed);
    mgr_state_->active.fetch_sub(1, std::memory_order_relaxed);
}

// ── Introspection ─────────────────────────────────────────────────────────────

std::vector<std::string> DistributedTransaction::participatingShards() const {
    std::vector<std::string> result;
    result.reserve(pending_ops_.size());
    for (const auto& [shard_id, _] : pending_ops_) {
        result.push_back(shard_id);
    }
    return result;
}

size_t DistributedTransaction::operationCount() const {
    size_t total = 0;
    for (const auto& [_, ops] : pending_ops_) {
        total += ops.size();
    }
    return total;
}

// ─────────────────────────────────────────────────────────────────────────────
// DistributedTransactionManager — implementation
// ─────────────────────────────────────────────────────────────────────────────

DistributedTransactionManager::DistributedTransactionManager(
    std::vector<ShardConfig> shards,
    DistributedTxnConfig     config
)
    : config_(std::move(config))
    , state_(std::make_shared<ManagerSharedState>())
{
    for (auto& sc : shards) {
        registerShard(sc.shard_id, sc.participant);
    }
}

// ── Shard management ──────────────────────────────────────────────────────────

void DistributedTransactionManager::registerShard(
    const std::string&            shard_id,
    IDistributedShardParticipant* participant
) {
    if (shard_id.empty()) {
        throw std::invalid_argument("DistributedTransactionManager: shard_id must not be empty");
    }
    if (!participant) {
        throw std::invalid_argument("DistributedTransactionManager: participant must not be null");
    }
    // Wrap in a shared_ptr with a no-op deleter — the caller retains ownership,
    // but transactions can safely copy the shared_ptr under lock to get a
    // reference that outlives any concurrent unregisterShard() call.
    std::lock_guard<std::mutex> lk(state_->shards_mutex);
    state_->shards[shard_id] = std::shared_ptr<IDistributedShardParticipant>(
        participant, [](IDistributedShardParticipant*) {}
    );
    THEMIS_DEBUG("DistributedTransactionManager: registered shard '{}'", shard_id);
}

bool DistributedTransactionManager::unregisterShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lk(state_->shards_mutex);
    return state_->shards.erase(shard_id) > 0;
}

size_t DistributedTransactionManager::shardCount() const {
    std::lock_guard<std::mutex> lk(state_->shards_mutex);
    return state_->shards.size();
}

bool DistributedTransactionManager::hasShard(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lk(state_->shards_mutex);
    return state_->shards.count(shard_id) > 0;
}

// ── Transaction lifecycle ─────────────────────────────────────────────────────

std::shared_ptr<DistributedTransaction>
DistributedTransactionManager::beginDistributedTransaction() {
    const std::string txn_id = generateTransactionId();

    state_->total_transactions.fetch_add(1, std::memory_order_relaxed);
    state_->active.fetch_add(1, std::memory_order_relaxed);

    THEMIS_DEBUG("DistributedTransactionManager: began txn '{}' ({} shard(s) registered)",
                 txn_id, shardCount());

    return std::make_shared<DistributedTransaction>(
        DistributedTransaction::PrivateTag{},
        txn_id,
        config_.shard_key_separator,
        state_  // share ownership so transaction survives manager destruction
    );
}

// ── Statistics ────────────────────────────────────────────────────────────────

DistributedTransactionManager::Statistics
DistributedTransactionManager::statistics() const {
    // All counters use relaxed ordering — statistics are approximate.
    Statistics s;
    s.total_transactions = state_->total_transactions.load(std::memory_order_relaxed);
    s.committed          = state_->committed.load(std::memory_order_relaxed);
    s.aborted            = state_->aborted.load(std::memory_order_relaxed);
    s.active             = state_->active.load(std::memory_order_relaxed);
    return s;
}

// ── ID generation ─────────────────────────────────────────────────────────────

std::string DistributedTransactionManager::generateTransactionId() {
    const uint64_t counter = state_->txn_counter.fetch_add(1, std::memory_order_relaxed);
    const auto now = std::chrono::system_clock::now();
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch()).count();

    std::ostringstream oss;
    oss << "dtx-" << std::hex << std::setw(12) << std::setfill('0') << ms
        << "-" << std::setw(8) << counter;
    return oss.str();
}

} // namespace storage
} // namespace themis

