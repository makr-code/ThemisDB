# Enhanced Hot-Reload with State Preservation

## Overview

The enhanced hot-reload system provides zero-downtime plugin updates with automatic state preservation, dependency management, atomic operations, and comprehensive rollback capabilities.

## Features

### 1. State Preservation (`IStatefulPlugin`)

Plugins can implement the `IStatefulPlugin` interface to preserve their internal state during hot-reload:

```cpp
class IStatefulPlugin {
public:
    virtual ~IStatefulPlugin() = default;
    
    /**
     * @brief Save plugin state before reload
     * @return Serialized state as JSON string
     */
    virtual std::string saveState() = 0;
    
    /**
     * @brief Restore plugin state after reload
     * @param state Previously saved state
     * @return true if restored successfully
     */
    virtual bool restoreState(const std::string& state) = 0;
};
```

#### Example Implementation

```cpp
class MyStatefulPlugin : public IThemisPlugin, public IStatefulPlugin {
private:
    int request_count = 0;
    std::map<std::string, std::string> cache;
    
public:
    // IStatefulPlugin implementation
    std::string saveState() override {
        nlohmann::json state;
        state["request_count"] = request_count;
        state["cache"] = cache;
        return state.dump();
    }
    
    bool restoreState(const std::string& state_json) override {
        try {
            auto state = nlohmann::json::parse(state_json);
            request_count = state["request_count"];
            cache = state["cache"].get<std::map<std::string, std::string>>();
            return true;
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to restore state: {}", e.what());
            return false;
        }
    }
    
    // ... other IThemisPlugin methods
};
```

### 2. Reload Event System

Register listeners to be notified during different phases of plugin reload:

```cpp
enum class PluginReloadPhase {
    BEFORE_UNLOAD,  // About to unload old plugin
    AFTER_UNLOAD,   // Old plugin unloaded
    AFTER_LOAD      // New plugin loaded and initialized
};

using PluginReloadListener = std::function<void(
    const std::string& plugin_name, 
    PluginReloadPhase phase
)>;
```

#### Example Usage

```cpp
auto& pm = PluginManager::instance();

// Register a reload listener
pm.registerReloadListener([](const std::string& name, PluginReloadPhase phase) {
    switch (phase) {
        case PluginReloadPhase::BEFORE_UNLOAD:
            THEMIS_INFO("Plugin {} is about to be reloaded", name);
            // Pause incoming requests to this plugin
            break;
            
        case PluginReloadPhase::AFTER_UNLOAD:
            THEMIS_INFO("Plugin {} has been unloaded", name);
            break;
            
        case PluginReloadPhase::AFTER_LOAD:
            THEMIS_INFO("Plugin {} has been reloaded", name);
            // Resume incoming requests
            break;
    }
});

// Trigger reload
pm.reloadPlugin("my_plugin");
```

### 3. Dependency Management

The system automatically checks for dependent plugins before allowing reload:

```cpp
// If plugin B depends on plugin A, attempting to reload A will fail:
auto& pm = PluginManager::instance();

bool success = pm.reloadPlugin("plugin_a");
if (!success) {
    // Check logs: "Cannot reload plugin plugin_a - 1 plugins depend on it: plugin_b"
}
```

**Workaround:** Unload dependent plugins first, or reload them together.

### 4. Atomic Operations with Rollback

The reload process is atomic - if any step fails, the system automatically rolls back to the old plugin version:

#### Reload Process

1. **Check Dependencies** - Fail if other plugins depend on this one
2. **Save State** - Call `saveState()` if plugin is stateful
3. **Notify BEFORE_UNLOAD** - Alert registered listeners
4. **Unload Old Plugin** - Shutdown and unload old binary
5. **Notify AFTER_UNLOAD** - Alert registered listeners
6. **Verify New Binary** - Check signatures and security
7. **Load New Binary** - Load plugin library
8. **Create Instance** - Call `createPlugin()`
9. **Initialize** - Call `initialize()` with restored state
10. **Restore State** - Call `restoreState()` if available
11. **Notify AFTER_LOAD** - Alert registered listeners

