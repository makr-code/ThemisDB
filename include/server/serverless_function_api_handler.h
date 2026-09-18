/**
 * @file serverless_function_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct ServerlessFunction {
    std::string id;          ///< Unique function ID (UUID-like)
    std::string name;        ///< Human-readable name
    std::string tenant_id;   ///< Owning tenant (empty = global)
    std::string description; ///< Optional description
    nlohmann::json code;
    uint32_t timeout_ms{5000};    ///< Max execution time in milliseconds
    uint32_t memory_limit_kb{4096}; ///< Soft memory guard (tracked allocations)
    uint32_t version{1};          ///< Monotonically increasing version counter
    std::string created_at;       ///< ISO-8601 creation timestamp
    std::string updated_at;       ///< ISO-8601 last-update timestamp

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class ServerlessFunctionApiHandler {
public:
    ServerlessFunctionApiHandler() = default;


    /**
     * @brief Handle Register.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRegister(
        const http::request<http::string_body>& req);

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
     * @brief Handle Update.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdate(
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


    /**
     * @brief Handle Versions.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleVersions(
        const http::request<http::string_body>& req,
        const std::string& id);

private:
    // ── Registry ─────────────────────────────────────────────────────────────

    mutable std::mutex registry_mutex_;
    std::unordered_map<std::string, ServerlessFunction> registry_;

    std::unordered_map<std::string, std::vector<ServerlessFunction>> version_history_;


    /**
     * @brief Execute Function.
     * @param[in] fn Input parameter.
     * @param[in] input Input parameter.
     * @param[in,out] output Input/output parameter.
     * @param[in,out] error Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool executeFunction(const ServerlessFunction& fn,
                         const nlohmann::json& input,
                         nlohmann::json& output,
                         std::string& error) const;


    /**
     * @brief Make Json Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeJsonResponse(
        http::status status,
        const nlohmann::json& body,
        const http::request<http::string_body>& req) const;

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;


    /**
     * @brief Generate Id.
     * @return Return value.
     */
    static std::string generateId();

    /**
     * @brief Utc Now.
     * @return Return value.
     */
    static std::string utcNow();

    /**
     * @brief Validate Code.
     * @param[in] code Input parameter.
     * @return Return value.
     */
    static std::string validateCode(const nlohmann::json& code);
};

} // namespace server
} // namespace themis

