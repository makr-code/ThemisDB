/**
 * @file async_job_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

/**
 * @brief Async Job Status To String.
 * @param[in] s Input parameter.
 * @return Return value.
 */
std::string asyncJobStatusToString(AsyncJobStatus s);

// ---------------------------------------------------------------------------
// AsyncJobRecord
// ---------------------------------------------------------------------------

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

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// AsyncJobRegistry
// ---------------------------------------------------------------------------

class AsyncJobRegistry {
public:
    static constexpr std::chrono::seconds kDefaultTTL{3600};  ///< 1 hour

    explicit AsyncJobRegistry(
        std::chrono::seconds ttl = kDefaultTTL);

    /**
     * @brief Add.
     * @param[in] job Input parameter.
     */
    void add(std::shared_ptr<AsyncJobRecord> job);

    /**
     * @brief Get.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::shared_ptr<AsyncJobRecord> get(const std::string& id) const;

    /**
     * @brief All.
     * @return Return value.
     */
    std::vector<std::shared_ptr<AsyncJobRecord>> all() const;

    /**
     * @brief Get Json Snapshot.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::optional<nlohmann::json> getJsonSnapshot(const std::string& id) const;

    /**
     * @brief All Json Snapshots.
     * @return Return value.
     */
    std::vector<nlohmann::json> allJsonSnapshots() const;

    std::optional<std::pair<AsyncJobStatus, bool>> requestCancel(const std::string& id);

    /**
     * @brief Prune.
     */
    void prune();

private:
    mutable std::mutex                               mutex_;
    std::map<std::string, std::shared_ptr<AsyncJobRecord>> jobs_;
    std::chrono::seconds                             ttl_;
};

// ---------------------------------------------------------------------------
// AsyncJobApiHandler
// ---------------------------------------------------------------------------

class AsyncJobApiHandler {
public:
    using AqlExecutor = std::function<
        nlohmann::json(const std::string& query,
                       const std::string& auth_header)>;

    explicit AsyncJobApiHandler(
        AqlExecutor                                        executor,
        std::shared_ptr<AuthMiddleware>                    auth         = nullptr,
        std::shared_ptr<AsyncJobRegistry>                  registry     = nullptr,
        std::shared_ptr<AdaptiveQueryCache>         result_cache = nullptr);

    ~AsyncJobApiHandler();

    // Non-copyable / non-movable
    AsyncJobApiHandler(const AsyncJobApiHandler&)            = delete;
    AsyncJobApiHandler& operator=(const AsyncJobApiHandler&) = delete;

    /**
     * @brief Handle Submit.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSubmit(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetStatus(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Cancel.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCancel(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Health Check.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealthCheck(
        const http::request<http::string_body>& req);

private:
    /**
     * @brief Generate Job Id.
     * @return Return value.
     */
    static std::string generateJobId();

    /**
     * @brief Extract Job Id.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    static std::string extractJobId(const std::string& target);

    /**
     * @brief Make Json Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    static http::response<http::string_body> makeJsonResponse(
        http::status                           status,
        const nlohmann::json&                  body,
        const http::request<http::string_body>& req);

    /**
     * @brief Launch Job.
     * @param[in] job Input parameter.
     */
    void launchJob(std::shared_ptr<AsyncJobRecord> job);

    AqlExecutor                                    executor_;
    std::shared_ptr<AuthMiddleware>                auth_;
    std::shared_ptr<AsyncJobRegistry>              registry_;
    std::shared_ptr<AdaptiveQueryCache>     result_cache_;

    // Track live futures so the destructor can join them.
    mutable std::mutex                           futures_mutex_;
    std::vector<std::future<void>>               futures_;
};

} // namespace server
} // namespace themis
