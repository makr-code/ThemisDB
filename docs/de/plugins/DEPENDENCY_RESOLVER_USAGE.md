# Plugin Dependency Resolver Usage Examples

This document provides examples of using the Plugin Dependency Graph Resolver in ThemisDB.

## Overview

The `PluginDependencyResolver` class provides:
- Dependency graph building from plugin manifests
- Circular dependency detection
- Topological sorting for safe load order
- Helper methods for dependency queries

## Basic Usage

### 1. Simple Plugin Chain

```cpp
// Plugins with dependencies: A -> B -> C
PluginManager& manager = PluginManager::instance();

// Scan plugin directory
manager.scanPluginDirectory("/path/to/plugins");

// Auto-load will automatically handle dependencies
size_t loaded = manager.autoLoadPlugins();
// Result: C loads first, then B, then A
```

### 2. Diamond Dependency

```cpp
// Diamond pattern: A -> B, A -> C, B -> D, C -> D
PluginManager& manager = PluginManager::instance();
manager.scanPluginDirectory("/path/to/plugins");

// The resolver ensures D loads before B and C,
// and B and C load before A
size_t loaded = manager.autoLoadPlugins();
// Load order: D, B, C, A (or D, C, B, A - both valid)
```

### 3. Manual Dependency Loading

```cpp
PluginManager& manager = PluginManager::instance();
manager.scanPluginDirectory("/path/to/plugins");

// Load a specific plugin - dependencies are auto-loaded
auto* plugin = manager.loadPlugin("my-plugin");
// If my-plugin depends on plugin-a and plugin-b,
// they will be loaded automatically before my-plugin
```

## Advanced Usage

### 1. Checking Dependencies Before Loading

```cpp
#include "plugins/plugin_dependency_resolver.h"
#include "plugins/plugin_manager.h"

PluginManager& manager = PluginManager::instance();
manager.scanPluginDirectory("/path/to/plugins");

// Build dependency graph (manual)
auto graph = PluginDependencyResolver::buildGraph(manager.getPlugins());

// Check if plugin has specific dependency
if (PluginDependencyResolver::hasDependency(graph, "plugin-a", "plugin-b")) {
    std::cout << "plugin-a depends on plugin-b\n";
}

// Get all transitive dependencies
auto deps = PluginDependencyResolver::getTransitiveDependencies(graph, "plugin-a");
std::cout << "plugin-a has " << deps.size() << " total dependencies\n";
```

### 2. Detecting Circular Dependencies

```cpp
#include "plugins/plugin_dependency_resolver.h"
#include "plugins/plugin_manager.h"

PluginManager& manager = PluginManager::instance();
manager.scanPluginDirectory("/path/to/plugins");

// Build graph and check for cycles
auto graph = PluginDependencyResolver::buildGraph(manager.getPlugins());
auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);

if (!cycles.empty()) {
    std::cerr << "Circular dependencies detected:\n";
    for (const auto& cycle : cycles) {
        std::cerr << "  Cycle: ";
        for (size_t i = 0; i < cycle.size(); ++i) {
            std::cerr << cycle[i];
            if (i < cycle.size() - 1) std::cerr << " -> ";
        }
        std::cerr << "\n";
    }
}
```

### 3. Computing Custom Load Order

```cpp
#include "plugins/plugin_dependency_resolver.h"
#include "plugins/plugin_manager.h"

PluginManager& manager = PluginManager::instance();
manager.scanPluginDirectory("/path/to/plugins");

// Build graph
auto graph = PluginDependencyResolver::buildGraph(manager.getPlugins());

try {
    // Compute safe load order
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    // Load plugins in custom order with custom logic
    for (const auto& plugin_name : load_order) {
        // Custom loading logic here
        if (/* custom condition */) {
            manager.loadPlugin(plugin_name);
        }
    }
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to compute load order: " << e.what() << "\n";
    // Circular dependency detected
}
```

## Plugin Manifest Format

Define dependencies in your `plugin.json`:

