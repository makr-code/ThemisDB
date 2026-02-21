/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_api_handler.h                                ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:35:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     194                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ad82b76f1  2026-02-21  feat(metadata): production-ready metadata module – statis... ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
    • 8bf786ff9  2026-02-10  Implement autonomous prompt engineering system with domai... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <optional>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class GraphIndexManager;
class LLMInteractionStore;
class SemanticCache;
class FieldEncryption;
class KeyProvider;
class AuthMiddleware;
class IndexRecommender;
class StatisticsCollector;

namespace prompt_engineering {
class PromptManager;
}

namespace security {
class PKIKeyProvider;
}

// Config flags forwarded from HttpServer; use booleans to avoid circular include

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

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
     * @param graph_index Graph index manager
     * @param field_encryption Field encryption handler
     * @param key_provider Key provider for encryption
     * @param semantic_cache Semantic cache for query results
     * @param llm_store LLM interaction store (optional)
     * @param prompt_manager Prompt template manager (optional)
     * @param auth Authentication/authorization middleware
     * @param config Server configuration
     */
    QueryApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<FieldEncryption> field_encryption,
        std::shared_ptr<KeyProvider> key_provider,
        std::shared_ptr<SemanticCache> semantic_cache,
        std::shared_ptr<LLMInteractionStore> llm_store,
        std::shared_ptr<themis::prompt_engineering::PromptManager> prompt_manager,
        std::shared_ptr<::themis::AuthMiddleware> auth,
        bool feature_llm_query_enhancement,
        bool feature_llm_store
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

    /**
     * @brief Inject a StatisticsCollector for cardinality-based predicate ordering.
     *
     * When set, every local QueryEngine created by this handler receives the
     * collector so that equality predicates are sorted by selectivity before
     * execution.  The pointer is non-owning; the caller manages lifetime.
     */
    void setStatisticsCollector(StatisticsCollector* sc) noexcept { stats_collector_ = sc; }

    /**
     * @brief Inject an IndexRecommender for access-pattern recording.
     *
     * When set, every successful AQL translation records the accessed columns
     * in the recommender so that `GET /api/v1/metadata/index_recommendations`
     * can suggest beneficial indexes.
     *
     * The pointer is non-owning; the caller manages the lifetime.
     * Pass nullptr to disable recording.
     */
    void setIndexRecommender(IndexRecommender* rec) noexcept { index_recommender_ = rec; }

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<KeyProvider> key_provider_;
    std::shared_ptr<SemanticCache> semantic_cache_;
    std::shared_ptr<LLMInteractionStore> llm_store_;
    std::shared_ptr<themis::prompt_engineering::PromptManager> prompt_manager_;
    std::shared_ptr<::themis::AuthMiddleware> auth_;
    bool feature_llm_query_enhancement_{false};
    bool feature_llm_store_{false};
    IndexRecommender*   index_recommender_{nullptr};   ///< Optional; non-owning
    StatisticsCollector* stats_collector_{nullptr};    ///< Optional; non-owning

    // Helper methods
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Authorization helper
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& permission,
        const std::string& resource_type,
        const std::string& resource_id);
    
    // Auth context extraction
    struct AuthContext {
        std::string user_id;
        std::vector<std::string> groups;
    };
    AuthContext extractAuthContext(const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
