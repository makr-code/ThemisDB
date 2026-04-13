/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_registry.cpp                                ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:27:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     62                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ae40fc781e  2026-03-15  feat(plugins): upgrade PluginRegistry global mutex to sha... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c3452af1e6  2026-02-26  feat(plugins): implement SignedPluginRepository with Ed25... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "plugins/plugin_registry.h"
#include <map>
#include <mutex>
#include <shared_mutex>

namespace themis {
namespace plugins {

// Static storage for type-specific registries only
PluginRegistry::TypeRegistries& PluginRegistry::getTypeRegistries() {
    static TypeRegistries type_registries;
    return type_registries;
}

PluginRegistry::Registry& PluginRegistry::getTypeRegistry(const std::type_info& type) {
    auto& type_registries = getTypeRegistries();
    size_t type_hash = type.hash_code();
    
    // Create entry if doesn't exist
    if (type_registries.find(type_hash) == type_registries.end()) {
        type_registries[type_hash] = Registry();
    }
    
    return type_registries[type_hash];
}

std::shared_mutex& PluginRegistry::getMutex() {
    static std::shared_mutex mutex;
    return mutex;
}

void PluginRegistry::clearRegistry() {
    std::unique_lock<std::shared_mutex> lock(getMutex());
    getTypeRegistries().clear();
}

} // namespace plugins
} // namespace themis
