/**
 * @file udf_api_handler.h
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

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

class UdfApiHandler {
public:
    UdfApiHandler() = default;

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
     * @param[in] name Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGet(
        const http::request<http::string_body>& req,
        const std::string& name);

    /**
     * @brief Handle Delete.
     * @param[in] req Input parameter.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string& name);

private:
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
};

} // namespace server
} // namespace themis
