# Generic Plugin System Architecture

## Overview

ThemisDB's plugin system has been refactored to use a **generic, type-erased registry** 
instead of a monolithic PluginManager that knows about all plugin types.

## Old Design (Before)

```cpp
// ❌ PROBLEM: PluginManager knows about all types
class PluginManager {
    IThemisPlugin* loadPlugin(const std::string& name);
    
    // Too many includes:
    #include "blob_storage_interface.h"
    #include "importer_interface.h"
    #include "exporter_interface.h"
    #include "image_analysis_interface.h"
    // ... 20 more includes
};
```

Problems:
- ❌ Tight coupling to specific plugin types
- ❌ PluginManager grows with each new plugin type
- ❌ Type-unsafe (void* casts)
- ❌ Not extensible without modifying PluginManager
- ❌ Violates Single Responsibility Principle

## New Design (After - This PR)

```cpp
// ✅ SOLUTION: Generic registry with type-safe access

// 1. Generic Loading (if needed)
IThemisPlugin* plugin = PluginManager::loadPluginFromPath(
    "/path/to/plugin.so"
);

// 2. Type-Safe Access
auto s3 = PluginAPI::get<storage::IBlobStorageBackend>("s3_plugin");
if (s3) {
    s3->put("key", data);
}

// 3. List All Plugins of Type
auto all_importers = PluginAPI::getAll<importers::IImporter>();

// 4. Automatic Registration
PluginAutoRegister<storage::IBlobStorageBackend> s3_registrar(
    "s3_plugin",
    []() { return std::make_unique<S3BlobPlugin>(); }
);
```

Benefits:
- ✅ No coupling to specific types
- ✅ Extensible without code changes
- ✅ Type-safe templates, no void* casts
- ✅ Clean separation of concerns
- ✅ Single Responsibility: PluginManager handles loading, PluginRegistry handles type binding

## Architecture

### Layer 1: PluginManager (Generic)
```
Responsibilities:
- Load DLL/SO from path
- Parse plugin.json manifest
- Verify signatures
- Initialize plugins
- Manage lifecycle
- Discover plugins

Does NOT know about:
- IBlobStorageBackend
- IImporter
- IExporter
- ... any specific types!
```

### Layer 2: PluginRegistry (Type-Safe)
```
Responsibilities:
- Register plugin factories
- Create plugin instances
- Type-safe templated access
- Type verification
- Plugin lookup by type

Knows about:
- Generic type_info
- Type hashing
- Factory creation
```

### Layer 3: PluginAPI (Convenient)
```
Responsibilities:
- Provide convenient template-based API
- Handle errors gracefully
- Support fallbacks
- List plugins by type
```

## Usage Patterns

### Pattern 1: Single Plugin Access
```cpp
// Get specific S3 plugin
auto s3 = PluginAPI::get<storage::IBlobStorageBackend>("s3_plugin");
if (s3) {
    s3->put("bucket/key", data);
}
```

### Pattern 2: All Plugins of Type
```cpp
// Get all configured importers
auto importers = PluginAPI::getAll<importers::IImporter>();
for (auto& importer : importers) {
    importer->importData("source.csv", options);
}
```

### Pattern 3: With Fallback
```cpp
// Use first available blob storage
auto storage = PluginAPI::getWithFallback<storage::IBlobStorageBackend>();
if (storage) {
    storage->put("key", data);
} else {
    // Fallback to built-in storage
}
```

### Pattern 4: Plugin Registration (In Plugin Code)
```cpp
// plugins/s3/s3_plugin.cpp
#include "storage/blob_storage_backend.h"
#include "plugins/plugin_registry.h"

class S3BlobPlugin : public storage::IBlobStorageBackend {
    // Implementation
};

// Auto-register on plugin load
PluginAutoRegister<storage::IBlobStorageBackend> s3_reg(
    "s3_plugin",
    []() { return std::make_unique<S3BlobPlugin>(); }
);
```

### Pattern 5: Check Availability
```cpp
if (PluginAPI::has<storage::IBlobStorageBackend>("s3_plugin")) {
    // S3 plugin is available
    auto s3 = PluginAPI::get<storage::IBlobStorageBackend>("s3_plugin");
    // Use it...
} else {
    // Fallback to filesystem
}
```

