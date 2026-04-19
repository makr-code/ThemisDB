# Themis Module - Future Enhancements

## Scope

- Public API enhancements for `include/themis/` headers
- Module dependency resolver API (`ModuleLoader::loadModuleWithDependencies`, `resolveDependencies`)
- Edition feature flag interface (`edition::IsFeatureEnabled`, `LicenseActivator`)
- Wire protocol v2 negotiation API (`WireProtocolServerV2`, stream multiplexing header)
- Build info query interface (`build_info::getBuildConfiguration`, `verifyReproducibility`)

## Design Constraints

- [ ] Feature flags (`edition::IsFeatureEnabled`) are read-only after initialization; no runtime override outside development builds
- [ ] Wire protocol version is negotiated at connection time; downgrade mid-connection is not permitted
- [ ] Edition API values are compile-time constants for non-license-activated builds
- [ ] Module dependency graph is immutable once all modules are loaded
- [ ] `LicenseActivator` requires cryptographically signed license; unsigned keys are rejected

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `edition::IsFeatureEnabled(feature_name)` | All modules | Read-only after init; ≤ 100 ns |
| `ModuleLoader::resolveDependencies(module_name)` | Module bootstrap | Returns ordered load list |
| `WireProtocolServerV2::setMaxConcurrentStreams(n)` | Network layer | Per-connection stream limit |
| `build_info::getBuildConfiguration()` | Admin API, diagnostics | Returns `BuildConfig` struct |
| `LicenseActivator::activate(license)` | Edition upgrade path | Validates signature before activation |

## Planned Features

### Dynamic Module Hot-Reload
**Priority:** Medium
**Target Version:** v1.7.0

Support for reloading modules without restarting the server.

**Features:**
- Hot-swap module DLLs/SOs at runtime
- Version compatibility checking
- Graceful connection migration
- State preservation during reload
- Automatic rollback on failure

**API:**
```cpp
ModuleLoader loader;

// Reload a module with zero downtime
auto result = loader.reloadModule("themis_storage", "/path/to/new/version.dll");
if (result.success) {
    std::cout << "Module reloaded successfully" << std::endl;
} else {
    std::cerr << "Reload failed, rolled back to previous version" << std::endl;
}

// Check if module supports hot reload
if (loader.supportsHotReload("themis_storage")) {
    // Safe to reload
}
```

**Use Cases:**
- Patch security vulnerabilities without downtime
- Upgrade modules incrementally
- Test new module versions in production
- A/B testing of module implementations

**Implementation Challenges:**
- State migration between versions
- Connection handling during reload
- Version compatibility matrix
- ABI stability requirements

---

### Module Dependency Graph
**Priority:** Medium
**Target Version:** v1.7.0

Automatic dependency resolution and load ordering for modules.

**Features:**
- Module dependency declaration
- Automatic load order calculation
- Circular dependency detection
- Missing dependency reporting
- Version constraint resolution

**Module Manifest:**
```json
{
  "module": "themis_query",
  "version": "1.7.0",
  "dependencies": {
    "themis_storage": ">=1.7.0",
    "themis_security": "^1.6.0"
  },
  "optional_dependencies": {
    "themis_llm": ">=1.7.0"
  },
  "provides": ["query_engine", "aql_parser"]
}
```

**API:**
```cpp
ModuleLoader loader;

// Load module and all dependencies automatically
auto result = loader.loadModuleWithDependencies("themis_query", "/modules/");
for (const auto& dep : result.loaded_modules) {
    std::cout << "Loaded: " << dep << std::endl;
}

// Check dependencies before loading
auto deps = loader.resolveDependencies("themis_query");
if (!deps.all_satisfied) {
    std::cerr << "Missing dependencies: ";
    for (const auto& missing : deps.missing) {
        std::cerr << missing << " ";
    }
    std::cerr << std::endl;
}

// Export dependency graph
loader.exportDependencyGraph("dependencies.dot");
// Render: dot -Tpng dependencies.dot -o dependencies.png
```

---

### Edition Upgrade at Runtime
**Priority:** Low
**Target Version:** v1.8.0

Support for upgrading editions without recompilation (license-based activation).

**Current Limitation:**
- Edition is compile-time only
- Upgrading requires rebuilding binary

**Proposed Solution:**
- Compile binary with all features
- License key enables/disables features at runtime
- Hardware limits enforced by license

