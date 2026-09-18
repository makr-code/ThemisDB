/**
 * @file prompt_api_handler.h
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

namespace prompt_engineering {
class PromptManager;
}

namespace server {

class PromptApiHandler {
public:
    PromptApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<prompt_engineering::PromptManager> prompt_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePost(const http::request<http::string_body>& req);

    /**
     * @brief Handle List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleList(const http::request<http::string_body>& req);

    /**
     * @brief Handle Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePut(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<prompt_engineering::PromptManager> prompt_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

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
};

} // namespace server
} // namespace themis
