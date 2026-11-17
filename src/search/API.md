---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-11-17"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References:
  - "include/search/hybrid_search.h"
  - "include/search/distributed_hybrid_search.h"
  - "include/search/search_result_stream.h"
  - "include/search/layered_retrieval_orchestrator.h"
  - "include/search/search_error_codes.h"
  - "include/index/ann_frontdoor.h"
  - "include/index/vector_index.h"
  - "include/tensor/tensor_fingerprint_graph.h"
  - "include/themis/base/interfaces/index_interface.h"
  - "src/search/hybrid_search.cpp"
  - "src/index/index_manager.cpp"
  - "src/index/vector_index.cpp"
  - "src/index/ann_frontdoor.cpp"
  - "src/search/distributed_hybrid_search.cpp"
  - "src/search/layered_retrieval_orchestrator.cpp"
  - "tests/search/CMakeLists.txt"
Related_Issue: "#7489"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Search Module API Contract

This document summarizes the public search interfaces and the invariants they enforce. It is intentionally aligned to the source definitions in the `include/search` directory.

## Primary API surfaces

### HybridSearch

File: `include/search/hybrid_search.h`

`HybridSearch` is the canonical local hybrid retrieval API. It combines:

- BM25/full-text candidate generation
- vector search candidates
- rank fusion
- optional LLM reranking

The main entry points are:

- `HybridSearch::search(...)`
- `HybridSearch::reciprocalRankFusion(...)`
- `HybridSearch::setAnnFrontdoor(...)`
- `HybridSearch::setReranker(...)`

Contract highlights:

- constructor validates configuration and may throw `std::invalid_argument`
- `search()` is implemented as a fail-safe operation and does not propagate backend exceptions
- `SearchStats` declares source-health, partial-result, count, and degradation fields
- `Result` carries lexical, vector, and hybrid scoring information

**Current implementation detail:** `HybridSearch::search()` writes `bm25_ok`,
`vector_ok`, `partial_result`, `bm25_count`, and `vector_count`. The declared
`primary_error_code`, `fusion_failed`, and `rerank_fallback` fields are not
populated in the current `src/search/hybrid_search.cpp` path. The error enum
exists as a public taxonomy, but the inspected search implementation files do
not currently reference it. **TODO:** wire and test those diagnostics before
describing them as active runtime signals.

The request-aware overload accepts a `RequestContext` and `StopReason*`. It
propagates the context into BM25 retrieval and `VectorIndexManager` KNN, and
discards accumulated candidates when stopped. If an `AnnFrontdoor` or reranker
is configured, those calls are checkpoint-only: the caller checks before and
after the synchronous call, but cannot interrupt its in-flight backend work.
Fusion, sorting, and result materialization are also checkpoint-only.

### ANN retrieval and index fallback

The ANN frontdoor clamps results to the requested `k`, sorts by ascending
distance, and removes negative or non-finite distances. A backend that returns
too many candidates or invalid distances reports
`ANN_INVALID_BACKEND_RESULT`: valid candidates that remain are marked
`degraded_continue`; if none remain, the result is `fail_closed`. This differs
from an ordinary no-match response, where no backend contract violation or
fallback reason is present. If an invalid backend result and a distributed
failure occur together, the invalid-result code is primary and the other
fallback code is retained in `routing_reason`.

When a selected backend is unavailable, the frontdoor currently cannot execute
its nominal flat brute-force path: `bruteForceSearch()` logs that the fallback
is unavailable and returns no candidates. The result reports
`ANN_BACKEND_UNAVAILABLE` with `fail_closed`; the strategy name is not evidence
that a brute-force scan ran, and `estimated_recall` is zero when it did not.
In distributed retrieval, failed shards are
reported separately; successful shards can produce `degraded_continue` only
when partial results are enabled. If all attempted shards fail, retrieval
fails closed even when partial results are allowed because there are no
successful candidates to return.

`HybridSearch` may separately call `VectorIndexManager::searchKnn()` when the
frontdoor returns no candidates for a `FLAT_BRUTE_FORCE` or `HNSW` strategy.
That is a caller-level fallback, not an ANN frontdoor brute-force scan; the
current `HybridSearch::SearchStats` does not preserve the frontdoor's fallback
reason, and its frontdoor path marks `vector_ok` even if that separate fallback
fails. Treat those source-health fields as incomplete for this case.

### Request-aware KNN through `VectorIndexAdapter`

