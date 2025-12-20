#ifndef THEMIS_MULTI_AGENT_API_HANDLER_H
#define THEMIS_MULTI_AGENT_API_HANDLER_H

#include <memory>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>

namespace rocksdb {
    class TransactionDB;
}

namespace themis {

namespace llm {
    class MultiAgentOrchestrator;
    class AgentRoleRegistry;
    class ConsensusBuilder;
    class LoRARegistry;
}

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

/**
 * @brief HTTP API Handler for Multi-Agent LLM Reasoning endpoints
 * 
 * Endpoints:
 * - POST /api/llm/multi-agent/analyze - General analysis with agents
 * - POST /api/llm/multi-agent/decompose - Decompose complex task
 * - POST /api/llm/multi-agent/orchestrate - Full orchestration workflow
 * - GET /api/llm/multi-agent/roles - List available agent roles
 * - GET /api/llm/multi-agent/lora - List LoRA adapters
 * - POST /api/llm/multi-agent/config - Apply configuration
 */
class MultiAgentAPIHandler {
public:
    /**
     * @brief Construct handler with database and multi-agent components
     */
    explicit MultiAgentAPIHandler(
        rocksdb::TransactionDB* db,
        std::shared_ptr<llm::MultiAgentOrchestrator> orchestrator,
        std::shared_ptr<llm::AgentRoleRegistry> role_registry,
        std::shared_ptr<llm::LoRARegistry> lora_registry
    );

    ~MultiAgentAPIHandler() = default;

    // API Endpoints
    http::response<http::string_body> handleAnalyze(
        const http::request<http::string_body>& req
    );

    http::response<http::string_body> handleDecompose(
        const http::request<http::string_body>& req
    );

    http::response<http::string_body> handleOrchestrate(
        const http::request<http::string_body>& req
    );

    http::response<http::string_body> handleListRoles(
        const http::request<http::string_body>& req
    );

    http::response<http::string_body> handleListLoRA(
        const http::request<http::string_body>& req
    );

    http::response<http::string_body> handleApplyConfig(
        const http::request<http::string_body>& req
    );

private:
    rocksdb::TransactionDB* db_;
    std::shared_ptr<llm::MultiAgentOrchestrator> orchestrator_;
    std::shared_ptr<llm::AgentRoleRegistry> role_registry_;
    std::shared_ptr<llm::LoRARegistry> lora_registry_;

    // Helper methods
    http::response<http::string_body> makeJsonResponse(
        const nlohmann::json& body,
        http::status status = http::status::ok
    );

    http::response<http::string_body> makeErrorResponse(
        const std::string& error,
        http::status status = http::status::bad_request
    );
};

} // namespace server
} // namespace themis

#endif // THEMIS_MULTI_AGENT_API_HANDLER_H
