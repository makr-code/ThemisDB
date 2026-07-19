/**
 * @file wasm_handler_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "themis/base/wasm_plugin_sandbox.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace beast = boost::beast;
namespace http  = beast::http;

namespace themis {
namespace server {

// =============================================================================
// WasmHandlerConfig – per-handler resource limits
// =============================================================================

/**
 * @brief Resource limits and execution policy for a single WASM handler invocation.
 * 
 * Defines the bounded execution envelope for untrusted WASM code.
 * Violations of these limits result in handler termination and error response.
 * 
 * ### Resource Constraints
 * - cpu_time_limit: Maximum wall-clock time; enforced via async timeout
 * - memory_limit_bytes: Max linear-memory size (64 MiB default)
 * - linear_memory_pages: Pre-allocated pages (64 KiB each); growing pages trigger error
 * - entry_point: Export function to invoke (default "handle" for all handlers)
 * 
 * ### Invocation Flow
 * 1. Allocate sandbox with linear_memory_pages
 * 2. Start execution timeout timer
 * 3. Call entry_point function with request data
 * 4. If timeout exceeded or memory exceeded, terminate handler
 * 5. Return error response to client:
 *    - HTTP 504 Gateway Timeout for wall-clock limit violations
 *    - HTTP 500 Internal Server Error for memory-limit and other runtime failures
 * 
 * ### Security Implications
 * - cpu_time_limit prevents infinite loops and DoS attacks
 * - memory_limit_bytes prevents memory exhaustion on the host
 * - linear_memory_pages prevents out-of-bounds memory access
 * - entry_point validation ensures handlers export the expected interface
 * 
 * @note These limits are per-invocation; multiple concurrent invocations each have their own envelope
 * @note Unused fields remain at their defaults; does not require full specification
 * @note Default limits are conservative; production deployments may adjust based on workload
 * 
 * @see WasmHandlerEntry for handler registration with config
 * @see themis::modules::WasmPluginSandbox for enforcement implementation
 */
struct WasmHandlerConfig {
    /// Maximum wall-clock execution time per invocation (default 500 ms).
    std::chrono::milliseconds cpu_time_limit{500};

    /// Maximum linear-memory the handler may allocate (default 64 MiB).
    size_t memory_limit_bytes = 64ULL * 1024 * 1024;

    /// WASM linear-memory pages pre-allocated for the sandbox (64 KiB each).
    uint32_t linear_memory_pages = 1024; // 64 MiB

    /// Export function name to call when invoking the handler (default "handle").
    std::string entry_point = "handle";
};

// =============================================================================
// WasmHandlerEntry – registry record for one WASM handler
// =============================================================================

