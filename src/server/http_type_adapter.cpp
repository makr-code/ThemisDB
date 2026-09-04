/**
 * @file http_type_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/http_type_adapter.h"
#include <array>
#include <sstream>

// RFC 3986-compliant percent-decoder.  Decodes %XX sequences (case-insensitive
// hex digits) and maps '+' to space (application/x-www-form-urlencoded).
// Malformed sequences (%XX where XX is not valid hex, or a lone %) are passed
// through unchanged so callers can detect and reject bad input if needed.
// UTF-8 multi-byte characters are decoded byte-by-byte — the resulting string
// is a valid UTF-8 byte sequence as long as the original URL was UTF-8 encoded
// (mandated by RFC 3986 §2.5 and HTML5 §application/x-www-form-urlencoded).

namespace themis {
namespace server {

namespace {

// Returns the decimal value of a hex nibble [-1, 15].
// int is intentional: the caller computes `(hi << 4) | lo` with standard
// integer arithmetic and then casts to char; using int avoids implicit
// signed-char promotion surprises on platforms where char is unsigned.
constexpr int hexDigit(char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

std::string urlDecodeTypeAdapter(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            out += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            int hi = hexDigit(str[i + 1]);
            int lo = hexDigit(str[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out += static_cast<char>((hi << 4) | lo);
                i += 2;
            } else {
                out += str[i]; // malformed: pass through the bare '%'
            }
        } else {
            out += str[i];
        }
    }
    return out;
}

} // anonymous namespace

httplib::Request HttpTypeAdapter::beastToHttplib(
    const http::request<http::string_body>& beast_req
) {
    httplib::Request httplib_req;
    
    // Convert HTTP method
    httplib_req.method = methodToString(beast_req.method());
    
    // Convert target (path + query string)
    std::string target = std::string(beast_req.target());
    
    // Split path and query string
    size_t query_pos = target.find('?');
    if (query_pos != std::string::npos) {
        httplib_req.path = target.substr(0, query_pos);
        
        // Parse query parameters with RFC 3986-compliant percent-decoding.
        std::string query_string = target.substr(query_pos + 1);
        size_t start = 0;
        while (start < query_string.length()) {
            size_t eq_pos = query_string.find('=', start);
            size_t amp_pos = query_string.find('&', start);

            if (eq_pos != std::string::npos && (amp_pos == std::string::npos || eq_pos < amp_pos)) {
                std::string key = query_string.substr(start, eq_pos - start);
                size_t value_end = (amp_pos != std::string::npos) ? amp_pos : query_string.length();
                std::string value = query_string.substr(eq_pos + 1, value_end - eq_pos - 1);

                httplib_req.params.emplace(urlDecodeTypeAdapter(key), urlDecodeTypeAdapter(value));

                start = (amp_pos != std::string::npos) ? amp_pos + 1 : query_string.length();
            } else {
                break;
            }
        }
    } else {
        httplib_req.path = target;
    }
    
    // Convert headers
    for (const auto& field : beast_req) {
        std::string name(field.name_string());
        std::string value(field.value());
        httplib_req.headers.emplace(name, value);
    }
    
    // Convert body
    httplib_req.body = beast_req.body();
    
    // Set HTTP version
    httplib_req.version = std::to_string(beast_req.version() / 10) + "." + 
                          std::to_string(beast_req.version() % 10);
    
    return httplib_req;
}

http::response<http::string_body> HttpTypeAdapter::httplibToBeast(
    const httplib::Response& httplib_res,
    unsigned version
) {
    http::response<http::string_body> beast_res;
    
    // Set HTTP version
    beast_res.version(version);
    
    // Set status code
    beast_res.result(intToStatus(httplib_res.status));
    
    // Convert headers
    for (const auto& [name, value] : httplib_res.headers) {
        beast_res.set(name, value);
    }
    
    // Convert body
    beast_res.body() = httplib_res.body;
    
    // Set Content-Length and other payload-related headers
    beast_res.prepare_payload();
    
    return beast_res;
}

std::string HttpTypeAdapter::methodToString(http::verb method) {
    switch (method) {
        case http::verb::get:     return "GET";
        case http::verb::post:    return "POST";
        case http::verb::put:     return "PUT";
        case http::verb::delete_: return "DELETE";
        case http::verb::patch:   return "PATCH";
        case http::verb::head:    return "HEAD";
        case http::verb::options: return "OPTIONS";
        case http::verb::trace:   return "TRACE";
        case http::verb::connect: return "CONNECT";
        default:                  return "UNKNOWN";
    }
}

http::status HttpTypeAdapter::intToStatus([[maybe_unused]] int status_code) {
    // Map common status codes
    switch (status_code) {
        // 2xx Success
        case 200: return http::status::ok;
        case 201: return http::status::created;
        case 202: return http::status::accepted;
        case 204: return http::status::no_content;
        
        // 3xx Redirection
        case 301: return http::status::moved_permanently;
        case 302: return http::status::found;
        case 304: return http::status::not_modified;
        
        // 4xx Client Error
        case 400: return http::status::bad_request;
        case 401: return http::status::unauthorized;
        case 403: return http::status::forbidden;
        case 404: return http::status::not_found;
        case 405: return http::status::method_not_allowed;
        case 409: return http::status::conflict;
        case 413: return http::status::payload_too_large;
        case 415: return http::status::unsupported_media_type;
        case 429: return http::status::too_many_requests;
        
        // 5xx Server Error
        case 500: return http::status::internal_server_error;
        case 501: return http::status::not_implemented;
        case 502: return http::status::bad_gateway;
        case 503: return http::status::service_unavailable;
        case 504: return http::status::gateway_timeout;
        
        // Default: return internal server error for unmapped codes
        default:  
            // Only use static_cast for valid HTTP status codes (100-599)
            constexpr int MIN_HTTP_STATUS = 100;
            constexpr int MAX_HTTP_STATUS = 600;
            if (status_code >= MIN_HTTP_STATUS && status_code < MAX_HTTP_STATUS) {
                return static_cast<http::status>(status_code);
            }
            return http::status::internal_server_error;
    }
}

} // namespace server
} // namespace themis
