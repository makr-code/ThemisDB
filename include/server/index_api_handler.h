/**
 * @file index_api_handler.h
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

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class AdaptiveIndexManager;

namespace server {

class IndexApiHandler {
public:
    IndexApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<AdaptiveIndexManager> adaptive_index,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle Create.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreate(const http::request<http::string_body>& req);

    /**
     * @brief Handle Drop.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDrop(const http::request<http::string_body>& req);

    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle Rebuild.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRebuild(const http::request<http::string_body>& req);

    /**
     * @brief Handle Reindex.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleReindex(const http::request<http::string_body>& req);

    /**
     * @brief Handle Suggestions.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSuggestions(const http::request<http::string_body>& req);

    /**
     * @brief Handle Patterns.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePatterns(const http::request<http::string_body>& req);

    /**
     * @brief Handle Record Pattern.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRecordPattern(const http::request<http::string_body>& req);

    /**
     * @brief Handle Clear Patterns.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleClearPatterns(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<AdaptiveIndexManager> adaptive_index_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

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
};

} // namespace server
} // namespace themis
