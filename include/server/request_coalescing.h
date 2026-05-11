/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            request_coalescing.h                               ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:47:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     239                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 830284816f  2026-03-13  feat: implement RequestCoalescingManager and SmartRouter ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file request_coalescing.h
 * @brief Request coalescing for the API Gateway.
 *
 * Merges duplicate in-flight requests to the same resource so that only one
 * backend call is executed and the response is shared with all waiters.
 *
 * Scenario:
 * ```
 * t=0ms  Client A → GET /api/v1/entities/123  (backend call starts)
 * t=2ms  Client B → GET /api/v1/entities/123  (coalesced – waits for A)
 * t=5ms  Backend returns → both A and B receive the same response
 * ```
 *
 * Benefits:
 *  - Reduces backend load for hot resources.
 *  - Lowers tail latency for duplicate concurrent requests.
 *  - Especially effective for expensive read-only queries.
 *
 * Thread safety: all public methods are safe to call from multiple threads.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include &lt;optional&gt;
#include <string>
#include <unordered_map>

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;

namespace themis::server {

// ---------------------------------------------------------------------------
// RequestCoalescingManager
// ---------------------------------------------------------------------------

/**
 * @brief Coalesces duplicate in-flight GET requests to the same resource.
 *
 * Only idempotent (GET/HEAD) requests are eligible for coalescing; non-safe
 * methods are passed directly to the backend without waiting or sharing.
 *
 * Usage:
 * ```cpp
 * RequestCoalescingManager coalescer;
 *
 * auto response = coalescer.handle(req, [&](const auto& r) {
 *     return backend.execute(r);   // called at most once per in-flight key
 * });
 * ```
 */
class RequestCoalescingManager {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Maximum number of waiters allowed per in-flight request.
        /// If this limit is exceeded the new request is forwarded directly.
        uint32_t max_waiters_per_key{100};

        /// How long a waiter will block for an in-flight response before
        /// giving up and dispatching its own backend call.
        std::chrono::milliseconds waiter_timeout{5000};

        /// When false, the manager is transparent and every request reaches
        /// the backend (useful for benchmarking overhead).
        bool enabled{true};
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    struct Stats {
        uint64_t total_requests{0};
        uint64_t coalesced_requests{0};  ///< Requests merged with an in-flight call
        uint64_t backend_calls{0};       ///< Actual backend invocations
        uint64_t timeout_fallbacks{0};   ///< Waiters that fell back after timeout
        uint64_t capacity_fallbacks{0};  ///< Waiters that fell back due to max_waiters

        /// Fraction of requests that were served from coalescing (0.0–1.0).
        double coalescingRatio() const noexcept {
            if (total_requests == 0) return 0.0;
            return static_cast<double>(coalesced_requests) /
                   static_cast<double>(total_requests);
        }

        nlohmann::json toJson() const {
            return {
                {"total_requests",      total_requests},
                {"coalesced_requests",  coalesced_requests},
                {"backend_calls",       backend_calls},
                {"timeout_fallbacks",   timeout_fallbacks},
                {"capacity_fallbacks",  capacity_fallbacks},
                {"coalescing_ratio",    coalescingRatio()},
            };
        }
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    RequestCoalescingManager();
    explicit RequestCoalescingManager(const Config& config);

    // Non-copyable, movable
    RequestCoalescingManager(const RequestCoalescingManager&)            = delete;
    RequestCoalescingManager& operator=(const RequestCoalescingManager&) = delete;
    RequestCoalescingManager(RequestCoalescingManager&&)                 = default;
    RequestCoalescingManager& operator=(RequestCoalescingManager&&)      = default;

    ~RequestCoalescingManager() = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * @brief Handle a request, coalescing it with any in-flight request to the
     *        same resource key.
     *
     * @param req     Incoming HTTP request.
     * @param handler Backend handler invoked at most once per in-flight key.
     * @return HTTP response (either from the backend or shared from coalescing).
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(
            const http::request<http::string_body>&)> handler
    );

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Return a copy of current statistics.
     */
    Stats getStats() const;

    /**
     * @brief Reset all counters to zero.
     */
    void resetStats();

    /**
     * @brief Return the number of requests currently in-flight (being coalesced).
     */
    std::size_t inFlightCount() const;

private:
    // -----------------------------------------------------------------------
    // Internal structures
    // -----------------------------------------------------------------------

    /**
     * @brief State for a single in-flight backend call.
     *
     * Multiple threads may read future_ once it is set; the originating thread
     * owns the promise.
     */
    struct InFlight {
        std::shared_ptr<std::promise<http::response<http::string_body>>> promise;
        std::shared_future<http::response<http::string_body>> future;
        uint32_t waiter_count{0};
    };

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Build the coalescing key for a request.
     *
     * For GET/HEAD the key is "<METHOD>|&lt;path&gt;" (query-string excluded so that
     * minor query variations still share the same backend call).
     */
    static std::string makeKey(const http::request<http::string_body>& req);

    /**
     * @brief Return true if the request method is eligible for coalescing.
     */
    static bool isCoalescible(const http::request<http::string_body>& req) noexcept;

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------

    Config config_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, InFlight> in_flight_;

    // Atomic counters – updated without holding mutex_ for performance.
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> coalesced_requests_{0};
    std::atomic<uint64_t> backend_calls_{0};
    std::atomic<uint64_t> timeout_fallbacks_{0};
    std::atomic<uint64_t> capacity_fallbacks_{0};
};

} // namespace themis::server
