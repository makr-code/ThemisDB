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

# Chimera implementation architecture

**Purpose / audience:** source-grounded implementation map for Chimera maintainers. This is not a deployment or production-readiness guarantee.

## Components and boundaries

- Public contracts and adapter declarations are under [`include/chimera/`](../../include/chimera/); implementation is split across this directory. The shared `database_adapter.hpp` prefers the external CHIMERA header when present and otherwise supplies a deliberately minimal compatibility shim.
- `ThemisDBAdapter` implements the unified database adapter and async, streaming, and prepared-statement interfaces. It has a default in-process path and accepts optional query, vector-index, and graph-index pointers. Engine-specific dispatch is conditional on `THEMISDB_ENGINE_AVAILABLE`; the default CMake configuration does not enable the Chimera suite.
- `RetryExecutor` offers a retry wrapper and backoff calculation. Its current `should_retry` implementation does not classify the result's actual error code; do not rely on its policy names as proof of selective retry behavior.
- `batch_executor.cpp` is an anchor translation unit for the `IBatchAdapter` contract; it does not implement a general `BatchExecutor` or shared flush queue. Batching is adapter-specific.
- `TransactionContext` and `TransactionHandle` record local operation/state/savepoint metadata. Their presence does not establish distributed two-phase commit or backend ACID guarantees.
- Local MongoDB, Neo4j, and Qdrant adapter implementations are separate, partial integration paths. Driver operations are guarded by compile definitions (`THEMIS_CHIMERA_MONGO`, `THEMIS_CHIMERA_NEO4J`, `THEMIS_CHIMERA_QDRANT`); unavailable paths can return `NOT_IMPLEMENTED`. Do not infer that these drivers are enabled in the main build.

## Data flow and failure boundaries

1. A caller uses the adapter contract or factory; the ThemisDB translation unit registers `ThemisDBAdapter` when linked.
2. The adapter checks inputs and connection state, then executes its in-process behavior or attempts a supported injected-engine path.
3. Results and unsupported-path errors are returned through `Result<T>`. Vendor adapter paths additionally depend on their compile-time driver guard and external library.
4. Retry, batch, and transaction helpers are separate surfaces, not an implicit guarantee that every operation is retried, batched, or coordinated transactionally.

Connection and runtime state is held in adapter instances; persistence, complete backend parity, and production security configuration are not established by this module documentation.

## Build boundary

`THEMIS_BUILD_CHIMERA` defaults to `OFF` in root CMake. When enabled, `cmake/ChimeraAdapters.cmake` adds the external `external/chimera` adapter sources plus local `themisdb_adapter.cpp`; this is not equivalent to compiling every implementation in `src/chimera/`. In particular, that source list does not list the local retry, batch, transaction, MongoDB, Neo4j, or Qdrant `.cpp` files. The enabled path also depends on the external checkout. Tests and benchmarks have their own conditional wiring.

## Canonical evidence

- Headers: `include/chimera/{database_adapter,themisdb_adapter,retry_executor,batch_executor,transaction,mongodb_adapter,neo4j_adapter,qdrant_adapter}.hpp`
- Implementations: matching `src/chimera/*.cpp`
- Build/test/benchmark wiring: `CMakeLists.txt`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `benchmarks/CMakeLists.txt`
- Related: [`README.md`](README.md), [`ROADMAP.md`](ROADMAP.md), [`SECURITY.md`](SECURITY.md)

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
