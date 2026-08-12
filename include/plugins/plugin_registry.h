/**
 * @file plugin_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "plugins/plugin_interface.h"
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <typeinfo>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>

namespace themis {
namespace plugins {

/**
 * @brief Generic Plugin Registry with Type-Safe Binding
 * 
 * Replaces the tightly-coupled PluginManager type-specific code.
 * Uses type erasure and generic factories.
 * 
 * Design:  Each plugin type has its own registry entry.
 *          PluginManager only handles IThemisPlugin.
 *          Specific interfaces (IBlobStorage, etc.) are registered
 *          in a separate type-safe registry.
 * 
 * @note **Plugin Name Collision**: If a plugin with the same name is registered twice,
 *       the last registration wins. This is by design to allow runtime plugin replacement,
 *       but can lead to unexpected behavior if multiple modules register plugins with
 *       the same name. To avoid collisions, use namespaced plugin names (e.g., "mylib.s3_plugin").
 */
class PluginRegistry {
public:
    /**
     * @brief Register a plugin factory
     * 
     * @tparam PluginInterface  The interface type (e.g., IBlobStorageBackend)
     * @param plugin_name       Unique plugin identifier
     * @param factory           Factory function creating instances
     * 
     * Example:
     * ```cpp
     * PluginRegistry::registerFactory<IBlobStorageBackend>(
     *     "s3_plugin",
     *     []() { return std::make_unique<S3BlobPlugin>(); }
     * );
     * ```
     */
    template<typename PluginInterface>
    static void registerFactory(
        const std::string& plugin_name,
        std::function<std::unique_ptr<PluginInterface>()> factory
    ) {
        std::unique_lock<std::shared_mutex> lock(getMutex());
        
        // Store the factory with type erasure in type-specific registry
        // This avoids data duplication and ensures consistency
        auto& type_registry = getTypeRegistry(typeid(PluginInterface));
        
        FactoryEntry entry;
        entry.factory = [factory]() -> void* {
            return static_cast<void*>(factory().release());
        };
        entry.deleter = [](void* ptr) {
            delete static_cast<PluginInterface*>(ptr);
        };
        entry.type_hash = typeid(PluginInterface).hash_code();
        entry.type_name = typeid(PluginInterface).name();
        
        type_registry[plugin_name] = entry;
    }

    /**
     * @brief Create plugin instance from factory
     * 
     * @tparam PluginInterface  Expected interface type
     * @param plugin_name       Plugin identifier
     * @return Unique pointer to plugin instance (type-safe)
     * 
     * @throws std::runtime_error if plugin not found or type mismatch
     * 
     * Example:
     * ```cpp
     * auto s3_plugin = PluginRegistry::create<IBlobStorageBackend>("s3_plugin");
     * if (s3_plugin) {
     *     s3_plugin->write("key", data);
     * }
     * ```
     */
    template<typename PluginInterface>
    static std::unique_ptr<PluginInterface> create(const std::string& plugin_name) {
        std::shared_lock<std::shared_mutex> lock(getMutex());
        
        // Look up in type-specific registry (read-only: no creation of missing entries)
        const auto& type_registries = getTypeRegistries();
        size_t type_hash = typeid(PluginInterface).hash_code();
        auto trIt = type_registries.find(type_hash);
        if (trIt == type_registries.end()) {
            throw std::runtime_error(
                "No plugins registered for interface type '" +
                std::string(typeid(PluginInterface).name()) +
                "' (requested plugin: " + plugin_name + ")"
            );
        }
        const auto& type_registry = trIt->second;
        auto it = type_registry.find(plugin_name);
        
        if (it == type_registry.end()) {
            throw std::runtime_error("Plugin not registered: " + plugin_name);
        }
        
        const auto& entry = it->second;
        
        // Verify type matches - check both hash_code and type_name
        // Hash check is fast but may differ across compilation units
        // Type name check is slower but more robust across boundaries
        // Note: std::type_info::name() returns implementation-defined strings
        // that may be mangled (e.g., "N6themis7storage19IBlobStorageBackendE" on GCC).
        // This is acceptable for error messages as they're for developer debugging.
        size_t expected_hash = typeid(PluginInterface).hash_code();
        const std::string& expected_name = typeid(PluginInterface).name();
        
        // Primary check: hash code match (fast)
        if (entry.type_hash == expected_hash) {
            // Hash matched, proceed
        } else if (entry.type_name == expected_name) {
            // Hash didn't match but name did - likely cross-compilation unit issue
            // This is acceptable and should not throw
        } else {
            // Neither hash nor name matched - definite type mismatch
            throw std::runtime_error(
                "Plugin type mismatch for '" + plugin_name + "' " +
                "(expected interface: " + expected_name + 
                ", but plugin implements: " + entry.type_name + ")"
            );
        }
        
        // Call factory and wrap in unique_ptr
        // The factory creates a unique_ptr<PluginInterface> and releases it to void*
        // for type-erased storage, so we reconstruct it with the correct type here.
        void* raw_ptr = nullptr;
        try {
            raw_ptr = entry.factory();
        } catch (...) {
            // If factory throws, there's no memory to clean up since it failed
            throw;
        }
        
        if (!raw_ptr) {
            throw std::runtime_error(
                "Factory for plugin '" + plugin_name + 
                "' (interface: " + expected_name + ") returned null pointer"
            );
        }
        
        return std::unique_ptr<PluginInterface>(
            static_cast<PluginInterface*>(raw_ptr)
        );
    }

