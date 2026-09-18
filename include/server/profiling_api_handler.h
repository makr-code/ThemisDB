/**
 * @file profiling_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include "observability/query_profiler.h"
#include "observability/storage_profiler.h"
#include "observability/performance_analyzer.h"

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class ProfilingApiHandler {
public:
    ProfilingApiHandler(
        std::shared_ptr<observability::QueryProfiler> query_profiler,
        std::shared_ptr<observability::StorageProfiler> storage_profiler,
        std::shared_ptr<observability::PerformanceAnalyzer> analyzer
    );
    
    /**
     * @brief Handle request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_request(
        const http::request<http::string_body>& req);
    
private:
    std::shared_ptr<observability::QueryProfiler> query_profiler_;
    std::shared_ptr<observability::StorageProfiler> storage_profiler_;
    std::shared_ptr<observability::PerformanceAnalyzer> analyzer_;
    
    // Handler methods
    /**
     * @brief Handle enable.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_enable(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle disable.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_disable(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle get queries.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_get_queries(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle get slow queries.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_get_slow_queries(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle get storage.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_get_storage(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle analyze.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_analyze(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle export.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_export(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle clear.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_clear(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle get config.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_get_config(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle set config.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle_set_config(
        const http::request<http::string_body>& req);
    
    // Utility methods
    /**
     * @brief Make response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> make_response(
        http::status status, 
        const nlohmann::json& body);
    
    /**
     * @brief Make error response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> make_error_response(
        http::status status,
        const std::string& message);
    
    /**
     * @brief Get query param int.
     * @param[in] target Input parameter.
     * @param[in] param_name Name of the param.
     * @param[in] default_value Input parameter.
     * @param[in,out] value Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool get_query_param_int(const std::string& target,
                             const std::string& param_name,
                             int default_value,
                             int& value);
};

} // namespace server
} // namespace themis
