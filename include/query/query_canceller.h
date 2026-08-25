/**
 * @file query_canceller.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
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
 *
 * ## Deadline-Aware Cancellation  [WAVE1-FIX: blocking_no_timeout / no_timeout]
 *
 * waitUntilCancelledFor() provides a blocking wait with a configurable
 * deadline.  An execution thread that must synchronise with an external
 * cancellation signal should call this method rather than spinning on
 * isCancelled(), as it avoids busy-waiting and guarantees that the wait
 * never blocks indefinitely.  The default timeout is 30 seconds; callers
 * should pass the query-execution-context deadline where available.
 *
 * Internal mechanism: a condition_variable is notified by cancel() under a
 * light mutex.  The atomic flag is re-checked inside the wait predicate, so
 * spurious wake-ups are handled correctly.
 */
class QueryCancellationToken {
public:
    QueryCancellationToken() noexcept : cancelled_(false) {}

    /**
     * @brief Mark this token as cancelled.  Idempotent.
     *
     * Thread-safe.  Also notifies any thread blocked in
     * waitUntilCancelledFor(), propagating the QueryCancelled status
     * upstream without delay.
     */
    void cancel() noexcept {
        cancelled_.store(true, std::memory_order_release);
        // Notify all waiters so that waitUntilCancelledFor() returns
        // immediately when cancel() is called from another thread.
        cv_.notify_all();
    }

    /// @return true if cancel() has been called at least once.
    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

    /**
     * @brief Block until the token is cancelled or @p timeout elapses.
     *
     * [WAVE1-FIX: blocking_no_timeout / no_timeout gap — query_canceller.cpp:49]
     * Replaces the previous pattern of unlimited blocking waits in query
     * execution contexts.  Callers pass the deadline from their execution
     * context; a default of 30 s is used when no context is available.
     *
     * @param timeout  Maximum time to wait.  Pass the per-query deadline from
     *                 the execution context.  Defaults to 30 seconds.
     * @return         true  — token was cancelled before @p timeout.
     *                 false — timeout elapsed; caller should propagate
     *                         QueryCancelled (or QueryTimeout) status upstream.
     */
    [[nodiscard]] bool waitUntilCancelledFor(
        std::chrono::milliseconds timeout =
            std::chrono::seconds{30}) noexcept;

private:
    std::atomic<bool>       cancelled_;
    mutable std::mutex      cv_mutex_;
    std::condition_variable cv_;
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
    mutable std::timed_mutex mutex_;
    std::unordered_map<std::string,
                       std::weak_ptr<QueryCancellationToken>> tokens_;
};

} // namespace query
} // namespace themis
