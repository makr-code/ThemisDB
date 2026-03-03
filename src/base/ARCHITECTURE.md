# Base Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/base/`

---

## 1. Overview

The `base` module provides the foundational plugin and module loading infrastructure for
ThemisDB. It enables secure dynamic loading of shared libraries (`.so`, `.dll`, `.dylib`) at
runtime, verifies their authenticity via digital signatures and SHA-256 hashes, and manages the
full plugin lifecycle (init → execute → shutdown). All other plugin-capable modules
(`src/acceleration/`, `src/plugins/`, `src/storage/`) build on top of this module's loader.

---

## 2. Design Principles

- **Security First** – every loaded module is verified (signature + hash) before execution in
  production; an explicit dev-mode flag allows unsigned modules during development.
- **Cross-Platform** – a single API abstracts `dlopen`/`dlsym` (Linux/macOS) and
  `LoadLibrary`/`GetProcAddress` (Windows).
- **Minimal Footprint** – the base module has no dependencies on higher-level ThemisDB
  modules; it only depends on system libraries and spdlog.
- **Trust Levels** – modules are classified as `TRUSTED`, `VERIFIED`, or `UNTRUSTED`,
  allowing callers to make trust-based decisions.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `module_loader.cpp` | Cross-platform shared library load/unload, capability query, per-plugin audit trail |
| `hot_reload_manager.cpp` | Zero-downtime hot-reload and rollback |
| `module_sandbox.cpp` | OS-level resource-limit sandbox and ABI compatibility checking |
| `wasm_plugin_sandbox.cpp` | WASM-based memory-safe isolation for untrusted plugin code |
| `remote_registry_client.cpp` | Authenticated download and installation of marketplace plugins |
| `plugin_dependency_graph.cpp` | Dependency declaration, visualization, and topological ordering |
| `ab_test_manager.cpp` | Traffic-split A/B testing via module swapping |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                  Module Consumer                                 │
│  (acceleration, plugins, storage, etc.)                         │
└──────────────────────────┬──────────────────────────────────────┘
                           │ loadModule(path)
┌──────────────────────────▼──────────────────────────────────────┐
│                    ModuleLoader                                  │
│  loadModule() → verify → dlopen/LoadLibrary → getInterface()    │
└──────────┬──────────────────────┬────────────────────────────────┘
           │                      │
┌──────────▼──────────┐  ┌────────▼──────────────────────────────┐
│  ModuleSecurityVerif│  │         Platform Abstraction           │
│  SHA-256 hash check │  │  Linux: dlopen/dlsym                  │
│  Signature verify   │  │  Windows: LoadLibrary/GetProcAddress  │
│  Revocation check   │  │  macOS: dlopen (Mach-O)               │
│  Trust level assign │  └───────────────────────────────────────┘
└─────────────────────┘
           │
┌──────────▼──────────┐
│  Plugin Interface   │
│  getCapabilities()  │
│  initialize()       │
│  execute()          │
│  shutdown()         │
└─────────────────────┘
```

---

## 4. Data Flow

```
Consumer: loadModule("my_plugin.so")
    │
    ▼
ModuleLoader:
    ├─ resolve absolute path
    ├─ check registry (already loaded?)
    ├─ ModuleSecurityVerifier:
    │       ├─ compute SHA-256(file) → compare with manifest hash
    │       ├─ verify digital signature (production mode)
    │       ├─ check certificate revocation
    │       └─ assign trust level: TRUSTED / VERIFIED / UNTRUSTED
    ├─ dlopen/LoadLibrary
    ├─ resolve getInterface() symbol
    └─ return ModuleHandle

Consumer: module->getInterface<IStoragePlugin>()
    │
    ▼
Cast to requested interface type → ready to use
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Provides to** | `src/acceleration/` | GPU backend plugin loading |
| **Provides to** | `src/plugins/` | Full plugin lifecycle management |
| **Provides to** | `src/storage/` | Custom storage backend loading |
| **Provides to** | `src/updates/` | Hot-reload infrastructure |
| **Uses** | System libraries | `dlopen`/`LoadLibrary` |

---

## 6. Threading & Concurrency Model

- `ModuleLoader::loadModule()` and `unloadModule()` are protected by an internal mutex.
- Once loaded, module interfaces can be called concurrently by worker threads.
- `hot_reload_manager.cpp` serializes hot-reload operations to prevent concurrent
  load/unload of the same module.
- Module handles use reference counting to prevent unloading while in use.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Module registry | Already-loaded modules returned immediately without re-verification |
| Lazy loading | Modules loaded on first access, not at startup (configurable) |
| Signature cache | Verification results cached to avoid repeat file reads |

Performance targets:
- Module load time: < 100 ms typical
- Signature verification: 10–50 ms per module
- Memory overhead: < 1 MB per loaded module

---

## 8. Security Considerations

- **Production mode**: `requireSignature = true`; modules with invalid or missing signatures
  are rejected; process terminates with fatal error if a critical module fails.
- **Development mode**: `allowUnsigned = true`; unsigned loads emit a warning log.
- **Revocation**: Certificate revocation is checked via OCSP or a local CRL.
- **Trust enforcement**: Callers can reject `UNTRUSTED` modules regardless of mode.
- **No code execution before verification**: signature check happens before `dlopen`.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `base.security.require_signature` | true (production) | Reject unsigned modules |
| `base.security.allow_unsigned` | false | Allow unsigned in dev mode |
| `base.security.min_trust_level` | TRUSTED | Minimum acceptable trust level |
| `base.hot_reload.enabled` | false | Enable hot-reload |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Signature verification failure | Reject module; log security alert |
| Hash mismatch | Reject module; log integrity error |
| `dlopen` failure | Return error with OS error message |
| Missing interface symbol | Return structured error |
| Revocation check failure | Fail closed (reject module) |

---

## 11. Known Limitations & Future Work

- Hot-reload (`hot_reload_manager.cpp`) is production-ready; the `HotReloadManager` provides atomic module swapping with rollback on failure.
- WASM-based plugin isolation is implemented in `wasm_plugin_sandbox.cpp`; a concrete WasmRuntime (Wasmtime, WasmEdge, etc.) must be injected for full execution support.
- Plugin dependency management and version conflict resolution are planned (Issue: #1566).

---

## 12. References

- `src/base/README.md` — module overview
- `src/base/FUTURE_ENHANCEMENTS.md` — roadmap
- `src/acceleration/plugin_security.cpp` — GPU plugin security policy
- `src/plugins/README.md` — high-level plugin system
- `ARCHITECTURE.md` (root) — full system architecture
