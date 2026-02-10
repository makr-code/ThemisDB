# Base Module

Base utility functions and common code for ThemisDB.

## Components

- **Module Loader**: Secure DLL/shared library loading with signature verification
- **Export Macros**: Platform-specific export/import macros
- **Common Interfaces**: Base interfaces for plugins and extensions
- **Platform Abstraction**: Cross-platform utility functions

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
├─→ ModuleLoader (Secure dynamic loading)
├─→ ModuleSecurityVerifier (Signature verification)
├─→ PluginInterface (Common plugin interface)
└─→ PlatformUtils (Cross-platform utilities)
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
- [Module Loader](../../docs/base/module_loader.md)
- [Plugin Development](../../docs/base/plugin_development.md)
- [Security](../../docs/base/module_security.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)

## Version History

- **v1.0.0**: Secure module loader with signature verification
- **v1.1.0**: Planned - Plugin marketplace integration
- **v1.2.0**: Planned - Hot reload support

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
- [Plugin Development Guide](../../docs/base/plugin_development.md)
- [Security Guide](../../docs/security/module_security.md)