/**
 * @brief Complete metadata and binary for a registered WASM API handler.
 * 
 * Represents a deployed serverless function implemented in WebAssembly.
 * Handlers are stored in a tenant-keyed registry and invoked for matching HTTP routes.
 * 
 * ### Handler Lifecycle
 * 1. Tenant uploads WASM binary via serverless API
 * 2. Handler is validated, sandboxed, and stored in registry
 * 3. When a matching route is invoked, handler is loaded and executed
 * 4. After execution, result is returned to client
 * 5. Handler may be updated (version incremented) or deleted
 * 
 * ### Registry Key
 * Handlers are indexed by (tenant_id, id) pair:
 * - tenant_id: Owner of the handler (empty = globally accessible)
 * - id: Unique function/handler identifier
 * 
 * ### Resource Limits
 * Each handler has a WasmHandlerConfig that defines:
 * - CPU time limit per invocation
 * - Memory limit per invocation
 * - Entry point function name
 * 
 * ### Metrics
 * - version: Incremented each time the WASM binary is re-uploaded
 * - invocation_count: Total successful invocations (for monitoring)
 * - created_at, updated_at: Timestamps for auditing
 * 
 * ### Move Semantics
 * WasmHandlerEntry is move-only (non-copyable) due to atomic<uint64_t> member.
 * Moving a handler is efficient; use std::move when storing in containers.
 * 
 * @note Handler ID must be URL-safe (alphanumeric, hyphens, underscores)
 * @note WASM binary is validated at upload time; malformed binaries are rejected
 * @note Handler invocation is always sandboxed with resource limits
 * @note Concurrent invocations of the same handler are allowed and isolated
 * 
 * @see WasmHandlerRegistry for management operations
 * @see WasmHandlerConfig for resource limit details
 */
    struct WasmHandlerEntry {
    std::string id;          ///< Unique function / handler ID
    std::string tenant_id;   ///< Owning tenant (empty = global)
    std::string name;        ///< Human-readable handler name
    std::string description; ///< Optional description

    /// Raw .wasm binary as uploaded by the tenant.
    std::vector<uint8_t> wasm_bytes;

    /// Metadata parsed from the binary header at upload time.
    themis::modules::WasmModuleInfo module_info;

    WasmHandlerConfig config; ///< Resource-limit configuration

    std::string created_at; ///< ISO-8601 creation timestamp
    std::string updated_at; ///< ISO-8601 last-update timestamp

    /// Monotonically increasing version counter (incremented on re-upload).
    uint32_t version = 1;

    /// Total successful invocations since registration.
    std::atomic<uint64_t> invocation_count{0};

    // WasmHandlerEntry is move-only (atomic member).
    WasmHandlerEntry() = default;
    WasmHandlerEntry(const WasmHandlerEntry&)            = delete;
    WasmHandlerEntry& operator=(const WasmHandlerEntry&) = delete;
    WasmHandlerEntry(WasmHandlerEntry&& other) noexcept
        : id(std::move(other.id)),
          tenant_id(std::move(other.tenant_id)),
          name(std::move(other.name)),
          description(std::move(other.description)),
          wasm_bytes(std::move(other.wasm_bytes)),
          module_info(std::move(other.module_info)),
          config(std::move(other.config)),
          created_at(std::move(other.created_at)),
          updated_at(std::move(other.updated_at)),
          version(other.version),
          invocation_count(other.invocation_count.load(std::memory_order_relaxed)) {}

    WasmHandlerEntry& operator=(WasmHandlerEntry&& other) noexcept {
        if (this != &other) {
            id = std::move(other.id);
            tenant_id = std::move(other.tenant_id);
            name = std::move(other.name);
            description = std::move(other.description);
            wasm_bytes = std::move(other.wasm_bytes);
            module_info = std::move(other.module_info);
            config = std::move(other.config);
            created_at = std::move(other.created_at);
            updated_at = std::move(other.updated_at);
            version = other.version;
            invocation_count.store(
                other.invocation_count.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
        }
        return *this;
    }

    /// Serialise to JSON for API responses (does not include binary bytes).
    nlohmann::json toJson() const;
};

// =============================================================================
// WasmInvokeResult – outcome of a single handler invocation
// =============================================================================

/**
 * @brief Return value from WasmHandlerRegistry::invoke().
 */
struct WasmInvokeResult {
    bool        success      = false;
    std::string error;               ///< Non-empty on failure
    std::string output;              ///< Handler output as a UTF-8 string
    uint64_t    duration_us  = 0;    ///< Wall-clock invocation time (µs)
    bool        timeout      = false;///< true when CPU-time limit was exceeded
    bool        oom          = false;///< true when memory cap was exceeded
};

// =============================================================================
// WasmHandlerRegistry – registry and router for WASM API handlers
// =============================================================================

/**
 * @brief Registry and HTTP API handler for WASM-based edge-computing functions.
 *
 * ## Lifecycle
 * 1. Tenant calls `handleUpload()` with a `.wasm` binary in the request body.
 * 2. The binary is validated via `WasmModuleValidator::validate()`.
 * 3. On success the entry is stored in the in-memory registry.
 * 4. On invocation (`handleInvoke()`), a fresh `WasmPluginSandbox` is created
 *    from the stored bytes, the entry point is called, and the sandbox is
 *    destroyed after the call returns.
 *
 * ## Notes on binary transport
 *
 * For the upload endpoint the request body is treated as raw binary bytes.
 * Clients should set `Content-Type: application/wasm`.  If the body is
 * Base64-encoded (Content-Type: application/json with a `"wasm_base64"` field)
 * the handler decodes it transparently.
 *
 * For the invoke endpoint the request body is a UTF-8 JSON object.  It is
 * serialised to bytes and passed to the WASM entry function as the argument
 * blob.  The handler expects the WASM module to write a UTF-8 JSON response
 * into its output buffer.
 *
 * @note This class does NOT depend on a specific WASM runtime.  Invoke will
 *       work only if a runtime was injected via
 *       `themis::modules::WasmRuntimeInjector::registerRuntime()` before use,
 *       or if the sandbox is in validation-only mode.
 */
class WasmHandlerRegistry {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    WasmHandlerRegistry() = default;

    WasmHandlerRegistry(const WasmHandlerRegistry&)            = delete;
    WasmHandlerRegistry& operator=(const WasmHandlerRegistry&) = delete;

    // ── HTTP endpoint handlers ────────────────────────────────────────────────

    /**
     * @brief Handle POST /api/v1/functions/{id}/wasm – upload a WASM binary.
     *
     * Request body: raw `.wasm` bytes (Content-Type: application/wasm), OR
     * a JSON object with a `"wasm_base64"` field containing the Base64-encoded
     * binary.  Optional JSON fields: `"name"`, `"tenant_id"`, `"description"`,
     * `"cpu_time_ms"`, `"memory_limit_mb"`.
     *
     * Responses:
     *   - 201 Created  – binary validated and handler registered.
     *   - 200 OK       – existing handler re-uploaded (version incremented).
     *   - 400 Bad Request – invalid .wasm binary or missing body.
     */
    http::response<http::string_body> handleUpload(
        const http::request<http::string_body>& req,
        const std::string& id);

    /**
     * @brief Handle GET /api/v1/functions/wasm – list all registered handlers.
     *
     * Optional query parameter: `?tenant_id=<id>` to filter by tenant.
     *
     * Response: 200 OK with JSON array of handler metadata objects.
     */
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/v1/functions/{id}/wasm – get handler metadata.
     *
     * Response: 200 OK with handler metadata, or 404 if not found.
     */
    http::response<http::string_body> handleGet(
        const http::request<http::string_body>& req,
        const std::string& id);

    /**
     * @brief Handle DELETE /api/v1/functions/{id}/wasm – remove a handler.
     *
     * Response: 204 No Content on success, 404 if not found.
     */
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string& id);

    /**
     * @brief Handle POST /api/v1/functions/{id}/wasm/invoke – run the handler.
     *
     * Request body: JSON object passed as input to the WASM entry function.
     *
     * Responses:
     *   - 200 OK            – invocation succeeded.
     *   - 404 Not Found     – handler ID not registered.
     *   - 400 Bad Request   – invalid input JSON.
     *   - 504 Gateway Timeout – CPU time limit exceeded.
     *   - 500 Internal Server Error – memory overflow or runtime trap.
     */
    http::response<http::string_body> handleInvoke(
        const http::request<http::string_body>& req,
        const std::string& id);

    // ── Programmatic API ─────────────────────────────────────────────────────

    /**
     * @brief Register a WASM binary programmatically.
     *
     * @param id         Function ID.
     * @param wasm_bytes Raw .wasm binary bytes.
     * @param config     Resource-limit configuration.
     * @param tenant_id  Optional owning tenant.
     * @param name       Human-readable name.
     * @param description Optional description.
     * @param error      Set to a human-readable message on failure.
     * @return true on success; false if the binary is invalid.
     */
    bool registerHandler(const std::string&          id,
                         const std::vector<uint8_t>& wasm_bytes,
                         const WasmHandlerConfig&    config      = {},
                         const std::string&          tenant_id   = {},
                         const std::string&          name        = {},
                         const std::string&          description = {},
                         std::string*                error       = nullptr);

    /**
     * @brief Remove a registered handler.
     * @return true if it existed and was removed; false if not found.
     */
    bool unregisterHandler(const std::string& id);

    /**
     * @brief Invoke a registered WASM handler.
     *
     * Creates a fresh `WasmPluginSandbox`, loads the stored binary, calls the
     * configured entry point, and destroys the sandbox after the call.
     *
     * @param id    Function ID.
     * @param input JSON payload forwarded to the WASM handler as UTF-8 bytes.
     * @return WasmInvokeResult describing success, output, duration.
     */
    WasmInvokeResult invoke(const std::string&    id,
                            const nlohmann::json& input = nlohmann::json::object());

    /// @brief Return true if a handler with the given ID is registered.
    bool hasHandler(const std::string& id) const;

    /// @brief Return the number of registered handlers.
    size_t size() const;

    /// @brief Return metadata for all handlers (no binary bytes).
    std::vector<nlohmann::json> listHandlers(const std::string& tenant_id_filter = {}) const;

private:
    // ── Registry storage ──────────────────────────────────────────────────────

    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, WasmHandlerEntry> registry_;

    // ── HTTP helpers ──────────────────────────────────────────────────────────

    http::response<http::string_body> makeJsonResponse(
        http::status                            status,
        const nlohmann::json&                   body,
        const http::request<http::string_body>& req) const;

    http::response<http::string_body> makeErrorResponse(
        http::status                            status,
        const std::string&                      message,
        const http::request<http::string_body>& req) const;

    // ── Utilities ─────────────────────────────────────────────────────────────

    /// Return current UTC time as ISO-8601 string.
    static std::string utcNow();

    /// Decode a Base64-encoded string into bytes.  Returns empty on error.
    static std::vector<uint8_t> base64Decode(const std::string& encoded);
};

} // namespace server
} // namespace themis
