/**
 * @file serverless_function_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief Metadata and definition of a registered serverless function.
 *
 * A function is expressed as a JSON DSL that describes a pipeline of
 * operations (passthrough, transform, filter, enrich, validate) applied
 * to the request payload.  All operations execute in-process with
 * configurable CPU-time and memory guards.
 */
struct ServerlessFunction {
    std::string id;          ///< Unique function ID (UUID-like)
    std::string name;        ///< Human-readable name
    std::string tenant_id;   ///< Owning tenant (empty = global)
    std::string description; ///< Optional description
    /// JSON DSL code block.  Supported top-level keys:
    ///   "operations": array of operation objects (see README)
    nlohmann::json code;
    uint32_t timeout_ms{5000};    ///< Max execution time in milliseconds
    uint32_t memory_limit_kb{4096}; ///< Soft memory guard (tracked allocations)
    uint32_t version{1};          ///< Monotonically increasing version counter
    std::string created_at;       ///< ISO-8601 creation timestamp
    std::string updated_at;       ///< ISO-8601 last-update timestamp

    /// Serialise to JSON for API responses.
    nlohmann::json toJson() const;
};

/**
 * @brief HTTP API Handler for serverless in-process function hosting.
 *
 * Exposes a RESTful interface to register, inspect, invoke, and delete
 * user-defined functions that run inside the ThemisDB server process:
 *
 *   POST   /api/v1/functions              – register a new function
 *   GET    /api/v1/functions              – list all functions (tenant-aware)
 *   GET    /api/v1/functions/{id}         – get function definition
 *   PUT    /api/v1/functions/{id}         – update function definition
 *   DELETE /api/v1/functions/{id}         – remove function
 *   POST   /api/v1/functions/{id}/invoke  – invoke function with payload
 *   GET    /api/v1/functions/{id}/versions – list version history
 *
 * @section DSL Function Code DSL
 *
 * The `code` field is a JSON object with an `operations` array.  Each
 * operation is an object with a `type` key.  Supported types:
 *
 * - `passthrough`  – return the input unchanged.
 * - `transform`    – rename / project fields.
 *   Required: `"fields": { "<src>": "<dst>" }`
 * - `filter`       – keep only listed top-level fields.
 *   Required: `"keep": ["field1", "field2"]`
 * - `enrich`       – add metadata fields to the output.
 *   Required: `"add": { "<key>": "<literal_value>" }`
 * - `validate`     – assert that required fields are present.
 *   Required: `"required": ["field1", "field2"]`
 *
 * @note Thread-safe – all public methods are protected by an internal mutex.
 */
class ServerlessFunctionApiHandler {
public:
    ServerlessFunctionApiHandler() = default;

    // ── CRUD handlers ────────────────────────────────────────────────────────

    /** POST /api/v1/functions – register a new function. */
    http::response<http::string_body> handleRegister(
        const http::request<http::string_body>& req);

    /** GET /api/v1/functions – list all functions. */
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    /** GET /api/v1/functions/{id} – get a single function definition. */
    http::response<http::string_body> handleGet(
        const http::request<http::string_body>& req,
        const std::string& id);

    /** PUT /api/v1/functions/{id} – update a function definition. */
    http::response<http::string_body> handleUpdate(
        const http::request<http::string_body>& req,
        const std::string& id);

    /** DELETE /api/v1/functions/{id} – delete a function. */
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string& id);

    // ── Invocation ───────────────────────────────────────────────────────────

    /** POST /api/v1/functions/{id}/invoke – execute function in-process. */
    http::response<http::string_body> handleInvoke(
        const http::request<http::string_body>& req,
        const std::string& id);

    // ── Version history ──────────────────────────────────────────────────────

    /** GET /api/v1/functions/{id}/versions – list version snapshots. */
    http::response<http::string_body> handleVersions(
        const http::request<http::string_body>& req,
        const std::string& id);

private:
    // ── Registry ─────────────────────────────────────────────────────────────

    mutable std::mutex registry_mutex_;
    std::unordered_map<std::string, ServerlessFunction> registry_;

    /// Version history: id → ordered list of snapshots (newest last).
    std::unordered_map<std::string, std::vector<ServerlessFunction>> version_history_;

    // ── Execution engine ─────────────────────────────────────────────────────

    /**
     * @brief Execute a function's DSL operations against the input payload.
     *
     * @param fn     The function to execute.
     * @param input  Caller-supplied JSON payload.
     * @param output Result of the operation pipeline.
     * @param error  Human-readable error message on failure.
     * @return true on success, false on failure.
     */
    bool executeFunction(const ServerlessFunction& fn,
                         const nlohmann::json& input,
                         nlohmann::json& output,
                         std::string& error) const;

    // ── HTTP helpers ─────────────────────────────────────────────────────────

    http::response<http::string_body> makeJsonResponse(
        http::status status,
        const nlohmann::json& body,
        const http::request<http::string_body>& req) const;

    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;

    // ── Utilities ─────────────────────────────────────────────────────────────

    /// Generate a simple unique ID (timestamp + counter).
    static std::string generateId();

    /// Return current UTC time as ISO-8601 string.
    static std::string utcNow();

    /// Validate the DSL code block; returns an error string or empty on success.
    static std::string validateCode(const nlohmann::json& code);
};

} // namespace server
} // namespace themis

