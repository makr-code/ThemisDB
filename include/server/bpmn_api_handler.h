/**
 * @file bpmn_api_handler.h
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
class ProcessGraphManager;

namespace server {

class BpmnApiHandler {
public:
    struct AuthContext {
        std::string user_id;
        std::vector<std::string> groups;
    };

    BpmnApiHandler(
        std::shared_ptr<ProcessGraphManager> process_graph,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle Start Process.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStartProcess(const http::request<http::string_body>& req);

    /**
     * @brief Handle Task Complete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTaskComplete(const http::request<http::string_body>& req);

    /**
     * @brief Handle Query Instance.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQueryInstance(const http::request<http::string_body>& req);

private:
    std::shared_ptr<ProcessGraphManager> process_graph_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods
    /**
     * @brief Extract Path Param.
     * @param[in] target Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string extractPathParam(const std::string& target, const std::string& prefix);
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
    
    // Authorization helpers
    /**
     * @brief Extract Auth Context.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    AuthContext extractAuthContext(const http::request<http::string_body>& req) const;
    /**
     * @brief Require Access.
     * @param[in] req Input parameter.
     * @param[in] scope Input parameter.
     * @param[in] action Input parameter.
     * @param[in] resource Input parameter.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& scope,
        const std::string& action,
        const std::string& resource
    );
};

} // namespace server
} // namespace themis
