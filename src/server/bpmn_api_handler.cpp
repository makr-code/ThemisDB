/**
 * @file bpmn_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/bpmn_api_handler.h"
#include "index/process_graph.h"
#include "server/auth_middleware.h"
#include "utils/input_validator.h"
#include "utils/logger.h"

#include <sstream>
#include <algorithm>
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

constexpr size_t kMaxBpmnIdentifierLength = 256;

bool isValidBpmnIdentifier(const std::string& value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(value, kMaxBpmnIdentifierLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

} // namespace

BpmnApiHandler::BpmnApiHandler(
    std::shared_ptr<ProcessGraphManager> process_graph,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : process_graph_(std::move(process_graph))
    , auth_(std::move(auth))
{
}

BpmnApiHandler::AuthContext BpmnApiHandler::extractAuthContext(
    const http::request<http::string_body>& req
) const {
    AuthContext ctx;
    
    // If auth is disabled, return empty context
    if (!auth_ || !auth_->isEnabled()) {
        return ctx;
    }
    
    // Extract Authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return ctx; // No token -> empty context
    }
    
    // Extract Bearer token
    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(), auth_header.size())
    );
    if (!token) {
        return ctx; // Invalid token format -> empty context
    }
    
    // Validate token and extract user_id + groups
    auto ar = auth_->validateToken(*token);
    if (ar.authorized) {
        ctx.user_id = ar.user_id;
        ctx.groups = ar.groups;
    }
    
    return ctx;
}

std::optional<http::response<http::string_body>> BpmnApiHandler::requireAccess(
    const http::request<http::string_body>& req,
    const std::string& scope,
    const std::string& /*action*/,
    const std::string& /*resource*/
) {
    // If auth is disabled, allow all
    if (!auth_ || !auth_->isEnabled()) {
        return std::nullopt;
    }

    // Extract Authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        // No Authorization header -> 401 Unauthorized
        return makeErrorResponse(http::status::unauthorized, "Authentication required", req);
    }

    // Extract Bearer token
    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(), auth_header.size())
    );
    if (!token) {
        // Malformed or missing Bearer token -> 401 Unauthorized
        return makeErrorResponse(http::status::unauthorized, "Invalid authorization header", req);
    }

    // GAP-001: Enforce scope-based authorization (CWE-862).
    // Previously, requireAccess() discarded the scope/action/resource parameters
    // and only verified token validity (authentication).  That left the BPMN API
    // open to any authenticated user regardless of their role.  Now we use
    // auth_->authorize() which checks that the token contains the required scope.
    auto ar = auth_->authorize(*token, scope);
    if (!ar.authorized) {
        THEMIS_WARN("[AUDIT] authorize result=DENY scope={}", scope);
        return makeErrorResponse(http::status::forbidden,
                                 "Insufficient permissions for scope: " + scope, req);
    }
    THEMIS_INFO("[AUDIT] authorize result=ALLOW scope={}", scope);

    return std::nullopt; // Access granted
}

std::string BpmnApiHandler::extractPathParam(const std::string& target, const std::string& prefix) {
    if (target.size() <= prefix.size()) {
        return "";
    }
    
    size_t start = prefix.size();
    size_t end = target.find('?', start);
    if (end == std::string::npos) {
        end = target.size();
    }
    
    return target.substr(start, end - start);
}

