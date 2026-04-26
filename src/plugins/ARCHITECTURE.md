> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Plugins Module — Architecture Guide
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: src/plugins/README.md · src/plugins/ROADMAP.md · src/plugins/FUTURE_ENHANCEMENTS.md · docs/de/plugins/README.md -->

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/plugins/`

---

## 1. Overview

The Plugins module provides ThemisDB's extensibility infrastructure. It implements dynamic
shared library loading, manifest validation, Ed25519 signature verification, capability-based
permission enforcement, plugin lifecycle management, hot-plug monitoring, and a health monitor
for loaded plugins. Third-party functionality can be added without recompiling ThemisDB.

---

## 2. Design Principles

- **Manifest-Driven** – every plugin must include a signed manifest describing its
  capabilities, version, dependencies, and required permissions. The manifest is verified
  before any code is executed.
- **Capability-Based Permissions** – plugins declare the capabilities they require (e.g.,
  `STORAGE_READ`, `NETWORK_OUTBOUND`); the plugin manager grants only declared capabilities.
- **Ed25519 Signing** – plugin binaries are signed with Ed25519; the verifier checks the
  signature against the known publisher key before loading.
- **Hot-Plug** – `plugin_hot_plug_monitor.cpp` watches a plugin directory for new or
  updated plugins and triggers load/reload without server restart.
- **Health Monitoring** – `plugin_health_monitor.cpp` periodically checks loaded plugin
  health and unloads consistently failing plugins.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `plugin_registry.cpp` | Central registry of loaded plugins and their metadata |
| `plugin_manager.cpp` | Lifecycle orchestrator: load, init, execute, shutdown |
| `plugin_system_edition.cpp` | Edition-aware plugin limits and capability gates |
| `plugin_hot_plug_monitor.cpp` | Directory watcher for hot-plug install/update |
| `plugin_health_monitor.cpp` | Periodic health checks and auto-unload of failing plugins |
| `plugin_metrics.cpp` | Per-plugin call counts, latency, error rates (Prometheus) |
| `rpc_service_registry.cpp` | Registry of RPC services exposed by plugins |
| `huggingface_ingestion_plugin.cpp` | Built-in plugin: HuggingFace ingestion |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│            Plugin Directory (filesystem)                         │
│   my_plugin.so + my_plugin.manifest + my_plugin.sig             │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                PluginHotPlugMonitor (inotify)                    │
│                new/updated .so detected                         │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    PluginManager                                 │
│                                                                  │
│  1. ManifestValidator: parse + validate manifest schema         │
│  2. PluginSigner: verify Ed25519 signature                      │
│  3. CapabilityChecker: requested caps within allowed set?       │
│  4. base::ModuleLoader: dlopen/LoadLibrary                      │
│  5. plugin->initialize(ctx)                                     │
│  6. PluginRegistry: register handle + metadata                  │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  PluginHealthMonitor                             │
│   periodic health check → unload failing plugins                │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Plugin Load

```
plugin_hot_plug_monitor: new "analytics_plugin.so" detected
    │
    ▼
plugin_manager.load("analytics_plugin.so")
    │
    ├─ read analytics_plugin.manifest (JSON)
    ├─ manifest_validator: schema valid? version compatible?
    ├─ plugin_signer: verify Ed25519 signature(manifest + binary)
    ├─ capability_check: requested caps ⊆ allowed caps for edition?
    │       → capability denied → reject; log security event
    │
    ├─ base::ModuleLoader::loadModule("analytics_plugin.so")
    │       → dlopen → verify SHA-256 hash
    │
    ├─ plugin->initialize(PluginContext{storage, index, ...})
    │
    └─ plugin_registry.register(plugin_id, handle, metadata)
```

### 4.2 Plugin Execution

```
Caller: plugin_registry.get("analytics_plugin")
    │
    ├─ plugin_health_monitor: plugin healthy? → proceed
    │
    ├─ plugin.execute(request)
    │       ├─ plugin_metrics: record call + latency
    │       └─ result or error
    │
    └─ plugin_metrics: update success/failure counters
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/base/` | `ModuleLoader` for secure shared library loading |
| **Provides to** | All consumers | Plugin API via `plugin_registry.cpp` |
| **Used by** | `src/ingestion/` | HuggingFace ingestion plugin |
| **Used by** | `src/llm/` | LLM backend plugins |
| **Used by** | `src/acceleration/` | GPU backend plugins |

---

## 6. Threading & Concurrency Model

- `PluginRegistry` uses a read-write lock; lookups are concurrent, registration is exclusive.
- `PluginHotPlugMonitor` runs on a dedicated background thread.
- `PluginHealthMonitor` runs on a dedicated background thread.
- Plugin lifecycle operations (load/unload) hold the registry's exclusive lock.
- Plugin execution is thread-safe if the plugin declares it so in the manifest.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Registry read path | Lock-free read after initial load (read-heavy) |
| Lazy initialization | Plugins initialize on first use, not at startup |
| Metrics | Lock-free atomic counters per plugin |

---

## 8. Security Considerations

- Ed25519 signing prevents loading of tampered or unofficial plugins.
- Capability-based permissions sandbox what a plugin can access.
- Plugin signing key rotation is supported; old keys can be revoked via the keystore.
- Plugin health monitor auto-unloads consistently failing plugins to prevent cascading failures.
- WASM sandboxing is planned as an additional isolation layer.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `plugins.dir` | "plugins/" | Plugin installation directory |
| `plugins.require_signature` | true | Require Ed25519 signature |
| `plugins.hot_plug.enabled` | true | Enable directory watcher |
| `plugins.health_check.interval_s` | 60 | Health check interval |
| `plugins.health_check.failure_threshold` | 5 | Failures before auto-unload |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Manifest parse failure | Reject plugin; log error |
| Signature verification failure | Reject plugin; log security alert |
| Capability denied | Reject plugin; log with denied capabilities |
| Initialize failure | Unload; log error; mark as unavailable |
| Health check failure (repeated) | Auto-unload; log; alert operator |

---

## 11. Known Limitations & Future Work

- WASM sandboxing is planned for untrusted plugins (target v0.9.0).
- Plugin dependency management is implemented via `PluginDependencyResolver` (topological sort, cycle detection); see `include/plugins/plugin_dependency_resolver.h`.
- The `plugin_system_edition.cpp` edition gates are enforced at load time only; runtime capability escalation is not yet blocked programmatically.

---

## 12. References

- `src/plugins/README.md` — module overview
- `docs/plugins/PLUGIN_SECURITY.md` — security model
- `docs/plugins/MANIFEST_SIGNATURES.md` — signing guide
- `tools/plugin_signer/` — plugin signing tool
- `ARCHITECTURE.md` (root) — full system architecture
