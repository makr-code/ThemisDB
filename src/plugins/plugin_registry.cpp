/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_registry.cpp                                ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "plugins/plugin_registry.h"
#include <map>
#include <mutex>

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

std::mutex& PluginRegistry::getMutex() {
    static std::mutex mutex;
    return mutex;
}

void PluginRegistry::clearRegistry() {
    std::lock_guard<std::mutex> lock(getMutex());
    getTypeRegistries().clear();
}

} // namespace plugins
} // namespace themis
