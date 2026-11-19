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

# Chimera module gaps and evidence limits

**Purpose / audience:** current documentation-level gaps visible from headers, implementations, tests, and CMake. This is not a code-scanner report.

## Verified documentation gaps

- **Build boundary:** the feature defaults off; enabled wiring references the external CHIMERA checkout. Local adapter source files should not be presented as automatically built or production-enabled.
- **CMake source reconciliation:** the canonical adapter list adds the local ThemisDB implementation and external adapter sources, but does not list the local retry, batch, transaction, MongoDB, Neo4j, or Qdrant implementation files. The focused-test helper also appends local adapter source paths not present in the current `src/chimera/` inventory (including `adapter_factory.cpp` and several external-vendor adapter names). Whether those paths cause a configure/build failure was not tested.
- **Vendor support boundary:** MongoDB, Neo4j, and Qdrant code is conditional and partial. MongoDB batch and Neo4j graph capabilities are withheld when their drivers are not compiled in; MongoDB transaction handles and Neo4j transaction placeholders are not advertised as transaction support. Qdrant's generated gRPC operation stubs are absent, so it reports no vector or batch capabilities and returns `NOT_IMPLEMENTED` for insert, search, collection creation, and non-empty batch insert. The build configuration inspected does not establish that all three compile definitions or libraries are enabled.
- **Retry policy:** concrete error-code classification, exclusive explicit allowlists, default-off category retries, invalid-backoff rejection, and idempotency opt-in are implemented. The operation callable still has no enforced timeout/deadline or cancellation mechanism, so wall-clock duration is not bounded by the retry helper.
- **Capability contract:** the current external header lacks unique tokens for prepared statements and connection pooling. Their compile-time aliases are not emitted as supported capabilities. The local adapter also omits unsupported distributed/full-text/geospatial/time-series/secondary-index and ACID transaction capabilities.
- **Connection setup:** URI authority is validated and non-empty options return `UNSUPPORTED`, but a successful `connect()` still means only local execution state. Callers must not use it as remote reachability or TLS/authentication evidence.
- **Batch/transaction claims:** `batch_executor.cpp` is an anchor, while concrete queues are adapter-specific. Transaction context/state recording is not proof of distributed transaction coordination.
- **Evidence quality:** old maturity scores, LOC totals, broad gap counts, and "ready/validated" statements are not corroborated by the source inspected for this update and have been removed from the current status record.
- **Tests and performance:** source presence and CMake declarations are not test execution evidence. Retry and capability regression tests are present, but require a configured GTest build to produce pass evidence. Dedicated benchmark cases include stubs and must not be used as measurements of production routing.

## Follow-up needed

Confirm supported feature combinations and external dependency setup in a real configure/build review; run the focused tests and identify exactly which targets register; validate retry behavior with controlled failure tests; run live-backend benchmarks only with a documented environment and maintained baseline. These release-level actions remain outstanding.

The Developer LLM Wiki currently has no curated Chimera module page; the source-near module documents remain the available compiled context until one is added.

## References

`src/chimera/*.cpp`, `include/chimera/*.hpp`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `benchmarks/chimera/*.cpp`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