**License Activation:**
```cpp
#include "themis/license_activator.h"

LicenseActivator activator;

// Load license file
auto license = activator.loadLicense("enterprise_license.lic");

// Activate features
if (activator.activate(license)) {
    std::cout << "Upgraded to " << license.edition << std::endl;

    // Features now available
    if (edition::IsFeatureEnabled("field_encryption")) {
        // Enterprise features activated
    }
} else {
    std::cerr << "License activation failed" << std::endl;
}

// Query active edition
auto active = activator.getActiveEdition();
std::cout << "Active edition: " << active.name << std::endl;
std::cout << "Licensed to: " << active.organization << std::endl;
```

**Benefits:**
- Single binary for all editions
- Easy upgrades for customers
- Trial/evaluation support
- Feature toggles for testing

**Security Requirements:**
- Cryptographically signed licenses
- Anti-tampering measures
- Online validation (optional)
- Periodic license checks

---

### Build Reproducibility
**Priority:** High
**Target Version:** v1.6.0

Ensure builds are reproducible for security auditing.

**Features:**
- Deterministic builds
- Embedded source commit hash
- Build environment metadata
- Toolchain version tracking
- Dependency pinning

**Build Metadata:**
```cpp
auto config = build_info::getBuildConfiguration();

// Full reproducibility information
std::cout << "Commit: " << config.git_commit << std::endl;
std::cout << "Commit date: " << config.git_commit_date << std::endl;
std::cout << "Build host: " << config.build_host << std::endl;
std::cout << "Build user: " << config.build_user << std::endl;

// Verify reproducibility
if (build_info::verifyReproducibility("expected_hash")) {
    std::cout << "Build is reproducible" << std::endl;
}

// Export full build manifest
build_info::exportBuildManifest("build_manifest.json");
```

**Build Manifest Example:**
```json
{
  "build_id": "20240215-ab12cd34",
  "git_commit": "ab12cd34ef56...",
  "git_branch": "main",
  "build_timestamp": "2024-02-15T10:30:00Z",
  "compiler": "GCC 13.2.0",
  "cmake_version": "3.28.1",
  "vcpkg_commit": "xyz789...",
  "dependencies": {
    "boost": "1.84.0",
    "rocksdb": "8.10.0",
    "openssl": "3.2.0"
  }
}
```

---

### Module Sandboxing
**Priority:** Medium
**Target Version:** v1.8.0

Isolate modules in sandboxes to prevent malicious code execution.

**Sandboxing Techniques:**
- **Process isolation**: Each module in separate process
- **Namespace isolation**: Linux namespaces (PID, network, mount)
- **Resource limits**: CPU, memory, I/O limits per module
- **Capability restrictions**: Limit system calls (seccomp-bpf)

**Architecture:**
```
Main Process (themis)
    ├── Module Sandbox (themis_storage)
    │   ├── Process isolation
    │   ├── Resource limits
    │   └── IPC channel
    ├── Module Sandbox (themis_query)
    │   ├── Process isolation
    │   ├── Resource limits
    │   └── IPC channel
    └── Module Sandbox (themis_security)
        ├── Process isolation
        ├── Resource limits
        └── IPC channel
```

**API:**
```cpp
ModuleLoader loader;

// Load module in sandbox
ModuleSandbox::Config sandbox_config;
sandbox_config.max_memory_mb = 1024;
sandbox_config.max_cpu_percent = 50;
sandbox_config.network_access = false;
sandbox_config.filesystem_access = ModuleSandbox::FilesystemAccess::READ_ONLY;

loader.loadModuleSandboxed("themis_storage", sandbox_config);

// Monitor sandbox
auto stats = loader.getSandboxStats("themis_storage");
std::cout << "Memory: " << stats.memory_mb << "MB" << std::endl;
std::cout << "CPU: " << stats.cpu_percent << "%" << std::endl;
```

**Security Benefits:**
- Contain exploits in single module
- Prevent privilege escalation
- Limit blast radius of vulnerabilities
- Enforce least privilege

---

### Wire Protocol V2
**Priority:** High
**Target Version:** v1.7.0

Enhanced wire protocol with improved performance and features.

**New Features:**
- **HTTP/2-style multiplexing**: Multiple requests per connection
- **Server push**: Proactive data delivery
- **Header compression**: HPACK-style compression
- **Streaming**: Chunked transfer for large results
- **Priority**: Request prioritization
- **Flow control**: Backpressure handling

