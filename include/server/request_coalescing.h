/**
 * @file request_coalescing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;

namespace themis::server {

// ---------------------------------------------------------------------------
// RequestCoalescingManager
// ---------------------------------------------------------------------------

class RequestCoalescingManager {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        uint32_t max_waiters_per_key{100};

        std::chrono::milliseconds waiter_timeout{5000};

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

        double coalescingRatio() const noexcept {
            if (total_requests == 0) {
              return 0.0;
            }
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
    /**
     * @brief Request Coalescing Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit RequestCoalescingManager(const Config& config);

    // Non-copyable, movable
    RequestCoalescingManager(const RequestCoalescingManager&)            = delete;
    RequestCoalescingManager& operator=(const RequestCoalescingManager&) = delete;
    RequestCoalescingManager(RequestCoalescingManager&&)                 noexcept = default;
    RequestCoalescingManager& operator=(RequestCoalescingManager&&)      noexcept = default;

    ~RequestCoalescingManager() = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        std::function<http::response<http::string_body>(
            const http::request<http::string_body>&)> handler
    );

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    /**
     * @brief Reset Stats.
     */
    void resetStats();

    /**
     * @brief In Flight Count.
     * @return Return value.
     */
    std::size_t inFlightCount() const;

private:
    // -----------------------------------------------------------------------
    // Internal structures
    // -----------------------------------------------------------------------

    struct InFlight {
        std::shared_ptr<std::promise<http::response<http::string_body>>> promise;
        std::shared_future<http::response<http::string_body>> future;
        uint32_t waiter_count{0};
    };

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Make Key.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    static std::string makeKey(const http::request<http::string_body>& req);

    /**
     * @brief Is Coalescible.
     * @param[in] req Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
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

