# CROSS_MODULE_INTEGRATION

## Scope

Cross-module integration observations for `src/` with source-backed bottlenecks and boundary validation status.

## 1) Design Bottlenecks

### `http_server` aggregation hotspot

- `include/server/http_server.h` includes a very broad dependency surface (82 direct `#include` lines).
- Evidence: `include/server/http_server.h:37-136`
- Direct implications:
  - high rebuild fan-out on server header changes,
  - broad compile-time coupling across query/storage/transaction/LLM/security/metadata.

### Runtime composition concentration in `http_server.cpp`

- Large integration fan-in from storage/query/LLM/rag/security/transaction handlers.
- Evidence examples: `src/server/http_server.cpp:39-42`, `src/server/http_server.cpp:78-80`, `src/server/http_server.cpp:177-181`, `src/server/http_server.cpp:488-508`

## 2) Compiler Circular Dependency Analysis

Include-graph SCC analysis on `src/*` found:

- one large SCC with 41 modules (see `docs/architecture/MODULE_ARCHITECTURE.md`),
- representative bidirectional dependencies:
  - `llm <-> server`
  - `api <-> server`
  - `storage <-> transaction`
  - `storage <-> sharding`
  - `aql <-> query`
  - `auth <-> security`

Key evidence:
- `src/server/http_server.cpp:78-80` and `src/llm/mcp_tool_bridge.cpp:15`
- `include/transaction/distributed_transaction_manager.h:49-51` and `src/sharding/two_phase_commit_coordinator.cpp:31`

## 3) Module-Boundary Test Coverage

### Present cross-boundary tests

- `tests/cross/test_cross_shard_coordinator.cpp`
  - validates coordinator races, protocol behavior (2PC/3PC/SAGA/Percolator), deadlock policy and callback paths.
- `tests/cross/test_cross_shard_ssi.cpp`
  - validates SSI conflict detection and coordinator integration (`tests/cross/test_cross_shard_ssi.cpp:294-367`).

### Coverage interpretation

- Distributed transaction and sharding boundaries have explicit cross-module tests.
- Query/storage, server/llm, and rag/search/index/storage paths are heavily integrated in production code but are less represented by dedicated cross-module boundary suites in `tests/cross/`.

## 4) Optimization Suggestions

1. **Header decoupling for server ingress**
   - Replace broad includes in `include/server/http_server.h` with forward declarations + narrower translation-unit includes where possible.
2. **Contract-first dependency inversion for LLM/server coupling**
   - Isolate MCP and LLM endpoint wiring behind narrower interfaces to reduce `llm <-> server` bidirectionality.
3. **Boundary-focused integration tests**
   - Add focused cross-module tests for:
     - query->storage->transaction write path,
     - llm->rag->search->index->storage end-to-end path,
     - server<->llm fail-closed behavior under missing backend conditions.
4. **SCC reduction tracking**
   - Track mutual dependency pairs over time and set a wave-level target to reduce 41-module SCC size.
