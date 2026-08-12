/**
 * @file policy_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
class PolicyEngine;

namespace server {
class RangerClient;

/**
 * @brief Handler for Policy Management Operations
 * 
 * This handler manages all policy-related endpoints:
 * - POST /policies/import/ranger - Import policies from Apache Ranger
 * - GET /policies/export/ranger - Export policies to Apache Ranger format
 * 
 * Features:
 * - Apache Ranger integration
 * - Policy import/export
 * - Access control policy management
 * - Row-level security policies
 * 
 * Extracted from http_server.cpp (~200 lines) to improve maintainability.
 */
class PolicyApiHandler {
public:
    /**
     * @brief Construct a new Policy API Handler
     * 
     * @param storage Storage backend
     * @param ranger_client Apache Ranger client
     * @param policy_engine Policy engine for policy management
     * @param auth Authentication/authorization middleware
     * @param service_name Service name for Ranger exports (default: "themisdb")
     */
    PolicyApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        RangerClient* ranger_client,
        PolicyEngine* policy_engine,
        std::shared_ptr<themis::AuthMiddleware> auth,
        const std::string& service_name = "themisdb"
    );

    /**
     * @brief Handle POST /policies/import/ranger request
     * @param req HTTP request with Ranger policies to import
     * @return HTTP response with import status
     */
    http::response<http::string_body> handleImportRanger(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /policies/export/ranger request
     * @param req HTTP request
     * @return HTTP response with policies in Ranger format
     */
    http::response<http::string_body> handleExportRanger(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    RangerClient* ranger_client_;
    PolicyEngine* policy_engine_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    std::string service_name_;

    // Helper methods
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
