---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera tests

**Purpose / audience:** inventory the checked-in focused test sources and explain their conditional registration.

## Local test sources

| File | Scope |
|---|---|
| `test_themisdb_adapter.cpp` | ThemisDB adapter API and selected engine-injection guard behavior; default fixture uses the in-process adapter path. |
| `test_chimera_streaming.cpp` | Result-stream and stream configuration behavior. |
| `test_chimera_prepared_statements.cpp` | Prepared statements, binding, execution, statistics, and escaping cases. |
| `test_mongodb_adapter_phase2.cpp` | MongoDB adapter edge cases, including unavailable/unsupported paths. |
| `test_chimera_highcardinality_stress.cpp` | High-volume/concurrency exercises over local router/fallback stubs; not production adapter coverage. |

`themis_adapter_fallback.cpp` is a conditional fallback implementation, not a test.

## Build and registration

`THEMIS_BUILD_CHIMERA` defaults to `OFF`. Under that option, root `tests/CMakeLists.txt` declares ThemisDB adapter, streaming, and prepared-statement focused executables. Other helper registrations seek local or `external/chimera` test sources. The helper also appends local adapter implementation paths absent from the current `src/chimera/` inventory; no configure was run to determine whether targets resolve successfully. The local `tests/chimera/CMakeLists.txt` describes a glob-based focused-test recipe, but the inspected CMake files do not show it being added via `add_subdirectory(chimera)`; registration through that fragment is uncertain.

### Quickstart

Configure with `-DTHEMIS_BUILD_CHIMERA=ON` and the required external checkout, then inspect the generated build/CTest inventory. If present, `ThemisDBAdapterFocusedTests` is a root-CMake target; build it with `cmake --build <build-dir> --target ThemisDBAdapterFocusedTests` and run the discovered test names with CTest. This guide does not claim that configure, target generation, or tests succeeded in the current environment.

## Running status

No build, test discovery, or test execution was performed for this update. Do not treat this inventory as pass evidence. Check the generated targets for the selected checkout/configuration before invoking a test.

## References

`tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, test sources above, [`../../src/chimera/README.md`](../../src/chimera/README.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