## Migration from Old System

### Old Code
```cpp
// Loading
IThemisPlugin* plugin = PluginManager::loadPlugin("s3_plugin");

// Manual casting (type-unsafe)
storage::IBlobStorageBackend* s3 = 
    static_cast<storage::IBlobStorageBackend*>(plugin->getInstance());

// Use it
s3->put("key", data);
```

### New Code
```cpp
// Type-safe loading
auto s3 = PluginAPI::get<storage::IBlobStorageBackend>("s3_plugin");

// No casting needed!
if (s3) {
    s3->put("key", data);
}
```

## API Reference

### PluginRegistry

#### `registerFactory<T>(name, factory)`
Register a plugin factory for type T.

```cpp
PluginRegistry::registerFactory<storage::IBlobStorageBackend>(
    "s3_plugin",
    []() { return std::make_unique<S3BlobPlugin>(); }
);
```

#### `create<T>(name)`
Create plugin instance (throws on error).

```cpp
auto plugin = PluginRegistry::create<storage::IBlobStorageBackend>("s3_plugin");
```

#### `hasPlugin<T>(name)`
Check if plugin is registered for type T.

```cpp
bool has = PluginRegistry::hasPlugin<storage::IBlobStorageBackend>("s3_plugin");
```

#### `listPlugins<T>()`
Get all plugin names for type T.

```cpp
auto names = PluginRegistry::listPlugins<storage::IBlobStorageBackend>();
```

### PluginAPI

#### `get<T>(name)`
Get single plugin (returns nullptr on error).

```cpp
auto plugin = PluginAPI::get<storage::IBlobStorageBackend>("s3_plugin");
```

#### `getAll<T>()`
Get all plugins of type T.

```cpp
auto plugins = PluginAPI::getAll<storage::IBlobStorageBackend>();
```

#### `has<T>(name)`
Check if plugin is available.

```cpp
bool has = PluginAPI::has<storage::IBlobStorageBackend>("s3_plugin");
```

#### `getWithFallback<T>()`
Get first available plugin of type T.

```cpp
auto plugin = PluginAPI::getWithFallback<storage::IBlobStorageBackend>();
```

## Design Decisions

### Why Type Erasure?
- PluginManager doesn't need to know about specific types
- Enables unlimited plugin types without code changes
- Maintains type safety through templates

### Why Templates?
- Compile-time type checking
- No manual casting required
- Clear, expressive API

### Why Separate PluginAPI?
- Convenience layer for common patterns
- Graceful error handling (nullptr vs exceptions)
- Fallback support built-in

### Why Factory Pattern?
- Deferred instantiation
- Supports multiple instances of same plugin
- Easy testing with mock factories

## Thread Safety

All operations are thread-safe:
- `PluginRegistry` uses internal mutex
- Multiple threads can register/create plugins concurrently
- Factory functions should be thread-safe

## Testing

See `tests/test_generic_plugin_registry.cpp` for comprehensive tests:
- Basic registration and creation
- Type mismatch detection
- Plugin listing by type
- PluginAPI convenience methods
- Auto-registration
- Edge cases

## Performance Considerations

- **Registration**: O(log n) - map insertion
- **Creation**: O(log n) - map lookup + factory call
- **Listing**: O(n) - iterate over type registry
- **Type checking**: O(1) - hash comparison

The overhead is minimal compared to dynamic library loading.

## Limitations

- Type information relies on `std::type_info::hash_code()`
- Plugins must be registered before use
- No automatic plugin discovery (use PluginManager for that)

## Future Enhancements

Potential improvements:
1. Plugin dependency resolution
2. Automatic registration from manifest
3. Plugin versioning and compatibility checks
4. Hot-reload support
5. Plugin sandboxing

## Summary

The generic plugin system provides:
- ✅ Clean separation of concerns
- ✅ Type-safe plugin access
- ✅ Extensibility without code changes
- ✅ Simple, intuitive API
- ✅ Full backward compatibility

This refactoring makes ThemisDB's plugin system scalable and maintainable for the future.
