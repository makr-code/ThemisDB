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

# Chimera test architecture

**Purpose / audience:** map the local tests to their code under test and state the build boundary. Tests are evidence only when executed; none were run for issue #7336.

## Test groups

- `test_themisdb_adapter.cpp`: connection, relational/vector/graph/document methods, transaction-related methods, capability/system information, and engine-injection guard cases. The source describes its ordinary fixture as in-process simulation.
- `test_chimera_streaming.cpp` and `test_chimera_prepared_statements.cpp`: streaming and prepared-statement surfaces on `ThemisDBAdapter`.
- `test_mongodb_adapter_phase2.cpp`: focused Mongo adapter edge cases; its source being present does not establish a registered or driver-enabled test.
- `test_chimera_highcardinality_stress.cpp`: exercises local router/fallback stubs, not the production adapter or router.
- `themis_adapter_fallback.cpp`: fallback implementation source used conditionally by the local CMake fragment; it is not itself a test case.

## Registration boundary

Root `tests/CMakeLists.txt` adds ThemisDB, streaming, and prepared-statement test executables inside `if(THEMIS_BUILD_CHIMERA)`. Other focused registrations search local and external test paths. That CMake file's shared adapter source list also names local `src/chimera/` paths absent from the current local inventory (for example `adapter_factory.cpp`, `postgresql_adapter.cpp`, and `weaviate_adapter.cpp`); no configure was run to determine the effect. `tests/chimera/CMakeLists.txt` contains a local glob/target recipe, but a repository search found no `add_subdirectory(chimera)` reference in the inspected CMake files; whether that fragment is reached is therefore not asserted here. The feature option defaults off.

## References

`tests/chimera/*.cpp`, `tests/chimera/CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakeLists.txt`, [`../../src/chimera/ARCHITECTURE.md`](../../src/chimera/ARCHITECTURE.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