**Protocol Changes:**
```
V1: Request-response per frame
    Client → [Request]
    Server → [Response]
    (Repeat)

V2: Multiplexed streams
    Client → [Stream 1: Request]
    Client → [Stream 2: Request]
    Server → [Stream 2: Response]  (out of order)
    Server → [Stream 1: Response]
    Server → [Stream 3: Push]      (server push)
```

**Stream Frame Header:**
```cpp
struct WireFrameHeaderV2 {
    uint32_t magic;          // 0x544D4442 ("TMDB")
    uint8_t version;         // 0x02
    uint8_t opcode;          // Operation code
    uint16_t flags;          // Message flags
    uint32_t stream_id;      // Stream identifier (NEW)
    uint32_t payload_length; // Payload size
    uint8_t priority;        // Stream priority 0-255 (NEW)
    uint8_t reserved;        // Reserved for future use
};
```

**API:**
```cpp
WireProtocolServerV2 server(io_context, 9091);

// Enable multiplexing
server.setMaxConcurrentStreams(100);

// Enable server push
server.enableServerPush(true);

// Start server
server.start();
```

**Performance Improvements:**
- 5-10x lower latency for small requests (reduced round-trips)
- 2-3x higher throughput (connection reuse)
- Better resource utilization (fewer connections)

---

### License Server Integration
**Priority:** Medium
**Target Version:** v1.7.0

Online license validation and management.

**Features:**
- Online license activation
- Periodic license validation
- License leasing (checkout/checkin)
- Usage reporting
- Automatic renewal
- Grace period handling

**License Server Communication:**
```cpp
LicenseClient client("https://license.themisdb.com");

// Activate license
auto activation = client.activate("LICENSE-KEY-HERE");
if (activation.success) {
    std::cout << "License activated" << std::endl;
    std::cout << "Valid until: " << activation.expiry << std::endl;
}

// Validate license periodically
client.setValidationInterval(std::chrono::hours(24));
client.onValidationFailure([](const LicenseError& error) {
    std::cerr << "License validation failed: " << error.message << std::endl;
});

// Lease license (floating license)
auto lease = client.leaseLicense("themis_enterprise", std::chrono::hours(8));
// Use license...
lease.checkin();  // Return license to pool
```

**License Types:**
- **Perpetual**: One-time purchase, no expiry
- **Subscription**: Monthly/yearly renewals
- **Floating**: License pool, check out when needed
- **Node-locked**: Tied to specific hardware
- **Trial**: Time-limited evaluation

---

### Module Plugin API
**Priority:** High
**Target Version:** v1.7.0

Standardized plugin API for third-party extensions.

**Plugin Interface:**
```cpp
namespace themis::plugin {

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Plugin metadata
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual std::string getDescription() const = 0;

    // Lifecycle
    virtual bool initialize(PluginContext& context) = 0;
    virtual void shutdown() = 0;

    // Capabilities
    virtual std::vector<std::string> getProvidedServices() const = 0;
    virtual std::vector<std::string> getRequiredServices() const = 0;
};

// Storage plugin example
class IStoragePlugin : public IPlugin {
public:
    virtual bool put(string_view key, string_view value) = 0;
    virtual std::optional<std::string> get(string_view key) = 0;
    virtual bool remove(string_view key) = 0;
};

// Index plugin example
class IIndexPlugin : public IPlugin {
public:
    virtual bool insert(const Document& doc) = 0;
    virtual std::vector<Document> search(const Query& query) = 0;
};

} // namespace themis::plugin
```

**Plugin Registration:**
```cpp
// In plugin DLL
extern "C" {
    THEMIS_PLUGIN_API themis::plugin::IPlugin* createPlugin() {
        return new MyCustomStoragePlugin();
    }

    THEMIS_PLUGIN_API void destroyPlugin(themis::plugin::IPlugin* plugin) {
        delete plugin;
    }
}

// In main application
PluginLoader loader;
auto plugin = loader.loadPlugin("/path/to/plugin.dll");
if (plugin->initialize(context)) {
    // Plugin ready to use
}
```

**Plugin Marketplace:**
- Verified plugins signed by ThemisDB
- Community plugins with ratings/reviews
- Plugin documentation and examples
- Automated security scanning
- Version compatibility matrix

---

### Build-Time Optimization Selection
**Priority:** Low
**Target Version:** v1.8.0

Select optimizations at build time for specific use cases.

**Optimization Profiles:**
- **LATENCY**: Optimize for low latency (small queries)
- **THROUGHPUT**: Optimize for high throughput (bulk operations)
- **MEMORY**: Optimize for low memory usage
- **BALANCED**: Balanced optimization (default)

