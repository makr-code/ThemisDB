/*
 * ThemisDB | File: plugin_registry.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 47
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4256 feat(plugins): upgrade Plug... (2026-03-15) | #1292 Plugin system production-re... (2026-03-11) | #625 Add generic type-safe plugi... (2026-03-11)
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
