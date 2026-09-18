/**
 * @file update_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast.hpp>
#include <memory>
#include <string>
#include "utils/update_checker.h"

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class UpdateApiHandler {
public:
    /**
     * @brief Update Api Handler.
     * @param[in] checker Input parameter.
     * @return Return value.
     */
    explicit UpdateApiHandler(std::shared_ptr<utils::UpdateChecker> checker);
    
    /**
     * @brief Handle Request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<utils::UpdateChecker> checker_;
    
    /**
     * @brief Handle Get Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetStatus(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Check Now.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCheckNow(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Get Config.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetConfig(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Update Config.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateConfig(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Create Json Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> createJsonResponse(
        http::status status,
        const nlohmann::json& body,
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Create Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> createErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );
};

} // namespace server
} // namespace themis
