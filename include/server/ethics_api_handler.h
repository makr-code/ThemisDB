#pragma once

#include "server/auth_middleware.h"
#include "plugins/ethics_ai/ethics_ai_plugin_interface.h"
#include "plugins/ethics_ai/ethics_storage_integration.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

using namespace themis::plugins::ethics;

/**
 * @brief Handler for Ethics AI Operations
 * 
 * This handler manages all ethics AI-related endpoints:
 * - POST /ethics/debate/init - Initialize ethical debate
 * - POST /ethics/decision/make - Make ethical decision
 * - POST /ethics/argument/store - Store ethical argument
 * - GET /ethics/argument/:id - Get argument by ID
 * - POST /ethics/argument/search - Search arguments
 * - POST /ethics/evaluation - Evaluate decision
 * - GET /ethics/philosophy/list - List philosophy profiles
 * - GET /ethics/philosophy/:school - Get philosophy profile
 * - POST /ethics/rag/context - Build RAG context
 * - GET /ethics/metrics - Get ethics metrics
 * - GET /ethics/dashboard - Get dashboard data
 * 
 * Features:
 * - Multi-philosophy ethical debate orchestration
 * - RAG-based context retrieval
 * - 5-dimension evaluation system
 * - Graph/Relational/Vector storage integration
 * - Prometheus metrics export
 * 
 * Integration similar to VectorApiHandler and GraphApiHandler.
 */
