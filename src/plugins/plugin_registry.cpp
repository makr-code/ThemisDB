#include "plugins/plugin_registry.h"
#include <map>
#include <mutex>

namespace themis {
namespace plugins {

// Static storage for registries
PluginRegistry::Registry& PluginRegistry::getRegistry() {
    static Registry registry;
    return registry;
}

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
    getRegistry().clear();
    getTypeRegistries().clear();
}

} // namespace plugins
} // namespace themis
