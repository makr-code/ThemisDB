/*
 * ThemisDB | File: i_maintenance_task_handler.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    [[nodiscard]] virtual Result<std::string> execute(const std::string& job_id,
                                        MaintenanceTaskType task_type) = 0;

    /**
     * @brief Human-readable handler name for diagnostics.
     *
     * Returned by GET /api/v1/maintenance/task-handlers.
     * Should be unique and stable across restarts (e.g. "StorageCompactionHandler").
     */
    [[nodiscard]] virtual std::string handlerName() const = 0;
};

} // namespace maintenance
} // namespace themis
