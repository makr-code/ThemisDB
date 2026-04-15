/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_canceller.h                                  ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:04:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     181                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ab3b22a88e  2026-03-09  feat(query): implement query cancellation via request ID ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file query_canceller.h
 * @brief Query cancellation via request ID for the AQL execution pipeline.
 *
 * Provides cooperative cancellation of in-flight AQL queries.  A caller
 * registers a query with a unique request ID and receives a shared
 * QueryCancellationToken.  Any thread that holds the same request ID can
 * cancel the token via QueryCanceller::cancel(); the executing query checks
 * the token at cancellation checkpoints and returns ERR_QUERY_CANCELLED.
 *
 * ## Usage – canceller side
 * @code
 *   auto token = QueryCanceller::instance().registerQuery("req-42");
 *   // … hand token to the execution thread …
 *   // Later, from a different thread (e.g. HTTP cancel handler):
 *   QueryCanceller::instance().cancel("req-42");
 * @endcode
 *
 * ## Usage – execution side
 * @code
 *   auto token = QueryCanceller::instance().registerQuery(request_id);
 *   auto guard = QueryCanceller::ScopedRegistration(request_id);
 *   if (token->isCancelled()) { ... }
 * @endcode
 *
 * Thread safety:
 *   QueryCancellationToken – individual operations are atomic; safe to call
 *   isCancelled() and cancel() from different threads concurrently.
 *
 *   QueryCanceller – all public methods are protected by an internal mutex.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace themis {
namespace query {

// ============================================================================
// QueryCancellationToken
// ============================================================================

/**
 * @brief Shared cancellation flag for a single in-flight query.
 *
 * Created by QueryCanceller::registerQuery() and shared (via shared_ptr)
 * with the execution thread.  The cancel() method is lock-free (atomic
 * store), so it is safe to call from a signal handler or a high-priority
 * HTTP worker thread.
 */
class QueryCancellationToken {
public:
    QueryCancellationToken() noexcept : cancelled_(false) {}

    /// Mark this token as cancelled.  Idempotent.
    void cancel() noexcept {
        cancelled_.store(true, std::memory_order_release);
    }

    /// @return true if cancel() has been called at least once.
    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> cancelled_;
};

// ============================================================================
// QueryCanceller
// ============================================================================

/**
 * @brief Thread-safe registry that maps request IDs to cancellation tokens.
 *
 * The registry holds only weak_ptr references to the tokens; the strong
 * reference lives with the executing query.  Tokens are automatically
 * cleaned up when the query completes and the strong reference is dropped.
 *
 * Use the process-singleton via QueryCanceller::instance() or create a
 * local instance for unit testing.
 */
class QueryCanceller {
public:
    QueryCanceller() = default;

    /// Access the process-wide singleton.
    static QueryCanceller& instance();

    /**
     * @brief Register a query with the given request ID.
     *
     * If a token for @p request_id already exists (i.e. a previous query
     * with the same ID has not yet been unregistered) the existing token is
     * replaced.
     *
     * @param request_id  Caller-assigned unique identifier for this query.
     * @return            A shared token whose isCancelled() flag the
     *                    execution thread should poll at checkpoints.
     */
    std::shared_ptr<QueryCancellationToken> registerQuery(const std::string& request_id);

    /**
     * @brief Cancel the query identified by @p request_id.
     *
     * Signals the associated token.  The executing thread will observe the
     * cancellation at its next cooperative checkpoint.
     *
     * @param request_id  The ID passed to registerQuery().
     * @return            true  if a live token was found and cancelled.
     *                    false if no live token exists for this ID (already
     *                          finished or never registered).
     */
    bool cancel(const std::string& request_id);

    /**
     * @brief Remove the registration for @p request_id.
     *
     * Called automatically by ScopedRegistration.  Safe to call even if the
     * ID was never registered.
     */
    void unregisterQuery(const std::string& request_id);

    /**
     * @brief RAII guard that unregisters a query ID when it goes out of scope.
     */
    class ScopedRegistration {
    public:
        ScopedRegistration(std::string request_id,
                           QueryCanceller& canceller = QueryCanceller::instance())
            : request_id_(std::move(request_id)), canceller_(canceller) {}

        ~ScopedRegistration() { canceller_.unregisterQuery(request_id_); }

        // Non-copyable, non-moveable
        ScopedRegistration(const ScopedRegistration&) = delete;
        ScopedRegistration& operator=(const ScopedRegistration&) = delete;

    private:
        std::string    request_id_;
        QueryCanceller& canceller_;
    };

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string,
                       std::weak_ptr<QueryCancellationToken>> tokens_;
};

} // namespace query
} // namespace themis