    /**
     * @brief Get all registered plugins for a type
     * 
     * @tparam PluginInterface  Interface type to filter by
     * @return Vector of plugin names implementing that interface
     */
    template<typename PluginInterface>
    static std::vector<std::string> listPlugins() {
        std::shared_lock<std::shared_mutex> lock(getMutex());
        const auto& type_registries = getTypeRegistries();
        size_t type_hash = typeid(PluginInterface).hash_code();
        auto it = type_registries.find(type_hash);
        if (it == type_registries.end()) {
            return {};
        }
        const auto& type_registry = it->second;
        
        std::vector<std::string> names;
        for (const auto& [name, _] : type_registry) {
            names.push_back(name);
        }
        return names;
    }

    /**
     * @brief Check if plugin is registered
     * 
     * @tparam PluginInterface  Interface type
     * @param plugin_name       Plugin identifier
     * @return true if plugin implements this interface
     */
    template<typename PluginInterface>
    static bool hasPlugin(const std::string& plugin_name) {
        std::shared_lock<std::shared_mutex> lock(getMutex());
        const auto& type_registries = getTypeRegistries();
        size_t type_hash = typeid(PluginInterface).hash_code();
        auto it = type_registries.find(type_hash);
        if (it == type_registries.end()) {
            return false;
        }
        return it->second.count(plugin_name) > 0;
    }

    /**
     * @brief Unregister a previously registered plugin factory
     * 
     * @tparam PluginInterface  Interface type
     * @param plugin_name       Plugin identifier to remove
     * @return true if the factory was found and removed, false otherwise
     */
    template<typename PluginInterface>
    static bool unregisterFactory(const std::string& plugin_name) {
        std::unique_lock<std::shared_mutex> lock(getMutex());
        auto& type_registries = getTypeRegistries();
        size_t type_hash = typeid(PluginInterface).hash_code();
        auto trIt = type_registries.find(type_hash);
        if (trIt == type_registries.end()) {
            return false;
        }
        auto& type_registry = trIt->second;
        auto it = type_registry.find(plugin_name);
        if (it == type_registry.end()) {
            return false;
        }
        type_registry.erase(it);
        return true;
    }

    /**
     * @brief Clear all registered plugins (testing only)
     */
    static void clearRegistry();

private:
    struct FactoryEntry {
        std::function<void*()> factory;
        std::function<void(void*)> deleter;
        size_t type_hash = 0;
        std::string type_name;
    };

    using Registry = std::map<std::string, FactoryEntry>;
    using TypeRegistries = std::map<size_t, Registry>;

    static TypeRegistries& getTypeRegistries();
    static Registry& getTypeRegistry(const std::type_info& type);
    static std::shared_mutex& getMutex();
};

/**
 * @brief Helper for automatic plugin registration
 * 
 * Usage in plugin implementation:
 * ```cpp
 * class S3BlobPlugin : public IBlobStorageBackend { ... };
 * 
 * // In .cpp file:
 * PluginAutoRegister<IBlobStorageBackend> s3_registrar(
 *     "s3_plugin",
 *     []() { return std::make_unique<S3BlobPlugin>(); }
 * );
 * ```
 */
template<typename PluginInterface>
class PluginAutoRegister {
public:
    PluginAutoRegister(
        const std::string& name,
        std::function<std::unique_ptr<PluginInterface>()> factory
    ) {
        PluginRegistry::registerFactory<PluginInterface>(name, factory);
    }
};

} // namespace plugins
} // namespace themis
