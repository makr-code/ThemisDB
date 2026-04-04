<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Geo Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Geo module public headers expose spatial computation, GPU dispatch, and clustering interfaces. Security concerns focus on coordinate validation, resource exhaustion via large geometry inputs, and GPU memory safety.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Invalid coordinate injection (NaN, ±Inf, out-of-range) | `ISpatialComputeBackend` implementations validate input coordinates before kernel dispatch |
| Resource exhaustion via oversized geometry batches | `SpatialBatchInputs` batch size capped at implementation layer; `GpuKernelDispatcher` enforces GPU memory limits |
| GPU memory overflow in CUDA kernels | `GeoDeviceDetector` queries available VRAM before dispatch; fallback to CPU on insufficient memory |
| Raster grid allocation overflow | `RasterGrid` dimensions validated against configurable maximum before allocation |
| Cross-tenant spatial data leakage | Spatial operations are tenant-scoped at the `IGeoRegistry` level |

## Security Controls

- Coordinate validation enforced before GPU kernel dispatch.
- GPU memory budgets checked before CUDA allocation via `GpuKernelDispatcher`.
- Tenant-scoped geo registry prevents cross-tenant data access.

## Known Limitations

- CUDA kernel floating-point determinism is hardware-dependent; results may differ between GPU generations.
- GPU memory exhaustion handling falls back to CPU; monitor `device_detector.h` capability flags.
- Implementation-level security details: `../../src/geo/SECURITY.md`.
