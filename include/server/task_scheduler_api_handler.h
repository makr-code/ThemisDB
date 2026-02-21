/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            task_scheduler_api_handler.h                       ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     108                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file task_scheduler_api_handler.h
 * @brief HTTP API handler for task scheduler operations
 * 
 * ⚠️ SECURITY CRITICAL: This API exposes task scheduling functionality.
 * ALL endpoints MUST be protected by:
 * - Strong authentication (API keys, JWT, mutual TLS)
 * - Authorization (RBAC - only admins can manage tasks)
 * - Rate limiting (prevent API abuse)
 * - Input validation (sanitize all inputs)
 * - Audit logging (log all operations)
 * - HTTPS only (no plain HTTP)
 */

#ifndef THEMIS_TASK_SCHEDULER_API_HANDLER_H
#define THEMIS_TASK_SCHEDULER_API_HANDLER_H

#include "scheduler/task_scheduler.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace themis {
namespace server {

/**
 * @brief API handler for task scheduler HTTP endpoints
 * 
 * ⚠️ SECURITY WARNING: These endpoints allow task management and execution.
 * 
 * Provides RESTful API for:
 * - POST /api/tasks - Register a new task [AUTH REQUIRED]
 * - GET /api/tasks - List all tasks [AUTH REQUIRED]
 * - GET /api/tasks/:id - Get task details [AUTH REQUIRED]
 * - PUT /api/tasks/:id - Update a task [AUTH REQUIRED]
 * - DELETE /api/tasks/:id - Unregister a task [AUTH REQUIRED]
 * - POST /api/tasks/:id/enable - Enable a task [AUTH REQUIRED]
 * - POST /api/tasks/:id/disable - Disable a task [AUTH REQUIRED]
 * - POST /api/tasks/:id/execute - Execute task immediately [AUTH REQUIRED]
 * - GET /api/tasks/stats - Get scheduler statistics [AUTH REQUIRED]
 * 
 * SECURITY REQUIREMENTS:
 * - All endpoints must verify user authentication
 * - All endpoints must check user authorization (admin role)
 * - All operations must be audit logged
 * - Input validation on all parameters
 * - Rate limiting on execute endpoint
 */
class TaskSchedulerApiHandler {
public:
    explicit TaskSchedulerApiHandler(TaskScheduler* scheduler)
        : scheduler_(scheduler) {}
    
    // Task registration and management
    nlohmann::json registerTask(const nlohmann::json& request);
    nlohmann::json listTasks();
    nlohmann::json getTask(const std::string& task_id);
    nlohmann::json updateTask(const std::string& task_id, const nlohmann::json& request);
    nlohmann::json unregisterTask(const std::string& task_id);
    
    // Task control
    nlohmann::json enableTask(const std::string& task_id);
    nlohmann::json disableTask(const std::string& task_id);
    nlohmann::json executeTask(const std::string& task_id);
    
    // Statistics
    nlohmann::json getStats();
    
private:
    TaskScheduler* scheduler_;
    
    // Helper to convert ScheduledTask to JSON
    nlohmann::json taskToJson(const ScheduledTask& task);
    
    // Helper to parse task from JSON request
    ScheduledTask parseTaskFromJson(const nlohmann::json& json);
};

} // namespace server
} // namespace themis

#endif // THEMIS_TASK_SCHEDULER_API_HANDLER_H