**CMake Configuration:**
```cmake
# Configure optimization profile
cmake -B build -DTHEMIS_OPTIMIZATION_PROFILE=LATENCY

# Profiles set different flags
# LATENCY: -O3 -march=native -mtune=native -flto
# THROUGHPUT: -O3 -march=native -funroll-loops
# MEMORY: -Os -ffunction-sections -fdata-sections
```

**Runtime Detection:**
```cpp
auto config = build_info::getBuildConfiguration();
std::cout << "Optimization: " << config.optimization_profile << std::endl;

if (config.optimization_profile == "LATENCY") {
    // Use latency-optimized code paths
}
```

---

## Wire Protocol Compression Algorithms
**Priority:** Medium
**Target Version:** v1.7.0

Additional compression algorithms for wire protocol.

**Current Support:**
- LZ4 (fast compression/decompression)

**Planned Algorithms:**
- **Zstd**: Better compression ratio than LZ4
- **Brotli**: Best compression for JSON/text
- **Snappy**: Google's compression (fast)
- **Dictionary compression**: Pre-trained dictionaries for repeated data

**Compression Selection:**
```cpp
// Client negotiates compression
HelloRequest hello;
hello.set_supported_compressions({
    CompressionType::LZ4,
    CompressionType::ZSTD,
    CompressionType::BROTLI
});

// Server selects best algorithm
auto selected = selectBestCompression(
    hello.supported_compressions(),
    server_config.preferred_compression
);

// Compress with selected algorithm
auto compressed = compress(payload, selected);
```

**Performance Comparison:**
```
Algorithm   | Ratio | Comp Speed | Decomp Speed | Use Case
------------|-------|------------|--------------|------------------
LZ4         | 2-3x  | 500 MB/s   | 2000 MB/s    | Default, fast
Zstd        | 3-5x  | 300 MB/s   | 800 MB/s     | Better compression
Brotli      | 5-7x  | 50 MB/s    | 300 MB/s     | JSON/text-heavy
Snappy      | 2x    | 400 MB/s   | 1500 MB/s    | Google ecosystem
Dictionary  | 10x+  | 200 MB/s   | 600 MB/s     | Repeated data
```

---

### Module Telemetry
**Priority:** High
**Target Version:** v1.7.0

Comprehensive telemetry for module health and performance.

**Metrics:**
- Module load time
- Memory usage per module
- CPU usage per module
- API call latency
- Error rates
- Crash statistics

**Integration:**
```cpp
ModuleTelemetry telemetry;

// Prometheus metrics
telemetry.exposePrometheus("/metrics");

// OpenTelemetry integration
telemetry.exportToOTel("localhost:4317");

// Query metrics
auto stats = telemetry.getModuleStats("themis_storage");
std::cout << "Load time: " << stats.load_time_ms << "ms" << std::endl;
std::cout << "Memory: " << stats.memory_mb << "MB" << std::endl;
std::cout << "Uptime: " << stats.uptime_seconds << "s" << std::endl;
std::cout << "Error rate: " << stats.error_rate << "%" << std::endl;

// Alerts
telemetry.onAlert([](const Alert& alert) {
    if (alert.severity == AlertSeverity::CRITICAL) {
        // Send to PagerDuty/Slack
    }
});
```

**Metrics Example:**
```
# HELP themis_module_load_time_seconds Time to load module
# TYPE themis_module_load_time_seconds gauge
themis_module_load_time_seconds{module="themis_storage"} 0.123

# HELP themis_module_memory_bytes Module memory usage
# TYPE themis_module_memory_bytes gauge
themis_module_memory_bytes{module="themis_storage"} 524288000

# HELP themis_module_errors_total Module error count
# TYPE themis_module_errors_total counter
themis_module_errors_total{module="themis_storage",type="io_error"} 5
```

---

## License Key Rotation
**Priority:** Medium
**Target Version:** v1.8.0

Support for rotating license keys without downtime.

**Features:**
- Multiple active licenses (old + new)
- Grace period for rotation
- Automatic key rollover
- Revocation list support

**API:**
```cpp
LicenseManager manager;

// Add new license
manager.addLicense("NEW-LICENSE-KEY");

// Both old and new licenses valid during grace period
auto status = manager.getLicenseStatus();
std::cout << "Active licenses: " << status.active_count << std::endl;

// Remove old license after grace period
manager.removeLicense("OLD-LICENSE-KEY");
```

