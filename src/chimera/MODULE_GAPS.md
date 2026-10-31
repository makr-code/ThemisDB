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

# Chimera module gaps and evidence limits

**Purpose / audience:** current documentation-level gaps visible from headers, implementations, tests, and CMake. This is not a code-scanner report.

## Verified documentation gaps

- **Build boundary:** the feature defaults off; enabled wiring references the external CHIMERA checkout. Local adapter source files should not be presented as automatically built or production-enabled.
- **CMake source reconciliation:** the canonical adapter list adds the local ThemisDB implementation and external adapter sources, but does not list the local retry, batch, transaction, MongoDB, Neo4j, or Qdrant implementation files. The focused-test helper also appends local adapter source paths not present in the current `src/chimera/` inventory (including `adapter_factory.cpp` and several external-vendor adapter names). Whether those paths cause a configure/build failure was not tested.
- **Vendor support boundary:** MongoDB, Neo4j, and Qdrant code is conditional and partial. The build configuration inspected does not establish that all three compile definitions or libraries are enabled.
- **Retry policy:** `RetryExecutor::should_retry` does not inspect the concrete error code, despite the policy carrying retry categories/codes.
- **Batch/transaction claims:** `batch_executor.cpp` is an anchor, while concrete queues are adapter-specific. Transaction context/state recording is not proof of distributed transaction coordination.
- **Evidence quality:** old maturity scores, LOC totals, broad gap counts, and "ready/validated" statements are not corroborated by the source inspected for this update and have been removed from the current status record.
- **Tests and performance:** source presence and CMake declarations are not test execution evidence. Dedicated benchmark cases include stubs and must not be used as measurements of production routing.

## Follow-up needed

Confirm supported feature combinations and external dependency setup in a real configure/build review; run the focused tests and identify exactly which targets register; run benchmarks only with a documented environment and compare against a maintained baseline. Those actions were not performed for issue #7336.

## References

`src/chimera/*.cpp`, `include/chimera/*.hpp`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `benchmarks/chimera/*.cpp`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
