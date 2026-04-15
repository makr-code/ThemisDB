/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_api_handler.h                                ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     233                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once
#include "server/auth_middleware.h"
#include "security/query_masking_policy.h"

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
     * @brief Handle GET /v2/query/stream request (SSE streaming)
     *
     * Executes an AQL query and streams each result row as a Server-Sent Event.
     * The AQL query string is passed via the `q` URL query parameter.
     *
     * Optional parameters:
     *   - max_seconds (int, 1-60): maximum stream duration; default 30
     *   - heartbeat_ms (int, 100-60000): heartbeat comment interval; default 15000
     *   - retry_ms (int, 100-120000): SSE reconnect hint; default 3000
     *
     * @param req HTTP request
     * @return HTTP response with Content-Type: text/event-stream
     */
    http::response<http::string_body> handleQueryStreamSse(const http::request<http::string_body>& req);

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

    /**
     * @brief Inject a QueryMaskingPolicy for dynamic PII masking of query results.
     *
     * When set, entity results are passed through the masking policy before
     * being serialised in the response.  The caller owns the policy lifetime.
     * Pass nullptr to disable masking.
     */
    void setQueryMaskingPolicy(
        std::shared_ptr<security::QueryMaskingPolicy> policy) noexcept {
        masking_policy_ = std::move(policy);
    }

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
    std::shared_ptr<security::QueryMaskingPolicy> masking_policy_;  ///< Optional PII masking

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

    // Apply QueryMaskingPolicy to a JSON entities array if policy is configured.
    nlohmann::json applyMasking(
        const nlohmann::json& entities,
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
