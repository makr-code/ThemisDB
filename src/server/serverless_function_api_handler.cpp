/**
 * @file serverless_function_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/serverless_function_api_handler.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <atomic>
#include <climits>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <thread>
#include <future>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "utils/input_validator.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

constexpr size_t kMaxServerlessIdentifierLength = 128;

bool isValidServerlessIdentifier(const std::string& value, const bool allow_empty = false) {
    if (value.empty()) {
        return allow_empty;
    }

    themis::utils::InputValidator validator;
    return validator.validateStringLength(value, kMaxServerlessIdentifierLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// ServerlessFunction helpers
// ─────────────────────────────────────────────────────────────────────────────

json ServerlessFunction::toJson() const {
    return json{
        {"id",               id},
        {"name",             name},
        {"tenant_id",        tenant_id},
        {"description",      description},
        {"code",             code},
        {"timeout_ms",       timeout_ms},
        {"memory_limit_kb",  memory_limit_kb},
        {"version",          version},
        {"created_at",       created_at},
        {"updated_at",       updated_at}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Utilities
// ─────────────────────────────────────────────────────────────────────────────

std::string ServerlessFunctionApiHandler::generateId() {
    static std::atomic<uint64_t> counter{0};
    uint64_t ts = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    uint64_t seq = counter.fetch_add(1, std::memory_order_relaxed);
    std::ostringstream oss = {};
    oss << "fn-" << std::hex << ts << "-" << seq;
    return oss.str();
}

std::string ServerlessFunctionApiHandler::utcNow() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss = {};
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string ServerlessFunctionApiHandler::validateCode(const json& code) {
    if (!code.is_object()) {
        return "code must be a JSON object";
    }
    if (!code.contains("operations") || !code["operations"].is_array()) {
        return "code must contain an 'operations' array";
    }
    for (const auto& op : code["operations"]) {
        if (!op.is_object() || !op.contains("type") || !op["type"].is_string()) {
            return "each operation must be an object with a string 'type' field";
        }
        const std::string type = op["type"].get<std::string>();
        if (type == "passthrough") {
            // no additional fields required
        } else if (type == "transform") {
            if (!op.contains("fields") || !op["fields"].is_object()) {
                return "transform operation requires a 'fields' object";
            }
        } else if (type == "filter") {
            if (!op.contains("keep") || !op["keep"].is_array()) {
                return "filter operation requires a 'keep' array";
            }
        } else if (type == "enrich") {
            if (!op.contains("add") || !op["add"].is_object()) {
                return "enrich operation requires an 'add' object";
            }
        } else if (type == "validate") {
            if (!op.contains("required") || !op["required"].is_array()) {
                return "validate operation requires a 'required' array";
            }
        } else {
            return "unknown operation type: " + type;
        }
    }
    return "";
}

// ─────────────────────────────────────────────────────────────────────────────
// Execution Engine
// ─────────────────────────────────────────────────────────────────────────────

bool ServerlessFunctionApiHandler::executeFunction(
    const ServerlessFunction& fn,
    const json& input,
    json& output,
    std::string& error) const
{
    // The async task owns its own result state to avoid sharing references
    // across threads.  This eliminates potential data races between the
    // timeout path on the main thread and a still-running async thread.
    struct TaskResult {
        bool        ok{false};
        json        output;
        std::string error = {};
    };

    // Capture only by value so the lambda has no references to caller locals.
    auto pipeline = [fn_code = fn.code, input_copy = input]() -> TaskResult {
        TaskResult r;
        json current = input_copy;
        for (const auto& op : fn_code["operations"]) {
            const std::string type = op["type"].get<std::string>();

            if (type == "passthrough") {
                // no-op

            } else if (type == "transform") {
                json next = json::object();
                for (const auto& [src, dst] : op["fields"].items()) {
                    if (current.contains(src)) {
                        next[dst.get<std::string>()] = current[src];
                    }
                }
                // carry over fields not mentioned in the mapping
                for (const auto& [k, v] : current.items()) {
                    if (!next.contains(k) && !op["fields"].contains(k)) {
                        next[k] = v;
                    }
                }
                current = std::move(next);

            } else if (type == "filter") {
                json next = json::object();
                for (const auto& field : op["keep"]) {
                    const std::string key = field.get<std::string>();
                    if (current.contains(key)) {
                        next[key] = current[key];
                    }
                }
                current = std::move(next);

            } else if (type == "enrich") {
                for (const auto& [key, val] : op["add"].items()) {
                    current[key] = val;
                }

            } else if (type == "validate") {
                for (const auto& req : op["required"]) {
                    const std::string key = req.get<std::string>();
                    if (!current.contains(key)) {
                        r.error = "validation failed: required field '" + key + "' is missing";
                        return r;
                    }
                }
            }
        }
        r.ok     = true;
        r.output = std::move(current);
        return r;
    };

    // Enforce timeout via std::async + wait_for.
    // The lambda captures all state by value (no shared references), so there
    // is no data race between the main thread and the async thread regardless
    // of when the future is settled.  The future's destructor (std::launch::async)
    // blocks until the thread completes, ensuring safe cleanup on all paths.
    auto future = std::async(std::launch::async, std::move(pipeline));
    const auto deadline = std::chrono::milliseconds(fn.timeout_ms);
    if (future.wait_for(deadline) == std::future_status::timeout) {
        error = "function execution timed out after " +
                std::to_string(fn.timeout_ms) + "ms";
        return false; // future destructor blocks until the thread finishes
    }

    TaskResult r = future.get();
    if (!r.ok) {
        error  = std::move(r.error);
        return false;
    }
    output = std::move(r.output);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP helpers
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::makeJsonResponse(
    http::status status,
    const json& body,
    const http::request<http::string_body>& req) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
ServerlessFunctionApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req) const
{
    return makeJsonResponse(status, json{{"error", message}}, req);
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /api/v1/functions
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::handleRegister(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleRegister");
    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid JSON body", req);
    }

    if (!body.contains("name") || !body["name"].is_string() ||
        body["name"].get<std::string>().empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "'name' field is required", req);
    }
    if (!isValidServerlessIdentifier(body["name"].get<std::string>())) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid function name", req);
    }
    if (!body.contains("code")) {
        return makeErrorResponse(http::status::bad_request,
                                 "'code' field is required", req);
    }
    if (body.contains("tenant_id") && (!body["tenant_id"].is_string() ||
        !isValidServerlessIdentifier(body["tenant_id"].get<std::string>(), true))) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid tenant_id", req);
    }

    std::string code_err = validateCode(body["code"]);
    if (!code_err.empty()) {
        return makeErrorResponse(http::status::bad_request, code_err, req);
    }

    ServerlessFunction fn;
    fn.id          = generateId();
    fn.name        = body["name"].get<std::string>();
    fn.tenant_id   = body.value("tenant_id", "");
    fn.description = body.value("description", "");
    fn.code        = body["code"];
    fn.timeout_ms       = body.value("timeout_ms", 5000);
    // GAP-022: Cap creation-time memory_limit_kb at 16 GB (16,777,216 KB).
    static constexpr uint32_t kMaxMemoryLimitKb = 16'777'216;
    fn.memory_limit_kb  = std::min(body.value("memory_limit_kb", 4096), kMaxMemoryLimitKb);
    fn.version     = 1;
    fn.created_at  = utcNow();
    fn.updated_at  = fn.created_at;

    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        registry_[fn.id] = fn;
        version_history_[fn.id].push_back(fn);
    }

    return makeJsonResponse(http::status::created, fn.toJson(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/functions
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::handleList(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleList");
    // Optional ?tenant_id= filter via query string.
    std::string tenant_filter = {};
    const std::string target{req.target()};
    auto qpos = target.find('?');
    if (qpos != std::string::npos) {
        std::string_view query{target};
        query.remove_prefix(qpos + 1);
        while (!query.empty()) {
            const auto amp = query.find('&');
            const auto token = query.substr(0, amp);
            const auto eq = token.find('=');
            const auto key = token.substr(0, eq);
            if (key == "tenant_id") {
                if (eq == std::string_view::npos) {
                    return makeErrorResponse(http::status::bad_request,
                                             "invalid tenant_id filter", req);
                }

                tenant_filter = std::string(token.substr(eq + 1));
                if (!tenant_filter.empty() && !isValidServerlessIdentifier(tenant_filter, true)) {
                    return makeErrorResponse(http::status::bad_request,
                                             "invalid tenant_id filter", req);
                }
                break;
            }

            if (amp == std::string_view::npos) {
                break;
            }
            query.remove_prefix(amp + 1);
        }
    }

    json arr = json::array();
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        for (const auto& [id, fn] : registry_) {
            if (tenant_filter.empty() || fn.tenant_id == tenant_filter) {
                arr.push_back(fn.toJson());
            }
        }
    }
    return makeJsonResponse(http::status::ok, json{{"functions", arr}}, req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/functions/{id}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::handleGet(
    const http::request<http::string_body>& req,
    const std::string& id)
{
    auto span = Tracer::startSpan("handleGet");
    if (!isValidServerlessIdentifier(id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid function id", req);
    }
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = registry_.find(id);
    if (it == registry_.end()) {
        return makeErrorResponse(http::status::not_found,
                                 "function not found: " + id, req);
    }
    return makeJsonResponse(http::status::ok, it->second.toJson(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// PUT /api/v1/functions/{id}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::handleUpdate(
    const http::request<http::string_body>& req,
    const std::string& id)
{
    auto span = Tracer::startSpan("handleUpdate");
    if (!isValidServerlessIdentifier(id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid function id", req);
    }
    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid JSON body", req);
    }

    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = registry_.find(id);
    if (it == registry_.end()) {
        return makeErrorResponse(http::status::not_found,
                                 "function not found: " + id, req);
    }

    ServerlessFunction& fn = it->second;

    if (body.contains("name") && body["name"].is_string() &&
        !body["name"].get<std::string>().empty()) {
        if (!isValidServerlessIdentifier(body["name"].get<std::string>())) {
            return makeErrorResponse(http::status::bad_request,
                                     "invalid function name", req);
        }
        fn.name = body["name"].get<std::string>();
    }
    if (body.contains("description")) {
        fn.description = body.value("description", fn.description);
    }
    if (body.contains("tenant_id")) {
        if (!body["tenant_id"].is_string() ||
            !isValidServerlessIdentifier(body["tenant_id"].get<std::string>(), true)) {
            return makeErrorResponse(http::status::bad_request,
                                     "invalid tenant_id", req);
        }
        fn.tenant_id = body.value("tenant_id", fn.tenant_id);
    }
    if (body.contains("code")) {
        std::string code_err = validateCode(body["code"]);
        if (!code_err.empty()) {
            return makeErrorResponse(http::status::bad_request, code_err, req);
        }
        fn.code = body["code"];
    }
    if (body.contains("timeout_ms") && body["timeout_ms"].is_number()) {
        const int64_t v = body["timeout_ms"].get<int64_t>();
        if (v > 0 && v <= static_cast<int64_t>(UINT32_MAX)) {
            fn.timeout_ms = static_cast<uint32_t>(v);
        }
    }
    if (body.contains("memory_limit_kb") && body["memory_limit_kb"].is_number()) {
        const int64_t v = body["memory_limit_kb"].get<int64_t>();
        // GAP-022: Cap memory_limit_kb to prevent DoS via absurdly large allocation
        // requests.  UINT32_MAX (~4TB) is the previous upper bound; now capped at
        // 16GB (16,777,216 KB) which is the practical maximum per-function limit.
        static constexpr int64_t kMaxMemoryLimitKb = 16'777'216; // 16 GB
        if (v > 0 && v <= kMaxMemoryLimitKb) {
            fn.memory_limit_kb = static_cast<uint32_t>(v);
        } else if (v > kMaxMemoryLimitKb) {
            spdlog::warn("[SECURITY] Serverless: memory_limit_kb={} exceeds cap {}; "
                         "clamping to cap (GAP-022/CWE-400)", v, kMaxMemoryLimitKb);
            fn.memory_limit_kb = static_cast<uint32_t>(kMaxMemoryLimitKb);
        }
    }

    fn.version++;
    fn.updated_at = utcNow();

    version_history_[id].push_back(fn);

    return makeJsonResponse(http::status::ok, fn.toJson(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// DELETE /api/v1/functions/{id}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::handleDelete(
    const http::request<http::string_body>& req,
    const std::string& id)
{
    auto span = Tracer::startSpan("handleDelete");
    if (!isValidServerlessIdentifier(id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid function id", req);
    }
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = registry_.find(id);
    if (it == registry_.end()) {
        return makeErrorResponse(http::status::not_found,
                                 "function not found: " + id, req);
    }
    registry_.erase(it);
    version_history_.erase(id);
    return makeJsonResponse(http::status::ok,
                            json{{"deleted", true}, {"id", id}}, req);
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /api/v1/functions/{id}/invoke
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::handleInvoke(
    const http::request<http::string_body>& req,
    const std::string& id)
{
    auto span = Tracer::startSpan("handleInvoke");
    if (!isValidServerlessIdentifier(id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid function id", req);
    }
    // Fetch function under lock, then release before execution to avoid
    // holding the registry mutex during potentially long-running work.
    ServerlessFunction fn;
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = registry_.find(id);
        if (it == registry_.end()) {
            return makeErrorResponse(http::status::not_found,
                                     "function not found: " + id, req);
        }
        fn = it->second; // copy
    }

    json input = {};
    if (!req.body().empty()) {
        try {
            input = json::parse(req.body());
        } catch (const json::exception&) {
            return makeErrorResponse(http::status::bad_request,
                                     "invalid JSON body", req);
        }
    } else {
        input = json::object();
    }

    auto invoke_start = std::chrono::steady_clock::now();

    json output;
    std::string exec_error = {};
    bool ok = executeFunction(fn, input, output, exec_error);

    auto invoke_end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        invoke_end - invoke_start).count();

    if (!ok) {
        json err_body = {
            {"error",        exec_error},
            {"function_id",  id},
            {"duration_ms",  duration_ms}
        };
        return makeJsonResponse(http::status::internal_server_error,
                                err_body, req);
    }

    json result = {
        {"result",       output},
        {"function_id",  id},
        {"version",      fn.version},
        {"duration_ms",  duration_ms}
    };
    return makeJsonResponse(http::status::ok, result, req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/functions/{id}/versions
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
ServerlessFunctionApiHandler::handleVersions(
    const http::request<http::string_body>& req,
    const std::string& id)
{
    auto span = Tracer::startSpan("handleVersions");
    if (!isValidServerlessIdentifier(id)) {
        return makeErrorResponse(http::status::bad_request,
                                 "invalid function id", req);
    }
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = version_history_.find(id);
    if (it == version_history_.end()) {
        return makeErrorResponse(http::status::not_found,
                                 "function not found: " + id, req);
    }

    json arr = json::array();
    for (const auto& snap : it->second) {
        arr.push_back(snap.toJson());
    }
    return makeJsonResponse(http::status::ok, json{{"versions", arr}}, req);
}

} // namespace server
} // namespace themis
