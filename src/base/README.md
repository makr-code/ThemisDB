# Base Module

Base utility functions and common code for ThemisDB.

## Module Purpose

Provides foundational module loading infrastructure for ThemisDB plugins and extensions, including secure DLL/SO/DYLIB loading, digital signature verification, trust level enforcement, and plugin lifecycle management across Windows, Linux, and macOS.

## Subsystem Scope

**In scope:** Cross-platform shared library loading, digital signature and hash verification, trust level classification (TRUSTED/VERIFIED/UNTRUSTED), revocation checking, dev mode for unsigned modules, plugin lifecycle (init/execute/shutdown), hot-reload, OS-level sandboxing, WASM-based isolation, remote registry client, plugin dependency graph, A/B testing via module swapping, per-plugin audit trail.

**Out of scope:** Plugin business logic, ordered dependency loading with version conflict resolution (in progress).

## Relevant Interfaces

- `module_loader.h/cpp` — secure shared library loading, lifecycle, trust enforcement, per-plugin audit trail
- `hot_reload_manager.h/cpp` — zero-downtime hot-reload and rollback
- `module_sandbox.h/cpp` — OS-level resource limits (CPU/memory) and ABI compatibility checking
- `wasm_plugin_sandbox.h/cpp` — WASM-based memory-safe isolation for untrusted plugins
- `remote_registry_client.h/cpp` — authenticated download and installation of marketplace plugins
- `plugin_dependency_graph.h/cpp` — dependency declaration, visualization, and topological ordering
- `ab_test_manager.h/cpp` — traffic-split A/B testing via module swapping

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Core module loading, signature verification, WASM isolation, hot-reload, OS-level sandboxing, remote registry client, plugin dependency graph, A/B testing, and per-plugin audit trail are complete. Ordered dependency loading with version conflict resolution is in progress (Issue: #1566).

## Components

- **Module Loader**: Secure DLL/shared library loading with signature verification and per-plugin audit trail
- **Hot Reload Manager**: Zero-downtime module reload and rollback without database restart
- **Module Sandbox**: OS-level resource limits (CPU, memory) and ABI compatibility checking
- **WASM Plugin Sandbox**: Memory-safe isolation for untrusted WASM-based plugins
- **Remote Registry Client**: Authenticated download and installation of marketplace plugins
- **Plugin Dependency Graph**: Dependency declaration, visualization, and topological ordering
- **A/B Test Manager**: Traffic-split module swapping for controlled experiments
- **Export Macros**: Platform-specific export/import macros

## Features

### Module Loading
- **Security Verification**: Digital signature verification for loaded modules
- **Plugin Architecture**: Support for loadable plugins and extensions
- **Version Checking**: Ensure module compatibility
- **Error Handling**: Detailed error reporting for load failures
- **Platform Support**: Windows (DLL), Linux (SO), macOS (DYLIB)

### Security
- **Signature Verification**: Prevent loading of unsigned/corrupted modules
- **Hash Validation**: Verify file integrity
- **Trust Levels**: TRUSTED, VERIFIED, UNTRUSTED
- **Revocation Checking**: Check for revoked certificates
- **Development Mode**: Allow unsigned modules during development

### Plugin System
- **Dynamic Loading**: Load plugins at runtime
- **Interface Discovery**: Query plugin capabilities
- **Lifecycle Management**: Initialize, execute, shutdown
- **Resource Cleanup**: Automatic cleanup on unload

## Architecture

```
BaseModule
├─→ ModuleLoader (Secure dynamic loading, audit trail)
├─→ ModuleSecurityVerifier (Signature + hash verification)
├─→ HotReloadManager (Zero-downtime reload and rollback)
├─→ ModuleSandbox / AbiChecker (OS resource limits, ABI check)
├─→ WasmPluginSandbox (WASM-based untrusted plugin isolation)
├─→ RemoteRegistryClient (Marketplace plugin download)
├─→ PluginDependencyGraph (Dependency declaration and ordering)
├─→ ABTestManager (Traffic-split A/B module swapping)
└─→ Export Macros (Cross-platform DLL export/import)
```

## Use Cases

### Plugin Extensions
- Load custom storage backends
- Add new query operators
- Extend index types
- Custom authentication providers

### Modular Deployment
- Deploy features as separate modules
- Update modules independently
- Feature flags via module loading
- Edition-specific modules

### Third-Party Integration
- Load verified third-party plugins
- Extend functionality without recompiling
- Marketplace for plugins
- Community contributions

## Configuration

### Module Loading
```cpp
#include "themis/base/module_loader.h"

using namespace themis::modules;

// Create module loader
ModuleLoader loader;

// Configure security policy
ModuleSecurityPolicy policy;
#ifdef NDEBUG
policy.requireSignature = true;
policy.allowUnsigned = false;
#else
policy.requireSignature = false;  // Dev mode
policy.allowUnsigned = true;
#endif

loader.setSecurityPolicy(policy);

// Load module
auto result = loader.loadModule("my_plugin.dll");
if (result.is_ok()) {
    auto module = result.value();
    // Use module...
    loader.unloadModule(module);
} else {
    std::cerr << "Failed to load: " << result.error() << std::endl;
}
```

### Export/Import Macros
```cpp
#include "themis/base/export.h"

// Define exported function
THEMIS_EXPORT int myFunction(int arg);

// Define exported class
class THEMIS_EXPORT MyClass {
public:
    void doSomething();
};
```

## Performance Characteristics

- **Module load time**: <100ms typical
- **Signature verification**: 10-50ms per module
- **Memory overhead**: <1 MB per loaded module
- **Thread safety**: Safe for concurrent loading

## Security

### Production Mode
- **Required**: Digital signatures on all modules
- **Verification**: SHA-256 hash validation
- **Trust Level**: Minimum TRUSTED
- **Revocation**: Check certificate revocation

### Development Mode
- **Allowed**: Unsigned modules for testing
- **Warning**: Log unsigned module loads
- **Trust Level**: Allow UNTRUSTED
- **No Revocation**: Skip revocation checks

## Integration Points

- **Acceleration Module**: Plugin security integration
- **Storage Module**: Custom storage backends
- **Index Module**: Custom index implementations
- **Query Module**: Custom operators

## Thread Safety

- Thread-safe module loading/unloading
- Mutex-protected module registry
- Safe for concurrent operations

## Dependencies

- **Plugin Security**: Signature verification (from acceleration module)
- **spdlog**: Logging
- **Platform APIs**: dlopen/LoadLibrary

## Documentation

For detailed implementation documentation, see:
- [Architecture Guide](ARCHITECTURE.md)
- [Roadmap](ROADMAP.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)

## Version History

- **v1.0.0**: Secure module loader with signature verification
- **v1.1.0**:
  - Hot-reload support without database restart
  - WASM-based plugin sandboxing
  - OS-level module sandbox (CPU/memory limits)
  - Remote registry client for marketplace plugins
  - A/B testing framework via module swapping
  - Plugin dependency graph and topological ordering
  - Per-plugin audit trail (load, unload, errors)
- **v1.2.0**: Planned — Plugin dependency ordered loading and version conflict resolution

## Examples

### Load Plugin
```cpp
ModuleLoader loader;

auto result = loader.loadModule("storage_plugin.dll");
if (result.is_ok()) {
    auto module = result.value();
    
    // Get plugin interface
    auto interface = module->getInterface<IStoragePlugin>();
    if (interface) {
        interface->initialize();
        // Use plugin...
    }
    
    loader.unloadModule(module);
}
```

### Verify Module Security
```cpp
ModuleSecurityVerifier verifier;

std::string error;
if (verifier.verifyModule("plugin.dll", error)) {
    std::cout << "Module verified successfully" << std::endl;
} else {
    std::cerr << "Verification failed: " << error << std::endl;
}
```

### Query Module Capabilities
```cpp
auto module = loader.loadModule("plugin.dll").value();

auto capabilities = module->getCapabilities();
std::cout << "Plugin name: " << capabilities.name << std::endl;
std::cout << "Version: " << capabilities.version << std::endl;
std::cout << "Author: " << capabilities.author << std::endl;
```

## Best Practices

### Security
1. **Always verify signatures in production**: Set `requireSignature = true`
2. **Use trusted sources**: Only load modules from verified publishers
3. **Monitor load failures**: Log and alert on verification failures
4. **Keep certificates updated**: Ensure certificate validity

### Performance
1. **Load modules at startup**: Avoid runtime loading overhead
2. **Cache module handles**: Reuse loaded modules
3. **Unload unused modules**: Free resources when not needed

### Development
1. **Test both modes**: Verify behavior with and without signatures
2. **Document plugin interfaces**: Clear API contracts
3. **Version compatibility**: Check version compatibility
4. **Error handling**: Handle load failures gracefully

## See Also

- [Acceleration Module](../acceleration/README.md) - Plugin security
- [Architecture Guide](ARCHITECTURE.md)
- [Security Policy](../../SECURITY.md)

## Scientific References

1. Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). **Design Patterns: Elements of Reusable Object-Oriented Software**. Addison-Wesley. ISBN: 978-0-201-63361-0

2. Stroustrup, B. (2013). **The C++ Programming Language (4th ed.)**. Addison-Wesley. ISBN: 978-0-321-56384-2

3. Alexandrescu, A. (2001). **Modern C++ Design: Generic Programming and Design Patterns Applied**. Addison-Wesley. ISBN: 978-0-201-70431-0

4. Meyers, S. (2014). **Effective Modern C++: 42 Specific Ways to Improve Your Use of C++11 and C++14**. O'Reilly Media. ISBN: 978-1-491-90399-5
