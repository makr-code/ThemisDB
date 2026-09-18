/**
 * @file icdc_replay_controller.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Replay Controller Interface
 *
 * Provides time-based and sequence-based changefeed replay for CDC
 * consumers.  Callers can request a bounded replay window (by wall-clock
 * range or sequence range) and iterate the resulting events as a snapshot.
 *
 * Design constraints:
 *  - Replay is always a bounded, read-only view; it never affects the
 *    live event stream or any subscriber state.
 *  - ReplaySession objects are single-use; once drained they return an
 *    empty vector and `done()` returns true.
 *  - All methods on ICDCReplayController and ReplaySession are thread-safe.
 *  - A maximum of ReplayOptions::max_events_per_session events are
 *    returned per session to bound memory usage.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

// ── ReplayOptions ─────────────────────────────────────────────────────────────

struct ReplayOptions {
    uint64_t from_sequence = 0;

    uint64_t to_sequence = 0;

    int64_t from_timestamp_ms = 0;

    int64_t to_timestamp_ms = 0;

    std::string key_prefix;

    std::vector<Changefeed::ChangeEventType> event_types;

    std::size_t batch_size = 100;

    std::size_t max_events_per_session = 0;
};

// ── ReplaySessionState ────────────────────────────────────────────────────────

enum class ReplaySessionState {
    Active,    ///< Session has more events available
    Done,      ///< All events within the replay window have been delivered
    Cancelled, ///< Session was explicitly cancelled
};

// ── IReplaySession ────────────────────────────────────────────────────────────

class IReplaySession {
public:
    /**
     * @brief IReplay Session.
     * @return Return value.
     */
    virtual ~IReplaySession() = default;

    [[nodiscard]] virtual std::vector<Changefeed::ChangeEvent> nextBatch() = 0;

    [[nodiscard]] virtual bool done() const = 0;

    /**
     * @brief Cancel.
     */
    virtual void cancel() = 0;

    [[nodiscard]] virtual ReplaySessionState state() const = 0;

    [[nodiscard]] virtual std::size_t deliveredCount() const = 0;
};

// ── ICDCReplayController ──────────────────────────────────────────────────────

class ICDCReplayController {
public:
    /**
     * @brief ICDCReplay Controller.
     * @return Return value.
     */
    virtual ~ICDCReplayController() = default;

    [[nodiscard]] virtual std::unique_ptr<IReplaySession> beginReplay(
        const ReplayOptions& options) = 0;

    [[nodiscard]] virtual std::unique_ptr<IReplaySession> replayFromTimestamp(
        int64_t from_timestamp_ms,
        int64_t to_timestamp_ms = 0) = 0;

    [[nodiscard]] virtual std::unique_ptr<IReplaySession> replayFromSequence(
        uint64_t from_sequence,
        uint64_t to_sequence = 0) = 0;

    [[nodiscard]] virtual std::size_t totalSessionsCreated() const = 0;
};

// ── InMemoryReplaySession ─────────────────────────────────────────────────────

class InMemoryReplaySession : public IReplaySession {
public:
    InMemoryReplaySession(std::vector<Changefeed::ChangeEvent> events,
                          std::size_t batch_size)
        : events_(std::move(events))
        , batch_size_(batch_size == 0 ? 100 : batch_size)
        , cursor_(0)
        , delivered_(0)
        , state_(ReplaySessionState::Active)
    {}

    std::vector<Changefeed::ChangeEvent> nextBatch() override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        if (state_ != ReplaySessionState::Active) return {};

        const std::size_t remaining = events_.size() - cursor_;
        if (remaining == 0) {
            state_ = ReplaySessionState::Done;
            return {};
        }

        const std::size_t n = std::min(batch_size_, remaining);
        std::vector<Changefeed::ChangeEvent> batch(
            events_.begin() + static_cast<std::ptrdiff_t>(cursor_),
            events_.begin() + static_cast<std::ptrdiff_t>(cursor_ + n));
        cursor_    += n;
        delivered_ += n;

