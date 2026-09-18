/**
 * @file plugin_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/plugin_registry.h"
#include <map>
#include <mutex>
#include <shared_mutex>

namespace themis {
namespace plugins {

/**
 * @brief Static storage for type-specific registries only
 * @return Return value.
 * @details Implements getTypeRegistries without additional internal calls.
 */
PluginRegistry::TypeRegistries& PluginRegistry::getTypeRegistries() {
    static TypeRegistries type_registries;
    return type_registries;
}

/**
 * @brief Get Type Registry.
 * @param[in] type Input parameter.
 * @return Return value.
 * @details Calls: getTypeRegistries(), hash_code(), find(), end(), Registry().
 */
PluginRegistry::Registry& PluginRegistry::getTypeRegistry(const std::type_info& type) {
    auto& type_registries = getTypeRegistries();
    size_t type_hash = type.hash_code();
    
    // Create entry if doesn't exist
    if (type_registries.find(type_hash) == type_registries.end()) {
        type_registries[type_hash] = Registry();
    }
    
    return type_registries[type_hash];
}

/**
 * @brief Get Mutex.
 * @return Return value.
 * @details Implements getMutex without additional internal calls.
 */
std::shared_mutex& PluginRegistry::getMutex() {
    static std::shared_mutex mutex;
    return mutex;
}

/**
 * @brief Clear Registry.
 * @details Calls: lock(), getMutex(), getTypeRegistries(), clear().
 */
void PluginRegistry::clearRegistry() {
    std::unique_lock<std::shared_mutex> lock(getMutex());
    getTypeRegistries().clear();
}

} // namespace plugins
} // namespace themis