`IVectorIndex::search(query, k, filter, request)` returns a
`VectorSearchOutcome`. `VectorIndexAdapter` forwards that overload to
`VectorIndexManager::searchKnnEvaluated(..., request, ...)`; the existing
`search(query, k, filter)` and `VectorIndexManager::searchKnn(...)` signatures
remain unchanged. The legacy path does not acquire a request deadline or
cancellation source.

The request-aware flat search checks cancellation/deadline every 64 cache or
storage entries, and evaluator candidate filtering checks every 32 candidates.
An observed stop returns no results, even if candidates had already been
collected. `VectorSearchOutcome::stop_reason` distinguishes `Cancelled` and
`DeadlineExceeded` from a successful empty search; ordinary index errors remain
separate status failures and are not classified as request stops.

HNSW, ScaNN, DiskANN, and other synchronous ANN backend calls are
**checkpoint-only**: the request is checked immediately before and after the
call, and late results are discarded. The backend call itself is not
interruptible. Polling also cannot interrupt one in-progress storage read,
entity deserialization, distance calculation for one vector, or evaluator
invocation; these operations may overrun the deadline until they return.
Range-search, rotation, and attribute pre-filter entry points are not covered
by this KNN slice.

### DistributedHybridSearch

File: `include/search/distributed_hybrid_search.h`

This API extends the hybrid search path to multiple shards. It is intended for cluster or distributed query execution where shard failure and timeout handling matter.

Important invariants:

- local shard is evaluated first and remote shards are executed in bounded batches
- failures are tracked individually and aggregated in `SearchStats`
- partial results can be returned when some shards are unavailable and at least
  one shard succeeds; if every attempted shard fails, the result is fail-closed
- the input configuration requires a valid `search_endpoint` and positive shard-concurrency settings

Request-context overload (QRY-07 slices; not a completed QRY-07):

```cpp
std::vector<HybridSearch::Result> search(
    const std::string& text, const std::vector<float>& vec,
    const themis::query::RequestContext& ctx, SearchStats* stats = nullptr);
```

- `RequestContext` (`include/query/request_context.h`) carries an absolute
  deadline and a `QueryCancellationToken`. The legacy
  `search(text, vec, stats)` overload is unchanged and uses an unbounded context.
- The context is checked before the local shard and before each fan-out batch;
  after it stops, no further batch is launched and unstarted shards are counted
  in `SearchStats::shards_skipped`. Each remote call receives
  `ctx.child(shard_timeout_ms)`, i.e. min(remaining request time, shard timeout).
- Every launched shard task is joined before `search()` returns; nothing is
  detached. Completed shards are merged and reported via `partial_result`.
  `skip_failed_shards=false` returns empty if any shard failed or was skipped.
- `SearchStats` adds `shards_skipped`, `shards_timed_out` (shard context
  deadline), `shards_stage_timed_out` (transport stage timeout only),
  `shards_cancelled`, `cancelled`, `deadline_exceeded`. `deadline_exceeded` is
  true only when the caller's request deadline expired; per-shard budget and
  transport stage timeouts never set it; `ShardSearchResult::stop_reason` marks a
  shard that stopped on cancellation/deadline, so a stop is distinguishable from
  an empty or ordinarily failed shard.
- `setShardSearchFunction(ShardSearchFn)` replaces the default RemoteExecutor
  transport; the function must honour its context to stay cancellable.
- The local `HybridSearch::search` call receives the same context and records
  cancellation/deadline as a failed local shard; BM25 scans and manager-backed
  KNN poll cooperatively.
- **Not interruptible:** an in-flight ANN frontdoor/backend call, reranker,
  fusion/sort operation, custom shard function that ignores its context, and
  transport paths without a request-aware overload (see `src/sharding/API.md`).

### SearchResultStream

File: `include/search/search_result_stream.h`

This API provides cursor-style pagination over a result vector. `open()` calls `HybridSearch::search()` for `total_k` and stores that full vector; page-sized return values do not make underlying result materialization streaming or bounded to one page. The current header declares an `open_timeout_ms` setting; consult the implementation before treating it as cancellation of backend work.

### LayeredRetrievalOrchestrator

File: `include/search/layered_retrieval_orchestrator.h`

This retrieval chain executes the stages:

- ANN
- Tensor
- Graph
- LLM