```json
{
  "name": "my-plugin",
  "version": "1.0.0",
  "type": "custom",
  "dependencies": [
    "dependency-plugin-1",
    "dependency-plugin-2"
  ],
  "auto_load": true,
  "load_priority": 100,
  "binary": {
    "windows": "my-plugin.dll",
    "linux": "libmy-plugin.so",
    "macos": "libmy-plugin.dylib"
  }
}
```

## Error Handling

### Missing Dependency

```cpp
// If a plugin has a missing dependency, loadPlugin returns nullptr
auto* plugin = manager.loadPlugin("plugin-with-missing-dep");
if (!plugin) {
    // Check logs for error:
    // "Plugin plugin-with-missing-dep has missing dependency: some-dep"
}
```

### Circular Dependency

```cpp
// Auto-load detects circular dependencies and returns 0
size_t loaded = manager.autoLoadPlugins();
if (loaded == 0) {
    // Check logs for circular dependency errors
    // Example: "Cycle: plugin-a -> plugin-b -> plugin-c -> plugin-a"
}
```

## Performance Characteristics

- **Graph Building**: O(n) where n = number of plugins
- **Cycle Detection**: O(n + e) where e = number of dependency edges
- **Topological Sort**: O(n + e)
- **Transitive Dependencies**: O(n + e) per query

## Best Practices

1. **Keep dependency chains shallow**: Deep chains increase load time
2. **Avoid circular dependencies**: They prevent plugins from loading
3. **Use `auto_load` carefully**: Set to true only for essential plugins
4. **Set appropriate `load_priority`**: Lower numbers load first (within same dependency level)
5. **Document plugin dependencies**: Make it clear what each plugin needs
6. **Test dependency scenarios**: Use unit tests to verify complex dependency graphs

## Integration Examples

### Web Server with Plugins

```cpp
int main() {
    PluginManager& manager = PluginManager::instance();
    
    // Scan for plugins
    manager.scanPluginDirectory("./plugins");
    
    // Auto-load all plugins marked with auto_load=true
    size_t loaded = manager.autoLoadPlugins();
    std::cout << "Loaded " << loaded << " plugins\n";
    
    // Start web server
    // Plugins are now available through manager.getPlugin()
    
    // ... server logic ...
    
    // Cleanup
    manager.unloadAllPlugins();
    return 0;
}
```

### Dynamic Plugin Loading

```cpp
// Load plugins on-demand
void handleRequest(const std::string& plugin_name) {
    PluginManager& manager = PluginManager::instance();
    
    auto* plugin = manager.getPlugin(plugin_name);
    if (!plugin) {
        // Not loaded yet, try loading
        plugin = manager.loadPlugin(plugin_name);
        if (!plugin) {
            throw std::runtime_error("Failed to load plugin: " + plugin_name);
        }
    }
    
    // Use plugin
    plugin->execute();
}
```

## Troubleshooting

### Plugin Won't Load

1. Check if all dependencies are available:
   ```cpp
   auto manifest = manager.getManifest("my-plugin");
   if (manifest) {
       for (const auto& dep : manifest->dependencies) {
           if (!manager.isPluginLoaded(dep)) {
               std::cout << "Missing dependency: " << dep << "\n";
           }
       }
   }
   ```

2. Check for circular dependencies:
   ```cpp
   auto graph = PluginDependencyResolver::buildGraph(manager.getPlugins());
   auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
   // Check cycles vector
   ```

3. Verify plugin manifest is valid and signed (in production builds)

### Load Order Issues

If plugins are loading in the wrong order:

1. Verify `dependencies` array in plugin.json is correct
2. Check that dependency names match exactly
3. Use `computeLoadOrder()` to see the computed order
4. Remember: `load_priority` only matters within the same dependency level

## See Also

- [Plugin System Documentation](PLUGIN_SYSTEM_INTEGRATION.md)
- [Plugin Security](MANIFEST_SIGNATURES.md)
- [Plugin Metrics](../../include/plugins/plugin_metrics.h)
