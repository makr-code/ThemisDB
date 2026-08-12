/**
 * @file async_job_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Ensure correct WinSock include order on Windows
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#endif

// Windows macros undefine - MUST be before any includes
#ifdef ERROR
#undef ERROR
#endif

#include "server/async_job_api_handler.h"
#include "server/auth_middleware.h"
#include "cache/adaptive_query_cache.h"
#include "utils/input_validator.h"
#include "utils/logger.h"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

constexpr size_t kMaxAsyncJobQueryLength = 1'000'000;
constexpr size_t kMaxAsyncJobIdLength = 256;

bool isValidAsyncQuery(std::string_view query) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(query), kMaxAsyncJobQueryLength) &&
           validator.validateAQLQuery(std::string(query));
}

bool isValidAsyncJobId(std::string_view job_id) {
    themis::utils::InputValidator validator;
    return !job_id.empty() &&
           validator.validateStringLength(std::string(job_id), kMaxAsyncJobIdLength) &&
           validator.validatePathSegment(std::string(job_id));
}

bool isValidCapturedHeader(std::string_view header_value) {
    if (header_value.empty()) {
        return true;
    }

    themis::utils::InputValidator validator;
    return validator.validateHeaderValue(std::string(header_value));
}

} // namespace

// ============================================================================
// Helpers
// ============================================================================

std::string asyncJobStatusToString(AsyncJobStatus s) {
    switch (s) {
        case AsyncJobStatus::PENDING:   return "pending";
        case AsyncJobStatus::RUNNING:   return "running";
        case AsyncJobStatus::COMPLETED: return "completed";
        case AsyncJobStatus::FAILED:    return "failed";
        case AsyncJobStatus::CANCELLED: return "cancelled";
    }
    return "unknown";
}

static std::string timePointToIso(std::chrono::system_clock::time_point tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// ============================================================================
// AsyncJobRecord
// ============================================================================

json AsyncJobRecord::toJson() const {
    std::lock_guard<std::mutex> lock(mu);
    json j;
    j["job_id"]     = id;
    j["status"]     = asyncJobStatusToString(status);
    j["query"]      = query;
    j["created_at"] = timePointToIso(created_at);
    j["updated_at"] = timePointToIso(updated_at);
    if (status == AsyncJobStatus::COMPLETED) {
        j["result"] = result;
    }
    if (status == AsyncJobStatus::FAILED) {
        j["error"] = error;
    }
    return j;
}

// ============================================================================
// AsyncJobRegistry
// ============================================================================

AsyncJobRegistry::AsyncJobRegistry(std::chrono::seconds ttl)
    : ttl_(ttl)
{}

void AsyncJobRegistry::add(std::shared_ptr<AsyncJobRecord> job) {
    std::lock_guard<std::mutex> lock(mutex_);
    jobs_[job->id] = std::move(job);
    // TTL pruning on every add – keeps the map size bounded
    auto now = std::chrono::system_clock::now();
    for (auto it = jobs_.begin(); it != jobs_.end(); ) {
        const auto& rec = *it->second;
        std::lock_guard<std::mutex> rlock(rec.mu);
        bool terminal = (rec.status == AsyncJobStatus::COMPLETED ||
                         rec.status == AsyncJobStatus::FAILED    ||
                         rec.status == AsyncJobStatus::CANCELLED);
        if (terminal && (now - rec.updated_at) > ttl_) {
            it = jobs_.erase(it);
        } else {
            ++it;
        }
    }
}

std::shared_ptr<AsyncJobRecord> AsyncJobRegistry::get(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(id);
    return (it != jobs_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<AsyncJobRecord>> AsyncJobRegistry::all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<AsyncJobRecord>> out;
    out.reserve(jobs_.size());
    for (const auto& kv : jobs_) {
        out.push_back(kv.second);
    }
    return out;
}

std::optional<nlohmann::json> AsyncJobRegistry::getJsonSnapshot(const std::string& id) const {
    std::shared_ptr<AsyncJobRecord> job;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = jobs_.find(id);
        if (it == jobs_.end()) {
            return std::nullopt;
        }
        job = it->second;
    }
    return job->toJson();
}

std::vector<nlohmann::json> AsyncJobRegistry::allJsonSnapshots() const {
    auto jobs = all();
    std::vector<nlohmann::json> out;
    out.reserve(jobs.size());
    for (const auto& job : jobs) {
        out.push_back(job->toJson());
    }
    return out;
}

std::optional<std::pair<AsyncJobStatus, bool>> AsyncJobRegistry::requestCancel(
    const std::string& id) {
    std::shared_ptr<AsyncJobRecord> job;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = jobs_.find(id);
        if (it == jobs_.end()) {
            return std::nullopt;
        }
        job = it->second;
    }

    std::lock_guard<std::mutex> rlock(job->mu);
    AsyncJobStatus status = job->status;
    if (status == AsyncJobStatus::COMPLETED ||
        status == AsyncJobStatus::FAILED ||
        status == AsyncJobStatus::CANCELLED) {
        return std::make_pair(status, true);
    }

    job->cancel_requested.store(true, std::memory_order_release);
    if (job->status == AsyncJobStatus::PENDING) {
        job->status = AsyncJobStatus::CANCELLED;
        job->updated_at = std::chrono::system_clock::now();
    }
    return std::make_pair(job->status, false);
}

void AsyncJobRegistry::prune() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    for (auto it = jobs_.begin(); it != jobs_.end(); ) {
        const auto& rec = *it->second;
        std::lock_guard<std::mutex> rlock(rec.mu);
        bool terminal = (rec.status == AsyncJobStatus::COMPLETED ||
                         rec.status == AsyncJobStatus::FAILED    ||
                         rec.status == AsyncJobStatus::CANCELLED);
        if (terminal && (now - rec.updated_at) > ttl_) {
            it = jobs_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// AsyncJobApiHandler – construction / destruction
// ============================================================================

AsyncJobApiHandler::AsyncJobApiHandler(
    AqlExecutor                                        executor,
    std::shared_ptr<AuthMiddleware>                    auth,
    std::shared_ptr<AsyncJobRegistry>                  registry,
    std::shared_ptr<AdaptiveQueryCache>                result_cache)
    : executor_(std::move(executor))
    , auth_(std::move(auth))
    , registry_(registry ? std::move(registry)
                          : std::make_shared<AsyncJobRegistry>())
{
    if (result_cache) {
        result_cache_ = std::move(result_cache);
    } else {
        // Create a dedicated AdaptiveQueryCache with TTL = 1 hour for job results.
        // All three tier TTLs are set to 3600 s (1 h) per the AC requirement.
        AdaptiveQueryCache::Config cfg;
        cfg.l1_ttl_seconds = 3600;   // 1 hour, as required by the AC
        cfg.l2_ttl_seconds = 3600;
        cfg.l3_ttl_seconds = 3600;
        // Use a temp-dir path unique to this process so the L3 RocksDB does not
        // persist across restarts and parallel test instances do not collide.
        try {
            // Combine PID with full steady_clock nanosecond count for uniqueness;
            // no modulo so the full range is used even when multiple instances
            // start within the same second.
            const auto ns = static_cast<unsigned long long>(
                std::chrono::steady_clock::now().time_since_epoch().count());
            cfg.l3_db_path = std::filesystem::temp_directory_path().string() +
                             "/themis_async_jobs_cache_" +
                             std::to_string(static_cast<long long>(::getpid())) +
                             "_" + std::to_string(ns);
            result_cache_ = std::make_shared<AdaptiveQueryCache>(cfg);
        } catch (const std::exception& ex) {
            THEMIS_WARN("AsyncJobApiHandler: failed to create result cache: {}", ex.what());
        }
    }
}

AsyncJobApiHandler::~AsyncJobApiHandler() {
    // Give in-flight jobs a short window to complete so background threads
    // do not access executor_ / auth_ after they have been destroyed.
    std::vector<std::future<void>> to_join;
    {
        std::lock_guard<std::mutex> lock(futures_mutex_);
        to_join = std::move(futures_);
    }
    for (auto& f : to_join) {
        if (f.valid()) {
            // Wait up to 2 s; if the job is stuck the thread is detached.
            f.wait_for(std::chrono::seconds(2));
        }
    }
}

// ============================================================================
// Static helpers
// ============================================================================

std::string AsyncJobApiHandler::generateJobId() {
    static std::atomic<uint64_t> counter{0};
    uint64_t ts = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    uint64_t seq = counter.fetch_add(1, std::memory_order_relaxed);
    std::ostringstream oss;
    oss << "job-" << std::hex << ts << "-" << seq;
    return oss.str();
}

std::string AsyncJobApiHandler::extractJobId(const std::string& target) {
    // Strip query string
    std::string path = target;
    auto qpos = path.find('?');
    if (qpos != std::string::npos) path = path.substr(0, qpos);

    static constexpr std::string_view kPrefix{"/v2/jobs/"};
    if (path.size() <= kPrefix.size()) return {};
    if (path.rfind(kPrefix.data(), 0) != 0) return {};
    return path.substr(kPrefix.size());
}

http::response<http::string_body> AsyncJobApiHandler::makeJsonResponse(
    http::status status,
    const json&  body,
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server,       "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

// ============================================================================
// Background execution
// ============================================================================

void AsyncJobApiHandler::launchJob(std::shared_ptr<AsyncJobRecord> job) {
    // Capture strong refs so the lambda keeps them alive beyond the handler.
    auto registry     = registry_;
    auto executor     = executor_;
    auto result_cache = result_cache_;

    auto fut = std::async(std::launch::async,
        [job, registry, executor, result_cache]() mutable {
            // Transition: PENDING → RUNNING (under per-record lock)
            {
                std::lock_guard<std::mutex> rlock(job->mu);
                job->status     = AsyncJobStatus::RUNNING;
                job->updated_at = std::chrono::system_clock::now();
            }

            if (job->cancel_requested.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> rlock(job->mu);
                job->status     = AsyncJobStatus::CANCELLED;
                job->updated_at = std::chrono::system_clock::now();
                return;
            }

            try {
                auto result = executor(job->query, job->auth_header);
                std::string final_status;
                {
                    std::lock_guard<std::mutex> rlock(job->mu);
                    if (job->cancel_requested.load(std::memory_order_acquire)) {
                        job->status = AsyncJobStatus::CANCELLED;
                    } else {
                        job->result = std::move(result);
                        job->status = AsyncJobStatus::COMPLETED;
                    }
                    job->updated_at = std::chrono::system_clock::now();
                    final_status    = asyncJobStatusToString(job->status);
                }
                // Snapshot the job state outside the lock so toJson() does not
                // re-acquire job->mu (which would self-deadlock).
                nlohmann::json job_snapshot = job->toJson();
                // Persist completed/cancelled state in AdaptiveQueryCache (TTL=1h).
                if (result_cache) {
                    result_cache->put(job->id, {{"job_id", job->id}}, job_snapshot,
                                      "async_jobs");
                }
                THEMIS_DEBUG("AsyncJob {} finished with status {}", job->id, final_status);
            } catch (const std::exception& ex) {
                std::string final_status;
                {
                    std::lock_guard<std::mutex> rlock(job->mu);
                    job->error      = ex.what();
                    job->status     = AsyncJobStatus::FAILED;
                    job->updated_at = std::chrono::system_clock::now();
                    final_status    = asyncJobStatusToString(job->status);
                }
                // Snapshot outside the lock to avoid re-locking job->mu.
                nlohmann::json job_snapshot = job->toJson();
                // Persist failed state in AdaptiveQueryCache (TTL=1h).
                if (result_cache) {
                    result_cache->put(job->id, {{"job_id", job->id}}, job_snapshot,
                                      "async_jobs");
                }
                THEMIS_DEBUG("AsyncJob {} finished with status {}", job->id, final_status);
            } catch (...) {
                THEMIS_WARN("async_job_api_handler: unhandled exception caught");
                {
                    std::lock_guard<std::mutex> rlock(job->mu);
                    job->error      = "unknown error during async AQL execution";
                    job->status     = AsyncJobStatus::FAILED;
                    job->updated_at = std::chrono::system_clock::now();
                }
                // Snapshot outside the lock to avoid re-locking job->mu.
                nlohmann::json job_snapshot = job->toJson();
                if (result_cache) {
                    result_cache->put(job->id, {{"job_id", job->id}}, job_snapshot,
                                      "async_jobs");
                }
                THEMIS_DEBUG("AsyncJob {} finished with status failed", job->id);
            }
        });

    std::lock_guard<std::mutex> lock(futures_mutex_);
    // Collect completed futures to avoid unbounded growth
    futures_.erase(
        std::remove_if(futures_.begin(), futures_.end(),
            [](const std::future<void>& f) {
                return !f.valid() ||
                       f.wait_for(std::chrono::seconds(0)) ==
                           std::future_status::ready;
            }),
        futures_.end());
    futures_.push_back(std::move(fut));
}

// ============================================================================
// Route handlers
// ============================================================================

// POST /v2/jobs
http::response<http::string_body> AsyncJobApiHandler::handleSubmit(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleSubmit");
    // Optional auth check
    if (auth_ && auth_->isEnabled()) {
        const auto auth_hdr = req[http::field::authorization];
        if (std::string(auth_hdr).empty()) {
            return makeJsonResponse(http::status::unauthorized,
                {{"error", true}, {"message", "Authorization required"}}, req);
        }
        // Let the executor handle deeper token validation; here we just
        // confirm a token is present so we can capture it for the background job.
    }

    // Parse body
    json body;
    try {
        body = json::parse(req.body());
    } catch (const std::exception& ex) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true},
             {"message", std::string("Invalid JSON body: ") + ex.what()}},
            req);
    }

    if (!body.contains("query") || !body["query"].is_string()) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Missing required field: query"}},
            req);
    }

    const std::string aql_query = body["query"].get<std::string>();
    if (aql_query.empty()) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Field 'query' must not be empty"}},
            req);
    }
    const std::string auth_header = std::string(req[http::field::authorization]);

    if (!isValidAsyncQuery(aql_query)) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Field 'query' failed validation"}},
            req);
    }

    if (!isValidCapturedHeader(auth_header)) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Authorization header failed validation"}},
            req);
    }

    // Build job record
    auto job           = std::make_shared<AsyncJobRecord>();
    job->id            = generateJobId();
    job->query         = aql_query;
    job->auth_header   = auth_header;
    job->created_at    = std::chrono::system_clock::now();
    {
        std::lock_guard<std::mutex> rlock(job->mu);
        job->status     = AsyncJobStatus::PENDING;
        job->updated_at = job->created_at;
    }

    registry_->add(job);
    launchJob(job);

    THEMIS_INFO("AsyncJob {} submitted: query length={}", job->id, aql_query.size());

    return makeJsonResponse(http::status::accepted,
        {{"job_id", job->id}, {"status", "pending"}}, req);
}

// GET /v2/jobs
http::response<http::string_body> AsyncJobApiHandler::handleList(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleList");
    auto jobs = registry_->allJsonSnapshots();
    return makeJsonResponse(http::status::ok, jobs, req);
}

// GET /v2/jobs/{id}
http::response<http::string_body> AsyncJobApiHandler::handleGetStatus(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleGetStatus");
    const std::string target(req.target());
    const std::string job_id = extractJobId(target);

    if (job_id.empty()) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Missing job ID in path"}}, req);
    }
    if (!isValidAsyncJobId(job_id)) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Invalid job ID in path"}}, req);
    }

    auto job = registry_->getJsonSnapshot(job_id);
    if (!job.has_value()) {
        return makeJsonResponse(http::status::not_found,
            {{"error", true}, {"message", "Job not found: " + job_id}}, req);
    }

    return makeJsonResponse(http::status::ok, *job, req);
}

// DELETE /v2/jobs/{id}
http::response<http::string_body> AsyncJobApiHandler::handleCancel(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleCancel");
    const std::string target(req.target());
    const std::string job_id = extractJobId(target);

    if (job_id.empty()) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Missing job ID in path"}}, req);
    }
    if (!isValidAsyncJobId(job_id)) {
        return makeJsonResponse(http::status::bad_request,
            {{"error", true}, {"message", "Invalid job ID in path"}}, req);
    }

    auto status_result = registry_->requestCancel(job_id);
    if (!status_result.has_value()) {
        return makeJsonResponse(http::status::not_found,
            {{"error", true}, {"message", "Job not found: " + job_id}}, req);
    }

    const AsyncJobStatus current = status_result->first;
    const bool already_terminal = status_result->second;

    if (already_terminal)
    {
        return makeJsonResponse(http::status::conflict,
            {{"error", true},
             {"message", "Job already in terminal state: " +
                          asyncJobStatusToString(current)},
             {"job_id", job_id},
             {"status", asyncJobStatusToString(current)}},
            req);
    }

    THEMIS_INFO("AsyncJob {} cancel requested", job_id);

    return makeJsonResponse(http::status::ok,
        {{"job_id", job_id},
         {"status", asyncJobStatusToString(current)},
         {"message", "Cancellation requested"}},
        req);
}

} // namespace server
} // namespace themis

