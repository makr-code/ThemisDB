/**
 * @file rope_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class VectorIndexManager;
class AuthMiddleware;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class RopeApiHandler {
public:
    RopeApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<::themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle Config Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Config Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfigDelete(const http::request<http::string_body>& req);

    /**
     * @brief Handle Add Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAddPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle Add Relational Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAddRelationalPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle Search Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSearchPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle Batch Add Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBatchAddPost(const http::request<http::string_body>& req);

    /**
     * @brief Handle Stats Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStatsGet(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<::themis::AuthMiddleware> auth_;

    // ─── Bridges (stubs #280, #307) ───────────────────────────────────────────

    using AuthorizeFn = std::function<bool(const std::string& token,
                                           const std::string& action)>;

    /**
     * @brief Set Authorize Fn.
     * @param[in] fn Input parameter.
     */
    void setAuthorizeFn(AuthorizeFn fn);

    /**
     * @brief Clear Authorize Fn.
     */
    void clearAuthorizeFn();

    using StatsQueryFn = std::function<nlohmann::json()>;

    /**
     * @brief Set Stats Query Fn.
     * @param[in] fn Input parameter.
     */
    void setStatsQueryFn(StatsQueryFn fn);

    /**
     * @brief Clear Stats Query Fn.
     */
    void clearStatsQueryFn();

    AuthorizeFn authorizeFn_;
    StatsQueryFn statsQueryFn_;

    // Helper methods
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    /**
     * @brief Require Access.
     * @param[in] req Input parameter.
     * @param[in] permission Input parameter.
     * @param[in] resource Input parameter.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& permission,
        const std::string& resource,
        const std::string& path);
    
    /**
     * @brief Extract Index Name.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::optional<std::string> extractIndexName(const std::string& path);
};

} // namespace server
} // namespace themis
