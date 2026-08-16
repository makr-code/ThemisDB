/**
 * @file query_canceller.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * ## Blocking Operations & Timeout Safety
 *
 * All public methods that interact with the shared token registry use
 * std::timed_mutex with explicit timeout. If the lock cannot be acquired
 * within kLockTimeout, the operation returns a safe default (typically
 * registering a token that cannot be cancelled via the registry, or returning
 * false for cancel/unregister operations).
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

// ── QueryCanceller ──────────────────────────────────────────────────────────

QueryCanceller& QueryCanceller::instance() {
    static QueryCanceller inst;
    return inst;
}

std::shared_ptr<QueryCancellationToken>
QueryCanceller::registerQuery(const std::string& request_id) {
    auto token = std::make_shared<QueryCancellationToken>();
    // Use timed_lock to prevent indefinite blocking if the mutex is contended.
    // If we timeout, the token is still valid for the caller, just not 
    // cancellable via this registry (caller can cancel directly via the token).
    std::unique_lock<std::timed_mutex> lock(mutex_, kLockTimeout);
    if (!lock.owns_lock()) {
        THEMIS_WARN("QueryCanceller::registerQuery: lock timeout for '{}'; token not registered",
                    request_id);
        return token; // token still usable by caller; just not cancelable via registry
    }
    tokens_[request_id] = token;
    return token;
}

bool QueryCanceller::cancel(const std::string& request_id) {
    // Use timed_lock to prevent indefinite blocking.
    // This ensures cancellation requests don't hang if the registry is locked
    // by another thread for an extended period.
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

void QueryCanceller::unregisterQuery(const std::string& request_id) {
    // Use timed_lock to prevent indefinite blocking on cleanup.
    // If timeout occurs, the token will eventually be cleaned up by weak_ptr
    // expiration when all references are released.
    std::unique_lock<std::timed_mutex> lock(mutex_, kLockTimeout);
    if (!lock.owns_lock()) {
        THEMIS_WARN("QueryCanceller::unregisterQuery: lock timeout for '{}'", request_id);
        return;
    }
    tokens_.erase(request_id);
}

} // namespace query
} // namespace themis