The orchestrator records per-layer decisions and enforces guardrails.
`LayeredRetrievalContext::request` (a `query::RequestContext`, default
unbounded) carries a request deadline and client cancellation token. Layers run
inline on the caller thread (no detached workers). The context is checked before
every layer and after each backend call; a late result is discarded and reported
as `TIMEOUT_SKIP` or `CANCELLED` (distinct from `FALLBACK`, "ran, found
nothing"), with `LayeredRetrievalResult::cancelled` / `partial` set and earlier
layers' outputs retained. `AdvancedVectorIndex::search(q, k, ctx, &stop)` is
checkpoint-only.

HTTP `POST /vector/search` passes a server-derived `RequestContext` (deadline =
dispatch time + `request_timeout_ms`; `0` = unbounded; never from caller headers)
to `VectorIndexManager::searchKnn(query, k, nullptr, context, &stop_reason)` and
polls it while slicing/serializing results; a stop returns 504 with no results.
The HTTP session adds a borrowed peer-reset/error probe to this context. It is
observed at cooperative polling sites, and the session suppresses response
writing after a detected reset/error. An orderly TCP half-close is not treated
as cancellation because the client may still be waiting to receive the
response.

The tensor layer calls the request-aware
`TensorFingerprintGraph::findSimilar(query_key, k, request, stop_reason)`
overload. Candidate collection and boundaries between scores poll
cancellation/deadline and discard partial candidates. A single TT
inner-product, injected exact-similarity callback, or result sort remains
synchronous and checkpoint-only. Graph inference, LLM generation, and the ANN backend call
also remain synchronous/checkpoint-only and can overrun the deadline. Overall
timeout/cancellation enforcement is not complete; see `src/search/ROADMAP.md`
(QRY-07).

`SecondaryIndexManager::scanFulltextWithScores(..., RequestContext, stop_reason)`
provides cooperative BM25 full-text cancellation. It polls prefix-scan callbacks
and candidate/ranking loops every 64 entries and at stage boundaries; it stops
the current prefix iteration and clears partial results. The legacy overload
remains unbounded. The HTTP `POST /search/fulltext` endpoint derives its deadline
from the server's live `request_timeout_ms` (zero means unbounded) and returns
HTTP 504 for an observed stop. Synchronous RocksDB reads and sorting are
checkpoint-only; the session peer reset/error probe is observed at cooperative
poll sites. An orderly TCP half-close is not treated as cancellation.
A valid no-match query returns an empty success, distinct from
invalid index state (HTTP 400) or an observed deadline/cancellation.

`POST /search/fusion` also uses the server-configured request deadline. The
context is passed to the BM25 text stage and request-aware KNN vector stage,
then polled while candidates are fused and materialized. Any observed stop
returns HTTP 504 and discards partial fusion results; a valid empty fusion
remains HTTP 200. RocksDB reads, synchronous ANN backend calls, sorting, and
serialization are checkpoint-only. The session peer reset/error probe is
observed only at cooperative poll sites; the server does not write after a
detected reset/error. An orderly TCP half-close is not treated as cancellation.

## Error taxonomy

The module error taxonomy is defined in `include/search/search_error_codes.h`.

It is grouped into:

- retrieval errors (`0x0000-0x0FFF`)
- fusion errors (`0x1000-0x1FFF`)
- distributed merge errors (`0x2000-0x2FFF`)
- utility errors (`0x3000-0x3FFF`)
- analytics errors (`0x4000-0x4FFF`)

The string conversion helper `searchErrorCodeToString()` maps declared values
to strings. This declaration alone does not establish that the current
`HybridSearch`/`DistributedHybridSearch` paths populate or log those codes.

## Threading and safety expectations

Public documentation in the headers states that:

- a single `HybridSearch` instance is not thread-safe
- `LayeredRetrievalOrchestrator` requires safe backend wiring before concurrent execution
- the module favors caller-owned synchronization instead of hidden locks

This is an intentional design trade-off: the retrieval objects remain lightweight and predictable, while safety edges remain explicit.

## Typical usage pattern

A caller typically:

1. configure `HybridSearch::Config`
2. attach the lexical and vector backends
3. optionally attach an ANN frontdoor and LLM reranker
4. call `search()` with a text query and, if available, a vector query
5. inspect `SearchStats` for partial-result conditions
6. when using distributed search, handle shard anomalies and merge degradation explicitly

## Source-grounded references

- `include/search/hybrid_search.h`
- `include/search/search_error_codes.h`
- `include/search/layered_retrieval_orchestrator.h`
- `src/search/hybrid_search.cpp`
- `src/search/distributed_hybrid_search.cpp`
- `src/search/llm_reranker.cpp`