If any step 6-10 fails, the system rolls back to the old plugin instance.

#### Rollback Example

```cpp
// Scenario: New plugin binary is corrupted
bool success = pm.reloadPlugin("my_plugin");

// Logs show:
// ERROR: Plugin verification failed after reload for my_plugin: Invalid signature
// WARN: Attempting to rollback plugin my_plugin to previous version

// Result: Old plugin is still loaded and functional
EXPECT_FALSE(success);
EXPECT_NE(pm.getPlugin("my_plugin"), nullptr);  // Old instance still works
```

### 5. Configuration Migration

When a plugin is reloaded, the saved state is passed in the initialization config:

```cpp
bool MyPlugin::initialize(const char* config_json) {
    auto config = nlohmann::json::parse(config_json);
    
    // Check for restored state
    if (config.contains("restored_state")) {
        std::string state = config["restored_state"];
        restoreState(state);
    }
    
    // Apply new configuration
    if (config.contains("new_setting")) {
        apply_new_setting(config["new_setting"]);
    }
    
    return true;
}
```

## API Reference

### PluginManager Methods

```cpp
/**
 * @brief Register a reload event listener
 */
void registerReloadListener(PluginReloadListener listener);

/**
 * @brief Clear all reload event listeners
 */
void clearReloadListeners();

/**
 * @brief Reload a plugin (enhanced version)
 * @param name Plugin name
 * @return true if reload successful, false otherwise
 * 
 * Features:
 * - Checks for dependent plugins
 * - Saves state (if IStatefulPlugin)
 * - Atomic operation with rollback
 * - Event notifications
 * - Thread-safe
 */
bool reloadPlugin(const std::string& name);
```

## Usage Examples

### Example 1: Basic Hot-Reload

```cpp
auto& pm = PluginManager::instance();

// Load plugin
pm.loadPlugin("my_plugin");

// ... plugin is in use ...

// Hot-reload (e.g., after deploying new binary)
if (pm.reloadPlugin("my_plugin")) {
    THEMIS_INFO("Plugin reloaded successfully");
} else {
    THEMIS_ERROR("Plugin reload failed - old version still running");
}
```

### Example 2: Stateful Plugin Hot-Reload

```cpp
class DatabaseConnectionPlugin : public IThemisPlugin, public IStatefulPlugin {
private:
    std::map<std::string, Connection> active_connections;
    
public:
    std::string saveState() override {
        nlohmann::json state;
        for (const auto& [id, conn] : active_connections) {
            state["connections"][id] = {
                {"host", conn.host},
                {"port", conn.port},
                {"database", conn.database}
            };
        }
        return state.dump();
    }
    
    bool restoreState(const std::string& state_json) override {
        try {
            auto state = nlohmann::json::parse(state_json);
            for (const auto& [id, conn_data] : state["connections"].items()) {
                // Reconnect to database
                Connection conn;
                conn.host = conn_data["host"];
                conn.port = conn_data["port"];
                conn.database = conn_data["database"];
                conn.reconnect();
                
                active_connections[id] = std::move(conn);
            }
            return true;
        } catch (...) {
            return false;
        }
    }
};

// Usage:
auto& pm = PluginManager::instance();
pm.reloadPlugin("database_connection");
// All active connections are automatically restored!
```

### Example 3: Coordinated Reload with Events

```cpp
class RequestRouter {
private:
    std::atomic<bool> paused_{false};
    
public:
    void setup() {
        auto& pm = PluginManager::instance();
        
        pm.registerReloadListener([this](const std::string& name, PluginReloadPhase phase) {
            if (name == "critical_plugin") {
                switch (phase) {
                    case PluginReloadPhase::BEFORE_UNLOAD:
                        // Pause routing to this plugin
                        paused_ = true;
                        THEMIS_INFO("Paused requests to {}", name);
                        break;
                        
                    case PluginReloadPhase::AFTER_LOAD:
                        // Resume routing
                        paused_ = false;
                        THEMIS_INFO("Resumed requests to {}", name);
                        break;
                }
            }
        });
    }
    
    void routeRequest(const Request& req) {
        if (paused_) {
            // Queue or reject request
            return;
        }
        // Process normally
    }
};
```

