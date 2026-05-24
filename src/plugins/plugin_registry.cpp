/*
 * ThemisDB | File: plugin_registry.cpp | Version: 0.0.47 | Last Modified: 2026-04-15 18:58:58
 * Author: ThemisDB Version Bot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 46
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=9 | delta=6 | status=divergent
 * External Severity (v3): C=1, H=7, M=1
 * PR: #4256 feat(plugins): upgrade PluginRegistry global mutex to shared_mutex ... (2026-03-15T15:57:39Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
