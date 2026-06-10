/**
 * @file i_maintenance_task_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
