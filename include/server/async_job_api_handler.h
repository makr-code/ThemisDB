/**
 * @file async_job_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

class AuthMiddleware;

class AdaptiveQueryCache;

namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

// ---------------------------------------------------------------------------
// Job lifecycle types
// ---------------------------------------------------------------------------

enum class AsyncJobStatus {
    PENDING,    ///< Accepted, not yet started
    RUNNING,    ///< Executing in background thread
    COMPLETED,  ///< Finished successfully
    FAILED,     ///< Finished with an error
    CANCELLED   ///< Cancelled by client request
};

/// Serialise status to a lowercase string (e.g. "running").
std::string asyncJobStatusToString(AsyncJobStatus s);

// ---------------------------------------------------------------------------
// AsyncJobRecord
// ---------------------------------------------------------------------------

/**
 * @brief Holds the mutable state of a single async AQL job.
 *
 * Instances are always owned by shared_ptr and stored inside
 * AsyncJobRegistry.  The `mu` mutex protects `status`, `result`,
 * `error`, and `updated_at` for concurrent read/write access.
 * `cancel_requested` is a lock-free flag polled by the background thread.
 */
struct AsyncJobRecord {
    std::string                             id;
    std::string                             query;         ///< original AQL string
    std::string                             auth_header;   ///< captured Authorization header
    std::chrono::system_clock::time_point   created_at;

    // Fields below are protected by `mu`
    mutable std::mutex                      mu;
    AsyncJobStatus                          status{AsyncJobStatus::PENDING};
    nlohmann::json                          result;        ///< populated on COMPLETED
    std::string                             error;         ///< populated on FAILED
    std::chrono::system_clock::time_point   updated_at;

    std::atomic<bool>                       cancel_requested{false};

    // Non-copyable, non-movable (mutex/atomic members)
    AsyncJobRecord() = default;
    AsyncJobRecord(const AsyncJobRecord&)            = delete;
    AsyncJobRecord& operator=(const AsyncJobRecord&) = delete;

    /// Serialise to JSON (acquires `mu` internally).
    nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// AsyncJobRegistry
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe store for AsyncJobRecord instances.
 *
 * Completed/failed/cancelled jobs older than `ttl` are pruned automatically
 * during every `add()` call so the registry does not grow without bound.
 *
 * Default TTL matches the FUTURE_ENHANCEMENTS.md specification: 1 hour.
 */
class AsyncJobRegistry {
public:
    static constexpr std::chrono::seconds kDefaultTTL{3600};  ///< 1 hour

    explicit AsyncJobRegistry(
        std::chrono::seconds ttl = kDefaultTTL);

    /// Register a new job.  Triggers TTL pruning.
    void add(std::shared_ptr<AsyncJobRecord> job);

    /// Look up a job by ID.  Returns nullptr if not found.
    std::shared_ptr<AsyncJobRecord> get(const std::string& id) const;

    /// Return a snapshot of all known jobs.
    std::vector<std::shared_ptr<AsyncJobRecord>> all() const;

    /// Return JSON snapshot for one job; std::nullopt if not found.
    std::optional<nlohmann::json> getJsonSnapshot(const std::string& id) const;

    /// Return JSON snapshots of all known jobs.
    std::vector<nlohmann::json> allJsonSnapshots() const;

    /// Request cancellation and return {status, already_terminal}; nullopt if not found.
    std::optional<std::pair<AsyncJobStatus, bool>> requestCancel(const std::string& id);

