/**
 * @file query_canceller.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * ## Blocking Operations & Timeout Safety  [WAVE1-FIX: blocking_no_timeout / no_timeout]
 *
 * All public methods that interact with the shared token registry use
 * std::timed_mutex with explicit timeout (kLockTimeout = 200 ms). If the
 * lock cannot be acquired within kLockTimeout, the operation returns a safe
 * default (typically registering a token that cannot be cancelled via the
 * registry, or returning false for cancel/unregister operations).
 *
 * Additionally, QueryCancellationToken::waitUntilCancelledFor() adds
 * deadline-aware cancellation: execution threads blocked on an external
 * cancellation signal call this method instead of an unlimited wait().
 * The default timeout is 30 s; callers pass the per-query execution-context
 * deadline.  On timeout, the caller propagates QueryCancelled status upstream.
 *
 * This prevents indefinite blocking due to contention or deadlock, ensuring
 * the cancellation registry is never a bottleneck in query execution.
 */


#include "query/query_canceller.h"

#include "utils/logger.h"

#include <chrono>

namespace themis {
namespace query {

// Timeout for acquiring the internal registry lock.  A 200 ms budget is
// generous for a simple hash-map operation; if we cannot acquire the lock
// within this window the registry is considered unavailable for that call.
static constexpr std::chrono::milliseconds kLockTimeout{200};

// ── QueryCancellationToken ──────────────────────────────────────────────────

bool QueryCancellationToken::waitUntilCancelledFor(
        std::chrono::milliseconds timeout) noexcept {
    /**
     * @brief Lock.
     * @param[in] cv_mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::mutex> lock(cv_mutex_);
    return cv_.wait_for(lock, timeout,
                        [this]() noexcept { return isCancelled(); });
}


/**
 * @brief Instance.
 * @return Return value.
 * @details Implements instance without additional internal calls.
 */
QueryCanceller& QueryCanceller::instance() {
    static QueryCanceller inst;
    return inst;
}

std::shared_ptr<QueryCancellationToken>
QueryCanceller::registerQuery(const std::string& request_id) {
    auto token = std::make_shared<QueryCancellationToken>();
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @param[in] kLockTimeout Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::timed_mutex> lock(mutex_, kLockTimeout);
    if (!lock.owns_lock()) {
        THEMIS_WARN("QueryCanceller::registerQuery: lock timeout for '{}'; token not registered",
                    request_id);
        return token; // token still usable by caller; just not cancelable via registry
    }
    tokens_[request_id] = token;
    return token;
}

/**
 * @brief Cancel.
 * @param[in] request_id Identifier of the request.
 * @return True when the operation succeeds.
 * @details Calls: lock(), owns_lock(), THEMIS_WARN(), find(), end(), erase().
 */
bool QueryCanceller::cancel(const std::string& request_id) {
    std::unique_lock<std::timed_mutex> lock(mutex_, kLockTimeout);
    if (!lock.owns_lock()) {
        THEMIS_WARN("QueryCanceller::cancel: lock timeout for '{}'", request_id);
        return false;
    }
    auto it = tokens_.find(request_id);
    if (it == tokens_.end()) {
        return false;
    }
    auto token = it->second.lock();
    if (!token) {
        // Token expired; clean up the dead weak_ptr.
        // Use erase-after-find pattern to ensure iterator validity.
        tokens_.erase(it);
        return false;
    }
    token->cancel();
    return true;
}

/**
 * @brief Unregister Query.
 * @param[in] request_id Identifier of the request.
 * @details Calls: lock(), owns_lock(), THEMIS_WARN(), erase().
 */
void QueryCanceller::unregisterQuery(const std::string& request_id) {
    std::unique_lock<std::timed_mutex> lock(mutex_, kLockTimeout);
    if (!lock.owns_lock()) {
        THEMIS_WARN("QueryCanceller::unregisterQuery: lock timeout for '{}'", request_id);
        return;
    }
    tokens_.erase(request_id);
}

} // namespace query
} // namespace themis
