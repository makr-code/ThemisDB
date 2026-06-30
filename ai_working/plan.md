## Kernel registry refactor plan

- Affected files:
  - `/home/runner/work/ThemisDB/ThemisDB/include/acceleration/compute_backend.h`
  - `/home/runner/work/ThemisDB/ThemisDB/src/acceleration/backend_registry.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/src/geo/gpu_backend_stub.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/tests/test_backend_registry_startup.cpp`

- Acceptance criteria:
  - Add a central kernel registry keyed by backend type for ANN, geo, and matrix launch tables.
  - Register existing CPU/CUDA/HIP/Vulkan/DirectX backend launch tables through `BackendRegistry::registerBackend()`.
  - Refactor geo GPU-driver dispatch-table bootstrap to resolve launchers through the registry instead of ad-hoc local tables.
  - Cover lookup success, missing-backend handling, and selected-backend consistency with focused tests.

- Test scope:
  - Re-run focused backend-registry and geo-dispatch tests if repository dependencies become available.
  - If configure remains blocked by missing toolchain prerequisites, record the exact limitation and rely on static review of the focused changes.
