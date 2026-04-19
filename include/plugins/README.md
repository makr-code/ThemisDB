<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: src/plugins/README.md · src/plugins/ROADMAP.md · src/plugins/ARCHITECTURE.md · src/plugins/FUTURE_ENHANCEMENTS.md · include/plugins/FUTURE_ENHANCEMENTS.md -->

# ThemisDB Plugins Module Headers

The Plugins module headers define the public interfaces for ThemisDB's plugin subsystem. These headers expose the complete plugin lifecycle (load, initialize, execute, hot-reload, unload), manifest validation, Ed25519 signing, capability negotiation, dependency resolution, health monitoring, per-plugin metrics, OCI registry integration, RPC endpoint registration, and WASM sandbox bridging without requiring consumers to depend on implementation details.

## Module Purpose

Provides a unified, cross-platform plugin architecture for ThemisDB that consolidates compute-backend loading (via `acceleration/plugin_loader.h`), HSM/PKCS#11 dynamic loading, and ZLUDA backend loading into a single coherent model. Third-party and first-party plugins declare their capabilities in a JSON Schema v2 manifest; the runtime enforces signing, capability isolation, and dependency ordering before any plugin code executes.

## Scope

**In Scope:**
- `IThemisPlugin` — base contract implemented by every plugin
- `PluginManager` — singleton lifecycle orchestrator: scan, load, init, hot-reload, unload, capability negotiation
- `PluginRegistry` — type-safe, `std::shared_mutex`-protected factory registry
- `PluginAPI` — convenience typed-access layer (`get<T>`, `getAll<T>`)
- `PluginDependencyResolver` — header-only topological sort / cycle detection
- `PluginHealthMonitor` — liveness probe loop with auto-restart
- `PluginHotPlugMonitor` — inotify/FSEvents/ReadDirectoryChangesW directory watcher
- `PluginMetrics` / `PluginMetricsCollector` — atomic per-plugin counters, P95/P99 latency, `IMetrics` sink
- `ISelfHealingPlugin` — autonomous recovery contract with checkpointing
- `WasmHostAPI` — WASM-to-IThemisPlugin C-ABI bridge (gated on `THEMIS_WASM_SUPPORT`)
- `SignedPluginRepository` — Ed25519 key-pinning repository for signed catalog entries
- `OciRegistryClient` — OCI-compatible remote plugin pull with signed manifest verification
- `RpcPlugin` / `RpcServiceRegistry` — plugins that expose gRPC / Thrift / JSON-RPC endpoints
- `manifest_schema_v2.json` — JSON Schema v2 for capability-aware manifests

**Out of Scope:**
- Plugin business logic (in individual plugin packages, e.g. `huggingface_ingestion_plugin.h`)
- WASM runtime instantiation (`src/plugins/wasm_plugin_loader.cpp` — requires Wasmtime/WasmEdge)
- Community marketplace REST client (planned — concrete implementation file will be introduced in the plugin source tree)
- Query execution (handled by the query module)
- Network transport (handled by the server module)

## Key Components

---

### Core Plugin Contract
**Location:** `plugin_interface.h`

Defines `IThemisPlugin`, the base interface that **every** plugin must implement, together with supporting types (`PluginType`, `PluginCapabilities`, `PluginVersionRange`, `PluginCapabilityRequirement`, `PluginManifest`, and `PluginCapabilityNegotiator`).

**Key Interface:**
```cpp
class IThemisPlugin {
public:
    virtual const char*         getName()        const = 0;
    virtual const char*         getVersion()     const = 0;
    virtual PluginType          getType()        const = 0;
    virtual PluginCapabilities  getCapabilities() const = 0;
    virtual bool  initialize(const char* config_json) = 0;
    virtual void  shutdown()                           = 0;
    virtual bool  isHealthy()                    const = 0;
};
```

**PluginType categories:**
```cpp
enum class PluginType {
    COMPUTE_BACKEND, BLOB_STORAGE, IMPORTER, EXPORTER,
    HSM_PROVIDER, EMBEDDING, LLM_BACKEND, CUSTOM
};
```

**Capability Negotiation:**
```cpp
// Runtime version-range matching — called by PluginManager::negotiateCapabilities()
PluginCapabilityNegotiator negotiator;
auto result = negotiator.negotiate(
    {"storage:read", "1.0.0", "2.0.0"},  // required
    plugin->getCapabilities()             // provided
);
```