---

### Module Capability Negotiation
**Priority:** Medium
**Target Version:** v1.7.0

Dynamic capability negotiation between modules.

**Problem:**
- Modules may support different feature sets
- Need runtime capability discovery
- Graceful degradation when features unavailable

**Solution:**
```cpp
ModuleCapabilities caps;

// Query module capabilities
auto storage_caps = caps.query("themis_storage");
if (storage_caps.supports("field_encryption")) {
    // Use field encryption
} else {
    // Fall back to basic storage
}

// Version-specific capabilities
if (storage_caps.version >= Version("1.7.0")) {
    // Use v1.7.0+ features
}

// Negotiate compatible feature set
auto compatible = caps.negotiate({
    "themis_storage",
    "themis_query",
    "themis_security"
});

for (const auto& feature : compatible.features) {
    std::cout << "Available: " << feature << std::endl;
}
```

---

## Backward Compatibility

### API Versioning
**Target Version:** v1.7.0

Versioned public API to ensure backward compatibility.

**Versioning Strategy:**
```cpp
namespace themis::v1 {
    // v1 API
}

namespace themis::v2 {
    // v2 API with new features
}

namespace themis {
    // Default to latest
    using namespace v2;
}
```

---

### Deprecation Policy

**Rules:**
1. Features deprecated in v1.x are removed in v2.0
2. Minimum 2 minor versions (6 months) deprecation period
3. Clear deprecation warnings in logs and documentation
4. Migration guides provided

---

## Security Enhancements

### Trusted Execution Environment (TEE)
**Priority:** Low
**Target Version:** v1.9.0

Support for Intel SGX and AMD SEV for module isolation.

**Features:**
- Modules run in secure enclaves
- Memory encryption
- Attestation support
- Sealed storage

---

### Module Signing with HSM
**Priority:** High
**Target Version:** v1.7.0

Sign modules using Hardware Security Modules.

**Benefits:**
- Private keys never leave HSM
- FIPS 140-2 compliant signing
- Audit trail for all signatures
- Key rotation support

---

## Performance Roadmap

### v1.7.0 Performance Targets
- <100ms module load time
- <10ms wire protocol latency (p99)
- Zero-copy serialization for wire protocol

### v1.8.0 Performance Targets
- <50ms module hot-reload
- <5ms wire protocol latency (p99)
- Multiplexed wire protocol (10x throughput)

---

## Contributing

Priority areas for contribution:

**High Priority:**
1. Build reproducibility
2. Wire protocol V2
3. Module telemetry
4. Module plugin API

**Medium Priority:**
1. Dynamic module hot-reload
2. License server integration
3. Module sandboxing

**Low Priority:**
1. Edition upgrade at runtime
2. TEE support

For detailed guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## Test Strategy

- Unit tests: `edition::IsFeatureEnabled` returns correct value for each edition tier
- Unit tests: `resolveDependencies` detects circular dependencies and missing dependencies
- Integration tests: wire protocol v2 negotiation handshake between client and server
- Regression tests: feature flag state is unchanged after all modules are loaded
- Negative tests: license with invalid signature is rejected; edition downgrade at runtime is rejected

## Performance Targets

- `edition::IsFeatureEnabled()` call latency: ≤ 100 ns (cache-friendly flag lookup)
- Wire protocol v2 version negotiation (full handshake): ≤ 1 ms per new connection
- Module dependency resolution (for ≤ 20 modules): ≤ 10 ms
- `build_info::getBuildConfiguration()` call: ≤ 1 µs (returns pre-computed struct)

## Security / Reliability

- Edition downgrade not allowed at runtime; `LicenseActivator::activate` rejects lower-tier licenses
- Feature flag overrides restricted to builds compiled with `THEMIS_DEV_BUILD`; blocked in release
- Module signing with HSM required for production module loads (verified via `IPlugin` signature)
- License keys stored in sealed storage; never logged or exposed via telemetry API
- Wire protocol v2 connections require TLS 1.3 minimum; plaintext negotiation is opt-in for localhost only

## See Also

- [README.md](README.md) - Current Themis module documentation
- [Base Interfaces Future Enhancements](base/FUTURE_ENHANCEMENTS.md)
- [Storage Module Future Enhancements](../storage/FUTURE_ENHANCEMENTS.md)
- [Architecture Roadmap](../../docs/ROADMAP.md)
