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
- [ROADMAP.md](ROADMAP.md) - Feature status and verification evidence

---

## Scientific References (IEEE Format)

The following references underpin the design and future enhancements of this module.

### Dependency Resolution

[1] C. Tucker, D. Shuffelton, R. Jhala, and S. Lerner, "OPIUM: Optimal Package Install/Uninstall Manager," in *Proc. 29th Int. Conf. Software Engineering (ICSE)*, Minneapolis, MN, USA, 2007, pp. 178–188.

[2] A. Abate, P. Bourdoncle, B. Durak, J. Vouillon, and R. Di Cosmo, "A formal study of the package dependency problem," in *Proc. 2012 Int. Conf. Software Engineering and Advanced Applications (SEAA)*, Cesme, Turkey, 2012, pp. 109–116.

[3] R. Di Cosmo, B. Durak, X. Leroy, F. Mancinelli, and J. Vouillon, "Maintaining Large Software Distributions: New Challenges from the FOSS Era," in *Proc. Workshop Future Trends Distributed Computing Systems (FTDCS)*, Suzhou, China, 2008, pp. 138–145.

### WebAssembly and Plugin Sandboxing

[4] A. Haas, A. Rossberg, D. L. Schuff, B. L. Titzer, M. Holman, D. Gohman, L. Wagner, A. Zakai, and J. Bastien, "Bringing the Web up to Speed with WebAssembly," in *Proc. 38th ACM SIGPLAN Conf. Programming Language Design and Implementation (PLDI)*, Barcelona, Spain, 2017, pp. 185–200.

[5] N. Narayan, S. Raychaudhuri, and V. Laxmi, "SandTrap: Securing JavaScript-driven Trigger-Action Platforms," in *Proc. 30th USENIX Security Symp.*, online, 2021, pp. 3753–3770.

[6] E. Wen and G. Weber, "Wasmachine: Bring the Power of SGX to Web," in *Proc. IEEE Security & Privacy Workshops (SPW)*, San Francisco, CA, USA, 2020, pp. 27–33.

### Ed25519 Signature Verification

[7] D. J. Bernstein, N. Duif, T. Lange, P. Schwabe, and B.-Y. Yang, "High-Speed High-Security Signatures," in *Proc. 13th Int. Workshop Cryptographic Hardware and Embedded Systems (CHES)*, Nara, Japan, 2011, vol. 6917, pp. 124–142.

[8] D. J. Bernstein and T. Lange, "SafeCurves: Choosing Safe Curves for Elliptic-Curve Cryptography," [Online]. Available: https://safecurves.cr.yp.to, accessed Mar. 2026.

### Hot Module Reload / Live Update

[9] G. Hayward and M. Ott, "Live Update for Fault-Tolerant Distributed Systems," in *Proc. 2019 IEEE 22nd Int. Symp. Real-Time Distributed Computing (ISORC)*, Valencia, Spain, 2019, pp. 27–34.

[10] K. Makris and K. Ryu, "Dynamic and Adaptive Updates of Non-Quiescent Subsystems in Commodity Operating System Kernels," in *Proc. 4th ACM SIGOPS/EuroSys European Conf. Computer Systems (EuroSys)*, Nuremberg, Germany, 2009, pp. 287–300.

### Plugin Marketplace Security and Key Pinning

[11] M. Georgiev, S. Iyengar, S. Jana, R. Anubhai, D. Boneh, and V. Shmatikov, "The Most Dangerous Code in the World: Validating SSL Certificates in Non-Browser Software," in *Proc. 2012 ACM Conf. Computer and Communications Security (CCS)*, Raleigh, NC, USA, 2012, pp. 38–49.

[12] C. Evans, C. Palmer, and R. Sleevi, "Public Key Pinning Extension for HTTP," RFC 7469, IETF, April 2015.

### Mutation Testing and Test Coverage

[13] R. A. DeMillo, R. J. Lipton, and F. G. Sayward, "Hints on Test Data Selection: Help for the Practicing Programmer," *IEEE Computer*, vol. 11, no. 4, pp. 34–41, Apr. 1978.

[14] Y. Jia and M. Harman, "An Analysis and Survey of the Development of Mutation Testing," *IEEE Trans. Software Engineering*, vol. 37, no. 5, pp. 649–678, Sep.–Oct. 2011.

---

*Last Updated: March 2026*  
*Module Version: v1.1.0*
