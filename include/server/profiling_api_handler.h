/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            profiling_api_handler.h                            ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * @brief Handler for Performance Profiling API
 * 
 * This handler manages profiling endpoints:
 * - POST /api/profiling/enable - Enable profiling
 * - POST /api/profiling/disable - Disable profiling
 * - GET /api/profiling/queries - Get query profiles
 * - GET /api/profiling/slow-queries - Get slow queries
 * - GET /api/profiling/storage - Get storage statistics
 * - POST /api/profiling/analyze - Run performance analysis
 * - GET /api/profiling/export - Export all profiles
 * - POST /api/profiling/clear - Clear all profiles
 * - GET /api/profiling/config - Get profiling configuration
 * - POST /api/profiling/config - Update profiling configuration
 */
class ProfilingApiHandler {
public:
    /**
     * @brief Construct a new Profiling API Handler
     * 
     * @param query_profiler Query profiler instance
     * @param storage_profiler Storage profiler instance
     * @param analyzer Performance analyzer instance
     */
    ProfilingApiHandler(
        std::shared_ptr<observability::QueryProfiler> query_profiler,
        std::shared_ptr<observability::StorageProfiler> storage_profiler,
        std::shared_ptr<observability::PerformanceAnalyzer> analyzer
    );
    
    /**
     * @brief Handle profiling API request
     * @param req HTTP request
     * @return HTTP response
     */
    http::response<http::string_body> handle_request(
        const http::request<http::string_body>& req);
    
private:
    std::shared_ptr<observability::QueryProfiler> query_profiler_;
    std::shared_ptr<observability::StorageProfiler> storage_profiler_;
    std::shared_ptr<observability::PerformanceAnalyzer> analyzer_;
    
    // Handler methods
    http::response<http::string_body> handle_enable(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_disable(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_get_queries(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_get_slow_queries(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_get_storage(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_analyze(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_export(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_clear(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_get_config(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handle_set_config(
        const http::request<http::string_body>& req);
    
    // Utility methods
    http::response<http::string_body> make_response(
        http::status status, 
        const nlohmann::json& body);
    
    http::response<http::string_body> make_error_response(
        http::status status,
        const std::string& message);
    
    int get_query_param_int(const std::string& target, 
                           const std::string& param_name, 
                           int default_value = 0);
};

} // namespace server
} // namespace themis