**PluginManifest** — parsed from `plugin.json` at load time. Key fields:
```cpp
struct PluginManifest {
    std::string           name;
    std::string           version;
    PluginType            type;
    std::vector<std::string> dependencies;    // topological sort input
    std::vector<std::string> capabilities;    // validated at load time
    std::string           sha256;             // verified by wasm_plugin_loader
    std::string           runtime;            // "native" | "wasm"
    std::string           signature;          // Ed25519 signature (Base64)
};
```

**Thread-Safety:** `PluginCapabilityNegotiator` is stateless and safe to call concurrently.

---

### Plugin Manager
**Location:** `plugin_manager.h`

Singleton lifecycle orchestrator. Consolidates `acceleration::PluginLoader`, PKCS#11 dynamic loading, and ZLUDA dynamic loading under a single type-safe interface.

**Key Features:**
```cpp
class PluginManager {
public:
    static PluginManager& getInstance();

    // Discovery and loading
    std::vector<std::string> scanPluginDirectory(const std::string& dir);
    bool loadPlugin(const std::string& path);
    bool unloadPlugin(const std::string& name);
    bool reloadPlugin(const std::string& name);  // two-phase: verify then atomic swap

    // Auto-loading from directory
    int autoLoad(const std::string& dir);

    // Typed access
    template<typename T>
    T* getPlugin(const std::string& name);

    // Hot-plug configuration
    void enableHotPlug(const HotPlugConfig& config);
    void disableHotPlug();

    // Capability negotiation
    bool negotiateCapabilities(
        const std::string& plugin_name,
        const std::vector<PluginCapabilityRequirement>& requirements);

    // Reload listener (for testing / observability)
    void addReloadListener(PluginReloadListener listener);
};
```

**Hot-Reload Safety:** `reloadPlugin()` verifies the Ed25519 signature before the atomic swap so the old plugin continues serving requests during verification.

**Reload Phases:**
```cpp
enum class PluginReloadPhase {
    BEFORE_UNLOAD,  // about to unload old plugin
    AFTER_UNLOAD,   // old plugin unloaded
    AFTER_LOAD      // new plugin live
};
using PluginReloadListener = std::function<void(const std::string& name, PluginReloadPhase)>;
```

**Thread-Safety:** All public methods are protected by `std::mutex`.

---

### Generic Plugin Registry
**Location:** `plugin_registry.h`

Type-safe factory registry using type erasure. Upgraded from `std::mutex` to `std::shared_mutex` (v1.3.0): read-concurrent, write-exclusive.

**Key Features:**
```cpp
class PluginRegistry {
public:
    // Register a factory (exclusive lock)
    template<typename PluginInterface>
    static void registerFactory(
        const std::string& name,
        std::function<std::unique_ptr<PluginInterface>()> factory);

    // Create an instance (shared lock)
    template<typename PluginInterface>
    static std::unique_ptr<PluginInterface> create(const std::string& name);

    // Query (shared lock)
    template<typename PluginInterface>
    static bool hasPlugin(const std::string& name);

    template<typename PluginInterface>
    static std::vector<std::string> listPlugins();

    // Unregister (exclusive lock)
    template<typename PluginInterface>
    static void unregisterFactory(const std::string& name);

    // Testing only — clears all registrations
    static void clearRegistry();
};
```

**Example usage:**
```cpp
PluginRegistry::registerFactory<IBlobStorageBackend>(
    "s3_plugin",
    []() { return std::make_unique<S3BlobPlugin>(); }
);
auto s3 = PluginRegistry::create<IBlobStorageBackend>("s3_plugin");
```

**Thread-Safety:** `registerFactory`/`unregisterFactory`/`clearRegistry` use `std::unique_lock`; `create`/`hasPlugin`/`listPlugins` use `std::shared_lock`.

---

### Typed Plugin API
**Location:** `plugin_api.h`

Convenience layer over `PluginRegistry` that returns `nullptr` instead of throwing, simplifying graceful fallback patterns.

```cpp
class PluginAPI {
public:
    // Single plugin — returns nullptr on miss
    template<typename T>
    static std::unique_ptr<T> get(const std::string& name);

    // All plugins of a type
    template<typename T>
    static std::vector<std::unique_ptr<T>> getAll();
};
```

