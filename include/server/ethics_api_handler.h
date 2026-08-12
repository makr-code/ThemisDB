/**
 * @file ethics_api_handler.h
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
namespace query { class QueryEngine; }
using QueryEngine = query::QueryEngine;

namespace server {

/**
 * @brief Handler for Ethics AI Operations
 * 
 * This handler manages all ethics-related endpoints:
 * - POST /ethics/debate/init - Initialize ethical debate session
 * - POST /ethics/decision/make - Make ethical decision
 * - POST /ethics/evaluation - Evaluate decision quality
 * - GET /ethics/arguments - List arguments
 * - POST /ethics/arguments/search - Search arguments
 * - GET /ethics/philosophies - List philosophy schools
 * - POST /ethics/rag/context - Build RAG context
 * - GET /ethics/metrics - Get system metrics
 * 
 * Features:
 * - Multi-philosophy ethical decision-making
 * - RAG-enhanced context retrieval
 * - 5-dimension evaluation metrics
 * - Vector similarity search
 * - Graph traversal of argument chains
 * - Philosophy profile management
 * 
 * Follows the same pattern as TimeSeriesApiHandler, VectorApiHandler.
 */
class EthicsApiHandler {
public:
    /**
     * @brief Construct a new Ethics API Handler
     * 
     * @param storage Storage backend (RocksDB)
     * @param query_engine Query execution engine for AQL
     * @param auth Authentication/authorization middleware
     */
    EthicsApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<QueryEngine> query_engine,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Dispatch ethics API requests by path and method.
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const std::string& target);

    /**
     * @brief Handle POST /ethics/debate/init request
     * 
     * Request body:
     * {
     *   "dilemma_description": "Should AI make hiring decisions?",
     *   "philosophy_schools": ["kant", "utilitarianism"],
     *   "category": "employment"
     * }
     * 
     * Response:
     * {
     *   "debate_id": "debate_12345",
     *   "status": "initialized",
     *   "created_at": 1234567890
     * }
     */
    http::response<http::string_body> handleDebateInit(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /ethics/decision/make request
     * 
     * Request body:
     * {
     *   "dilemma_description": "Should autonomous vehicles prioritize passengers?",
     *   "philosophy_schools": ["kant", "utilitarianism", "virtue_ethics"],
     *   "category": "autonomous_systems",
     *   "use_rag": true
     * }
     * 
     * Response:
     * {
     *   "decision_id": "decision_12345",
     *   "decision_text": "...",
     *   "primary_philosophy": "kant",
     *   "confidence": 0.85,
     *   "consensus_level": 0.78
     * }
     */
    http::response<http::string_body> handleMakeDecision(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /ethics/evaluation request
     * 
     * Request body:
    * {
    *   "decision": { "...": "decision object" },
    *   "arguments": [ "optional argument objects" ]
    * }
     * 
     * Response:
     * {
     *   "overall_score": 0.82,
     *   "decision_quality_score": 0.85,
     *   "consistency_score": 0.88,
     *   "fairness_score": 0.79,
     *   "alignment_score": 0.81,
     *   "transparency_score": 0.77
     * }
     */
    http::response<http::string_body> handleEvaluation(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /ethics/arguments request
     * 
     * Query parameters:
     * - philosophy: Philosophy school (e.g., "kant")
     * - type: Argument type filter (e.g., "pro,contra")
     * - limit: Maximum results (default: 20)
     * 
     * Response:
     * [
     *   {
     *     "id": "arg_001",
     *     "philosophy_school": "kant",
     *     "argument_type": "pro",
     *     "content": "...",
     *     "strength": "strong"
     *   }
     * ]
     */
    http::response<http::string_body> handleGetArguments(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /ethics/arguments/search request
     * 
     * Request body:
     * {
     *   "query_text": "human dignity",
     *   "philosophy_school": "kant",  // optional filter
     *   "threshold": 0.65,
     *   "limit": 10
     * }
     * 
     * Response:
     * [
     *   {
     *     "id": "arg_001",
     *     "similarity": 0.89,
     *     "content": "...",
     *     "philosophy_school": "kant"
     *   }
     * ]
     */
    http::response<http::string_body> handleSearchArguments(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /ethics/philosophies request
     * 
     * Response:
     * [
     *   {
     *     "name": "kant",
     *     "full_name": "Kantian Ethics",
     *     "founder": "Immanuel Kant",
     *     "available": true
     *   },
     *   ...
     * ]
     */
    http::response<http::string_body> handleListPhilosophies(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /ethics/philosophies/:school request
     * 
     * Returns detailed philosophy profile for a specific school.
     * 
     * Response:
     * {
     *   "school": "kant",
     *   "name": "Kantian Ethics",
     *   "founder": "Immanuel Kant",
     *   "main_thesis": "...",
     *   "decision_framework": {...}
     * }
     */
    http::response<http::string_body> handleGetPhilosophy(
        const http::request<http::string_body>& req,
        const std::string& school);

    /**
     * @brief Handle POST /ethics/rag/context request
     * 
     * Request body:
     * {
     *   "dilemma_description": "AI privacy concerns",
     *   "philosophy_schools": ["kant", "utilitarianism"],
     *   "category": "data_ethics"
     * }
     * 
     * Response:
     * {
     *   "similar_dilemmas": [...],
     *   "philosophy_arguments": {...},
     *   "best_practices": [...],
     *   "recent_debates": [...],
     *   "consensus_decisions": [...]
     * }
     */
    http::response<http::string_body> handleBuildContext(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /ethics/metrics request
     * 
     * Query parameters:
     * - format: "json" or "prometheus" (default: json)
     * 
     * Response (JSON format):
     * {
     *   "decisions_total": 1234,
     *   "arguments_total": 5678,
     *   "active_debates": 5,
     *   "avg_confidence": 0.82
     * }
     * 
     * Response (Prometheus format):
     * # HELP ethics_decisions_total Total decisions
     * # TYPE ethics_decisions_total counter
     * ethics_decisions_total 1234
     * ...
     */
    http::response<http::string_body> handleGetMetrics(
        const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<QueryEngine> query_engine_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods
    http::response<http::string_body> makeErrorResponse(
        http::status status, 
        const std::string& message, 
        const http::request<http::string_body>& req);
        
    http::response<http::string_body> makeResponse(
        http::status status, 
        const std::string& body, 
        const http::request<http::string_body>& req);
        
    nlohmann::json executeAQL(
        const std::string& aql_query,
        const nlohmann::json& bind_vars = nlohmann::json::object());
        
    std::string extractQueryParam(
        const std::string& target,
        const std::string& param) const;
};

} // namespace server
} // namespace themis
