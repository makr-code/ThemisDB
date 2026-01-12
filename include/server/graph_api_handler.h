#pragma once

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class GraphIndexManager;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class AuthMiddleware;

/**
 * @brief Handler for Graph Operations
 * 
 * This handler manages all graph-related endpoints:
 * - POST /graph/traverse - Execute graph traversal query
 * - POST /graph/edge - Create a graph edge
 * - DELETE /graph/edge/:id - Delete a graph edge
 * 
 * Features:
 * - Graph traversal algorithms (BFS, DFS, shortest path)
 * - Edge creation and deletion
 * - Property graph support
 * - Path finding and pattern matching
 * 
 * Extracted from http_server.cpp (~150 lines) to improve maintainability.
 */
class GraphApiHandler {
public:
    /**
     * @brief Construct a new Graph API Handler
     * 
     * @param storage Storage backend
     * @param graph_index Graph index manager
     * @param auth Authentication/authorization middleware
     */
    GraphApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /graph/traverse request
     * @param req HTTP request with traversal specification
     * @return HTTP response with traversal results
     */
    http::response<http::string_body> handleTraverse(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /graph/edge request
     * @param req HTTP request with edge data
     * @return HTTP response with creation status
     */
    http::response<http::string_body> handleEdgeCreate(const http::request<http::string_body>& req);

    /**
     * @brief Handle DELETE /graph/edge/:id request
     * @param req HTTP request
     * @return HTTP response with deletion status
     */
    http::response<http::string_body> handleEdgeDelete(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
