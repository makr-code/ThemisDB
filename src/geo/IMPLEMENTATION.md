---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
Source_Level: "L1"
SOT_Domain: "build-test"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo implementation and validation

## Purpose and scope

This guide is for developers changing geo runtime code. It maps public
contracts to implementation files, explains compile-time backend selection,
and gives focused test entry points. The header contract is normative;
implementation details and tests provide evidence for specific code paths,
not an automatic guarantee that every contract constant is enforced by every
operation.

## Source layout and dispatch

The build is integrated from `src/geo/` by
default. The parent CMake can select the optional plugin source (default path
`plugins/themisdb_geo`, configurable via `THEMIS_GEO_PLUGIN_SUBMODULE_DIR`) with
`THEMIS_EXTERNALIZE_GEO_PLUGIN=ON` when
`THEMIS_PLUGIN_GEO` is enabled and the configured submodule has a
`CMakeLists.txt`; otherwise it retains the integrated sources. This guide and
the source mapping below describe the integrated implementation. The optional
plugin's API/test parity was not validated here.

[`CMakeLists.txt`](./CMakeLists.txt) lists geometry parsing, CPU execution,
indexing, joins, clustering, raster, temporal-spatial queries, tiles, policy,
hardening, and k-NN integration in the always-built core source list. The local
core list does not include `geo_operator_diagnostics.cpp` or
`geo_edge_case_handler.cpp`; check the parent target's source registration
before assuming either implementation is linked into a particular binary.
GPU dispatch sources are selected in this order:

1. `THEMIS_GEO_CUDA` selects `gpu_backend_cuda.cu`.
2. Otherwise `THEMIS_GEO_HIP` selects `gpu_backend_hip.cpp`.
3. Otherwise `gpu_kernel_dispatcher_cpu.cpp` supplies CPU-only dispatch.

`gpu_backend_stub.cpp` and `gpu_backend_production.cpp` are included with the
selected source. The build configuration and runtime device state are
separate: compiling a GPU path does not prove a usable device is present.
The dispatcher and backend wrapper route supported failures to CPU paths;
individual operations can still report unsupported or failed outcomes.

## Implementation invariants

- Validate geometry and coordinate expectations before invoking a compute
  backend. `GeoErrorCode` in
  [`geo_api_contract.h`](../../include/geo/geo_api_contract.h) is the shared
  error vocabulary for contract-level paths.
- Keep approximate MBR candidate filtering separate from exact refinement.
- Preserve per-API coordinate order and distance units; the interfaces include
  both lon/lat and lat/lon forms and both metres and kilometres.
- When adding a GPU kernel, keep the CPU result path available and test
  fallback behavior as well as successful dispatch.
- Keep resource policy distinct from authorization: the geo query policy
  describes limits and an advisory collection allowlist; the server boundary
  owns access control.
- For parser and geometry changes, cover malformed/empty input, invalid
  coordinate ranges, ring closure/winding, and unsupported geometry types.

## Focused validation

The module-focused CMake loop in
[`tests/geo/CMakeLists.txt`](../../tests/geo/CMakeLists.txt) generates one
focused executable and CTest entry per `test_*.cpp`. In an already configured
build tree with tests enabled, the contract fixture can be built and run with:

```sh
cmake --build <build-dir> --target module_geo_test_geo_contract_hardening_focused_focused
ctest --test-dir <build-dir> -R '^test_geo_contract_hardening_focused_GeoFocusedTests$' --output-on-failure
```

That test uses self-contained mock fixtures. For implementation changes, also
build and run the matching concrete suite (for example
`test_geo_json_geometry.cpp`, `test_geo_spatial_join.cpp`,
`test_geo_clustering.cpp`, or `test_geo_raster.cpp`) and any relevant
integration test. CUDA/HIP tests require the corresponding build lane and
device/runtime; a CPU-only pass is not GPU execution evidence.

The roadmap records a historical focused test pass and separately leaves
fresh current-cycle executable evidence open until the build environment has
the required dependencies. Do not report the historical result as a current
validation.

## Change checklist

1. Update `include/geo` contract comments or
   [`geo_api_contract.h`](../../include/geo/geo_api_contract.h) when behavior
   or public error semantics change.
2. Add a focused implementation test under `tests/geo`; use contract tests for
   frozen invariants, not as a substitute for the real implementation path.
3. Update the specific method, backend, or operating document linked from
   [`README.md`](./README.md); keep future work in `ROADMAP.md` or
   `FUTURE_ENHANCEMENTS.md`.
4. Run the focused test in the same backend configuration as the change and
   record any missing CUDA/HIP or external dependency as a limitation.

## Sourcecode verification

- Build selection: [`src/geo/CMakeLists.txt`](./CMakeLists.txt) and
  [`cmake/CMakeLists.txt`](../../cmake/CMakeLists.txt).
- Shared invariants: [`geo_api_contract.h`](../../include/geo/geo_api_contract.h).
- Focused test registration and contract fixtures:
  [`tests/geo/CMakeLists.txt`](../../tests/geo/CMakeLists.txt) and
  [`test_geo_contract_hardening_focused.cpp`](../../tests/geo/test_geo_contract_hardening_focused.cpp).
- Current test-evidence status: [`ROADMAP.md`](./ROADMAP.md).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
