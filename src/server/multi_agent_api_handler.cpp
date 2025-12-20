#include "server/multi_agent_api_handler.h"
#include "llm/multi_agent_orchestrator.h"
#include "llm/agent_role_registry.h"
#include "llm/lora_registry.h"
#include "llm/llm_agent.h"
#include "llm/consensus_builder.h"
#include <rocksdb/db.h>

namespace themis {
namespace server {

using json = nlohmann::json;

// Constructor
MultiAgentAPIHandler::MultiAgentAPIHandler(
    rocksdb::TransactionDB* db,
    std::shared_ptr<llm::MultiAgentOrchestrator> orchestrator,
    std::shared_ptr<llm::AgentRoleRegistry> role_registry,
    std::shared_ptr<llm::LoRARegistry> lora_registry
) : db_(db),
    orchestrator_(orchestrator),
    role_registry_(role_registry),
    lora_registry_(lora_registry) {
}

// POST /api/llm/multi-agent/analyze
http::response<http::string_body> MultiAgentAPIHandler::handleAnalyze(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        
        if (!body.contains("input")) {
            return makeErrorResponse("Missing 'input' field");
        }
        
        std::string input = body["input"].get<std::string>();
        
        // Decompose the problem
        auto tasks = orchestrator_->decomposeProblem(input);
        
        // Process tasks
        auto result = orchestrator_->processTasks(tasks);
        
        // Return result
        return makeJsonResponse(result.toJson());
        
    } catch (const json::exception& e) {
        return makeErrorResponse(std::string("JSON parsing error: ") + e.what());
    } catch (const std::exception& e) {
        return makeErrorResponse(std::string("Error: ") + e.what());
    }
}

// POST /api/llm/multi-agent/decompose
http::response<http::string_body> MultiAgentAPIHandler::handleDecompose(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        
        if (!body.contains("prompt")) {
            return makeErrorResponse("Missing 'prompt' field");
        }
        
        std::string prompt = body["prompt"].get<std::string>();
        
        // Decompose the problem
        auto tasks = orchestrator_->decomposeProblem(prompt);
        
        // Convert tasks to JSON
        json tasks_json = json::array();
        for (const auto& task : tasks) {
            tasks_json.push_back(task.toJson());
        }
        
        json response = {
            {"tasks", tasks_json},
            {"count", tasks.size()}
        };
        
        return makeJsonResponse(response);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(std::string("JSON parsing error: ") + e.what());
    } catch (const std::exception& e) {
        return makeErrorResponse(std::string("Error: ") + e.what());
    }
}

// POST /api/llm/multi-agent/orchestrate
http::response<http::string_body> MultiAgentAPIHandler::handleOrchestrate(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        
        if (!body.contains("tasks")) {
            return makeErrorResponse("Missing 'tasks' field");
        }
        
        // Parse tasks
        std::vector<llm::MultiAgentOrchestrator::Task> tasks;
        for (const auto& task_json : body["tasks"]) {
            tasks.push_back(llm::MultiAgentOrchestrator::Task::fromJson(task_json));
        }
        
        // Process tasks
        auto result = orchestrator_->processTasks(tasks);
        
        return makeJsonResponse(result.toJson());
        
    } catch (const json::exception& e) {
        return makeErrorResponse(std::string("JSON parsing error: ") + e.what());
    } catch (const std::exception& e) {
        return makeErrorResponse(std::string("Error: ") + e.what());
    }
}

// GET /api/llm/multi-agent/roles
http::response<http::string_body> MultiAgentAPIHandler::handleListRoles(
    const http::request<http::string_body>& req
) {
    try {
        auto role_ids = role_registry_->listAllRoles();
        
        json roles_json = json::array();
        for (const auto& role_id : role_ids) {
            auto role_opt = role_registry_->getRole(role_id);
            if (role_opt) {
                roles_json.push_back(role_opt->toJson());
            }
        }
        
        json response = {
            {"roles", roles_json},
            {"count", roles_json.size()}
        };
        
        return makeJsonResponse(response);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(std::string("Error: ") + e.what());
    }
}

// GET /api/llm/multi-agent/lora
http::response<http::string_body> MultiAgentAPIHandler::handleListLoRA(
    const http::request<http::string_body>& req
) {
    try {
        auto adapters = lora_registry_->listAdapters();
        
        json adapters_json = json::array();
        for (const auto& adapter : adapters) {
            adapters_json.push_back(adapter.toJson());
        }
        
        auto stats = lora_registry_->getStats();
        
        json response = {
            {"adapters", adapters_json},
            {"total_adapters", stats.total_adapters},
            {"loaded_adapters", stats.loaded_adapters},
            {"total_memory_bytes", stats.total_memory_bytes}
        };
        
        return makeJsonResponse(response);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(std::string("Error: ") + e.what());
    }
}

// POST /api/llm/multi-agent/config
http::response<http::string_body> MultiAgentAPIHandler::handleApplyConfig(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        
        if (!body.contains("config_name")) {
            return makeErrorResponse("Missing 'config_name' field");
        }
        
        std::string config_name = body["config_name"].get<std::string>();
        
        // TODO: Load and apply configuration from file
        // For now, return stub response
        
        json response = {
            {"status", "success"},
            {"message", "Configuration application is a stub in v1.4.0"},
            {"config_name", config_name},
            {"note", "Full configuration loading will be implemented in v1.5.0"}
        };
        
        return makeJsonResponse(response);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(std::string("JSON parsing error: ") + e.what());
    } catch (const std::exception& e) {
        return makeErrorResponse(std::string("Error: ") + e.what());
    }
}

// Helper methods
http::response<http::string_body> MultiAgentAPIHandler::makeJsonResponse(
    const json& body,
    http::status status
) {
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> MultiAgentAPIHandler::makeErrorResponse(
    const std::string& error,
    http::status status
) {
    json error_json = {
        {"error", error}
    };
    return makeJsonResponse(error_json, status);
}

} // namespace server
} // namespace themis
