---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-11-19
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
| `test_chimera_retry.cpp` | Concrete error classification, idempotency opt-in, invalid policies, retry exhaustion, and backoff cap. |
| `test_mongodb_adapter_phase2.cpp` | MongoDB adapter edge cases, including unavailable/unsupported paths. |
| `test_chimera_highcardinality_stress.cpp` | High-volume/concurrency exercises over local router/fallback stubs; not production adapter coverage. |

`themis_adapter_fallback.cpp` is a conditional fallback implementation, not a test.

## Build and registration

The root `tests/CMakeLists.txt` auto-discovers `tests/chimera/CMakeLists.txt` when the test suite is enabled and GTest is found. That fragment registers globbed `test_*.cpp` files as `module_chimera_<stem>_focused`; the retry test also compiles `src/chimera/retry_executor.cpp`. The root file separately declares additional adapter/stream/prepared targets under `THEMIS_BUILD_CHIMERA=ON`. Other CHIMERA helper registrations use local and external source paths; their resolution is configuration-dependent.

### Quickstart

Configure the project with `-DTHEMIS_BUILD_TESTS=ON`, then inspect generated targets with `cmake --build <build-dir> --target help` and registered tests with `ctest --test-dir <build-dir> -N`. The focused retry commands are `cmake --build <build-dir> --target module_chimera_test_chimera_retry_focused` and `ctest --test-dir <build-dir> -R test_chimera_retry_ChimeraFocusedTests --output-on-failure`. The full configure attempted for issue #7511 stopped at the missing required fmt package, before test-target generation.

## Running status

The changed retry and ThemisDB adapter translation units compiled with GNU 13.3.0 and C++20. Project configure did not complete because fmt was unavailable, so no focused GTest target was built or executed. Do not treat source presence or translation-unit compilation as test or release-pass evidence.

## References

`tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, test sources above, [`../../src/chimera/README.md`](../../src/chimera/README.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
