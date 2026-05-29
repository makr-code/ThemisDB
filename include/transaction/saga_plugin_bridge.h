#pragma once

#include "plugins/plugin_manager.h"
#include "transaction/saga_orchestrator.h"

#include <string>

namespace themis::transaction {

/**
 * @brief Load (if necessary) and bind the SAGA orchestrator from a plugin.
 *
 * This helper wraps PluginManager interaction so callers can retrieve a typed
 * `SAGAOrchestrator*` from a dynamically loaded plugin DLL.
 *
 * @param plugin_manager Plugin manager used for discovery/load.
 * @param plugin_name Plugin manifest name (default: "saga_orchestrator").
 * @return Result with typed orchestrator pointer or an error when loading/
 *         binding fails.
 */
Result<SAGAOrchestrator*> bindSagaOrchestratorFromPlugin(
    plugins::PluginManager& plugin_manager,
    const std::string& plugin_name = "saga_orchestrator");

} // namespace themis::transaction
