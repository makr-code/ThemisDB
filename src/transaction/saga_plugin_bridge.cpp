#include "transaction/saga_plugin_bridge.h"

namespace themis::transaction {

Result<SAGAOrchestrator*> bindSagaOrchestratorFromPlugin(
    plugins::PluginManager& plugin_manager,
    const std::string& plugin_name) {
    if (!plugin_manager.isPluginLoaded(plugin_name)) {
        auto load_result = plugin_manager.loadPlugin(plugin_name);
        if (!load_result) {
            return tl::unexpected(load_result.error());
        }
    }

    auto plugin_result = plugin_manager.getPlugin(plugin_name);
    if (!plugin_result) {
        return tl::unexpected(plugin_result.error());
    }

    auto* plugin = plugin_result.value();
    if (plugin == nullptr) {
        return Err<SAGAOrchestrator*>(
            errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "Plugin instance is null: " + plugin_name);
    }

    auto* orchestrator = static_cast<SAGAOrchestrator*>(plugin->getInstance());
    if (orchestrator == nullptr) {
        return Err<SAGAOrchestrator*>(
            errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
            "Plugin does not expose a SAGAOrchestrator instance: " + plugin_name);
    }

    return orchestrator;
}

} // namespace themis::transaction
