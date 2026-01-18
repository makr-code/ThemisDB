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
        auto& registry = getRegistry();
        std::lock_guard<std::mutex> lock(getMutex());
        
        // Store the factory with type erasure
        FactoryEntry entry;
        entry.factory = [factory]() -> void* {
            return static_cast<void*>(factory().release());
        };
        entry.deleter = [](void* ptr) {
            delete static_cast<PluginInterface*>(ptr);
        };
        entry.type_hash = typeid(PluginInterface).hash_code();
        entry.type_name = typeid(PluginInterface).name();
        
        registry[plugin_name] = entry;
        
        // Also store in type-specific registry
        auto& type_registry = getTypeRegistry(typeid(PluginInterface));
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
        auto& registry = getRegistry();
        std::lock_guard<std::mutex> lock(getMutex());
        
        auto it = registry.find(plugin_name);
        
        if (it == registry.end()) {
            throw std::runtime_error("Plugin not registered: " + plugin_name);
        }
        
        const auto& entry = it->second;
        
        // Verify type matches
        size_t expected_hash = typeid(PluginInterface).hash_code();
        if (entry.type_hash != expected_hash) {
            throw std::runtime_error(
                "Plugin type mismatch for " + plugin_name + 
                " (expected: " + typeid(PluginInterface).name() + 
                ", registered: " + entry.type_name + ")"
            );
        }
        
        // Call factory and wrap in unique_ptr 
        // Note: The factory already returns a unique_ptr that was released,
        // so we can just use the default deleter here since it's the correct type
        void* raw_ptr = entry.factory();
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
        auto& type_registry = getTypeRegistry(typeid(PluginInterface));
        std::lock_guard<std::mutex> lock(getMutex());
        
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
        auto& type_registry = getTypeRegistry(typeid(PluginInterface));
        std::lock_guard<std::mutex> lock(getMutex());
        
        return type_registry.count(plugin_name) > 0;
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

    static Registry& getRegistry();
    static TypeRegistries& getTypeRegistries();
    static Registry& getTypeRegistry(const std::type_info& type);
    static std::mutex& getMutex();
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
