/**
 * @file http_type_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class HttpTypeAdapter {
public:
    /**
     * @brief Beast To Httplib.
     * @param[in] beast_req Input parameter.
     * @return Return value.
     */
    static httplib::Request beastToHttplib(
        const http::request<http::string_body>& beast_req
    );
    
    static http::response<http::string_body> httplibToBeast(
        const httplib::Response& httplib_res,
        unsigned version = 11
    );

private:
    /**
     * @brief Method To String.
     * @param[in] method Input parameter.
     * @return Return value.
     */
    static std::string methodToString(http::verb method);
    
    /**
     * @brief Int To Status.
     * @param[in] status_code Input parameter.
     * @return Return value.
     */
    static http::status intToStatus(int status_code);
};

} // namespace server
} // namespace themis
