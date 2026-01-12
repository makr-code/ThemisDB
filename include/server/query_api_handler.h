#pragma once

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class QueryEngine;
class QueryOptimizer;
class LLMInteractionStore;
class PromptManager;
class SemanticCache;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class AuthMiddleware;

/**
 * @brief Handler for Query Operations
 * 
 * This handler manages all query-related endpoints:
 * - POST /query - Execute a structured query
 * - POST /query/aql - Execute an AQL (Advanced Query Language) query
 * - POST /query/enhanced - Execute query with LLM context enhancement (Enterprise feature)
 * 
 * Features:
 * - Query validation and optimization
 * - AQL parsing and translation
 * - Semantic caching support
 * - LLM-enhanced queries (optional)
 * - Result set pagination
 * - Query statistics and profiling
 * 
 * Extracted from http_server.cpp (~850 lines) to improve maintainability.
 */
class QueryApiHandler {
public:
    /**
     * @brief Construct a new Query API Handler
     * 
     * @param storage Storage backend
     * @param secondary_index Secondary index manager
     * @param query_engine Query execution engine
     * @param query_optimizer Query optimizer
     * @param semantic_cache Semantic cache for query results
     * @param llm_store LLM interaction store (optional)
     * @param prompt_manager Prompt template manager (optional)
     * @param auth Authentication/authorization middleware
     */
    QueryApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<QueryEngine> query_engine,
        std::shared_ptr<QueryOptimizer> query_optimizer,
        std::shared_ptr<SemanticCache> semantic_cache,
        std::shared_ptr<LLMInteractionStore> llm_store,
        std::shared_ptr<PromptManager> prompt_manager,
        std::shared_ptr<AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /query request
     * 
     * Executes a structured query with filters, projections, and sorting.
     * 
     * @param req HTTP request with query specification
     * @return HTTP response with query results
     */
    http::response<http::string_body> handleQuery(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /query/aql request
     * 
     * Executes an AQL query string. Supports complex joins, aggregations, and sub-queries.
     * 
     * @param req HTTP request with AQL query string
     * @return HTTP response with query results
     */
    http::response<http::string_body> handleQueryAql(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /query/enhanced request (Enterprise)
     * 
     * Executes a query with LLM context enhancement for improved results.
     * 
     * @param req HTTP request with query and LLM context
     * @return HTTP response with enhanced query results
     */
    http::response<http::string_body> handleQueryEnhanced(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<QueryEngine> query_engine_;
    std::shared_ptr<QueryOptimizer> query_optimizer_;
    std::shared_ptr<SemanticCache> semantic_cache_;
    std::shared_ptr<LLMInteractionStore> llm_store_;
    std::shared_ptr<PromptManager> prompt_manager_;
    std::shared_ptr<AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
