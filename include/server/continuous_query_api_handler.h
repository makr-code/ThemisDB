/**
 * @file continuous_query_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 2.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/continuous_query_engine.h"
#include "query/continuous_query_registry.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json_fwd.hpp>
#include <memory>
#include <string>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

class ContinuousQueryApiHandler {
public:
    /**
     * @brief Continuous Query Api Handler.
     * @param[in] engine Input parameter.
     * @return Return value.
     */
    explicit ContinuousQueryApiHandler(
        std::shared_ptr<themis::query::ContinuousQueryEngine> engine);

    ~ContinuousQueryApiHandler() = default;

    // Non-copyable, movable
    ContinuousQueryApiHandler(const ContinuousQueryApiHandler&) = delete;
    ContinuousQueryApiHandler& operator=(const ContinuousQueryApiHandler&) = delete;
    ContinuousQueryApiHandler(ContinuousQueryApiHandler&&) noexcept = default;
    ContinuousQueryApiHandler& operator=(ContinuousQueryApiHandler&&) noexcept = default;

    [[nodiscard]] http::response<http::string_body> handleRegister(
        const http::request<http::string_body>& req);

    [[nodiscard]] http::response<http::string_body> handleDrop(
        const http::request<http::string_body>& req,
        const std::string& name);

    [[nodiscard]] http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);

    [[nodiscard]] http::response<http::string_body> handleStreamSse(
        const http::request<http::string_body>& req,
        const std::string& name);

private:
    std::shared_ptr<themis::query::ContinuousQueryEngine> engine_;

    [[nodiscard]] static http::response<http::string_body> makeError(
        http::status status, const std::string& message,
        const http::request<http::string_body>& req);

    [[nodiscard]] static http::response<http::string_body> makeJson(
        http::status status, const std::string& body,
        const http::request<http::string_body>& req);

    [[nodiscard]] static themis::query::WindowSpec windowFromJson(
        const nlohmann::json& j);

    [[nodiscard]] static nlohmann::json infoToJson(
        const themis::query::ContinuousQueryInfo& info);
};

}  // namespace server
}  // namespace themis