## Best Practices

### 1. Design for Stateless Operation

While state preservation is available, design plugins to be as stateless as possible:

```cpp
// Good: State can be reconstructed from external source
class CachePlugin : public IThemisPlugin, public IStatefulPlugin {
    std::string saveState() override {
        // Save cache keys only, not values
        nlohmann::json state;
        for (const auto& [key, _] : cache) {
            state["keys"].push_back(key);
        }
        return state.dump();
    }
    
    bool restoreState(const std::string& state_json) override {
        auto state = nlohmann::json::parse(state_json);
        for (const std::string& key : state["keys"]) {
            // Reload value from database
            cache[key] = database.get(key);
        }
        return true;
    }
};
```

### 2. Handle State Restoration Failure Gracefully

```cpp
bool MyPlugin::initialize(const char* config_json) {
    auto config = nlohmann::json::parse(config_json);
    
    if (config.contains("restored_state")) {
        if (!restoreState(config["restored_state"])) {
            THEMIS_WARN("Failed to restore state, starting fresh");
            // Continue with default state
        }
    }
    
    return true;
}
```

### 3. Use Events for External Coordination

```cpp
// Notify external systems about reload
pm.registerReloadListener([](const std::string& name, PluginReloadPhase phase) {
    if (phase == PluginReloadPhase::AFTER_LOAD) {
        // Update service discovery
        service_registry.update_plugin_version(name);
        
        // Warm up caches
        plugin_cache.invalidate(name);
    }
});
```

### 4. Test Reload Behavior

```cpp
TEST(PluginTest, ReloadPreservesState) {
    auto plugin = std::make_unique<MyPlugin>();
    plugin->initialize("{}");
    
    // Set state
    plugin->setCounter(42);
    
    // Save state
    std::string state = plugin->saveState();
    
    // Simulate reload
    plugin.reset();
    plugin = std::make_unique<MyPlugin>();
    plugin->initialize("{}");
    plugin->restoreState(state);
    
    // Verify state preserved
    EXPECT_EQ(plugin->getCounter(), 42);
}
```

## Limitations

1. **Binary Compatibility** - New plugin version must be binary compatible with saved state format
2. **Dependency Blocking** - Cannot reload if other plugins depend on it (must unload dependents first)
3. **No Parallel Reload** - Only one plugin can be reloaded at a time (thread-safe serialization)
4. **State Size** - Very large state may cause brief pause during serialization

## Troubleshooting

### Problem: Reload Fails with "plugins depend on it"

**Solution:** Unload dependent plugins first:

```cpp
// Get dependent plugins (hypothetical helper)
auto dependents = getDependents("plugin_a");
for (const auto& dep : dependents) {
    pm.unloadPlugin(dep);
}

// Now reload works
pm.reloadPlugin("plugin_a");

// Reload dependents
for (const auto& dep : dependents) {
    pm.loadPlugin(dep);
}
```

### Problem: State Not Restored After Reload

**Checklist:**
1. Plugin implements `IStatefulPlugin`
2. `saveState()` returns valid JSON
3. `restoreState()` is called in `initialize()`
4. State format is compatible between versions

### Problem: Reload Takes Too Long

**Solutions:**
- Reduce state size (save only essential data)
- Move long-running cleanup to background thread
- Use differential state updates

## See Also

- [Plugin System Overview](PLUGIN_SYSTEM_SUMMARY.md)
- [Plugin Migration Guide](PLUGIN_MIGRATION.md)
- [Dependency Resolution](DEPENDENCY_RESOLVER_USAGE.md)
- [Plugin Manifest Signatures](MANIFEST_SIGNATURES.md)
