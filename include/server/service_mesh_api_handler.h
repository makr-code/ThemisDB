/**
 * @file service_mesh_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast/http.hpp>
#include <memory>
#include <string>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

class ServiceMeshApiHandler {
public:
    explicit ServiceMeshApiHandler(std::shared_ptr<void> smi = nullptr);

    ~ServiceMeshApiHandler() = default;

    // Non-copyable, movable
    ServiceMeshApiHandler(const ServiceMeshApiHandler&) = delete;
    ServiceMeshApiHandler& operator=(const ServiceMeshApiHandler&) = delete;
    ServiceMeshApiHandler(ServiceMeshApiHandler&&) noexcept = default;
    ServiceMeshApiHandler& operator=(ServiceMeshApiHandler&&) noexcept = default;

    /**
     * @brief Handle Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req) const;

    /**
     * @brief Handle Config.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfig(
        const http::request<http::string_body>& req) const;

    /**
     * @brief Handle Annotations.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAnnotations(
        const http::request<http::string_body>& req) const;

private:
    // Opaque pointer to network::ServiceMeshIntegration (type-erased to
    // avoid including the conditionally-compiled network header here).
    std::shared_ptr<void> smi_;

    /**
     * @brief Make Json.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeJson(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req) const;

    /**
     * @brief Make Disabled.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeDisabled(
        const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis
