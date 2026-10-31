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

# Chimera capabilities (source profile)

**Purpose / audience:** concise engineering overview for developers assessing the checked-in Chimera surfaces. This is not a marketing or availability statement.

## Capabilities evidenced in source

- A ThemisDB adapter exposes relational, vector, graph, document, transaction, async, streaming, and prepared-statement operations through the CHIMERA adapter interfaces.
- The ThemisDB adapter has a default in-process path and optional injected query, vector-index, and graph-index dependencies; engine-backed work is conditional.
- Separate helpers define retry-policy/backoff, batch-operation, and transaction-context surfaces. Their actual behavior and limitations are documented in [`ARCHITECTURE.md`](ARCHITECTURE.md).
- Local MongoDB, Neo4j, and Qdrant adapter classes exist. Their backend paths are conditional on compile definitions and associated libraries; source presence is not proof of enabled or production-supported integration.

## Role and example scenario

These surfaces can be used by a developer evaluating a common adapter contract or exercising ThemisDB operations in an in-process test setup. A comparison harness or simulation is not an external-database interoperability result.

## Limitations

The root option `THEMIS_BUILD_CHIMERA` defaults to `OFF`. The enabled CMake adapter list references the external `external/chimera` checkout and includes the local ThemisDB adapter; it does not automatically make every local adapter a production-enabled feature. No runtime or performance validation is claimed.

## References

`include/chimera/*.hpp`, `src/chimera/*.cpp`, `cmake/ChimeraAdapters.cmake`, [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