http::response<http::string_body> BpmnApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) {
    json error_json;
    error_json["error"] = message;
    error_json["status"] = static_cast<int>(status);
    
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB/1.0");
    res.keep_alive(req.keep_alive());
    res.body() = error_json.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> BpmnApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB/1.0");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> BpmnApiHandler::handleStartProcess(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleStartProcess");
    // Check authorization
    auto auth_err = requireAccess(req, "bpmn", "start", "process");
    if (auth_err) {
        return *auth_err;
    }
    
    if (!process_graph_) {
        return makeErrorResponse(http::status::service_unavailable, "Process engine not available", req);
    }
    auto& process_graph = *process_graph_;
    
    try {
        // Parse request body
        json request = json::parse(req.body());
        
        std::string process_key = request.value("process_definition_key", "");
        json variables = request.value("variables", json::object());
        std::string business_key = request.value("business_key", "");
        
        if (process_key.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing process_definition_key", req);
        }
        if (!isValidBpmnIdentifier(process_key)) {
            return makeErrorResponse(http::status::bad_request, "Invalid process_definition_key", req);
        }
        if (!business_key.empty() && !isValidBpmnIdentifier(business_key)) {
            return makeErrorResponse(http::status::bad_request, "Invalid business_key", req);
        }
        
        // Start process instance
        auto [status, instance_id] = process_graph.startProcess(process_key, variables);
        
        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error, 
                                    "Failed to start process: " + status.message, req);
        }
        
        // Get instance state to return active tasks
        auto [get_status, instance] = process_graph.getProcessInstance(instance_id);
        
        // Build response
        json response;
        response["process_instance_id"] = instance_id;
        
        // Map process state to ProcessStatus enum
        std::string state_str = "RUNNING";
        int status_code = 0; // RUNNING
        if (get_status.ok) {
            switch (instance.state) {
                case ProcessInstance::State::RUNNING:
                    state_str = "RUNNING";
                    status_code = 0;
                    break;
                case ProcessInstance::State::COMPLETED:
                    state_str = "COMPLETED";
                    status_code = 1;
                    break;
                case ProcessInstance::State::FAILED:
                    state_str = "FAILED";
                    status_code = 2;
                    break;
                case ProcessInstance::State::SUSPENDED:
                    state_str = "SUSPENDED";
                    status_code = 3;
                    break;
                case ProcessInstance::State::TERMINATED:
                    state_str = "TERMINATED";
                    status_code = 4;
                    break;
                default:
                    state_str = "RUNNING";
                    status_code = 0;
            }
            
            // Extract active task IDs from tokens
            json active_tasks = json::array();
            for (const auto& token : instance.tokens) {
                if (token.state == ProcessToken::State::READY || 
                    token.state == ProcessToken::State::ACTIVE) {
                    active_tasks.push_back(instance_id + ":" + token.current_node);
                }
            }
            response["active_task_ids"] = active_tasks;
        } else {
            response["active_task_ids"] = json::array();
        }
        
        response["status"] = status_code;
        response["status_string"] = state_str;
        
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, 
                               std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, 
                               std::string("Internal error: ") + e.what(), req);
    }
}

http::response<http::string_body> BpmnApiHandler::handleTaskComplete(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleTaskComplete");
    // Check authorization
    auto auth_err = requireAccess(req, "bpmn", "complete", "task");
    if (auth_err) {
        return *auth_err;
    }
    
    if (!process_graph_) {
        return makeErrorResponse(http::status::service_unavailable, "Process engine not available", req);
    }
    auto& process_graph = *process_graph_;
    
    try {
        // Extract task ID from path: /api/v1/bpmn/task/:taskId/complete
        std::string target = std::string(req.target());
        std::string prefix = "/api/v1/bpmn/task/";
        
        size_t start = prefix.size();
        size_t end = target.find("/complete", start);
        if (end == std::string::npos) {
            return makeErrorResponse(http::status::bad_request, "Invalid URL format", req);
        }
        
        std::string task_id = target.substr(start, end - start);
        if (task_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing task ID", req);
        }
        
        // Parse request body
        json request = json::parse(req.body());
        json variables = request.value("variables", json::object());
        
        // Task ID format: "instance_id:node_id"
        std::string instance_id;
        std::string node_id;
        
        size_t colon_pos = task_id.find(':');
        if (colon_pos != std::string::npos) {
            instance_id = task_id.substr(0, colon_pos);
            node_id = task_id.substr(colon_pos + 1);
        } else {
            return makeErrorResponse(http::status::bad_request, 
                                   "Invalid task_id format. Expected 'instance_id:node_id'", req);
        }
        if (!isValidBpmnIdentifier(instance_id) || !isValidBpmnIdentifier(node_id)) {
            return makeErrorResponse(http::status::bad_request,
                                   "Invalid task_id format. Expected safe 'instance_id:node_id'", req);
        }
        
        // Complete the task
        auto status = process_graph.completeTask(instance_id, node_id, variables);
        
        // Build response
        json response;
        response["success"] = status.ok;
        
        if (!status.ok) {
            response["error"] = status.message;
            response["next_task_id"] = "";
        } else {
            response["error"] = "";
            
            // Try to find next active task
            auto [get_status, instance] = process_graph.getProcessInstance(instance_id);
            if (get_status.ok && !instance.tokens.empty()) {
                // Find first active token
                for (const auto& token : instance.tokens) {
                    if (token.state == ProcessToken::State::READY || 
                        token.state == ProcessToken::State::ACTIVE) {
                        response["next_task_id"] = instance_id + ":" + token.current_node;
                        break;
                    }
                }
                if (!response.contains("next_task_id")) {
                    response["next_task_id"] = "";
                }
            } else {
                response["next_task_id"] = "";
            }
        }
        
        http::status http_status = status.ok ? http::status::ok : http::status::internal_server_error;
        return makeResponse(http_status, response.dump(), req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, 
                               std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, 
                               std::string("Internal error: ") + e.what(), req);
    }
}