**Example:**
```cpp
auto s3 = PluginAPI::get<IBlobStorageBackend>("s3_plugin");
if (s3) { s3->write(key, data); }

auto importers = PluginAPI::getAll<IImporter>();
for (auto& imp : importers) { imp->import_data(source); }
```

---

### Plugin Dependency Resolver
**Location:** `plugin_dependency_resolver.h`

Header-only topological sort (Kahn's algorithm) with cycle detection. Stateless — all methods are `static`.

**Key Features:**
```cpp
class PluginDependencyResolver {
public:
    struct DependencyGraph {
        std::map<std::string, std::vector<std::string>> dependencies;   // fwd
        std::map<std::string, std::vector<std::string>> dependents;     // rev
    };

    // Build graph from any map-like container whose values have .manifest.dependencies
    template<typename MapType>
    static DependencyGraph buildGraph(const MapType& plugins);

    // Returns true + cycle path on cycle, false + empty on success
    static std::pair<bool, std::vector<std::string>>
    detectCycles(const DependencyGraph& graph);

    // Returns safe load order (dependencies first) or throws on cycle
    static std::vector<std::string>
    topologicalSort(const DependencyGraph& graph);
};
```

**Performance:** Dependency resolution for 100-plugin graph < 10 ms; memory < 1 MB for 500 plugins.

---

### Plugin Health Monitor
**Location:** `plugin_health_monitor.h`

Background thread that runs periodic liveness probes on all registered `ISelfHealingPlugin` instances. Triggers auto-restart on consecutive failures and emits `plugin_health_score` metric via `IMetrics`.

**Configuration:**
```cpp
struct HealthMonitorConfig {
    std::chrono::seconds check_interval{30};
    uint32_t             max_recovery_attempts = 3;
    std::string          backoff_strategy = "exponential";  // none | linear | exponential
    std::chrono::seconds initial_backoff{5};
    std::chrono::seconds max_backoff{300};
    bool                 auto_disable_on_failure = true;
    std::chrono::seconds health_check_timeout{10};
};
```

**Key API:**
```cpp
class PluginHealthMonitor {
public:
    explicit PluginHealthMonitor(PluginManager& manager,
                                  HealthMonitorConfig config = {});
    void start();
    void stop();
    void registerPlugin(const std::string& name, ISelfHealingPlugin* plugin);
    void attachMetrics(IMetrics* metrics);  // emits plugin_health_score gauge

    // Query current status
    PluginHealthStatus getStatus(const std::string& name) const;
};
```

**Thread-Safety:** All methods are thread-safe; the monitor runs on a dedicated background thread.

---

### Hot-Plug Monitor
**Location:** `plugin_hot_plug_monitor.h`

Platform-native filesystem watcher for automatic plugin discovery and reload without server restart.

**Platform backends:**
- Linux: `inotify`
- Windows: `ReadDirectoryChangesW`
- macOS: `FSEvents` / `kqueue`

**Configuration:**
```cpp
struct HotPlugConfig {
    bool enabled           = false;
    bool auto_load         = true;   // load newly-appeared .so/.dll
    bool auto_reload       = true;   // reload modified plugins
    bool auto_unload       = true;   // unload deleted plugins
    int  watch_interval_ms = 100;    // polling fallback interval
};
```

**File event types:**
```cpp
enum class FileEvent { CREATED, MODIFIED, DELETED };
```

**Key API:**
```cpp
class PluginHotPlugMonitor {
public:
    PluginHotPlugMonitor(const std::string& directory,
                          PluginManager& manager,
                          HotPlugConfig config = {});
    void start();
    void stop();
    bool isRunning() const;
    // Register callback for testing / observability
    void setEventCallback(std::function<void(const std::string&, FileEvent)> cb);
};
```

**TOCTOU Safety:** Signature re-verification occurs before every `dlopen` / WASM instantiation; the old plugin serves requests during verification.

---

### Plugin Metrics
**Location:** `plugin_metrics.h`

Atomic per-plugin telemetry with P95/P99 latency histograms. Integrates with any `IMetrics` sink (e.g. `PrometheusMetricsAdapter`) via `PluginMetricsCollector`.

**Per-plugin stats:**
```cpp
struct PluginStats {
    std::chrono::milliseconds load_time{0};
    std::chrono::milliseconds last_reload_time{0};
    uint64_t reload_count    = 0;
    uint64_t function_calls  = 0;
    uint64_t errors          = 0;
    size_t   memory_bytes    = 0;
    double   avg_call_latency_ms = 0.0;
    double   p95_call_latency_ms = 0.0;
    double   p99_call_latency_ms = 0.0;
};
```

**Key API:**
```cpp
class PluginMetrics {
public:
    void recordLoad(const std::string& plugin, std::chrono::milliseconds duration);
    void recordReload(const std::string& plugin, std::chrono::milliseconds duration);
    void recordCall(const std::string& plugin, std::chrono::milliseconds duration);
    void recordError(const std::string& plugin);
    void setMemoryUsage(const std::string& plugin, size_t bytes);

    PluginStats         getStats(const std::string& plugin) const;
    std::vector<PluginStats> getAllStats() const;
};

// Pushes all stats to an IMetrics sink (e.g. Prometheus)
class PluginMetricsCollector {
public:
    explicit PluginMetricsCollector(PluginMetrics& metrics);
    void collect(IMetrics& sink);  // emits labelled gauges via sink.setGauge()
};
```

**Performance:** Metrics scrape overhead < 0.5 ms per registered plugin; per-plugin histogram < 4 KB (HDR-style fixed-width buckets, max 1000 samples).

---

### Self-Healing Plugin Interface
**Location:** `self_healing_plugin.h`

Extends `IThemisPlugin` with autonomous error recovery, state checkpointing, and resource diagnostics. Used by `PluginHealthMonitor`.

**Health status:**
```cpp
enum class PluginHealthStatus {
    HEALTHY, DEGRADED, UNHEALTHY, CRITICAL, RECOVERING
};
```

**Diagnostics payload:**
```cpp
struct PluginDiagnostics {
    PluginHealthStatus status;
    std::string        error_message;
    uint64_t           error_count            = 0;
    uint64_t           recovery_attempts      = 0;
    uint64_t           successful_recoveries  = 0;
    size_t             memory_usage_bytes     = 0;
    double             cpu_usage_percent      = 0.0;
};
```

**Contract:**
```cpp
class ISelfHealingPlugin : public IThemisPlugin {
public:
    virtual PluginDiagnostics getDiagnostics() const = 0;
    virtual bool              attemptRecovery()      = 0;
    virtual bool              saveState(const std::string& path)    = 0;
    virtual bool              restoreState(const std::string& path) = 0;
    virtual void              cleanupResources()                    = 0;
};
```

---

### WASM Host API
**Location:** `wasm_host_api.h`

Maps the `IThemisPlugin` vtable to WASM C-ABI import functions. Requires `-DTHEMIS_WASM_SUPPORT` at compile time; without it, only `WasmPluginRuntime` enum and the `extern "C"` declarations are visible.

**Runtime selector:**
```cpp
enum class WasmPluginRuntime {
    NONE,      // native dlopen path
    WASMTIME,  // Bytecode Alliance
    WASMEDGE,  // CNCF
};
```

**WASM-side imports (`.wat` / C):**
```
(import "themis" "themis_plugin_get_name"     (func ...))
(import "themis" "themis_plugin_get_version"  (func ...))
(import "themis" "themis_plugin_initialize"   (func ...))
(import "themis" "themis_plugin_shutdown"     (func ...))
(import "themis" "themis_plugin_get_instance" (func ...))
(import "themis" "themis_plugin_save_state"   (func ...))
(import "themis" "themis_plugin_restore_state"(func ...))
```

**Host bridge (when `THEMIS_WASM_SUPPORT` is defined):**
```cpp
class WasmHostAPI {
public:
    explicit WasmHostAPI(IThemisPlugin* native_plugin);
    // Registers all host imports into the WASM linker
    void registerHostFunctions(WasmPluginRuntime runtime, void* linker);
};
```

**Security:** SHA-256 of the WASM binary is verified against the manifest `sha256` field before any WASM instantiation (in `wasm_plugin_loader.cpp`). Enterprise edition gate enforced by `plugin_system_edition.cpp`.

---

### Signed Plugin Repository
**Location:** `signed_plugin_repository.h`

Ed25519 key-pinning repository for plugin catalog entries. Prevents registry-substitution and MITM attacks by binding the repository to an administrator-controlled set of public keys.

**Key types:**
```cpp
struct PinnedKey {
    std::string          fingerprint;  // hex SHA-256 of 32-byte Ed25519 key
    std::vector<uint8_t> public_key;   // raw 32 bytes
    std::string          label;        // e.g. "ThemisDB Official Repository"
    bool                 active = true;
};

struct RepositoryEntry {
    MarketplaceManifest manifest;
    std::string         signature_b64;   // Base64 Ed25519 signature over manifest JSON
    std::string         signer_fingerprint;
};
```

**Key API:**
```cpp
class SignedPluginRepository {
public:
    void addPinnedKey(PinnedKey key);
    void removePinnedKey(const std::string& fingerprint);
    std::vector<PinnedKey> listPinnedKeys() const;

    bool addEntry(RepositoryEntry entry);     // verifies signature before inserting
    bool verifyEntry(const RepositoryEntry& entry) const;
    std::optional<RepositoryEntry> findEntry(const std::string& name,
                                              const std::string& version) const;
    std::vector<RepositoryEntry> listEntries() const;
};
```

---

### OCI Registry Client
**Location:** `oci_registry_client.h`

OCI-compatible remote plugin pull (Docker Hub, GHCR, or private registries) with signed manifest verification and key rotation support.

**Reference parsing:**
```cpp
struct OciReference {
    std::string registry;  // e.g. "ghcr.io"
    std::string name;      // e.g. "themisdb/plugins/s3_blob"
    std::string tag;       // e.g. "1.2.0"
    std::string digest;    // optional sha256:… pin

    static Result<OciReference> parse(const std::string& raw);
    std::string toString() const;
};
```

**Key API:**
```cpp
class OciRegistryClient {
public:
    explicit OciRegistryClient(OciAuthConfig auth = {});

    // Pull manifest + binary layer; verify SHA-256 and Ed25519 signature
    Result<std::string> pullPlugin(const std::string& reference,
                                    const std::string& dest_dir);

    // Low-level OCI API
    Result<std::string> fetchManifest(const OciReference& ref);
    Result<std::vector<uint8_t>> fetchLayer(const OciReference& ref,
                                             const std::string& digest);
};
```

---

### RPC Plugin Interface
**Location:** `rpc_plugin_interface.h`

Base types for plugins that expose gRPC / Thrift / JSON-RPC / MessagePack-RPC endpoints via `RpcServiceRegistry`.

**Supported protocols:**
```cpp
enum class RPCProtocol {
    GRPC, THRIFT, JSON_RPC, MSGPACK_RPC, WIRE_PROTOCOL, CUSTOM
};
```

**Key contracts:**
```cpp
class RpcPlugin : public IThemisPlugin {
public:
    virtual RPCProtocol          getProtocol()    const = 0;
    virtual RPCServerConfig      getServerConfig() const = 0;
    virtual std::vector<RPCServiceDefinition> getServices() const = 0;
    virtual bool startServer() = 0;
    virtual void stopServer()  = 0;
};

class RpcServiceRegistry {
public:
    static RpcServiceRegistry& getInstance();
    void registerService(const std::string& name, RpcPlugin* plugin);
    void unregisterService(const std::string& name);
    RpcPlugin* getService(const std::string& name) const;
    std::vector<std::string> listServices() const;
};
```

---

### Manifest Schema v2
**Location:** `manifest_schema_v2.json`

JSON Schema v2 that `plugin_registry.cpp` validates against before accepting a plugin manifest. Adds capability declarations, dependency versioning, `runtime` field (`"native"` | `"wasm"`), and `sha256` hash pinning on top of the v1 schema.

**Required fields:** `name`, `version`, `type`
**Optional fields:** `capabilities[]`, `dependencies[]`, `runtime`, `sha256`, `signature`, `min_host_version`

---

## Known Limitations

- `WasmHostAPI` bridges the host ABI but actual Wasmtime/WasmEdge instantiation (`wasm_plugin_loader.cpp`) contains placeholder TODO blocks pending Wasmtime linkage (Target: Q3 2027).
- Native plugins run in-process; memory corruption in a native plugin propagates to the host until the WASM sandbox is complete.
- Runtime capability escalation (post-load `getCapabilities()` returning a superset of the manifest-declared set) is not yet programmatically blocked (Target: Q4 2026).
- `PluginRegistry::clearRegistry()` is testing-only; calling it in production is unsupported.

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Phases 1–4 complete. All headers are stable from v1.x; new virtual methods to `IThemisPlugin` would require a major version bump.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
