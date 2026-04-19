<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Acceleration Module Public Headers

**Module Path:** `include/acceleration/`
**Implementation Security:** `../../src/acceleration/SECURITY.md`

---

## Scope

Security considerations for the public header API surface of the acceleration module.
This document covers threats relevant to dynamic plugin loading, shader binary integrity,
and GPU resource isolation exposed through these headers.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Malicious compute plugin injection | Dynamic `PluginLoader` loading unsigned `.so` | `plugin_security.h` — allowlist + signature verification |
| Tampered shader binary | Modified SPIR-V/PTX loaded at runtime | `shader_integrity.h` — hash verification before dispatch |
| GPU memory exhaustion (DoS) | Unbounded kernel submissions | `vllm_resource_manager.h` — per-tenant GPU budget |
| Cross-tenant GPU memory read | Unsafe device buffer aliasing | `IComputeBackend` contract requires tenant-scoped allocations |
| Fallback bypass to weak backend | Attacker forces CPU-only path | `kernel_fallback_dispatcher.h` — policy-controlled fallback |
| Kernel parameter injection | Malformed `KernelInvocation` | `batch_validator.h` — shape and dtype validation pre-dispatch |

---

## Security Controls

### Plugin Integrity
`plugin_security.h` exposes `PluginSecurity::verify(path, expectedHash)` — plugins are
only loaded after signature and allowlist checks pass.

### Shader Integrity
`shader_integrity.h` exposes `ShaderIntegrity::verify(binary, algorithm, expectedHash)` —
SPIR-V and PTX binaries are hash-verified before being dispatched to the GPU driver.

### Resource Budgeting
`vllm_resource_manager.h` enforces per-request GPU memory budgets; submissions exceeding
the budget are rejected with `AccelErrorCode::RESOURCE_QUOTA_EXCEEDED`.

### Error Information Leakage
`error_context.h` redacts device-internal addresses and driver internals from errors
surfaced to API callers; full context is logged only to internal audit logs.

---

## Known Limitations

- GPU driver vulnerabilities are outside the scope of this module; operators must keep
  GPU drivers patched.
- `shader_integrity.h` hash verification is advisory for Vulkan compute paths when the
  Vulkan pipeline cache is used — operators should disable pipeline cache in high-security
  deployments.
- TLS enforcement for distributed multi-GPU coordination is operator-managed; see
  `../../src/acceleration/SECURITY.md` for deployment guidance.
