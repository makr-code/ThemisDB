/**
 * @file icdc_batch_commit_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Batch Commit Coordinator Interface
 *
 * Coordinates exactly-once, multi-event batch commits for CDC consumers.
 * A batch groups a sequence of change events into an atomic unit; either
 * all events in the batch are committed (acknowledged) or none are, which
 * prevents partial acknowledgements and duplicate deliveries under consumer
 * restarts.
 *
 * Design constraints:
 *  - Only one open batch is allowed per coordinator at a time.  Calling
 *    beginBatch() while a batch is open returns an error (BatchAlreadyOpen).
 *  - commitBatch() is idempotent for the same BatchId if the batch has
 *    already been committed (returns CommitResult::AlreadyCommitted).
 *  - rollbackBatch() discards all events in the current batch; the
 *    coordinator returns to idle state.
 *  - All methods are thread-safe.
 *  - Maximum batch size is configurable via BatchConfig::max_batch_size;
 *    addEvent() returns BatchFull once this limit is reached.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

// ── BatchId ───────────────────────────────────────────────────────────────────

/// Opaque monotonically increasing batch identifier.
using BatchId = uint64_t;

// ── AddEventResult ────────────────────────────────────────────────────────────

enum class AddEventResult {
    Added,       ///< Event was added to the open batch
    NoBatchOpen, ///< No batch is currently open
    BatchFull,   ///< Batch has reached max_batch_size
};

// ── CommitResult ──────────────────────────────────────────────────────────────

enum class CommitResult {
    Committed,        ///< Batch was successfully committed
    NoBatchOpen,      ///< No batch is currently open
    AlreadyCommitted, ///< The batch with the given ID is already committed
    RolledBack,       ///< Batch was previously rolled back; cannot commit
};

// ── RollbackResult ────────────────────────────────────────────────────────────

enum class RollbackResult {
    RolledBack,   ///< Batch was successfully rolled back
    NoBatchOpen,  ///< No batch is currently open
};

// ── BatchConfig ───────────────────────────────────────────────────────────────

/**
 * @brief Configuration for the batch commit coordinator.
 */
struct BatchConfig {
    /// Maximum number of events per batch (0 = unlimited).
    std::size_t max_batch_size = 0;

    /// Maximum number of committed batches retained in the commit history
    /// (for AlreadyCommitted detection).  0 = use InMemoryBatchCommitCoordinator::kDefaultHistorySize.
    std::size_t commit_history_size = 1000;
};

// ── BatchStatus ───────────────────────────────────────────────────────────────

enum class BatchStatus {
    Idle,       ///< No open batch
    Open,       ///< A batch is currently accumulating events
    Committed,  ///< Current batch has been committed
    RolledBack, ///< Current batch was rolled back
};

// ── BatchInfo ─────────────────────────────────────────────────────────────────

/**
 * @brief Summary of the coordinator's current state.
 */
struct BatchInfo {
    BatchStatus status = BatchStatus::Idle; ///< Current lifecycle state
    BatchId     current_batch_id = 0;       ///< ID of the open batch (0 = none)
    std::size_t pending_event_count = 0;    ///< Events in the open batch
    std::size_t total_committed_batches = 0;///< Lifetime committed batches
    std::size_t total_rolled_back = 0;      ///< Lifetime rolled-back batches
};

// ── ICDCBatchCommitCoordinator ────────────────────────────────────────────────

/**
 * @brief Abstract interface for exactly-once CDC batch commit coordination.
 *
 * Thread-safety: all methods must be thread-safe in every implementation.
 */
class ICDCBatchCommitCoordinator {
public:
    virtual ~ICDCBatchCommitCoordinator() = default;

    /**
     * @brief Open a new batch.
     *
     * @return The new BatchId, or 0 if a batch is already open.
     */
    [[nodiscard]] virtual BatchId beginBatch() = 0;

    /**
     * @brief Add an event to the currently open batch.
     *
     * @param event  The event to stage.
     * @return AddEventResult indicating success or the failure reason.
     */
    [[nodiscard]] virtual AddEventResult addEvent(const Changefeed::ChangeEvent& event) = 0;

    /**
     * @brief Commit all staged events in the current batch.
     *
     * After a successful commit, the coordinator transitions to Committed and
     * is ready for the next beginBatch() call.  The committed events are
     * accessible via committedEvents(batch_id) until they age out of the
     * commit history.
     *
     * Calling commitBatch() when no batch is open returns NoBatchOpen.
     * If the most recent batch has already been committed (status ==
     * Committed), returns AlreadyCommitted — allowing safe duplicate-call
     * detection for the same batch lifecycle without a batch_id parameter.
     * If the most recent batch was rolled back, returns RolledBack.
     *
     * @return CommitResult indicating success or the failure reason.
     */
    [[nodiscard]] virtual CommitResult commitBatch() = 0;

    /**
     * @brief Discard all staged events and close the current batch.
     *
     * @return RollbackResult indicating success or the failure reason.
     */
    [[nodiscard]] virtual RollbackResult rollbackBatch() = 0;

