> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Base Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Base module manages the secure loading, execution, and lifecycle of plugins and modules. It is a critical security boundary: any weakness here could allow malicious code to be loaded into the ThemisDB process. Security concerns focus on plugin authentication, integrity, sandboxing, and safe remote registry access.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Malicious plugin injection | Digital signature verification required before any `dlopen`/`LoadLibrary` call; GPG (Linux/macOS) or platform verifier |
| Tampered plugin binary | SHA-256 manifest check (`ModuleHashVerifier`) validates file hash before loading |
| Unsigned plugin bypass | Development mode (`allowUnsigned`) is opt-in; production deployments require signed plugins |
| Remote registry MITM | TLS connection to registry; `CURLOPT_PINNEDPUBLICKEY` enforces SPKI pinning |
| Plugin supply-chain attack | Ed25519 application-layer key pinning in `SignedPluginRepository`; hash manifest prevents silent substitution |
| Privilege escalation via plugin | Resource limits (memory, CPU) enforced in `ModuleSandbox`; trust level governs capability grant |
| WASM untrusted kernel execution | `WasmPluginSandbox` infrastructure isolates untrusted WASM plugins; concrete WasmRuntime required for full isolation |
| Group/world-writable plugin bypass | File permission check in `PluginLoader::loadPlugin()` rejects writable files |
| Oversized plugin resource exhaustion | 128 MB file size cap in `loadPlugin()` prevents memory exhaustion during loading |
| Plugin crash cascading | Watchdog background thread isolates and auto-restarts failed plugins; crash does not propagate to database process |
| Dependency cycle causing deadlock | `ModuleDependencyResolver` performs cycle detection before load-order resolution; cyclic graphs are rejected |

## Security Controls

### Plugin Loading
- Platform-appropriate signature verification is performed before any dynamic loading:
  - Linux: `posix_spawn`+`execv` calling `gpg --verify` (no shell expansion).
  - macOS: `SecStaticCodeCheckValidity` Security framework call (no shell invocation).
  - Windows: code signing verification (implementation in progress).
- File integrity validated against SHA-256 hash manifest before load.
- `RTLD_NOW` used on Linux for fail-fast unresolved symbol detection.
- Files that are group/world-writable or larger than 128 MB are rejected.

### Remote Registry Security
- All registry communication uses TLS with SPKI pinning (`CURLOPT_PINNEDPUBLICKEY`).
- Downloaded plugin binaries are SHA-256 verified before installation.
- Ed25519 application-layer key pinning in `SignedPluginRepository` provides additional signing verification.

### Plugin Sandboxing
- `ModuleSandbox` enforces configurable memory and CPU resource limits per plugin.
- `WasmPluginSandbox` provides WASM isolation for untrusted third-party plugins; requires concrete WasmRuntime injection.
- Trust levels (TRUSTED, VERIFIED, UNTRUSTED) control which capabilities are granted.

### Hot-Reload Safety
- `HotReloadManager` performs signature and hash verification on newly loaded versions before swapping.
- A/B test manager (`ABTestManager`) can roll back to the previous module version on error.

## Data Handling

- Plugin binaries are loaded into process memory; no plugin data is persisted by this module.
- Audit trail entries (plugin name, operation, timestamp, trust level) are append-only in-memory records; exportable to persistent audit log.
- Remote registry credentials are injected via configuration; not logged or stored in the plugin manifest.

## Known Limitations

- Windows plugin signature verification is not yet implemented; unsigned plugins are accepted on Windows in production mode (tracked in the roadmap).
- WASM isolation (`WasmPluginSandbox`) requires a concrete WasmRuntime backend (Wasmtime or WasmEdge) registration before full isolation is active (Issue #1572).
- Integration tests for hot-reload and sandbox scenarios are in progress (Issue #1574).

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| libcurl | Remote registry HTTPS | SPKI pinning enforced via `CURLOPT_PINNEDPUBLICKEY` |
| OpenSSL | TLS for registry connections | System-provided; keep patched |
| GPG | Linux plugin signature verification | Must be available in PATH on production Linux hosts |
| macOS Security.framework | macOS plugin code signing | System framework; always available on macOS |
