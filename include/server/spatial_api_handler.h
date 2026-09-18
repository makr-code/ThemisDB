/**
 * @file spatial_api_handler.h
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
#include <unordered_map>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace index {
class SpatialIndexManager;
}

namespace server {

class SpatialApiHandler {
public:
    SpatialApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<index::SpatialIndexManager> spatial_index,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle Index Create.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexCreate(const http::request<http::string_body>& req);

    /**
     * @brief Handle Index Rebuild.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexRebuild(const http::request<http::string_body>& req);

    /**
     * @brief Handle Index Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle Metrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<index::SpatialIndexManager> spatial_index_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

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
    
    // Query parameter parsing
    std::unordered_map<std::string, std::string> parseQuery(const std::string& target);
    /**
     * @brief Url Decode.
     * @param[in] str Input parameter.
     * @return Return value.
     */
    std::string urlDecode(const std::string& str);
};

} // namespace server
} // namespace themis