    /**
     * @brief Return the current status of the coordinator.
     */
    [[nodiscard]] virtual BatchStatus status() const = 0;

    /**
     * @brief Return summary info for the current or most recently closed batch.
     */
    [[nodiscard]] virtual BatchInfo info() const = 0;

    /**
     * @brief Return the events committed in a specific batch.
     *
     * @param batch_id  ID of a previously committed batch.
     * @return Events in commit order, or an empty vector if not found.
     */
    [[nodiscard]] virtual std::vector<Changefeed::ChangeEvent> committedEvents(
        BatchId batch_id) const = 0;

    /**
     * @brief Return true if the given batch ID has been committed.
     */
    [[nodiscard]] virtual bool isCommitted(BatchId batch_id) const = 0;
};

// ── InMemoryBatchCommitCoordinator ────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory ICDCBatchCommitCoordinator.
 *
 * Suitable for unit tests and standalone use.  Committed batches are
 * retained in a FIFO ring bounded by BatchConfig::commit_history_size;
 * older batches are evicted to bound memory usage.
 */
class InMemoryBatchCommitCoordinator : public ICDCBatchCommitCoordinator {
public:
    static constexpr std::size_t kDefaultHistorySize = 1000;

    explicit InMemoryBatchCommitCoordinator(
        BatchConfig cfg = BatchConfig{})
        : config_(cfg)
        , history_limit_(cfg.commit_history_size == 0
                         ? kDefaultHistorySize
                         : cfg.commit_history_size)
        , next_batch_id_(1)
        , current_batch_id_(0)
        , status_(BatchStatus::Idle)
        , committed_count_(0)
        , rolled_back_count_(0)
    {}

    BatchId beginBatch() override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (status_ == BatchStatus::Open) return 0; // already open
        current_batch_id_ = next_batch_id_.fetch_add(1, std::memory_order_relaxed);
        pending_.clear();
        status_ = BatchStatus::Open;
        return current_batch_id_;
    }

    AddEventResult addEvent(const Changefeed::ChangeEvent& event) override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (status_ != BatchStatus::Open) {
          return AddEventResult::NoBatchOpen;
        }
        if (config_.max_batch_size > 0 &&
            pending_.size() >= config_.max_batch_size)
            return AddEventResult::BatchFull;
        pending_.push_back(event);
        return AddEventResult::Added;
    }

    CommitResult commitBatch() override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (status_ != BatchStatus::Open) {
            // Check history for idempotent re-commit detection
            if (committed_.count(current_batch_id_)) {
                return CommitResult::AlreadyCommitted;
            }
            if (status_ == BatchStatus::RolledBack) {
                return CommitResult::RolledBack;
            }
            return CommitResult::NoBatchOpen;
        }

        // Record in history
        evictOldestIfNeeded();
        committed_[current_batch_id_] = pending_;
        history_order_.push_back(current_batch_id_);

        pending_.clear();
        status_ = BatchStatus::Committed;
        committed_count_.fetch_add(1, std::memory_order_relaxed);
        return CommitResult::Committed;
    }

    RollbackResult rollbackBatch() override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (status_ != BatchStatus::Open) {
          return RollbackResult::NoBatchOpen;
        }
        pending_.clear();
        status_ = BatchStatus::RolledBack;
        rolled_back_count_.fetch_add(1, std::memory_order_relaxed);
        return RollbackResult::RolledBack;
    }

    BatchStatus status() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return status_;
    }

    BatchInfo info() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        BatchInfo i;
        i.status               = status_;
        i.current_batch_id     = current_batch_id_;
        i.pending_event_count  = pending_.size();
        i.total_committed_batches =
            committed_count_.load(std::memory_order_relaxed);
        i.total_rolled_back    =
            rolled_back_count_.load(std::memory_order_relaxed);
        return i;
    }

    std::vector<Changefeed::ChangeEvent> committedEvents(
        BatchId batch_id) const override
    {
        std::unique_lock<std::mutex> lk(mutex_);
        auto it = committed_.find(batch_id);
        if (it == committed_.end()) return {};
        return it->second;
    }

    bool isCommitted(BatchId batch_id) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return committed_.count(batch_id) > 0;
    }

private:
    void evictOldestIfNeeded() {
        while (history_order_.size() >= history_limit_) {
            committed_.erase(history_order_.front());
            history_order_.pop_front();
        }
    }

    mutable std::mutex mutex_;
    BatchConfig        config_;
    std::size_t        history_limit_;

    std::atomic<BatchId>   next_batch_id_;
    BatchId                current_batch_id_;
    BatchStatus            status_;
    std::vector<Changefeed::ChangeEvent> pending_;

    std::unordered_map<BatchId, std::vector<Changefeed::ChangeEvent>> committed_;
    std::deque<BatchId>    history_order_;

    std::atomic<std::size_t> committed_count_;
    std::atomic<std::size_t> rolled_back_count_;
};

} // namespace cdc
} // namespace themis