    /// Remove completed/failed/cancelled jobs older than `ttl_`.
    void prune();

private:
    mutable std::mutex                               mutex_;
    std::map<std::string, std::shared_ptr<AsyncJobRecord>> jobs_;
    std::chrono::seconds                             ttl_;
};

// ---------------------------------------------------------------------------
// AsyncJobApiHandler
// ---------------------------------------------------------------------------

/**
 * @brief HTTP handler for the async job API (POST/GET/DELETE /v2/jobs[/{id}]).
 *
 * Accepts a long-running AQL query via POST /v2/jobs, runs it in a detached
 * background future, and exposes polling and cancellation endpoints.
 *
 * Routes
 * ------
 * POST   /v2/jobs
 *   Body (JSON): { "query": "FOR x IN ... RETURN x" }
 *   Note: `bind_vars` are accepted for forward compatibility but the
 *   underlying AQL executor does not yet substitute them at runtime.
 *   Response 202: { "job_id": "...", "status": "pending" }
 *
 * GET    /v2/jobs
 *   Response 200: JSON array of job summaries.
 *
 * GET    /v2/jobs/{id}
 *   Response 200: full AsyncJobRecord JSON; 404 if not found.
 *
 * DELETE /v2/jobs/{id}
 *   Signals cancellation; response 200 with updated status.
 *
 * @see docs/api/FUTURE_ENHANCEMENTS.md – design constraints.
 */
class AsyncJobApiHandler {
public:
    /**
     * @brief Executor callable type.
     *
     * Called on a background thread with the AQL query string and the
     * captured Authorization header value.
     *
     * @returns JSON value (object or array) that becomes `result` on success.
     * @throws  std::exception (message stored in `error` on failure).
     */
    using AqlExecutor = std::function<
        nlohmann::json(const std::string& query,
                       const std::string& auth_header)>;

    /**
     * @param executor     Callable that executes an AQL query.
     * @param auth         Optional AuthMiddleware for access control at submission.
     *                     Pass nullptr to bypass authentication (tests only).
     * @param registry     Shared job registry (created internally if nullptr).
     * @param result_cache Optional AdaptiveQueryCache for persisting completed job
     *                     results with TTL = 1 hour, per the AC requirement.
     *                     Created internally with TTL=3600 s if nullptr.
     */
    explicit AsyncJobApiHandler(
        AqlExecutor                                        executor,
        std::shared_ptr<AuthMiddleware>                    auth         = nullptr,
        std::shared_ptr<AsyncJobRegistry>                  registry     = nullptr,
        std::shared_ptr<AdaptiveQueryCache>         result_cache = nullptr);

    /// Wait for running jobs to finish (up to a short grace period) on
    /// destruction so that background threads do not outlive dependencies.
    ~AsyncJobApiHandler();

    // Non-copyable / non-movable
    AsyncJobApiHandler(const AsyncJobApiHandler&)            = delete;
    AsyncJobApiHandler& operator=(const AsyncJobApiHandler&) = delete;

    // ── Route handlers ────────────────────────────────────────────────────
    /// POST /v2/jobs – submit a new async AQL job.
    http::response<http::string_body> handleSubmit(
        const http::request<http::string_body>& req);

    /// GET /v2/jobs – list all known jobs.
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    /// GET /v2/jobs/{id} – query status / result of a specific job.
    http::response<http::string_body> handleGetStatus(
        const http::request<http::string_body>& req);

    /// DELETE /v2/jobs/{id} – request cancellation of a job.
    http::response<http::string_body> handleCancel(
        const http::request<http::string_body>& req);

    /// GET /v2/health/jobs – OP-HEALTH-002 readiness probe (liveness check)
    /// Returns operational status suitable for Kubernetes health checks
    http::response<http::string_body> handleHealthCheck(
        const http::request<http::string_body>& req);

private:
    /// Generate a unique job ID (timestamp + monotonic counter).
    static std::string generateJobId();

    /// Extract the {id} segment from /v2/jobs/{id}[?...].
    static std::string extractJobId(const std::string& target);

    /// Build an HTTP response with a JSON body.
    static http::response<http::string_body> makeJsonResponse(
        http::status                           status,
        const nlohmann::json&                  body,
        const http::request<http::string_body>& req);

    /// Launch `job` on a background thread via `executor_`.
    void launchJob(std::shared_ptr<AsyncJobRecord> job);

    AqlExecutor                                    executor_;
    std::shared_ptr<AuthMiddleware>                auth_;
    std::shared_ptr<AsyncJobRegistry>              registry_;
    /// AdaptiveQueryCache used to persist completed job results (TTL = 1 h).
    std::shared_ptr<AdaptiveQueryCache>     result_cache_;

    // Track live futures so the destructor can join them.
    mutable std::mutex                           futures_mutex_;
    std::vector<std::future<void>>               futures_;
};

} // namespace server
} // namespace themis
