/**
 * @file request_coalescing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/request_coalescing.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>
#include "utils/logger.h"

namespace themis::server {

// ===========================================================================
// Construction
// ===========================================================================

RequestCoalescingManager::RequestCoalescingManager()
    : config_{}
{}

RequestCoalescingManager::RequestCoalescingManager(const Config& config)
    : config_(config)
{}

// ===========================================================================
// Core API
// ===========================================================================

http::response<http::string_body> RequestCoalescingManager::handle(
    const http::request<http::string_body>& req,
    std::function<http::response<http::string_body>(
        const http::request<http::string_body>&)> handler
)
{
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    // Fast path: coalescing disabled or method not eligible.
    if (!config_.enabled || !isCoalescible(req)) {
        backend_calls_.fetch_add(1, std::memory_order_relaxed);
        return handler([[maybe_unused]] req);
    }

    const std::string key = makeKey(req);

    // -----------------------------------------------------------------------
    // Phase 1: check if there is an in-flight request for the same key.
    // -----------------------------------------------------------------------
    std::shared_future<http::response<http::string_body>> shared_future;
    bool is_originator = false;
    std::shared_ptr<std::promise<http::response<http::string_body>>> my_promise;

    {
        std::lock_guard<std::mutex> lk(mutex_);

        auto it = in_flight_.find(key);
        if (it != in_flight_.end()) {
            // There is an in-flight request – attempt to coalesce.
            if (it->second.waiter_count >= config_.max_waiters_per_key) {
                // Too many waiters; fall through to a direct backend call.
                capacity_fallbacks_.fetch_add(1, std::memory_order_relaxed);
                backend_calls_.fetch_add(1, std::memory_order_relaxed);
                return handler([[maybe_unused]] req);
            }
            it->second.waiter_count++;
            shared_future = it->second.future;
            coalesced_requests_.fetch_add(1, std::memory_order_relaxed);
            spdlog::debug("RequestCoalescing: coalesced '{}' (waiters={})",
                          key, it->second.waiter_count);
        } else {
            // No in-flight request – this thread becomes the originator.
            InFlight slot;
            slot.promise = std::make_shared<
                std::promise<http::response<http::string_body>>>();
            slot.future  = slot.promise->get_future().share();
            slot.waiter_count = 0;
            shared_future = slot.future;
            my_promise    = slot.promise;
            in_flight_.emplace(key, std::move(slot));
            is_originator = true;
        }
    }

    // -----------------------------------------------------------------------
    // Phase 2: originator executes the backend call; waiters block on future.
    // -----------------------------------------------------------------------
    if (is_originator) {
        backend_calls_.fetch_add(1, std::memory_order_relaxed);
        try {
            auto response = handler([[maybe_unused]] req);
            my_promise->set_value(response);

            // Remove the in-flight entry so new requests create a fresh slot.
            {
                std::lock_guard<std::mutex> lk(mutex_);
                in_flight_.erase(key);
            }
            spdlog::debug("RequestCoalescing: originator completed key='{}'", key);
            return response;
        } catch (...) {
            THEMIS_WARN("request_coalescing: unhandled exception caught");
            // Propagate the exception to all waiters, then clean up.
            try { my_promise->set_exception(std::current_exception()); }
            catch (const std::future_error&) { /* promise already satisfied */ }
            {
                std::lock_guard<std::mutex> lk(mutex_);
                in_flight_.erase(key);
            }
            throw;
        }
    }

    // -----------------------------------------------------------------------
    // Phase 3: waiter – block on the shared future with timeout.
    // -----------------------------------------------------------------------
    auto status = shared_future.wait_for(config_.waiter_timeout);
    if (status == std::future_status::ready) {
        try {
            return shared_future.get();
        } catch (const std::exception& e) {
            // Originator threw – fall back to a direct call.
            spdlog::warn("RequestCoalescing: originator failed for '{}': {}; "
                         "falling back to direct call", key, e.what());
            timeout_fallbacks_.fetch_add(1, std::memory_order_relaxed);
            backend_calls_.fetch_add(1, std::memory_order_relaxed);
            return handler([[maybe_unused]] req);
        }
    }

    // Timeout: dispatch own backend call.
    spdlog::warn("RequestCoalescing: waiter timed out for '{}'; "
                 "falling back to direct call", key);
    timeout_fallbacks_.fetch_add(1, std::memory_order_relaxed);
    backend_calls_.fetch_add(1, std::memory_order_relaxed);
    return handler([[maybe_unused]] req);
}

// ===========================================================================
// Observability
// ===========================================================================

RequestCoalescingManager::Stats RequestCoalescingManager::getStats() const {
    Stats s;
    s.total_requests     = total_requests_.load(std::memory_order_relaxed);
    s.coalesced_requests = coalesced_requests_.load(std::memory_order_relaxed);
    s.backend_calls      = backend_calls_.load(std::memory_order_relaxed);
    s.timeout_fallbacks  = timeout_fallbacks_.load(std::memory_order_relaxed);
    s.capacity_fallbacks = capacity_fallbacks_.load(std::memory_order_relaxed);
    return s;
}

void RequestCoalescingManager::resetStats() {
    total_requests_.store(0, std::memory_order_relaxed);
    coalesced_requests_.store(0, std::memory_order_relaxed);
    backend_calls_.store(0, std::memory_order_relaxed);
    timeout_fallbacks_.store(0, std::memory_order_relaxed);
    capacity_fallbacks_.store(0, std::memory_order_relaxed);
}

std::size_t RequestCoalescingManager::inFlightCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(in_flight_.size());
}

// ===========================================================================
// Private helpers
// ===========================================================================

std::string RequestCoalescingManager::makeKey(
    const http::request<http::string_body>& req)
{
    // Include HTTP method so POST /x is never coalesced with GET /x.
    std::string method = std::string(req.method_string());

    std::string target = std::string(req.target());
    // Strip the query string so that identical paths with minor query-parameter
    // variations still share the same in-flight backend call.
    //
    // WARNING: this means requests with different query parameters are coalesced
    // together, which is only safe for endpoints whose response is independent of
    // query parameters (e.g., pure path-based entity lookups).  Callers that
    // serve query-sensitive endpoints MUST disable coalescing for those paths, or
    // replace this manager with a custom implementation that includes the full
    // query string in the key.
    auto qpos = target.find('?');
    if (qpos != std::string::npos) {
        target.erase(qpos);
    }

    return method + "|" + target;
}

bool RequestCoalescingManager::isCoalescible(
    const http::request<http::string_body>& req) noexcept
{
    // Only safe, idempotent methods benefit from coalescing.
    return req.method() == http::verb::get ||
           req.method() == http::verb::head;
}

} // namespace themis::server

