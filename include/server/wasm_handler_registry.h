/**
 * @file wasm_handler_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

struct WasmHandlerConfig {
    std::chrono::milliseconds cpu_time_limit{500};

    size_t memory_limit_bytes = 64ULL * 1024 * 1024;

    uint32_t linear_memory_pages = 1024; // 64 MiB

    std::string entry_point = "handle";
};

// =============================================================================
// WasmHandlerEntry – registry record for one WASM handler
// =============================================================================

    struct WasmHandlerEntry {
    std::string id;          ///< Unique function / handler ID
    std::string tenant_id;   ///< Owning tenant (empty = global)
    std::string name;        ///< Human-readable handler name
    std::string description; ///< Optional description

    std::vector<uint8_t> wasm_bytes;

    themis::modules::WasmModuleInfo module_info;

    WasmHandlerConfig config; ///< Resource-limit configuration

    std::string created_at; ///< ISO-8601 creation timestamp
    std::string updated_at; ///< ISO-8601 last-update timestamp

    uint32_t version = 1;

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

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// =============================================================================
// WasmInvokeResult – outcome of a single handler invocation
// =============================================================================

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

class WasmHandlerRegistry {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    WasmHandlerRegistry() = default;

    WasmHandlerRegistry(const WasmHandlerRegistry&)            = delete;
    WasmHandlerRegistry& operator=(const WasmHandlerRegistry&) = delete;


    /**
     * @brief Handle Upload.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUpload(
        const http::request<http::string_body>& req,
        const std::string& id);

    /**
     * @brief Handle List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Get.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGet(
        const http::request<http::string_body>& req,
        const std::string& id);

    /**
     * @brief Handle Delete.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string& id);

    /**
     * @brief Handle Invoke.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleInvoke(
        const http::request<http::string_body>& req,
        const std::string& id);

    // ── Programmatic API ─────────────────────────────────────────────────────

    bool registerHandler(const std::string&          id,
                         const std::vector<uint8_t>& wasm_bytes,
                         const WasmHandlerConfig&    config      = {},
                         const std::string&          tenant_id   = {},
                         const std::string&          name        = {},
                         const std::string&          description = {},
                         std::string*                error       = nullptr);

    /**
     * @brief Unregister Handler.
     * @param[in] id Input parameter.
     * @return True when the operation succeeds.
     */
    bool unregisterHandler(const std::string& id);

    WasmInvokeResult invoke(const std::string&    id,
                            const nlohmann::json& input = nlohmann::json::object());

    /**
     * @brief Has Handler.
     * @param[in] id Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasHandler(const std::string& id) const;

    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

    std::vector<nlohmann::json> listHandlers(const std::string& tenant_id_filter = {}) const;

private:
    // ── Registry storage ──────────────────────────────────────────────────────

    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, WasmHandlerEntry> registry_;


    /**
     * @brief Make Json Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeJsonResponse(
        http::status                            status,
        const nlohmann::json&                   body,
        const http::request<http::string_body>& req) const;

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status                            status,
        const std::string&                      message,
        const http::request<http::string_body>& req) const;


    /**
     * @brief Utc Now.
     * @return Return value.
     */
    static std::string utcNow();

    /**
     * @brief Base64 Decode.
     * @param[in] encoded Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> base64Decode(const std::string& encoded);
};

} // namespace server
} // namespace themis