        if (cursor_ >= events_.size()) {
            state_ = ReplaySessionState::Done;
        }
        return batch;
    }

    bool done() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return state_ != ReplaySessionState::Active;
    }

    void cancel() override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        state_ = ReplaySessionState::Cancelled;
    }

    ReplaySessionState state() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return state_;
    }

    std::size_t deliveredCount() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return delivered_;
    }

private:
    mutable std::mutex                   mutex_;
    std::vector<Changefeed::ChangeEvent> events_;
    std::size_t                          batch_size_;
    std::size_t                          cursor_;
    std::size_t                          delivered_;
    ReplaySessionState                   state_;
};

// ── InMemoryReplayController ──────────────────────────────────────────────────

class InMemoryReplayController : public ICDCReplayController {
public:
    /**
     * @brief In Memory Replay Controller.
     * @param[in,out] feed Input/output parameter.
     * @return Return value.
     */
    explicit InMemoryReplayController(Changefeed* feed)
        : feed_(feed), sessions_created_(0) {}

    // ── ICDCReplayController ─────────────────────────────────────────────────

    std::unique_ptr<IReplaySession> beginReplay(
        const ReplayOptions& options) override
    {
        auto events = fetchAndFilter(options);
        sessions_created_.fetch_add(1, std::memory_order_relaxed);
        return std::make_unique<InMemoryReplaySession>(
            std::move(events), options.batch_size);
    }

    std::unique_ptr<IReplaySession> replayFromTimestamp(
        int64_t from_timestamp_ms,
        int64_t to_timestamp_ms = 0) override
    {
        ReplayOptions opts;
        opts.from_timestamp_ms = from_timestamp_ms;
        opts.to_timestamp_ms   = to_timestamp_ms;
        return beginReplay(opts);
    }

    std::unique_ptr<IReplaySession> replayFromSequence(
        uint64_t from_sequence,
        uint64_t to_sequence = 0) override
    {
        ReplayOptions opts;
        opts.from_sequence = from_sequence;
        opts.to_sequence   = to_sequence;
        return beginReplay(opts);
    }

    std::size_t totalSessionsCreated() const override {
        return sessions_created_.load(std::memory_order_relaxed);
    }

private:
    std::vector<Changefeed::ChangeEvent> fetchAndFilter(
        const ReplayOptions& opts) const
    {
        Changefeed::ListOptions lo;
        // Both ICDCReplayController and Changefeed::ListOptions use the same
        // exclusive-lower-bound semantics: from_sequence returns events with
        // sequence > from_sequence (0 = all events from the beginning).
        lo.from_sequence = opts.from_sequence;
        lo.to_sequence   = opts.to_sequence;
        if (opts.max_events_per_session > 0) {
            lo.limit = opts.max_events_per_session;
        } else {
            lo.limit = std::numeric_limits<std::size_t>::max();
        }
        if (!opts.key_prefix.empty()) {
            lo.key_prefix = opts.key_prefix;
        }

        auto raw = feed_->listEvents(lo);

        // Post-filter by timestamp and event_types (not natively supported by
        // ListOptions for the combination we need).
        std::vector<Changefeed::ChangeEvent> out = {};

        out.reserve(raw.size());
        for (auto& ev : raw) {
            if (opts.from_timestamp_ms > 0 &&
                ev.timestamp_ms < opts.from_timestamp_ms) continue;
            if (opts.to_timestamp_ms > 0 &&
                ev.timestamp_ms > opts.to_timestamp_ms)  continue;
            if (!opts.event_types.empty()) {
                bool found = false;
                for (auto t : opts.event_types) {
                    if (ev.type == t) { found = true; break; }
                }
                if (!found) {
                  continue;
                }
            }
            if (opts.max_events_per_session > 0 &&
                out.size() >= opts.max_events_per_session) break;
            out.push_back(std::move(ev));
        }
        return out;
    }

    Changefeed*               feed_;
    std::atomic<std::size_t>  sessions_created_;
};

} // namespace cdc
} // namespace themis
