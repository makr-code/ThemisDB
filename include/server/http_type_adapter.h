/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            http_type_adapter.h                                ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:37:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <boost/beast.hpp>
#include <httplib.h>
#include <string>
#include <string_view>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief Temporary adapter to bridge Boost.Beast and cpp-httplib types
 * 
 * This class provides conversion functions between Beast's HTTP types
 * and cpp-httplib's HTTP types to allow interoperability during the
 * migration period.
 * 
 * TODO: Remove this adapter after full migration to cpp-httplib is complete
 * See: HTTP_SERVER_REFACTORING_ACTION_PLAN.md
 * 
 * @warning This adapter has performance overhead due to type conversions.
 *          Conversions include:
 *          - String copies for method, path, headers, body
 *          - URL decoding of query parameters
 *          - Object construction/destruction
 *          Estimated overhead: ~1-5% latency increase per request
 *          It should only be used as a temporary solution.
 */
class HttpTypeAdapter {
public:
    /**
     * @brief Convert Beast request to cpp-httplib request
     * 
     * @param beast_req Beast HTTP request
     * @return httplib::Request Converted cpp-httplib request
     */
    static httplib::Request beastToHttplib(
        const http::request<http::string_body>& beast_req
    );
    
    /**
     * @brief Convert cpp-httplib response to Beast response
     * 
     * @param httplib_res cpp-httplib HTTP response
     * @param version HTTP version (default: 11 for HTTP/1.1)
     * @return http::response<http::string_body> Converted Beast response
     */
    static http::response<http::string_body> httplibToBeast(
        const httplib::Response& httplib_res,
        unsigned version = 11
    );

private:
    /**
     * @brief Convert HTTP method from Beast to string
     */
    static std::string methodToString(http::verb method);
    
    /**
     * @brief Convert HTTP status from integer to Beast status
     */
    static http::status intToStatus(int status_code);
};

} // namespace server
} // namespace themis
