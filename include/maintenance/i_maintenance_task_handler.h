/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_maintenance_task_handler.h                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:45:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     75                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 717093f9bc  2026-03-12  feat: implement IMaintenanceTaskHandler registry for main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file i_maintenance_task_handler.h
 * @brief Interface for pluggable maintenance task execution handlers.
 *
 * Modules that own the real implementation of a delegated maintenance task
 * (e.g. StorageModule for STORAGE_COMPACTION) implement this interface and
 * register an instance with the DatabaseMaintenanceOrchestrator via
 * registerTaskHandler().  executeTask() invokes the registered handler
 * instead of immediately succeeding as a stub.
 */

#pragma once

#include "maintenance/maintenance_task.h"
#include "utils/expected.h"
#include <string>

namespace themis {
namespace maintenance {

/**
 * @brief Pluggable handler for a single maintenance task type.
 *
 * Implementations are expected to be thread-safe: the orchestrator may call
 * execute() from any thread.
 */
class IMaintenanceTaskHandler {
public:
    virtual ~IMaintenanceTaskHandler() = default;

    /**
     * @brief Execute the maintenance task.
     *
     * @param job_id    The orchestrator job identifier (for logging/tracing).
     * @param task_type The task type being executed.
     * @return Ok with a human-readable result summary string on success,
     *         or an Error on failure (the orchestrator will propagate the
     *         error message to the job's error_message field).
     */
    virtual Result<std::string> execute(const std::string& job_id,
                                        MaintenanceTaskType task_type) = 0;

    /**
     * @brief Human-readable handler name for diagnostics.
     *
     * Returned by GET /api/v1/maintenance/task-handlers.
     * Should be unique and stable across restarts (e.g. "StorageCompactionHandler").
     */
    virtual std::string handlerName() const = 0;
};

} // namespace maintenance
} // namespace themis