class EthicsApiHandler {
public:
    /**
     * @brief Construct a new Ethics API Handler
     * 
     * @param ethics_plugin Ethics AI Plugin instance
     * @param storage_manager Storage manager for multi-model access
     * @param auth Authentication/authorization middleware
     */
    EthicsApiHandler(
        std::shared_ptr<IEthicsAIPlugin> ethics_plugin,
        std::shared_ptr<EthicsStorageManager> storage_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    // ========== Debate Endpoints ==========
    
    /**
     * @brief Handle POST /ethics/debate/init
     * 
     * Request body:
     * {
     *   "dilemma_description": "string",
     *   "philosophy_schools": ["kant", "utilitarianism"],
     *   "category": "autonomous_systems"
     * }
     * 
     * Response:
     * {
     *   "debate_id": "string",
     *   "status": "initialized",
     *   "participants": ["kant", "utilitarianism"]
     * }
     */
    http::response<http::string_body> handleDebateInit(
        const http::request<http::string_body>& req
    );

    // ========== Decision Endpoints ==========
    
    /**
     * @brief Handle POST /ethics/decision/make
     * 
     * Request body:
     * {
     *   "dilemma_description": "string",
     *   "philosophy_schools": ["kant", "utilitarianism"],
     *   "category": "data_ethics",
     *   "use_rag": true
     * }
     * 
     * Response:
     * {
     *   "decision_id": "string",
     *   "decision_text": "string",
     *   "primary_philosophy": "kant",
     *   "supporting_philosophies": ["kant", "utilitarianism"],
     *   "confidence": 0.85,
     *   "consensus_level": 0.75
     * }
     */
    http::response<http::string_body> handleMakeDecision(
        const http::request<http::string_body>& req
    );

    // ========== Argument Endpoints ==========
    
    /**
     * @brief Handle POST /ethics/argument/store
     * 
     * Request body:
     * {
     *   "id": "arg_001",
     *   "philosophy_school": "kant",
     *   "argument_type": "pro",
     *   "content": "string",
     *   "principle_basis": ["categorical_imperative"],
     *   "strength": "strong"
     * }
     */
    http::response<http::string_body> handleStoreArgument(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle GET /ethics/argument/:id
     * 
     * Response:
     * {
     *   "id": "arg_001",
     *   "philosophy_school": "kant",
     *   "argument_type": "pro",
     *   "content": "string",
     *   ...
     * }
     */
    http::response<http::string_body> handleGetArgument(
        const http::request<http::string_body>& req,
        const std::string& argument_id
    );

    /**
     * @brief Handle POST /ethics/argument/search
     * 
     * Request body:
     * {
     *   "philosophy_school": "kant",  // optional
     *   "argument_types": ["pro", "contra"],  // optional
     *   "query_text": "string",  // for semantic search
     *   "limit": 20
     * }
     * 
     * Response:
     * {
     *   "arguments": [{...}, {...}],
     *   "total": 2
     * }
     */
    http::response<http::string_body> handleSearchArguments(
        const http::request<http::string_body>& req
    );

    // ========== Evaluation Endpoints ==========
    
    /**
     * @brief Handle POST /ethics/evaluation
     * 
     * Request body:
     * {
     *   "decision_id": "dec_001",
     *   "argument_ids": ["arg_1", "arg_2"]  // optional
     * }
     * 
     * Response:
     * {
     *   "overall_score": 0.82,
     *   "decision_quality_score": 0.85,
     *   "consistency_score": 0.80,
     *   "fairness_score": 0.78,
     *   "alignment_score": 0.84,
     *   "transparency_score": 0.83,
     *   "detailed_metrics": {...}
     * }
     */
    http::response<http::string_body> handleEvaluateDecision(
        const http::request<http::string_body>& req
    );

    // ========== Philosophy Endpoints ==========
    
    /**
     * @brief Handle GET /ethics/philosophy/list
     * 
     * Response:
     * {
     *   "schools": [
     *     {"id": "kant", "name": "Kantian Ethics", ...},
     *     {"id": "utilitarianism", "name": "Utilitarian Ethics", ...}
     *   ]
     * }
     */
    http::response<http::string_body> handleListPhilosophies(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle GET /ethics/philosophy/:school
     * 
     * Response:
     * {
     *   "school_id": "kant",
     *   "name": "Kantian Ethics",
     *   "main_theses": [...],
     *   "decision_framework": {...},
     *   ...
     * }
     */
    http::response<http::string_body> handleGetPhilosophy(
        const http::request<http::string_body>& req,
        const std::string& school_id
    );

    // ========== RAG Endpoints ==========
    
    /**
     * @brief Handle POST /ethics/rag/context
     * 
     * Request body:
     * {
     *   "dilemma_description": "string",
     *   "philosophy_schools": ["kant", "utilitarianism"],
     *   "category": "general"
     * }
     * 
     * Response:
     * {
     *   "similar_dilemmas": ["id1", "id2"],
     *   "philosophy_arguments": {...},
     *   "best_practices": [...],
     *   ...
     * }
     */
    http::response<http::string_body> handleBuildRAGContext(
        const http::request<http::string_body>& req
    );

    // ========== Metrics Endpoints ==========
    
    /**
     * @brief Handle GET /ethics/metrics
     * 
     * Returns Prometheus-formatted metrics
     */
    http::response<http::string_body> handleGetMetrics(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle GET /ethics/dashboard
     * 
     * Returns JSON dashboard data
     */
    http::response<http::string_body> handleGetDashboard(
        const http::request<http::string_body>& req
    );

    // ========== Graph Integration Endpoints ==========
    
    /**
     * @brief Handle POST /ethics/graph/traverse
     * 
     * Traverse argument chains using graph algorithms
     * 
     * Request body:
     * {
     *   "start_id": "arg_001",
     *   "max_depth": 5,
     *   "direction": "both",
     *   "algorithm": "BFS"
     * }
     */
    http::response<http::string_body> handleGraphTraverse(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle GET /ethics/graph/influence
     * 
     * Get argument influence scores (PageRank)
     * 
     * Response:
     * {
     *   "influence_scores": {
     *     "arg_001": 0.25,
     *     "arg_002": 0.18,
     *     ...
     *   }
     * }
     */
    http::response<http::string_body> handleGetInfluence(
        const http::request<http::string_body>& req
    );

private:
    std::shared_ptr<IEthicsAIPlugin> ethics_plugin_;
    std::shared_ptr<EthicsStorageManager> storage_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods
    http::response<http::string_body> createResponse(
        http::status status,
        const std::string& body,
        const std::string& content_type = "application/json"
    );

    http::response<http::string_body> createErrorResponse(
        http::status status,
        const std::string& error_message
    );

    bool validateAuthentication(const http::request<http::string_body>& req);
    
    nlohmann::json argumentToJson(const EthicalArgument& argument);
    nlohmann::json decisionToJson(const EthicalDecision& decision);
    nlohmann::json evaluationToJson(const EthicsEvaluationResult& evaluation);
    nlohmann::json philosophyProfileToJson(const PhilosophyProfile& profile);
};

} // namespace server
} // namespace themis
