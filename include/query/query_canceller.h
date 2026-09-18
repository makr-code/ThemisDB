/**
 * @file query_canceller.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

class QueryCancellationToken {
public:
    QueryCancellationToken() noexcept : cancelled_(false) {}

    void cancel() noexcept {
        cancelled_.store(true, std::memory_order_release);
        // Notify all waiters so that waitUntilCancelledFor() returns
        // immediately when cancel() is called from another thread.
        cv_.notify_all();
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

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

class QueryCanceller {
public:
    QueryCanceller() = default;

    /**
     * @brief Instance.
     * @return Return value.
     */
    static QueryCanceller& instance();

    /**
     * @brief Register Query.
     * @param[in] request_id Identifier of the request.
     * @return Return value.
     */
    std::shared_ptr<QueryCancellationToken> registerQuery(const std::string& request_id);

    /**
     * @brief Cancel.
     * @param[in] request_id Identifier of the request.
     * @return True when the operation succeeds.
     */
    bool cancel(const std::string& request_id);

    /**
     * @brief Unregister Query.
     * @param[in] request_id Identifier of the request.
     */
    void unregisterQuery(const std::string& request_id);

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
