/**
 * @file smart_routing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <atomic>
#include <deque>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis::server {

// ---------------------------------------------------------------------------
// BackendEndpoint – descriptor for a routable backend
// ---------------------------------------------------------------------------

struct BackendEndpoint {
    std::string backend_id;   ///< Unique identifier (e.g. "shard-0")
    std::string address;      ///< Host name or IP
    uint16_t    port{8080};   ///< HTTP port

    bool operator==(const BackendEndpoint& o) const noexcept {
        return backend_id == o.backend_id;
    }
};

// ---------------------------------------------------------------------------
// SmartRouter
// ---------------------------------------------------------------------------

class SmartRouter {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        uint32_t latency_window_size{100};

        double tail_latency_threshold_ms{500.0};

        uint32_t min_cache_prediction_hits{3};

        bool enable_cache_prediction{true};
    };

    // -----------------------------------------------------------------------
    // Per-backend runtime statistics
    // -----------------------------------------------------------------------

    struct BackendStats {
        std::string backend_id;
        double      avg_latency_ms{0.0};     ///< Rolling average over the window
        double      p99_latency_ms{0.0};     ///< 99th percentile over the window
        uint64_t    total_requests{0};        ///< All requests routed here
        uint32_t    active_connections{0};    ///< Currently in-flight requests
        uint64_t    cache_hits{0};            ///< Recorded cache hits
        uint64_t    cache_misses{0};          ///< Recorded cache misses
        uint32_t    latency_samples{0};       ///< Samples in current window

        nlohmann::json toJson() const {
            return {
                {"backend_id",        backend_id},
                {"avg_latency_ms",    avg_latency_ms},
                {"p99_latency_ms",    p99_latency_ms},
                {"total_requests",    total_requests},
                {"active_connections", active_connections},
                {"cache_hits",        cache_hits},
                {"cache_misses",      cache_misses},
                {"latency_samples",   latency_samples},
            };
        }
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    SmartRouter();

    /**
     * @brief Smart Router.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SmartRouter(const Config& config);

    SmartRouter(const SmartRouter&)            = delete;
    SmartRouter& operator=(const SmartRouter&) = delete;
    SmartRouter(SmartRouter&&)                 noexcept = default;
    SmartRouter& operator=(SmartRouter&&)      noexcept = default;

    ~SmartRouter() = default;

    // -----------------------------------------------------------------------
    // Backend registry
    // -----------------------------------------------------------------------

    /**
     * @brief Add Backend.
     * @param[in] endpoint Input parameter.
     */
    void addBackend(const BackendEndpoint& endpoint);

    /**
     * @brief Remove Backend.
     * @param[in] backend_id Identifier of the backend.
     */
    void removeBackend(const std::string& backend_id);

    /**
     * @brief List Backends.
     * @return Return value.
     */
    std::vector<BackendEndpoint> listBackends() const;


    /**
     * @brief Record Latency.
     * @param[in] backend_id Identifier of the backend.
     * @param[in] latency_ms Input parameter.
     */
    void recordLatency(const std::string& backend_id, double latency_ms);

    /**
     * @brief Record Cache Hit.
     * @param[in] backend_id Identifier of the backend.
     * @param[in] resource_key Input parameter.
     */
    void recordCacheHit(const std::string& backend_id,
                        const std::string& resource_key);

    /**
     * @brief Record Cache Miss.
     * @param[in] backend_id Identifier of the backend.
     * @param[in] resource_key Input parameter.
     */
    void recordCacheMiss(const std::string& backend_id,
                         const std::string& resource_key);

    /**
     * @brief Increment Active Connections.
     * @param[in] backend_id Identifier of the backend.
     */
    void incrementActiveConnections(const std::string& backend_id);

    /**
     * @brief Decrement Active Connections.
     * @param[in] backend_id Identifier of the backend.
     */
    void decrementActiveConnections(const std::string& backend_id);

    // -----------------------------------------------------------------------
    // Routing decisions
    // -----------------------------------------------------------------------

    /**
     * @brief Route.
     * @param[in] resource_key Input parameter.
     * @return Return value.
     */
    std::optional<BackendEndpoint> route(const std::string& resource_key) const;

    /**
     * @brief Route Least Loaded.
     * @return Return value.
     */
    std::optional<BackendEndpoint> routeLeastLoaded() const;

    /**
     * @brief Predict Cached Backend.
     * @param[in] resource_key Input parameter.
     * @return Return value.
     */
    std::optional<BackendEndpoint> predictCachedBackend(
        const std::string& resource_key) const;

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Get All Stats.
     * @return Return value.
     */
    std::vector<BackendStats> getAllStats() const;

    /**
     * @brief Get Backend Stats.
     * @param[in] backend_id Identifier of the backend.
     * @return Return value.
     */
    BackendStats getBackendStats(const std::string& backend_id) const;

private:
    // -----------------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------------

    struct BackendState {
        BackendEndpoint endpoint;

        // Rolling latency window (newest at back)
        std::deque<double> latency_window;

        std::atomic<uint32_t> active_connections{0};
        uint64_t total_requests{0};
        uint64_t cache_hits{0};
        uint64_t cache_misses{0};

        // Per-resource-key access counts for cache prediction
        // key → number of cache hits on this backend
        std::unordered_map<std::string, uint32_t> key_hit_counts;

        // Cached statistics – updated eagerly in recordLatency() while the
        // unique_lock is held.  Safe to read under shared_lock.
        double cached_avg_latency{0.0};
        double cached_p99_latency{0.0};
    };

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Compute Avg.
     * @param[in] window Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static double computeAvg(const std::deque<double>& window) noexcept;

    /**
     * @brief Compute P99.
     * @param[in] window Input parameter.
     * @return Return value.
     */
    static double computeP99(const std::deque<double>& window);

    /**
     * @brief Is High Tail.
     * @param[in] state Input parameter.
     * @param[in] has_other_candidates Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isHighTail(const BackendState& state,
                    bool has_other_candidates) const noexcept;

    /**
     * @brief Refresh Stats.
     * @param[in,out] state Input/output parameter.
     * @note Exception safety: noexcept.
     */
    static void refreshStats(BackendState& state) noexcept;

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------

    Config config_;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, BackendState> backends_;
};

} // namespace themis::server
