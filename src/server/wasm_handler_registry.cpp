/**
 * @file wasm_handler_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/wasm_handler_registry.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <future>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <spdlog/spdlog.h>
#include "utils/logger.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {
constexpr uint64_t kMinWasmCpuTimeLimitMs = 1;
constexpr uint64_t kMaxWasmCpuTimeLimitMs = 60'000;

WasmHandlerConfig sanitizeWasmConfig(const WasmHandlerConfig& config) {
    WasmHandlerConfig sanitized = config;

    const auto requested_ms_raw = std::chrono::duration_cast<std::chrono::milliseconds>(
        sanitized.cpu_time_limit).count();
    uint64_t requested_ms = requested_ms_raw <= 0
        ? kMinWasmCpuTimeLimitMs
        : static_cast<uint64_t>(requested_ms_raw);

    if (requested_ms_raw <= 0) {
        spdlog::warn("[SECURITY] WASM: cpu_time_limit={}ms is invalid; clamping to {}ms",
                     requested_ms_raw, kMinWasmCpuTimeLimitMs);
    } else if (requested_ms > kMaxWasmCpuTimeLimitMs) {
        spdlog::warn("[SECURITY] WASM: cpu_time_limit={}ms exceeds cap {}; clamping",
                     requested_ms, kMaxWasmCpuTimeLimitMs);
    }

    requested_ms = std::clamp(requested_ms, kMinWasmCpuTimeLimitMs, kMaxWasmCpuTimeLimitMs);
    sanitized.cpu_time_limit = std::chrono::milliseconds(requested_ms);
    return sanitized;
}
} // namespace

// =============================================================================
// WasmHandlerEntry
// =============================================================================

json WasmHandlerEntry::toJson() const {
    return json{
        {"id",               id},
        {"tenant_id",        tenant_id},
        {"name",             name},
        {"description",      description},
        {"version",          version},
        {"created_at",       created_at},
        {"updated_at",       updated_at},
        {"invocation_count", invocation_count.load(std::memory_order_relaxed)},
        {"wasm_size_bytes",  wasm_bytes.size()},
        {"module_info", json{
            {"valid",        module_info.valid},
            {"wasm_version", module_info.wasm_version},
            {"byte_size",    module_info.byte_size},
            {"module_name",  module_info.module_name},
            {"exports",      module_info.exports},
            {"imports",      module_info.imports}
        }},
        {"config", json{
            {"cpu_time_limit_ms",  static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    config.cpu_time_limit).count())},
            {"memory_limit_bytes", config.memory_limit_bytes},
            {"entry_point",        config.entry_point}
        }}
    };
}

// =============================================================================
// Utilities
// =============================================================================

std::string WasmHandlerRegistry::utcNow() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
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

// Base64 alphabet
static constexpr char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static bool isBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<uint8_t> WasmHandlerRegistry::base64Decode(const std::string& encoded) {
    std::vector<uint8_t> result;
    result.reserve((encoded.size() / 4) * 3);

    int i = 0;
    unsigned char char4[4];
    unsigned char char3[3];
    int len = static_cast<int>(encoded.size());

    int idx = 0;
    while (idx < len && encoded[idx] != '=' && isBase64(
               static_cast<unsigned char>(encoded[idx]))) {
        char4[i++] = static_cast<unsigned char>(encoded[idx]);
        ++idx;
        if (i == 4) {
            for (int j = 0; j < 4; ++j) {
                const char* pos = std::find(kBase64Chars,
                                            kBase64Chars + 64,
                                            static_cast<char>(char4[j]));
                char4[j] = static_cast<unsigned char>(pos - kBase64Chars);
            }
            char3[0] = static_cast<unsigned char>( (char4[0] << 2) | (char4[1] >> 4));
            char3[1] = static_cast<unsigned char>(((char4[1] & 0x0f) << 4) | (char4[2] >> 2));
            char3[2] = static_cast<unsigned char>(((char4[2] & 0x03) << 6) |  char4[3]);
            for (int j = 0; j < 3; ++j) {
                result.push_back(char3[j]);
            }
            i = 0;
        }
    }

    if (i > 0) {
        for (int j = i; j < 4; ++j) char4[j] = 0;
        for (int j = 0; j < 4; ++j) {
            const char* pos = std::find(kBase64Chars,
                                        kBase64Chars + 64,
                                        static_cast<char>(char4[j]));
            char4[j] = static_cast<unsigned char>(pos - kBase64Chars);
        }
        char3[0] = static_cast<unsigned char>( (char4[0] << 2) | (char4[1] >> 4));
        char3[1] = static_cast<unsigned char>(((char4[1] & 0x0f) << 4) | (char4[2] >> 2));
        char3[2] = static_cast<unsigned char>(((char4[2] & 0x03) << 6) |  char4[3]);
        for (int j = 0; j < i - 1; ++j) {
            result.push_back(char3[j]);
        }
    }

    return result;
}

// =============================================================================
// HTTP helpers
// =============================================================================

http::response<http::string_body>
WasmHandlerRegistry::makeJsonResponse(
    http::status                            status,
    const json&                             body,
    const http::request<http::string_body>& req) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
WasmHandlerRegistry::makeErrorResponse(
    http::status                            status,
    const std::string&                      message,
    const http::request<http::string_body>& req) const
{
    return makeJsonResponse(status, json{{"error", message}}, req);
}

// =============================================================================
// Programmatic API
// =============================================================================

bool WasmHandlerRegistry::registerHandler(
    const std::string&          id,
    const std::vector<uint8_t>& wasm_bytes,
    const WasmHandlerConfig&    config,
    const std::string&          tenant_id,
    const std::string&          name,
    const std::string&          description,
    std::string*                error)
{
    // Validate binary first (no lock needed – read-only).
    auto info = themis::modules::WasmModuleValidator::validate(wasm_bytes);
    if (!info.valid) {
        if (error) *error = "Invalid .wasm binary: magic bytes or version mismatch";
        return false;
    }

    const WasmHandlerConfig sanitized_config = sanitizeWasmConfig(config);

    std::unique_lock lock(registry_mutex_);

    auto it = registry_.find(id);
    std::string now = utcNow();

    if (it != registry_.end()) {
        // Re-upload: update in place.
        WasmHandlerEntry& entry = it->second;
        entry.wasm_bytes  = wasm_bytes;
        entry.module_info = info;
        entry.config      = sanitized_config;
        entry.name        = name.empty() ? entry.name : name;
        entry.description = description.empty() ? entry.description : description;
        entry.tenant_id   = tenant_id.empty() ? entry.tenant_id : tenant_id;
        entry.updated_at  = now;
        entry.version    += 1;
    } else {
        // Fresh registration.
        WasmHandlerEntry entry;
        entry.id          = id;
        entry.tenant_id   = tenant_id;
        entry.name        = name.empty() ? id : name;
        entry.description = description;
        entry.wasm_bytes  = wasm_bytes;
        entry.module_info = info;
        entry.config      = sanitized_config;
        entry.created_at  = now;
        entry.updated_at  = now;
        entry.version     = 1;

        registry_.emplace(id, std::move(entry));
    }

    return true;
}

bool WasmHandlerRegistry::unregisterHandler(const std::string& id) {
    std::unique_lock lock(registry_mutex_);
    return registry_.erase(id) > 0;
}

bool WasmHandlerRegistry::hasHandler(const std::string& id) const {
    std::shared_lock lock(registry_mutex_);
    return registry_.count(id) > 0;
}

size_t WasmHandlerRegistry::size() const {
    std::shared_lock lock(registry_mutex_);
    return registry_.size();
}

std::vector<json> WasmHandlerRegistry::listHandlers(
    const std::string& tenant_id_filter) const
{
    std::shared_lock lock(registry_mutex_);
    std::vector<json> result;
    result.reserve(registry_.size());

    for (const auto& [key, entry] : registry_) {
        if (!tenant_id_filter.empty() && entry.tenant_id != tenant_id_filter) {
            continue;
        }
        result.push_back(entry.toJson());
    }

    return result;
}

WasmInvokeResult WasmHandlerRegistry::invoke(
    const std::string& id,
    const json&        input)
{
    // Snapshot the entry data under a shared lock so we don't hold the lock
    // during the (potentially slow) sandbox invocation.
    std::vector<uint8_t> wasm_bytes;
    WasmHandlerConfig    config;
    std::string          entry_point;

    {
        std::shared_lock lock(registry_mutex_);
        auto it = registry_.find(id);
        if (it == registry_.end()) {
            WasmInvokeResult r;
            r.error = "Handler '" + id + "' not found";
            return r;
        }
        wasm_bytes  = it->second.wasm_bytes;
        config      = it->second.config;
        entry_point = it->second.config.entry_point;
    }

    // Serialise input to bytes.
    const std::string input_str = input.dump();
    const std::vector<uint8_t> args(input_str.begin(), input_str.end());

    // Build sandbox configuration.
    themis::modules::WasmPluginSandbox::Config sandbox_cfg;
    sandbox_cfg.linear_memory_pages   = config.linear_memory_pages;
    sandbox_cfg.max_memory_mb         = config.memory_limit_bytes / (1024ULL * 1024);
    sandbox_cfg.max_cpu_time_seconds  = 0; // Wall-clock limit enforced via future::wait_until below.
    sandbox_cfg.allow_unregistered_imports = false;

    // Snapshot the wall-clock limit before launching the async task so it can
    // be used safely after the future returns (avoids referencing a captured
    // local from a background thread after wait_until completes).
    const auto time_limit    = config.cpu_time_limit;
    const auto time_limit_ms = time_limit.count();

    // Run the sandbox inside a future so we can enforce the wall-clock time limit.
    auto future = std::async(std::launch::async,
        [wasm_bytes, sandbox_cfg, id, entry_point, args]() -> WasmInvokeResult {
        WasmInvokeResult r;

        themis::modules::WasmPluginSandbox sandbox(sandbox_cfg);

        if (!sandbox.loadFromBytes(wasm_bytes, id)) {
            r.error = "Sandbox load failed: " + sandbox.lastError();
            return r;
        }

        themis::modules::WasmCallResult call_result =
            sandbox.callExport(entry_point, args);

        r.duration_us = call_result.duration_us;

        if (!call_result.success) {
            // Distinguish memory overflow from generic trap by checking the
            // error message produced by the sandbox.
            if (call_result.error.find("memory") != std::string::npos ||
                call_result.error.find("OOM")    != std::string::npos) {
                r.oom   = true;
            }
            r.error = call_result.error;
            return r;
        }

        // Convert output bytes to UTF-8 string.
        r.output  = std::string(call_result.output.begin(),
                                call_result.output.end());
        r.success = true;
        return r;
    });

    const auto deadline = std::chrono::steady_clock::now() + time_limit;
    const auto status   = future.wait_until(deadline);

    if (status == std::future_status::timeout) {
        WasmInvokeResult r;
        r.timeout = true;
        r.error   = "Wall-clock time limit exceeded ("
                    + std::to_string(time_limit_ms) + " ms)";
        return r;
    }

    WasmInvokeResult r = future.get();

    // Increment invocation counter for successful calls.
    // Re-acquire the lock to safely access the entry (it may have been removed).
    if (r.success) {
        std::shared_lock lock(registry_mutex_);
        auto it = registry_.find(id);
        if (it != registry_.end()) {
            it->second.invocation_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    return r;
}

// =============================================================================
// HTTP endpoint handlers
// =============================================================================

http::response<http::string_body> WasmHandlerRegistry::handleUpload(
    const http::request<http::string_body>& req,
    const std::string&                      id)
{
    if (id.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Function ID must not be empty", req);
    }

    std::vector<uint8_t> wasm_bytes;
    std::string          name;
    std::string          tenant_id;
    std::string          description;
    WasmHandlerConfig    config;

    const auto& body = req.body();

    // Detect encoding: JSON envelope vs raw binary.
    const std::string content_type{req[http::field::content_type]};
    const bool is_json =
        content_type.find("application/json") != std::string::npos;

    if (is_json) {
        json j;
        try {
            j = json::parse(body);
        } catch (const json::exception& e) {
            return makeErrorResponse(http::status::bad_request,
                "Invalid JSON in request body: " + std::string(e.what()), req);
        }

        if (j.contains("wasm_base64") && j["wasm_base64"].is_string()) {
            wasm_bytes = base64Decode(j["wasm_base64"].get<std::string>());
            if (wasm_bytes.empty()) {
                return makeErrorResponse(http::status::bad_request,
                    "Failed to decode Base64 wasm_base64 field", req);
            }
        } else {
            return makeErrorResponse(http::status::bad_request,
                "JSON body must contain a 'wasm_base64' field", req);
        }

        if (j.contains("name") && j["name"].is_string())
            name = j["name"].get<std::string>();
        if (j.contains("tenant_id") && j["tenant_id"].is_string())
            tenant_id = j["tenant_id"].get<std::string>();
        if (j.contains("description") && j["description"].is_string())
            description = j["description"].get<std::string>();
        if (j.contains("cpu_time_ms") && j["cpu_time_ms"].is_number_integer())
            config.cpu_time_limit =
                std::chrono::milliseconds(j["cpu_time_ms"].get<uint64_t>());
        if (j.contains("memory_limit_mb") && j["memory_limit_mb"].is_number_integer()) {
            // GAP-022: Cap WASM memory_limit_mb at 16 GB to prevent DoS via
            // absurdly large sandbox allocation requests (CWE-400).
            static constexpr uint64_t kMaxMemoryLimitMb = 16'384; // 16 GB
            const uint64_t requested = j["memory_limit_mb"].get<uint64_t>();
            if (requested > kMaxMemoryLimitMb) {
                spdlog::warn("[SECURITY] WASM: memory_limit_mb={} exceeds cap {}; "
                             "clamping to cap (GAP-022/CWE-400)", requested, kMaxMemoryLimitMb);
            }
            const uint64_t clamped = std::min(requested, kMaxMemoryLimitMb);
            config.memory_limit_bytes = static_cast<size_t>(clamped) * 1024 * 1024;
        }
        if (j.contains("entry_point") && j["entry_point"].is_string())
            config.entry_point = j["entry_point"].get<std::string>();
    } else {
        // Raw binary upload.
        wasm_bytes.assign(body.begin(), body.end());
    }

    if (wasm_bytes.empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Request body must not be empty", req);
    }

    const bool already_exists = hasHandler(id);
    std::string error;

    if (!registerHandler(id, wasm_bytes, config, tenant_id, name, description, &error)) {
        return makeErrorResponse(http::status::bad_request, error, req);
    }

    // Build response payload.
    json response;
    {
        std::shared_lock lock(registry_mutex_);
        auto it = registry_.find(id);
        if (it != registry_.end()) {
            response = it->second.toJson();
        }
    }

    const http::status resp_status =
        already_exists ? http::status::ok : http::status::created;

    return makeJsonResponse(resp_status, response, req);
}

http::response<http::string_body> WasmHandlerRegistry::handleList(
    const http::request<http::string_body>& req)
{
    // Parse optional ?tenant_id= query parameter.
    std::string tenant_filter;
    const std::string target{req.target()};
    const auto qpos = target.find('?');
    if (qpos != std::string::npos) {
        const std::string query = target.substr(qpos + 1);
        const std::string key   = "tenant_id=";
        auto kpos = query.find(key);
        if (kpos != std::string::npos) {
            tenant_filter = query.substr(kpos + key.size());
            const auto amp = tenant_filter.find('&');
            if (amp != std::string::npos) tenant_filter = tenant_filter.substr(0, amp);
        }
    }

    const auto handlers = listHandlers(tenant_filter);
    return makeJsonResponse(http::status::ok,
                            json{{"handlers", handlers},
                                 {"count",    static_cast<uint64_t>(handlers.size())}},
                            req);
}

http::response<http::string_body> WasmHandlerRegistry::handleGet(
    const http::request<http::string_body>& req,
    const std::string&                      id)
{
    std::shared_lock lock(registry_mutex_);
    auto it = registry_.find(id);
    if (it == registry_.end()) {
        return makeErrorResponse(http::status::not_found,
                                 "Handler '" + id + "' not found", req);
    }
    return makeJsonResponse(http::status::ok, it->second.toJson(), req);
}

http::response<http::string_body> WasmHandlerRegistry::handleDelete(
    const http::request<http::string_body>& req,
    const std::string&                      id)
{
    if (!unregisterHandler(id)) {
        return makeErrorResponse(http::status::not_found,
                                 "Handler '" + id + "' not found", req);
    }

    http::response<http::string_body> res{http::status::no_content, req.version()};
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
}

http::response<http::string_body> WasmHandlerRegistry::handleInvoke(
    const http::request<http::string_body>& req,
    const std::string&                      id)
{
    if (!hasHandler(id)) {
        return makeErrorResponse(http::status::not_found,
                                 "Handler '" + id + "' not found", req);
    }

    json input = json::object();
    if (!req.body().empty()) {
        try {
            input = json::parse(req.body());
        } catch (const json::exception& e) {
            return makeErrorResponse(http::status::bad_request,
                "Invalid JSON in request body: " + std::string(e.what()), req);
        }
    }

    WasmInvokeResult result = invoke(id, input);

    if (!result.success) {
        if (result.timeout) {
            // CPU time limit exceeded → 504 Gateway Timeout.
            http::response<http::string_body> res =
                makeErrorResponse(http::status::gateway_timeout, result.error, req);
            res.set("grpc-status", "4"); // DEADLINE_EXCEEDED
            return res;
        }
        if (result.oom) {
            return makeErrorResponse(http::status::internal_server_error,
                                     "Memory limit exceeded: " + result.error, req);
        }
        return makeErrorResponse(http::status::internal_server_error,
                                 result.error, req);
    }

    // Try to parse output as JSON; fall back to wrapping in {"output": ...}.
    json output_json;
    try {
        output_json = json::parse(result.output);
    } catch (...) {
        THEMIS_DEBUG("wasm_handler_registry: unhandled exception caught");
        output_json = json{{"output", result.output}};
    }

    return makeJsonResponse(http::status::ok,
                            json{{"result",      output_json},
                                 {"duration_us", result.duration_us}},
                            req);
}

} // namespace server
} // namespace themis

