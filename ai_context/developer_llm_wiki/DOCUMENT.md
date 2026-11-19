---
Author: "ThemisDB Team"
Created: "2026-11-19"
Last Updated: "2026-11-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/document/document_store.h; include/document/document_schema_evolution.h; include/document/document_manager.h; src/document/README.md; src/document/ROADMAP.md; tests/document/CMakeLists.txt; benchmarks/document/RELEASE_GATES.md"
---

> Exact model backend not verifiable from final document text alone.

# Document — Developer LLM Wiki

This page is secondary coding context. Public headers and the module-local source-of-truth documents define behavior; this summary does not establish release readiness.

## Runtime API and boundaries

- [`document_store.h`](../../include/document/document_store.h) defines `IDocumentStore` and the mutex-protected `InMemoryDocumentStore`. Empty IDs and duplicate inserts return errors; missing reads return a successful empty optional; missing updates return `ERR_DOC_NOT_FOUND`; removing an absent record succeeds. The interface does not guarantee persistence or prescribe thread-safety for external adapters.
- [`document_manager.h`](../../include/document/document_manager.h) provides CRUD orchestration and synchronous lifecycle dispatch. `afterDelete` runs even when removal fails. `createEncrypted()` stores the supplied JSON body as-is in the in-memory implementation; it is not an encryption operation.
- [`document_schema_evolution.h`](../../include/document/document_schema_evolution.h) uses `uint32_t` schema identifiers as registry keys, not as ordered migration versions. Registration accepts zero, rejects duplicates, and can be sealed. It does not migrate documents or check version compatibility.
- Schema validation requires an object body and returns `ERR_DOC_VALIDATION_ABORTED` for null, scalar, or array JSON. For object bodies, it checks registered required fields and JSON types; unknown fields are accepted, defaults are informational, and an unregistered version returns `ERR_DOC_SCHEMA_VERSION_NOT_FOUND`. Validation is explicit: CRUD and round-trip persistence do not invoke it automatically.
- [`round_trip_editor.cpp`](../../src/document/round_trip_editor.cpp) stores snapshots through the injected `IDocumentStore` and propagates store errors. It has no retry, fallback, degraded-mode, or recovery loop; those behaviors belong to callers/integrations and must be bounded and observable.
- The in-memory XDOMEA connector is not an XSD validator. The encrypted-entity helper records key-ID state but does not re-cipher document bytes. See the [module README](../../src/document/README.md), [security boundary](../../src/document/SECURITY.md), and [production requirements](../../src/document/PRODUCTION_REQUIREMENTS.md).

## Tests and build evidence

- [`tests/document/CMakeLists.txt`](../../tests/document/CMakeLists.txt) glob-registers six direct `test_document_*.cpp` focused unit targets with a 120-second timeout. The schema regression is in [`test_document_schema_hardening_focused.cpp`](../../tests/document/test_document_schema_hardening_focused.cpp); it covers structural invalid-input rejection, version registration/sealing, and field violations.
- The focused test build is registered, but registration is not execution evidence. The latest recorded local configure attempt failed before test generation because required development dependencies were unavailable (`RocksDB`, then `fmt` in diagnostic mode). See the [module audit](../../src/document/AUDIT.md) for the attempted commands and the reproducible focused CTest recipe.
- Do not infer success from the current branch's Actions status where a run has zero jobs.

## Benchmarks and release gates

- Three benchmark suites exercise the in-memory store, diff/merge, and round-trip paths. The serialization-gates executable uses synthetic string/vector work and is not production-path evidence.
- The latency-labelled `GATE-DOC` checks compare means; they do not calculate p95/p99. `DOC-BM-01..04` counters do not fail the run when thresholds are missed.
- Require a documented workload, build/host metadata, raw samples for claimed percentiles, at least five independent runs per selected workload, variation reporting, and evidence that the consuming CI/release gate ran. See [`PERFORMANCE_EXPECTATIONS.md`](../../src/document/PERFORMANCE_EXPECTATIONS.md) and benchmark [`RELEASE_GATES.md`](../../benchmarks/document/RELEASE_GATES.md).

## Versioning and source priority

- Root [`ROADMAP.md`](../../ROADMAP.md) and [`VERSIONING.md`](../../VERSIONING.md) are authoritative for the product rollout: v2.5.0 M1 RC targets 2027-03-31; v3.0.0 M2 Beta and M3 Single-Node GA target 2027-06-30, with M3 subject to human sign-off. These dates do not approve document-module scope or a breaking change.
- The module roadmap keeps v2.5.0 evidence work and v3.0.0 design/implementation conditional on owner decisions. Do not claim a durable backend, XSD/schema-draft support, migration semantics, encryption, or release readiness without source and execution evidence.
- For changes, consult the public headers and module [README](../../src/document/README.md), [architecture](../../src/document/ARCHITECTURE.md), [roadmap](../../src/document/ROADMAP.md), [audit](../../src/document/AUDIT.md), and [performance expectations](../../src/document/PERFORMANCE_EXPECTATIONS.md). Root [documentation governance](../../DOCUMENTATION_GOVERNANCE.md) and release documents outrank this summary.
