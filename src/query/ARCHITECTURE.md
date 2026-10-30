---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/query/aql_parser.h"
  - "../../include/query/aql_runner.h"
  - "../../include/query/query_engine.h"
  - "../../include/query/query_optimizer.h"
  - "aql_runner.cpp"
  - "query_engine.cpp"
  - "query_federation.cpp"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Query Module Architecture

## Scope, boundaries, and ownership

`src/query/` owns AQL parsing, translation, planning/optimization, execution coordination, query functions, result handling, caching, and query-side federation. The `QueryEngine` receives storage and index abstractions; it does not own their persistence or index implementations. Network request admission is outside this module.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Parser and AST | [`AQLParser`](../../include/query/aql_parser.h), [`AQLParserService`](../../include/query/aql_parser_service.h) | Parse AQL into query/AST structures and report syntax or scope errors. |
| Dispatch and translation | [`executeAql`](../../include/query/aql_runner.h), [`AQLTranslator`](../../include/query/aql_translator.h) | Translate a parsed request and dispatch supported query or mutation work to an engine. |
| Planner and execution | [`QueryOptimizer`](../../include/query/query_optimizer.h), [`QueryEngine`](../../include/query/query_engine.h) | Estimate/reorder predicates and execute query operations through injected storage/index interfaces. |
| Runtime/result controls | [`QueryCancellationToken`](../../include/query/query_canceller.h), [`ResultIterator`](../../include/query/result_stream.h), [`QueryResourceLimits`](../../include/query/query_resource_limits.h) | Expose cancellation, result iteration, and caller-configured resource bounds. |
| Caches and federation | [`QueryCache`](../../include/query/query_cache.h), [`QueryFederation`](../../include/query/query_federation.h) | Cache eligible query work and route federated work through the shard-router interface. |

These are representative interfaces, not an exhaustive listing of query functions and specialized operators.

## Data flow

1. AQL input is parsed into query structures; SQL/SPARQL/Cypher/Gremlin compatibility parsers are separate frontends where enabled.
2. The runner translates and compiles the request, applies planning/optimization, and passes executable work to `QueryEngine`.
3. Execution consults storage/index interfaces and registered functions; runner/runtime paths also carry access-control, limits, and cancellation hooks.
4. Results are returned as JSON or through result-stream abstractions. Cache and federation paths are optional parts of this flow, not mandatory stages for every query.

The flow is visible in [`aql_runner.cpp`](aql_runner.cpp), [`query_engine.cpp`](query_engine.cpp), and [`query_federation.cpp`](query_federation.cpp).

## Failure domains and edge cases

- Parse, translation, or scope-validation errors prevent a valid executable request from being produced.
- A missing/invalid backend, denied access callback, resource limit, timeout, or cancellation can fail execution; these outcomes are not equivalent to an empty successful result.
- Federated requests depend on a usable shard router and remote execution path; remote failures are handled by federation-specific logic.
- Cache lookup, invalidation, and cache resource limits affect only the cache path; they do not make persistence part of query ownership.

## Explicit non-goals

- Durable key/value or blob storage and recovery (storage module).
- Index structure ownership and index maintenance (index module).
- HTTP/gRPC/WebSocket protocol handling or authentication at ingress.
- Owning the shard topology or distributed transaction coordinator.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Storage and indexes | `query_engine.cpp` includes storage and index interfaces; see [storage architecture](../storage/ARCHITECTURE.md) and `../../include/storage/storage_engine.h`. |
| Security | `aql_runner.cpp` includes row-level security/access-control interfaces; see [security architecture](../security/ARCHITECTURE.md). |
| Sharding | `query_federation.cpp` requires `sharding::ShardRouter`; see [sharding architecture](../sharding/ARCHITECTURE.md). |
| Observability | `query_optimizer.cpp` uses the metrics collector and query has profiler sources; see [observability architecture](../observability/ARCHITECTURE.md). |
| LLM | Query exposes LLM-related functions and LoRA integration headers; see [LLM architecture](../llm/ARCHITECTURE.md). |

## References

- [Module README](README.md) · [API notes](API.md) · [module roadmap](ROADMAP.md)
- [`aql_runner.cpp`](aql_runner.cpp) · [`query_engine.cpp`](query_engine.cpp) · [`query_optimizer.cpp`](query_optimizer.cpp)
