> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Acceleration Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Acceleration module manages loading and executing GPU compute kernels and backend plugins. Security concerns focus on: preventing execution of malicious or tampered plugins, safe driver interaction, kernel input validation, and avoiding resource exhaustion.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Malicious plugin loaded via `loadPlugin()` | `RTLD_NOW` for fail-fast symbol binding; file permission check rejects group/world-writable plugins; 128 MB file size cap |
| Tampered plugin binary | GPG signature verification via `posix_spawn`+`execv` (no shell injection); SHA-256 manifest check (`ModuleHashVerifier`) |
| macOS code signature bypass | Direct `SecStaticCodeCheckValidity` call (no shell invocation); previously used `popen`/shell |
| Shell injection via plugin path | All platform signature verifiers migrated from shell invocations to direct API calls or `posix_spawn`+`execv` |
| Resource exhaustion from large batch inputs | Null-pointer, zero-dimension, and batch size guards in all backends via `BatchValidator` |
| GPU memory starvation across tenants | Per-tenant VRAM quotas enforced at backend registry level; `MultiGPUVectorBackend` caps per-shard allocations |
| Untrusted kernel code execution | FNV-1a checksum kernel whitelist (validate-before-launch); WASM kernel sandbox path available for untrusted kernels |
| Regression from driver API changes | `BACKEND_CONTRACT_VERSION = 100` version pinning; API stability tests in `test_backend_api_stability.cpp` |

## Security Controls

### Plugin Loading
- `loadPlugin()` in `PluginLoader` enforces: file must exist, not be group/world-writable, be under 128 MB, and pass digital signature verification before any dynamic linking occurs.
- `RTLD_NOW` (Linux) is used instead of `RTLD_LAZY` to detect unresolved symbols at load time rather than at first call.
- TLS public-key pinning for remote plugin registry via `RegistryConfig::pinned_public_key` and `CURLOPT_PINNEDPUBLICKEY`.

### Kernel Input Validation
- All active backends (CUDA, HIP, CPU MT, CPU TBB, Vulkan) validate: non-null pointers, non-zero dimensions, positive batch count, and k-value clamped to dataset size before any kernel launch.
- Invalid geometry inputs (NaN coordinates, self-intersecting polygons) are rejected at the `GeoAccelerationBridge` layer.

### Signature Verification
- Linux: `posix_spawn`+`execv` calling `gpg --verify` (no shell expansion).
- macOS: `SecStaticCodeCheckValidity` from the Security framework (no shell invocation).

## Data Handling

- Vector embeddings (float32/fp16/bf16) are held in GPU device memory and not persisted by this module.
- No PII is processed directly; embeddings are assumed to be pre-transformed numeric representations.
- Multi-GPU sharding transfers data via NVLink/PCIe peer-to-peer; data does not leave the machine.

## Known Limitations

- WASM-based kernel sandbox (`wasm_plugin_sandbox.cpp`) requires a concrete WasmRuntime backend (Wasmtime or WasmEdge) injection before untrusted kernel execution is operational.
- `CUDAGraphBackend` for graph analytics (BFS, shortest-path) is still in stub state for GPU-accelerated graph traversal.
- Signature verification on Windows is not yet implemented; unsigned plugins are accepted on Windows in production mode.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| CUDA Toolkit | GPU kernel execution | Loaded dynamically; version pinned via `BACKEND_CONTRACT_VERSION` |
| ROCm/HIP | AMD GPU support | Loaded dynamically with non-HIP fallback stubs |
| Vulkan SDK | Cross-platform GPU compute | SPIR-V shaders pre-compiled; no runtime shader compilation |
| TBB (Intel) | CPU thread parallelism | System-provided; standard library dependency |
| libcurl | Remote plugin registry | `CURLOPT_PINNEDPUBLICKEY` enforced for registry TLS |
