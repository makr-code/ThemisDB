# Plugins Module - Future Header Enhancements

## Scope

- `IPlugin` interface extensions for lifecycle hooks and versioned capability negotiation
- Plugin manifest schema v2 API (`IPluginManifestV2`) with structured capability declarations
- Capability declaration interface (`ICapabilityDeclaration`) validated at load time
- WebAssembly sandbox interface (`IWASMSandbox`) for isolated plugin execution
- Plugin metrics hook interface (`IPluginMetricsHook`) for observable plugin behaviour
- Dependency resolution API (`IPluginDependencyResolver`) for inter-plugin dependency graphs

## Design Constraints

- `[ ]` All plugins must implement `IPlugin`; no plugin may be activated without passing the interface contract check
- `[ ]` Manifest schema v2 is backward-compatible with v1; existing plugins continue to load without modification
- `[ ]` WASM sandbox is opt-in and declared in the manifest; non-WASM plugins are unaffected
- `[ ]` Capability declarations are validated at load time and are **immutable** after the plugin is activated
- `[ ]` Metric hooks must be `noexcept`; any exception escaping a metric hook is a hard contract violation
- `[ ]` Dependency resolution is deterministic and cycle-free; circular dependencies are rejected at registration time

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IPlugin` | Plugin host, plugin loader | Base interface; all plugins derive from this; exposes `load()`, `unload()`, `getManifest()` |
| `IPluginManifestV2` | Plugin loader, capability validator | Extends v1 manifest; adds `capabilities()`, `dependencies()`, `sandboxMode()` |
| `ICapabilityDeclaration` | Capability validator, permission gate | Immutable after load; exposes `name()`, `version()`, `requires()` |
| `IWASMSandbox` | WASM plugin host | Opt-in; exposes `init()`, `invoke()`, `destroy()`; denies FS/network by default |
| `IPluginMetricsHook` | Metrics aggregator, observability layer | `noexcept`; exposes `onLoad()`, `onUnload()`, `onInvoke()`, `onError()` |
| `IPluginDependencyResolver` | Plugin registry | Resolves load order; exposes `resolve()`, `topoSort()`, `hasCycle()` |

## Planned Features

### WebAssembly Sandbox Plugin Interface

- `[ ]` Define `IWASMSandbox` header with `init(const WASMConfig&)`, `invoke(std::string_view fn, ByteSpan args)`, `destroy()` noexcept
- `[ ]` `WASMConfig` struct exposes `memoryLimitBytes`, `allowNetwork`, `allowFilesystem` (both default `false`)
- `[ ]` Sandbox interface is returned by `IPlugin::getSandbox()` returning `std::optional<IWASMSandbox*>`
- `[ ]` Header documents that `IWASMSandbox::invoke()` is thread-safe and re-entrant-safe

### Plugin Metrics Dashboard Hook

- `[ ]` Define `IPluginMetricsHook` with callbacks `onLoad(PluginId, std::chrono::nanoseconds loadTime) noexcept`
- `[ ]` Add `onInvoke(PluginId, std::string_view fn, std::chrono::nanoseconds duration) noexcept`
- `[ ]` Add `onError(PluginId, std::error_code) noexcept`
- `[ ]` `IPlugin` gains `registerMetricsHook(IPluginMetricsHook&)` — single hook slot, replaceable before activation only

### Manifest Schema v2 Capability Declarations

- `[ ]` Define `IPluginManifestV2` extending `IPluginManifest` with `capabilities() -> std::span<const ICapabilityDeclaration*>`
- `[ ]` `ICapabilityDeclaration` exposes `name() -> std::string_view`, `minVersion() -> Version`, `requires() -> std::span<std::string_view>`
- `[ ]` Manifest v2 includes `sandboxMode() -> SandboxMode` (enum: `None`, `WASM`)
- `[ ]` `IPluginManifestV2::isCompatibleWith(const IPluginManifestV2&)` returns `bool` for dependency compatibility checks

### Plugin Hot-Reload Interface

- `[ ]` Add `IPlugin::supportsHotReload() -> bool` with default `false`
- `[ ]` Define `IPluginHotReloader` with `reload(PluginId) -> std::future<ReloadResult>`
- `[ ]` `ReloadResult` carries `success`, `previousVersion`, `newVersion`, `reloadDurationNs`
- `[ ]` Hot-reload is rejected (returns error) if any capability declaration changed across versions

### Dependency Graph Query API

- `[ ]` Define `IPluginDependencyResolver` with `resolve(std::span<const PluginId>) -> DependencyGraph`
- `[ ]` `DependencyGraph` exposes `topoOrder() -> std::vector<PluginId>`, `hasCycle() -> bool`, `missingDeps() -> std::vector<PluginId>`
- `[ ]` Resolver is injected into the plugin host; header declares it `[[nodiscard]]` on `resolve()`
- `[ ]` All resolver methods are `const` and thread-safe

## Test Strategy

- Unit-test each interface contract via mock implementations verifying all `noexcept` callbacks never throw
- Integration tests load a WASM sandbox plugin and assert filesystem/network access is denied
- Manifest v2 backward-compatibility tests load a v1-manifested plugin under a v2 loader
- Dependency cycle detection tests feed circular graphs to `IPluginDependencyResolver` and expect structured errors
- Metrics hook tests inject a counting hook and assert `onLoad`/`onInvoke`/`onError` fire at correct times
- Hot-reload tests verify that changed capability declarations are rejected and unchanged ones succeed

## Performance Targets

- Plugin load (including manifest parse and capability validation): **≤ 500 ms**
- Capability check (`ICapabilityDeclaration` lookup): **≤ 100 ns**
- Manifest v2 parse from binary blob: **≤ 10 ms**
- WASM sandbox cold init (`IWASMSandbox::init()`): **≤ 200 ms**
- Metrics hook invocation overhead (`onInvoke`): **≤ 50 ns**
- Dependency graph topological sort (100 plugins): **≤ 1 ms**

## Security / Reliability

- Ed25519 plugin signature is verified against the manifest before any `IPlugin::load()` call proceeds; unsigned plugins are rejected
- `IWASMSandbox` denies filesystem and network access by default; explicit opt-in required via `WASMConfig` and must be declared in manifest
- Capability declarations are immutable after plugin activation; any attempt to mutate returns `std::errc::operation_not_permitted`
- Metric hooks cannot access plugin internals or raw invocation arguments; only timing and error codes are exposed
- `IPluginDependencyResolver` rejects plugins with unsatisfied or cyclic dependencies before any activation occurs
- Manifest schema v2 capability fields are validated against a sealed schema; unknown fields cause load rejection to prevent confused-deputy attacks
