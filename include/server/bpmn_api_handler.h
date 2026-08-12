/**
 * @file bpmn_api_handler.h
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
class ProcessGraphManager;

namespace server {

/**
 * @brief Handler for BPMN Process Operations
 * 
 * This handler manages all BPMN/process-related endpoints:
 * - POST /api/v1/bpmn/process/start - Start a new process instance
 * - POST /api/v1/bpmn/task/:taskId/complete - Complete a task
 * - GET /api/v1/bpmn/instance/:instanceId - Query process instance state
 * 
 * Features:
 * - Process instance management
 * - Task completion
 * - Process state queries
 * - Authorization checks
 */
class BpmnApiHandler {
public:
    /**
     * @brief Authentication context extracted from request
     */
    struct AuthContext {
        std::string user_id;
        std::vector<std::string> groups;
    };

    /**
     * @brief Construct a new BPMN API Handler
     * 
     * @param process_graph Process graph manager for BPMN operations
     * @param auth Authentication/authorization middleware
     */
    BpmnApiHandler(
        std::shared_ptr<ProcessGraphManager> process_graph,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /api/v1/bpmn/process/start request
     * 
     * Starts a new process instance.
     * 
     * Request body:
     * {
     *   "process_definition_key": "orderProcess",
     *   "variables": { "orderId": "123", "amount": 1000 },
     *   "business_key": "order-123"
     * }
     * 
     * Response:
     * {
     *   "process_instance_id": "inst-abc123",
     *   "status": 0,
     *   "status_string": "RUNNING",
     *   "active_task_ids": ["inst-abc123:userTask1"]
     * }
     * 
     * @param req HTTP request with process start data in body
     * @return HTTP response with process instance ID and state
     */
    http::response<http::string_body> handleStartProcess(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/bpmn/task/:taskId/complete request
     * 
     * Completes a user task and advances the process.
     * 
     * Request body:
     * {
     *   "variables": { "approved": true, "comment": "Looks good" }
     * }
     * 
     * Response:
     * {
     *   "success": true,
     *   "next_task_id": "inst-abc123:userTask2",
     *   "error": ""
     * }
     * 
     * @param req HTTP request with task completion data
     * @return HTTP response with completion status
     */
    http::response<http::string_body> handleTaskComplete(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/v1/bpmn/instance/:instanceId request
     * 
     * Queries the state of a process instance.
     * 
     * Query parameters:
     * - include_variables=true - Include process variables in response
     * - include_history=true - Include execution history in response
     * 
     * Response:
     * {
     *   "status": 0,
     *   "active_tasks": [...],
     *   "variables": {...},
     *   "history": [...],
     *   "start_time_ns": 1234567890000000,
     *   "end_time_ns": 0
     * }
     * 
     * @param req HTTP request
     * @return HTTP response with process instance state
     */
    http::response<http::string_body> handleQueryInstance(const http::request<http::string_body>& req);

private:
    std::shared_ptr<ProcessGraphManager> process_graph_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Authorization helpers
    AuthContext extractAuthContext(const http::request<http::string_body>& req) const;
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        const std::string& scope,
        const std::string& action,
        const std::string& resource
    );
};

} // namespace server
} // namespace themis
