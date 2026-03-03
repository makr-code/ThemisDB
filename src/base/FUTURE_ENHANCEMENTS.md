# Base Module - Future Enhancements

## Scope

- Plugin lifecycle management: discovery, loading, versioning, hot-reload, and unloading of shared-library plugins
- Secure module loading: path canonicalisation, allowlist enforcement, and OS-level permission checks before dlopen
- Digital signature verification: Ed25519 signatures on plugin binaries; reject unsigned or tampered plugins
- Plugin sandboxing: per-plugin resource limits (CPU, memory, file-system access) via OS namespaces/seccomp
- Plugin marketplace integration: discovery, dependency resolution, and automated update of community plugins
- Version migration support: state serialisation/deserialisation across plugin version upgrades

---

## Design Constraints

- `[ ]` Plugin load time (signature verify + dlopen + init hook) must be ≤ 200 ms per plugin on a warm filesystem
- `[ ]` Hot-reload must achieve zero-downtime: existing in-flight queries using the old plugin version complete before teardown
- `[ ]` Sandbox memory hard cap per plugin: 256 MB by default; configurable up to 2 GB
- `[ ]` Signature verification must use Ed25519 (RFC 8032); RSA-2048 not accepted for new plugins
- `[ ]` Plugin allowlist path checked on every load; symlink traversal outside the designated plugin directory is rejected
- `[ ]` Rollback of a failed hot-reload must complete within 500 ms and restore the previous plugin version atomically
- `[ ]` All lifecycle hooks (init, reload, shutdown) must complete within 5 s or are terminated and logged as failures

---

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `PluginLoader::load(path, manifest)` | Core module / plugin registry | Returns `PluginHandle` or structured error |
| `SignatureVerifier::verify(binary_path, sig_path, pubkey)` | `PluginLoader` | Ed25519; rejects on any mismatch |
| `HotReloadManager::reloadModule(name, new_path)` | Admin API / config watcher | Atomic swap; old handle kept until in-flight ops drain |
| `HotReloadManager::rollback(name)` | `HotReloadManager` error path | Must complete ≤ 500 ms |
| `PluginSandbox::createSandbox(plugin_id, limits)` | `PluginLoader` | OS namespace/seccomp; per-plugin resource policy |
| `MarketplaceClient::resolve(plugin_id, version)` | Plugin installer CLI / admin API | Returns download URL + signature; TLS required |

---

## Planned Features

### Hot Module Reload ✅ Implemented
**Priority:** Medium  
**Target Version:** v1.1.0  
**Status:** Implemented in `src/base/hot_reload_manager.cpp` (`include/themis/base/hot_reload_manager.h`)

Reload modules without restarting the database.

**Features:**
- Atomic module replacement
- State preservation across reloads
- Version migration support
- Rollback on failure
- Zero-downtime updates

**Implementation:**
```cpp
class HotReloadManager {
public:
    Result<bool> reloadModule(
        const std::string& module_name,
        const std::string& new_path
    );
    
    Result<bool> rollback(
        const std::string& module_name
    );
    
    Result<ModuleVersion> getCurrentVersion(
        const std::string& module_name
    );
};
```

---

### Plugin Marketplace Integration ✅ Partially Implemented
**Priority:** Low  
**Target Version:** v1.1.0  
**Status:** Partially implemented in `src/base/remote_registry_client.cpp` (`include/themis/base/remote_registry_client.h`); plugin discovery UI and automatic updates are still planned.

**Features:**
- Plugin discovery
- Automatic download and installation
- Dependency resolution
- Automatic updates
- User ratings and reviews

---

### Module Sandboxing ✅ Implemented
**Priority:** High  
**Target Version:** v1.1.0  
**Status:** Implemented in `src/base/module_sandbox.cpp` (`include/themis/base/module_sandbox.h`) and `src/base/wasm_plugin_sandbox.cpp` (`include/themis/base/wasm_plugin_sandbox.h`)

**Features:**
- Process isolation
- Resource limits (CPU, memory)
- Capability-based security
- IPC between sandbox and host
- Crash isolation

---

### Module Dependency Management ✅ Partially Implemented
**Priority:** Medium  
**Target Version:** v1.2.0  
**Status:** Partially implemented in `src/base/plugin_dependency_graph.cpp` (`include/themis/base/plugin_dependency_graph.h`); enforced ordered loading with version conflict resolution is in progress (Issue: #1566).

**Features:**
- Dependency declaration
- Automatic dependency resolution
- Version compatibility checking
- Circular dependency detection
- Lazy loading of dependencies

---

### Cross-Platform Module Format
**Priority:** Low  
**Target Version:** v1.4.0

Universal module format across platforms.

**Features:**
- Platform-independent packaging
- Automatic platform detection
- Native library bundling
- Resource embedding

---

## Test Strategy

- **Unit tests** (≥ 90 % line coverage): `PluginLoader` path-validation logic; `SignatureVerifier` with valid, tampered, and missing signatures; `HotReloadManager` state machine transitions
- **Integration tests**: load 10 real plugin binaries (including one with an invalid signature); verify hot-reload cycles complete without dropping in-flight queries; verify rollback restores functionality after a broken plugin
- **Sandbox tests**: attempt to exceed memory cap (256 MB) from within sandboxed plugin code; verify SIGKILL + structured error returned to caller within 500 ms
- **Fuzz tests** (libFuzzer): fuzz `PluginLoader` with malformed manifest JSON and adversarial binary paths (symlinks, null bytes, path traversal)
- **Marketplace mock tests**: dependency resolution with circular dependencies must return a clear error, never infinite loop
- **CI coverage gate**: line coverage ≥ 85 % enforced; sandbox tests run in an isolated container with seccomp enabled

## Performance Targets

- Plugin load (signature verify + dlopen + init): ≤ 200 ms per plugin on warm filesystem
- Hot-reload swap (old → new, no in-flight queries): ≤ 150 ms end-to-end
- Hot-reload rollback on failure: ≤ 500 ms to restore previous functional state
- Signature verification (Ed25519, 1 MB binary): ≤ 5 ms
- Sandbox creation (OS namespace setup): ≤ 50 ms per plugin
- Plugin discovery scan of a 500-plugin directory: ≤ 1 s

## Security / Reliability

- Ed25519 signature mandatory for all plugins; unsigned binaries are rejected before dlopen; public key pinned in server config
- Plugin paths canonicalised and restricted to the configured plugin root; symlink traversal outside root returns `EPERM`
- Sandboxed plugins run under seccomp-bpf allowlist: only `read`, `write`, `mmap`, and a declared set of syscalls permitted
- Plugin init/shutdown hooks killed via `SIGKILL` if they exceed 5 s timeout; crash reported as structured error, never propagated as C++ exception
- Marketplace downloads verified by TLS + Ed25519 signature before installation; SHA-256 checksum logged for audit trail
- All plugin load/unload/reload events written to immutable audit log with timestamp, plugin name, version, and outcome

---

## See Also

- [README.md](README.md) - Current module documentation

---

*Last Updated: March 2026*  
*Module Version: v1.0.0*