http::response<http::string_body> BpmnApiHandler::handleQueryInstance(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleQueryInstance");
    // Check authorization
    auto auth_err = requireAccess(req, "bpmn", "read", "instance");
    if (auth_err) {
        return *auth_err;
    }
    
    if (!process_graph_) {
        return makeErrorResponse(http::status::service_unavailable, "Process engine not available", req);
    }
    auto& process_graph = *process_graph_;
    
    try {
        // Extract instance ID from path: /api/v1/bpmn/instance/:instanceId
        std::string target = std::string(req.target());
        std::string instance_id = extractPathParam(target, "/api/v1/bpmn/instance/");
        
        if (instance_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing instance ID", req);
        }
        if (!isValidBpmnIdentifier(instance_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid instance ID", req);
        }
        
        // Parse query parameters
        bool include_variables = true;
        bool include_history = false;
        
        size_t query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query = target.substr(query_pos + 1);
            if (query.find("include_variables=false") != std::string::npos) {
                include_variables = false;
            }
            if (query.find("include_history=true") != std::string::npos) {
                include_history = true;
            }
        }
        
        // Get process instance
        auto [status, instance] = process_graph.getProcessInstance(instance_id);
        
        if (!status.ok) {
            return makeErrorResponse(http::status::not_found, 
                                   "Process instance not found: " + status.message, req);
        }
        
        // Build response
        json response;
        
        // Map status
        int status_code = 0;
        switch (instance.state) {
            case ProcessInstance::State::RUNNING:
                status_code = 0;
                break;
            case ProcessInstance::State::COMPLETED:
                status_code = 1;
                break;
            case ProcessInstance::State::FAILED:
                status_code = 2;
                break;
            case ProcessInstance::State::SUSPENDED:
                status_code = 3;
                break;
            case ProcessInstance::State::TERMINATED:
                status_code = 4;
                break;
            default:
                status_code = 0;
        }
        response["status"] = status_code;
        
        // Active tasks
        json active_tasks = json::array();
        for (const auto& token : instance.tokens) {
            if (token.state == ProcessToken::State::READY || 
                token.state == ProcessToken::State::ACTIVE) {
                json task;
                task["task_id"] = instance_id + ":" + token.current_node;
                task["task_name"] = token.current_node;
                task["task_type"] = "userTask"; // Default, could be enhanced
                task["assignee"] = ""; // Not stored in token currently
                task["created_at_ns"] = token.created_at_ms * 1000000; // Convert ms to ns
                active_tasks.push_back(task);
            }
        }
        response["active_tasks"] = active_tasks;
        
        // Variables
        if (include_variables) {
            response["variables"] = instance.variables;
        } else {
            response["variables"] = json::object();
        }
        
        // History — use per-node visit timestamps when available.
        if (include_history) {
            json history = json::array();
            for (const auto& token : instance.tokens) {
                for (const auto& node : token.visited_nodes) {
                    json event;
                    event["event_type"] = "node_visited";
                    // Use the precise per-node visit timestamp if stored; fall back to
                    // the token creation time for nodes that predate timestamp tracking.
                    const auto tsIt = token.visit_timestamps.find(node);
                    int64_t ts_ms = token.created_at_ms;
                    if (tsIt != token.visit_timestamps.end()) {
                        ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    tsIt->second.time_since_epoch()).count();
                    }
                    event["timestamp_ns"] = ts_ms * 1'000'000;
                    event["data"] = json::object();
                    event["data"]["node_id"] = node;
                    history.push_back([[maybe_unused]] event);
                }
            }
            response["history"] = history;
        } else {
            response["history"] = json::array();
        }
        
        // Timestamps
        response["start_time_ns"] = instance.started_at_ms * 1000000; // Convert ms to ns
        if (instance.completed_at_ms.has_value()) {
            response["end_time_ns"] = instance.completed_at_ms.value() * 1000000;
        } else {
            response["end_time_ns"] = 0;
        }
        
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, 
                               std::string("Internal error: ") + e.what(), req);
    }
}

} // namespace server
} // namespace themis
