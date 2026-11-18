---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-17"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "DOCUMENTATION_GOVERNANCE.md"
  - "cmake/ModularBuild.cmake"
  - "tests/search/CMakeLists.txt"
  - "tests/CMakeLists.txt"
  - "benchmarks/search/CMakeLists.txt"
Related_Issue: "#7489"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Search Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

This roadmap contains dated historical delivery notes and forward planning.
Those entries are not current build, test-run, performance, maturity, or release
evidence. The search contracts are defined by the current public headers and
implementation; release status must be taken from root [`ROADMAP.md`](../../ROADMAP.md).

### Current source/build/test evidence (2026-10-31)

- `cmake/ModularBuild.cmake` includes a query-source list containing a subset
  of `src/search/*.cpp`; do not infer all 20 listed historically are in every
  target.
- `tests/search/` currently has 16 `test_*.cpp` files. Its own CMake file
  declares globbed focused targets, 120-second timeouts, and a default
  `search` label (with conditional labels for selected test stems).
  `tests/CMakeLists.txt` auto-discovers eligible module CMake files and adds
  `search` when tests are configured. Target compilation and execution are not
  verified here.
- The test source files and CMake declarations are not runtime evidence. No
  test pass is claimed by this roadmap.
- `benchmarks/search/CMakeLists.txt` declares seven local benchmark targets
  and two conditional CTest smoke registrations; root CMake separately declares
  the two `benchmark_*hybrid_search` targets. No benchmark run or threshold
  pass is asserted here.

### QRY-07 timeout and cancellation status (2026-11-17)

- [ ] **QRY-07 remains OPEN.** Multiple bounded slices are implemented; several
  backends and legacy transport paths are still not interruptible (see "Remaining gaps").

**HTTP peer-disconnect polling slice (2026-11-17)**
- `Session` and `SslSession` supply a borrowed native-socket cancellation probe
  to bounded `/vector/search`, `/search/fulltext`, `/search/fusion`,
  `/search/hybrid`, and `/query/aql` request contexts. The probe checks peer
  reset/error state at cooperative checkpoints. Orderly TCP half-close is not
  treated as cancellation because the client may still receive a response.
  When reset/error is observed, partial results are discarded and the session
  does not write a response to the failed peer.
- Detection latency is bounded by the next backend poll; checkpoint-only
  synchronous operations can still overrun. POSIX and Windows socket polling
  are implemented, but platform-specific behavior needs integration validation.
  The plain-HTTP abortive-close test waits for HTTP route dispatch and checks
  single-worker recovery; it is authored but not built or run.

**HTTP `/vector/search` deadline slice (2026-11-16)**
- `VectorApiHandler::handleSearch(req, RequestContext)` added (single-argument
  overload retained, unbounded). `HttpServer` route dispatch builds the context
  from live `request_timeout_ms` (deadline starts at handler dispatch; `0` =
  unbounded; caller headers never trusted) and the handler calls request-aware
  `VectorIndexManager::searchKnn`. A stop returns 504, no results; slicing and
  serialization are polled and partial output is discarded.
- The HTTP session now supplies a peer-disconnect probe to cooperative search
  checkpoints; this does not interrupt an in-flight ANN backend call.
- Tests: `tests/api/test_api_vector_search_deadline.cpp`.

**HTTP RoPE search deadline slice (2026-11-17)**
- `POST /api/v1/vector-index/{name}/rope/search` now receives the live server
  timeout and session peer-disconnect context. Request bodies are capped at
  2,000,000 bytes and JSON decoding is checkpoint-only within that bound. The
  handler polls while parsing large query vectors, then passes the context
  through RoPE query rotation to request-aware KNN. Deadline/cancellation returns
  HTTP 504 with partial results discarded; the legacy handler and manager
  overloads remain unbounded.
- Query rotation and synchronous ANN calls remain checkpoint-only. Cooperative
  cache/storage KNN scans poll; an observed client reset suppresses the response
  write. This slice does not establish immediate interruption of synchronous
  ANN work.
- Focused manager and handler tests cover stop propagation and expired-deadline
  status. Their build and execution remain pending.

**HTTP `/search/fulltext` deadline slice (2026-11-16)**
- `SecondaryIndexManager::scanFulltextWithScores(..., RequestContext,
  StopReason*)` preserves the legacy unbounded overload and polls BM25 prefix
  scans and candidate/ranking loops every 64 entries plus stage boundaries.
  Prefix iteration stops cooperatively; partial candidates/results are
  discarded with an explicit cancellation/deadline reason.
- `ContentApiHandler` receives the live server-configured
  `request_timeout_ms` at dispatch (`0` means unbounded); observed stops return
  HTTP 504. Valid no-match queries remain HTTP 200 with zero results; missing
  index remains HTTP 400. Result materialization polls and serialization has a
  post-operation checkpoint.
- Synchronous RocksDB reads, candidate sorting, and response serialization
  cannot be interrupted mid-call. Peer disconnect is observed at the next
  cooperative request-context poll; QRY-07 stays OPEN.
- Tests: `tests/api/test_api_fulltext_search_deadline.cpp` covers empty result,
  expired deadline, and a stop during an index-prefix scan. Test execution is
  not claimed until the focused target is built and run.

**HTTP `/search/fusion` deadline slice (2026-11-16)**
- `ContentApiHandler::handleFusionSearch(req, RequestContext)` preserves the
  unbounded legacy overload. The route builds its context from the live server
  timeout (`0` means unbounded); caller-supplied values cannot extend it.
- The same context is passed through BM25 and request-aware KNN. Fusion,
  top-k materialization, and response construction poll cooperatively; observed
  stops return HTTP 504 with partial results discarded. Valid empty fusion
  results remain HTTP 200.
- Synchronous RocksDB reads, ANN calls, and sorting are checkpoint-only.
  Peer disconnect is observed at the next cooperative request-context poll.
- Tests: `tests/api/test_api_fusion_search_deadline.cpp` covers empty results,
  an expired deadline, and cancellation during text retrieval. Execution is not
  claimed until the focused target is built and run.

**HTTP `/search/hybrid` deadline slice (2026-11-16)**
- `ContentApiHandler::handleHybridSearch(req, RequestContext)` and
  `ContentManager::searchWithExpansion(..., RequestContext, StopReason*)` retain
  their unbounded legacy overloads. `HttpServer` supplies the live configured
  request timeout (`0` means unbounded); a stop returns HTTP 504 and discards
  partial results, while a valid no-match result remains HTTP 200. The existing
  content-manager fallback also returns an empty result for an unavailable or
  uninitialized vector index, missing text processor, or base KNN status error;
  that HTTP 200 is a degraded legacy fallback, not proof of a successful search.
- The context reaches request-aware KNN and is polled during graph expansion,
  result filtering, and materialization. The built-in `TextProcessor` fallback
  also polls during tokenization and embedding; injected callbacks, vector ANN,
  graph neighbor/Dijkstra reads, whitelist scans, and sorting are checkpoint-only.
  Peer disconnect is observed at the next cooperative request-context poll.
- Tests: `tests/api/test_api_hybrid_search_deadline.cpp` covers empty results,
  an expired deadline, and client cancellation at dispatch. No test/build pass
  is claimed until the focused target is built and run.

**HTTP `/query/aql` full-text deadline slice (2026-11-16)**
- `QueryApiHandler::handleQueryAql(req, RequestContext)` receives the live
  server-configured timeout from `HttpServer`; `0` leaves the request unbounded.
  The existing body `timeout_ms` is an additional bound, not an extension, and
  retains HTTP 408 when it expires before the server deadline. Server deadline
  or cancellation returns HTTP 504 without partial output.
- The context reaches full-text BM25 scans, score lookups and sequential entity
  hydration. `QueryEngine` and handler paths discard stopped results. Valid
  no-match searches remain HTTP 200 with zero results.
- Full-text queries with structural predicates poll around equality/range index
  reads, index sorting, and during key-list intersection. The storage/index
  calls and sorts themselves are synchronous and only observed at checkpoints.
  Non-full-text range-aware AQL queries also pass the request context through
  key processing and sequential entity hydration, while individual range-index
  calls and sorting remain checkpoint-only.
  Optimized equality AQL queries use request-aware sequential index-stage
  checkpoints, key intersection and entity hydration; individual index calls
  and sorting remain synchronous. Optimizer planning polls before/after stats,
  each equality estimate, plan sorting and cost advice; individual estimate,
  sort, stats, cost and metrics operations cannot be interrupted.
  The non-full-text `allow_full_scan` fallback now polls during prefix
  collection, sequential predicate filtering, and entity hydration; this
  request-aware path avoids spawning worker tasks, and `allow_full_scan` does
  not bypass the indexed `FULLTEXT()` predicate. AQL parsing, indexed execution outside
  request-aware stages, result sorting, serialization, synchronous entity
  reads and any individual non-polling operation remain checkpoint-only. The
  HTTP session supplies peer-disconnect polling to this request context.
- Tests: `tests/api/test_api_aql_fulltext_deadline.cpp` covers indexed results,
  valid empty results, expired deadline, cancellation, and a stop during the
  full-text prefix scan, structural predicate execution, full-scan fallback,
  range-aware key processing, and optimized equality cancellation.
  Direct planner coverage verifies cancellation between equality estimates.
  Test/build execution is not claimed until verified.

**Core gRPC AQL request-context slice (2026-11-17)**
- `ThemisCoreServiceImpl::ExecuteAQL` and `StreamQuery` derive a cooperative
  request context from the gRPC server deadline and cancellation state, then
  invoke the request-aware `IQueryEngine::execute` overload with the existing
  bind-variable JSON. Cancellation maps to gRPC `CANCELLED`; deadline expiry
  maps to `DEADLINE_EXCEEDED`. Unary output is cleared on stop, and streaming
  returns on a stop or failed writer call.
- Engines that use the interface's default request-aware overload remain
  checkpoint-only: an in-flight legacy execution cannot be interrupted, and
  the RPC waits for it to return before discarding its result.
- Focused core-service RPC tests cover pre-cancellation, in-flight
  cancellation/deadline, late legacy results, and stream cleanup. Test/build
  execution remains unverified until run with the gRPC-enabled CMake target.

**Implemented slice (2026-11-16)**
- `include/query/request_context.h`: shared `query::RequestContext`
  (absolute deadline + `QueryCancellationToken`), `StopReason`
  {None, Cancelled, DeadlineExceeded}, `child(timeout)` clamping.
- `LayeredRetrievalOrchestrator` no longer detaches worker threads. Layers run
  inline on the caller thread; nothing runs after `execute()` returns.
  `LayeredRetrievalContext::request` carries deadline/token (default:
  unbounded, source compatible). Request stop is checked before each layer and
  after each backend call; late results are discarded.
- Distinct outcomes: `CANCELLED` (new), `TIMEOUT_SKIP`, and `FALLBACK`
  ("ran, found nothing"). `LayeredRetrievalResult` gains `cancelled` and
  `partial`; earlier-layer outputs are retained when later layers are skipped.
- `AdvancedVectorIndex::search(q, k, ctx, &stop)` (checkpoint-only: pre/post
  check around one non-interruptible backend call).
- LLM layer derives `GenerationOptions.timeout_ms` from the remaining budget.
- Tests: `tests/query/test_request_context.cpp`; five new cases in
  `tests/search/test_layered_retrieval_integration_phase4.cpp` (pre-cancel,
  mid-LLM cancel with partial retention, expired deadline vs empty result,
  no work after `execute()`, ANN stop-vs-empty). Verified locally with an ad-hoc
  g++ build and test-local link shims (full CMake/vcpkg build not run).

**Behavior trade-off:** because workers are gone, a backend that ignores the
context (tensor, graph, LLM, and the ANN call itself) can make `execute()`
return later than the deadline by that backend's overrun; the caller wait is
no longer hard-bounded. The previous `EnforcesHardDeadlineAndMarksTimeoutSkip`
now measures overrun classification, not a hard wall-clock bound.

**Slice 2 (2026-11-16): distributed fan-out + mTLS transport**
- `DistributedHybridSearch::search(text, vec, ctx, &stats)` (legacy overload
  unchanged). Context is checked before the local shard and before every
  fan-out batch; once stopped, no further batch is launched
  (`SearchStats::shards_skipped`). Each shard call receives
  `ctx.child(shard_timeout_ms)`. All launched tasks are joined before return;
  the `wait_for`-then-abandon path is gone. New stats distinguish
  `shards_timed_out`, `shards_stage_timed_out`, `shards_cancelled`, `cancelled`, `deadline_exceeded` from
  ordinary failures and from empty results; completed shards stay merged and
  flagged `partial_result`. `setShardSearchFunction()` allows an alternative
  transport / deterministic tests.
- `MTLSClient::get/post(..., const RequestContext&)`: resolve, connect, TLS
  handshake, write, read run as async Boost.Asio operations on a private
  per-call `io_context` driven by the calling thread. A 10 ms poll timer on that
  executor evaluates the token, the request deadline and the per-stage timeout
  (`connect_timeout_ms` / `request_timeout_ms`) and aborts the pending operation
  (resolver cancel + socket close); `run()` returns only after all handlers
  finished, so no thread/socket/timer outlives the call. Responses carry
  `cancelled` / `timed_out` (context deadline) / `stage_timed_out` (transport stage timeout, kept distinct; `SearchStats::deadline_exceeded` reflects the parent request deadline only). Stage timeouts are not retried; retry back-off
  honours the context.
- `RemoteExecutor::get/post(..., ctx)` propagate the context; a client
  cancellation is not recorded as circuit-breaker failure, a timeout is.
- Finding: the legacy sync path calls `tcp_stream::expires_after()` around
  blocking `connect/handshake/write/read`; Beast timeouts only apply to async
  operations, so those configured timeouts are not enforced on the legacy
  overloads. They are unchanged here.
- Tests: `tests/search/test_distributed_request_context.cpp` (10 cases: budget
  clamping, pre-cancel launches nothing, cancel between batches, strict mode,
  in-flight cooperative shard + join/cleanup, timeout vs empty, ordinary partial
  failure, legacy overload) and `tests/sharding/test_mtls_client_cancellation.cpp`
  (6 cases against a real black-hole TCP listener: token cancel and request
  deadline abort a hanging TLS handshake in ~100-150 ms vs 5 s/30 s configured
  timeouts, stage timeout, pre-cancel, refused connection is neither
  cancelled nor timed out, RemoteExecutor propagation; registered only when
  CMake `UNIX` is true because the test uses POSIX sockets). Built ad hoc with g++
  and link shims; both suites also ran clean under ThreadSanitizer. Not yet run
  through the CMake/ctest build.

**Slice 3 (2026-11-16): gRPC vector and hybrid search entry points**
- `VectorSearch`, `FilteredVectorSearch`, and `HybridSearch` derive a
  `query::RequestContext` from the synchronous gRPC server context. The
  context maps the gRPC wall-clock deadline to a steady-clock deadline and
  polls `ServerContext::IsCancelled()` directly; no detached work or watcher
  threads are created.
- These routes call the cancellation-aware `IVectorIndex::search` overload.
  Its source-compatible default checks before and after the legacy call,
  discards late results, and reports cancellation separately from deadline
  expiry. Existing index implementations remain checkpoint-only; in-flight
  interruption requires overriding the new overload and polling the context.
- An observed stop returns gRPC `CANCELLED` or `DEADLINE_EXCEEDED` and clears
  response hits rather than returning a successful empty result. Hybrid AQL
  work remains checkpoint-only.
- Focused API tests cover legacy-index pre-cancellation and expired deadlines,
  cooperative index polling, and gRPC route status behavior when gRPC service
  test dependencies are available. No build/test pass is claimed here.

**Slice 4 (2026-11-16): request-aware vector adapter and KNN scan**
- `VectorIndexAdapter` overrides `IVectorIndex::search(..., request)` and
  forwards the request to new `VectorIndexManager::searchKnn` and
  `searchKnnEvaluated` overloads. Existing signatures and the unbounded legacy
  behavior remain unchanged; `VectorSearchOutcome::stop_reason` distinguishes
  cancellation and deadline from empty results and ordinary index errors.
- Brute-force cache, whitelist, and RocksDB prefix scans poll every 64 entries;
  evaluator candidate filtering polls every 32 candidates. An observed stop
  discards all accumulated candidates and returns an empty result.
- HNSW, ScaNN, DiskANN, and other synchronous ANN backend calls are
  checkpoint-only: check before and after each backend call and discard late
  results. They are not interrupted while running.
- Focused deterministic coverage was added for cancellation during a large
  cache scan and evaluator candidate filtering, plus adapter outcome
  propagation. Build/test execution was not verified on this runner: required
  RocksDB, fmt, spdlog, and GoogleTest development packages are unavailable;
  no test pass is claimed.

**Query-engine context slice (2026-11-16)**
- `IQueryEngine::execute(query, bind_vars_json, RequestContext, StopReason&)`
  added (source compatible, non-pure). Default: checkpoint-only; checks before
  (engine not invoked if stopped) and after the legacy call; the caller discards
  the result when a stop is observed. Engines must override it and poll
  `RequestContext::shouldStop()` for cooperative in-flight cancellation.
  **AQL engines that do not override it cannot be interrupted mid-call.** No
  in-tree engine overrides it yet.
- Wired into gRPC `FullTextSearch` and the sparse AQL stage of `HybridSearch`
  (stop => `CANCELLED`/`DEADLINE_EXCEEDED`, hits cleared). Unary `ExecuteAQL`
  and `StreamAQL` are wired too (see below).
- Tests added to `tests/api/test_api_grpc_search_cancellation.cpp`; not built or
  run on the authoring runner (gRPC/protobuf/GoogleTest/spdlog unavailable).

**Secondary-index scan slice (2026-11-17)**
- Request-aware equality and range index scans now poll cancellation/deadline
  during RocksDB iteration (every 64 entries), stop iteration, and discard
  partial key lists. The legacy signatures remain unbounded and delegate through
  an empty request context.
- Request-aware structural-query and fallback paths (optimized and
  non-optimized equality included) use these scans. Range reversal polls during
  its in-memory pass. Sorting and other index families remain checkpoint-only.
- Focused equality, range, query-propagation, and expired-deadline tests were
  added in `tests/index/test_secondary_index_request_cancellation.cpp`; they
  are authored but not built/run on this runner.

**Request-aware AQL runner slice (2026-11-17)**
- `executeAql(..., RequestContext, StopReason*)` now routes conjunctive AQL to
  the production request-aware `QueryEngine::executeAndEntitiesWithFallback`
  path. Request-scoped execution bypasses cached JIT closures so it cannot
  retain a stale per-request context. `executeAqlCancellable()` forwards its
  registered cancellation token into this path.
- Cancellation and deadline expiry return distinct query error codes and
  discard late/partial rows. Spatial JOIN geometry scans are now request-aware
  and conversion polls between 64-entity batches; its iterator polls candidate
  traversal. R-tree candidate lookup and one distance calculation remain
  synchronous. Phrase/fuzzy and other specialized AQL forms remain
  checkpoint-only around synchronous calls; this does not close QRY-07.
- Focused source tests cover cooperative full-scan cancellation, deadline
  classification, successful result parity, and a stopped spatial JOIN
  collection. Build/test evidence is pending.

**Layered graph inference slice (2026-11-17)**
- `KnowledgeGraphReasoner::infer(..., RequestContext, StopReason*)` is now
  source-compatible with the legacy overload and polls fact snapshotting,
  recursive rule matching, forward chaining, and result filtering. A stopped
  call discards the returned chain. Explain-store publication polls between
  entries after derivation completes; one synchronous write may overrun, and
  sound derivations committed before a stop may remain cached.
  `LayeredRetrievalOrchestrator` passes its child context to this overload.
  LoRA scoring remains checkpoint-only.
- Focused reasoner and layered-orchestrator tests cover mid-matching
  cancellation, expired-deadline distinction, empty returned results, and no
  partial explain-store writes. Build/test execution remains pending.

**Remaining gaps (QRY-07 still OPEN)**
- The gRPC `IQueryEngine` boundary still has no in-tree production implementation
  overriding its request-aware method; `ExecuteAQL` and `StreamAQL` pass a
  request context and poll before dispatch, after execution, after JSON parsing
  and between rows, but calls remain checkpoint-only (running call not
  interrupted, late result discarded).
- mTLS cancellation is poll-based (10 ms tick): `QueryCancellationToken` has no
  callback/wake-up registration, so transport abort latency is ~10 ms, not
  instant. Synchronous gRPC search polls its borrowed server context directly.
- `MTLSClient::get/post/put/del` and `RemoteExecutor::get/post/put/del` now
  have request-context overloads. `RemoteExecutor::executeQuery` and
  `postBinary` also propagate context, including cooperative checks while
  base64-encoding a binary payload. The authenticated POST overload now
  supports the same context. These direct async calls still bypass the
  connection-pool path; legacy overloads and pool operations remain
  non-interruptible.
- Focused transport tests have been authored for TLS handshake cancellation,
  stalled response reads, large-request writes against a TLS peer that does not
  read, and PUT/DELETE/query/binary executor propagation; execution is pending
  because this runner lacks Boost and GoogleTest development headers. Those TLS
  peers validate transport behavior only. An end-to-end trace is now authored
  against a real HTTPS `HttpServer`, real content/index services and
  `RemoteExecutor`: it imports a vector-backed chunk, runs a 1M-token hybrid
  search under a 500 ms server deadline, expects HTTP 504, verifies the
  single-threaded server handles a subsequent liveness request, stops the
  server, and verifies the endpoint is closed. Execution is pending because
  this runner lacks Boost and GoogleTest development headers. This trace only
  verifies cooperative cancellation in the built-in `TextProcessor` fallback,
  request-worker reuse, and server shutdown. Request-aware embedding polls
  during tokenization and per-token generation and returns no partial vector.
  Injected embedding callbacks, ANN frontdoor/backend calls, graph reads and
  other synchronous search backends remain checkpoint-only. The abortive-close
  test and this HTTPS deadline trace are authored but not executed, so no
  integration evidence is claimed yet.
- `DistributedHybridSearch` now passes its request context into the local
  `HybridSearch` call. The hybrid aggregator propagates it to BM25 and
  `VectorIndexManager` KNN; an ANN frontdoor/backend call or custom shard
  function that ignores its context can still delay completion and joining.
- The tensor layer now calls the request-aware
  `TensorFingerprintGraph::findSimilar` overload, which polls during candidate
  collection and between similarity operations and discards partial results.
  Each TT inner-product or injected exact-similarity callback remains
  synchronous/checkpoint-only; result sorting is checkpoint-only.
  `KnowledgeGraphReasoner::infer` polls cooperatively, but LoRA scoring remains
  checkpoint-only. `LLMClient::generate` only gets a timeout value and has no
  in-flight HTTP cancel. FAISS/cuVS ANN search is one blocking call.
- `POST /query/aql` passes a request context through full-text index scans,
  score lookups, entity hydration, and structural key-list intersection.
  The direct `executeAql(..., RequestContext)` conjunctive path now also invokes
  the cooperative QueryEngine path instead of relying on the cached JIT closure.
  The non-full-text `allow_full_scan` path polls the storage scan, sequential
  filter and entity hydration; `FULLTEXT()` remains on its indexed path.
  Range-aware non-full-text queries now poll equality/range index iteration,
  key processing, and entity hydration; sorting remains checkpoint-only.
  The optimized equality path polls within index scans and between index
  stages, during key intersection, and during sequential entity hydration; plan
  selection remains cooperatively checked between estimates and planning
  stages, but each estimate, sort, statistics lookup, cost-advice and metrics
  operation remains synchronous. Phrase/fuzzy and other spatial AQL searches
  retain their legacy synchronous implementations with pre/post stop checks.
  Spatial JOIN collection polls during request-aware scans and geometry
  conversion, and the iterator polls during candidate traversal; R-tree
  candidate lookup and one distance calculation remain synchronous. Parsing
  and other query stages remain checkpoint-only. Other HTTP search routes
  besides `POST /vector/search`, `POST /api/v1/vector-index/{name}/rope/search`,
  `POST /search/fulltext`,
  `POST /search/fusion`, `POST /search/hybrid`, and those AQL full-text branches
  remain unbound to a request context. Bounded routes receive a session-scoped
  peer probe, observed only at cooperative poll sites. The HTTP hybrid
  expansion route still cannot interrupt injected embedding callbacks, vector
  ANN, graph neighbor/Dijkstra reads, whitelist scans, or sorting. gRPC `ExecuteAQL`,
  `StreamAQL`, `FullTextSearch` and both stages of `HybridSearch` now pass one
  to their backends, but in-tree AQL engines still use the checkpoint-only
  default and cannot be interrupted mid-call. `ExecuteAQL` clears rows on stop;
  `StreamAQL` honors `Write` failure and stops between rows. Tests for these are
  authored but not built/run (dependencies unavailable on the runner).
- `VectorIndexAdapter` KNN is now cooperative only in brute-force cache/storage
  scans and evaluator candidate filtering. Synchronous HNSW/ScaNN/DiskANN
  backend calls are pre/post checkpoints and can overrun the deadline by the
  in-flight call duration; a late result is discarded, not interrupted.
- `VectorIndexManager` range, rotation, and attribute pre-filter entry points,
  and legacy `IVectorIndex` implementations that do not override the request
  overload remain checkpoint-only or unbound. The HybridSearch AQL path is
  request-aware at the interface boundary but remains checkpoint-only until an
  engine overrides the context-aware overload. Storage reads, entity
  deserialization, one vector-distance calculation, and one evaluator
  invocation cannot be interrupted mid-operation.
- Still required for acceptance: slow-search cleanup trace (end-to-end, real
  shards), end-to-end deadline/cancellation propagation through remaining HTTP
  and gRPC routes, and cancellation handling for the remaining
  in-flight synchronous backends in the layered pipeline.

The dated Phase 1–6, Wave, maturity, test-count, and performance statements
below are historical records and require re-validation before being used as
current status.

### Historical Build Configuration Note (2026-08-08; re-validation required)
- [x] Historical note claimed all 20 search module implementations were added to cmake/ModularBuild.cmake
  - [x] conversational_search.cpp (new, 126 lines)
  - [x] federated_search.cpp (new, 198 lines)
  - [x] search_result_stream.cpp (new, 171 lines)
  - [x] 17 existing implementations verified
- [x] Total codebase: 5,223 lines of implementation code (18/20 Komponenten tragen Gap-Scanner-Annotationen: TODO=1, Stub=1, Mock=1 je Datei; 40 `todo_as_productionlogic`-Einträge in `MODULE_GAPS.md` — Härtungsziel Q3/Q4 2026)
- [x] All implementations include constructor validation and error handling
- [x] Build configuration commit: 8888dc81
- [x] Documentation sync: README, IMPLEMENTATION_STATUS_REPORT updated
- [x] Production-ready maturity scores: 85-95% across all 20 modules

### Phase 1 Completion Summary (2026-08-06)
- [x] Frozen retrieval/fusion/distributed contracts (HybridSearch v2.0.0, DistributedHybridSearch v2.1.0, SearchResultStream v2.0.0)
- [x] Defined explicit error taxonomy via search_error_codes.h (32 error codes across 5 categories)
- [x] Documented contract invariants in ARCHITECTURE.md
- [x] Fixed CRITICAL gaps: exception_in_destructor (hybrid_search.cpp); added a 30s default `open_timeout_ms` setting to SearchResultStream (not proof of backend cancellation or QRY-07 timeout enforcement)
- [x] Contract boundaries documented for core components (retrieval, fusion, distributed, utility, analytics)

### Phase 2 Completion Summary (2026-08-06)
- [x] Enhanced distributed_hybrid_search.cpp with explicit shard-failure handling
  - [x] Shard failure reason tracking in SearchStats::failed_shard_reasons
  - [x] Merge underflow detection (merge_underflow flag)
  - [x] High-overlap variance detection (high_overlap_variance flag)
- [x] Extended SearchStats structures across HybridSearch and DistributedHybridSearch
  - [x] Added primary_error_code field (via search_error_codes.h)
  - [x] Added degradation flags: fusion_failed, rerank_fallback
  - [x] Added failed_shard_reasons tracking vector
- [x] Phase 2 focused tests (P2-01..P2-08): test_search_distributed_merge_phase2.cpp
- [x] Phase 2 degradation tests (P2H-01..P2H-04): test_search_hybrid_degradation_phase2.cpp
- [x] Updated CMakeLists.txt with distributed_hybrid_search.cpp target sources

### Phase 3 Completion Summary (2026-08-06)
- [x] Standardized fail-safe behavior for shard failures, merge limits, rerank faults
- [x] Unified diagnostics across retrieval/fusion/utility incident classes
- [x] Edge case tests P3-01..P3-08 with comprehensive error injection
- [x] Conformance tests for all 14 search components
- [x] SearchStats with degradation flags (shards_failed, partial_result, rerank_fallback)
- [x] PHASE_3_ERROR_HANDLING_GUIDE.md with 6 unified error patterns

### Phase 4 Completion Summary (2026-08-06)
- [x] **Test Expansion:** 7 test files created with 128+ test cases
  - [x] test_search_distributed_merge_stress.cpp (SDS-01..SDS-16)
  - [x] test_search_edge_cases_retrieval.cpp (RET-01..RET-16)
  - [x] test_search_edge_cases_fusion.cpp (FUS-01..FUS-16)
  - [x] test_search_edge_cases_distributed.cpp (DIS-01..DIS-16)
  - [x] test_search_edge_cases_utility.cpp (UTL-01..UTL-16)
  - [x] test_search_edge_cases_analytics.cpp (ANL-01..ANL-16)
  - [x] test_search_integration_phase4.cpp (INT-01..INT-16)
- [x] Deterministic seed=42 RNG for all fixtures per benchmark hygiene rules
- [x] CTest labels: search, phase4, stress, edge_case
- [x] Timeout 120s per themis_register_module_focused_test()

### Phase 5 Completion Summary (2026-08-06)
- [x] **Performance Gatekeeping:** bench_search_release_gates.cpp with 9 benchmarks
  - [x] SRCP-1: Hybrid dispatch (p99 ≤ 16.5 ms, 100 shards, 10K candidates)
  - [x] SRCP-2: Merge throughput (≥ 45K results/sec, 64 shards)
  - [x] SRCP-3: Reranking overhead (≤ 5.5 ms, LLM fallback)
  - [x] SRCP-4: GPU/CPU fallback (GPU ≤ 8.8 ms, CPU ≤ 11 ms)
  - [x] SRCP-5: Stream flush (≤ 11 ms per 1K batch)
  - [x] SRCP-6: Query expansion (≤ 55 ms for 1K queries)
  - [x] SRCP-ADV-1: Multimodal search (≤ 20 ms, nDCG@10 ≥ 0.85)
  - [x] SRCP-ADV-2: Learning-to-rank (≤ 25 ms with LTR overhead)
  - [x] SRCP-ADV-3: Concurrent indexing (latency increase ≤ 10%)
- [x] All gates with 10% regression tolerance
- [x] Baseline hardware: Intel Xeon + NVIDIA RTX
- [x] benchmarks/search/README.md with gate documentation, hardware profiles, thresholds

### Phase 6 Completion Summary (2026-08-06)
- [x] **Documentation & Acceptance:**
  - [x] PRODUCTION_REQUIREMENTS.md: Hardware profiles, SLA targets, data constraints, operational procedures
  - [x] FINAL_ACCEPTANCE_CHECKLIST.md: Phase 1-6 verification matrix (100+ checkboxes)
  - [x] benchmarks/search/README.md: Gate documentation with hardware tiers and regression rules
  - [x] ROADMAP.md: Phase 4-6 completion status with evidence links
  - [x] 14 component @file headers with Doxygen metadata (maturity_score, contract_frozen, open_gaps)
  - [x] Documentation governance sync: L1-L4 hierarchy maintained
  - [x] Research mapping: research/implementation_influence/by_module.md aligned

## Wave B Completion Status (2026-08-17 → 2026-08-18)

- [x] **Wave B: Retrieval Chain Integration** (COMPLETE 2026-08-17)
  - [x] CRITICAL gaps verification: no_timeout (30s default verified), exception_in_destructor (noexcept verified)
  - [x] HIGH severity gaps review: 71/71 gaps verified as non-blocking false positives or intentional patterns
  - [x] Test infrastructure validation: P2-01..P2-08, SDS-01..SDS-16 tests ready; SRCP-1 benchmark gate at 16.5ms p99
  - [x] Brace balance verification: All 22 implementation files balanced (31-101 braces each)
  - [x] Constructor initialization verification: All files properly initialize members
  - [x] Documentation update: WAVE_B_COMPLETION_PLAN.md, WAVE_B_VERIFICATION_REPORT.md created
  - [x] ROADMAP.md updated with Wave B completion status (THIS ENTRY)
  - **Deliverables (2026-08-17):** Comprehensive verification report; 854 total gaps assessed; 2 CRITICAL gaps verified as fixed; 71 HIGH gaps verified as non-blocking
  - **Status:** ✅ IMPLEMENTATION COMPLETE (2026-08-17)
  - **Evidence:** WAVE_B_VERIFICATION_REPORT.md with full analysis and sign-off

- [x] **Wave B: Documentation Closure** (COMPLETE 2026-08-18)
  - [x] LAYERED_RETRIEVAL_ARCHITECTURE.md: Comprehensive 4-layer chain design documentation
    - [x] Layer specifications (ANN, Tensor, Graph, LLM)
    - [x] Timeout model & concurrency
    - [x] Guardrail model with per-query bounds
    - [x] OpenTelemetry tracing integration
    - [x] Configuration & deployment topologies
    - [x] Scaling & performance considerations
    - [x] Testing & validation framework
  - [x] LAYERED_RETRIEVAL_SLA.md: Performance & resource contracts
    - [x] Latency targets (p50/p95/p99 per scenario)
    - [x] Per-layer latency breakdown
    - [x] Memory footprint analysis (bounded ~13.5 KB/query)
    - [x] Throughput & capacity planning (2–200 QPS)
    - [x] Reliability targets (99.9–99.99% availability)
    - [x] Degradation paths (4 key scenarios)
    - [x] Resource utilization profiles (CPU, GPU, Network)
    - [x] Hardware tier profiles (Tier 1–3)
    - [x] Tuning guidelines (latency/quality/cost-aware)
    - [x] Monitoring & alerting thresholds
    - [x] Known limitations & caveats
  - [x] ARCHITECTURE.md: Added v3.0.0 LayeredRetrievalOrchestrator contract section
    - [x] Cross-references to LAYERED_RETRIEVAL_ARCHITECTURE.md and LAYERED_RETRIEVAL_SLA.md
    - [x] Updated sourcecode verification section
  - **Deliverables (2026-08-18):** Complete architecture and SLA documentation for 4-layer chain
  - **Status:** ✅ DOCUMENTATION COMPLETE (2026-08-18)
  - **Evidence:** LAYERED_RETRIEVAL_ARCHITECTURE.md, LAYERED_RETRIEVAL_SLA.md, updated ARCHITECTURE.md

## In Progress

- [~] **EPIC #5423 Phase 4 — Integration Tests** (Target: Q3 2026): wire real layer implementations into `LayeredRetrievalOrchestrator`; replace gmock NiceMock with production adapters.
- [~] **EPIC #5423 Phase 5 — Performance & Hardening** (Target: Q3 2026): latency baselines, timeout enforcement, memory profiling, distributed tracing.
- [~] **HybridSearch Production Hardening (#1323)** (Target: Q3 2026): `SearchStats` extension, configurable metrics, `PerQueryRetrievalGuardrails`.
- [~] **Wave C: Federation Hardening** (Target: Q3 2026): Federated search coordination, cross-tenant isolation, distributed cache coherency.

## Planned Features

### Native Small-Model Retrieval (Proposal; Target: Q2 2027)

- [ ] Reconcile server/SDK filtering, metric-aware shard merge, ID/cursor and
  explicit partial-failure semantics before framework qualification
  (Priority: P0; Target: Q2 2027 proposal).
- [ ] Qualify policy-aware pre/post-filter strategy and measure filtered recall
  across selectivity levels against an exact oracle; retain hard predicates
  independently of ANN quality (Priority: P0; Target: Q2 2027 proposal).
- [ ] Reduce authorized candidates near their data, enforce aggregate shard
  budgets, and evaluate complementary evidence at fixed local-model context
  limits with RAG (Priority: P1; Target: Q2 2027 proposal).
- [ ] Measure cross-node bytes, coverage/freshness and DB p95/p99 under
  concurrent native inference; approve thresholds only after baselines
  (Priority: P1; Target: Q2 2027 proposal).

Use the existing design/implementation/error/test/hardening/documentation phases
below for this work; their historical completion labels do not close these new
items. Contracts, tests and research sources are in
[FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md). Existing hybrid/RRF and
filtered-query components are building blocks, not evidence that every
external endpoint already preserves their semantics.

### Q1 2027+ — Advanced Search Enhancements

### Short-term (Q1 2027+)
- [ ] Phase 7: Advanced Features and Future Enhancements
  - [ ] Multimodal search optimization (text + image + code)
  - [ ] Learning-to-rank model integration with search pipeline
  - [ ] Advanced analytics dashboard and real-time monitoring
  - Target: Q1 2027+

### Completed Phases

- [x] **Phase 1: Design / API Contract Freeze** — COMPLETE (2026-08-06)
- [x] **Phase 2: Core Implementation Hardening** — COMPLETE (2026-08-06)
- [x] **Phase 3: Error Handling and Edge Cases** — COMPLETE (2026-08-06)
- [x] **Phase 4: Test Expansion** — COMPLETE (2026-08-06)
- [x] **Phase 5: Performance Gatekeeping** — COMPLETE (2026-08-06)
- [x] **Phase 6: Documentation & Acceptance** — COMPLETE (2026-08-06)
- [x] **Phase 4+5: LayeredRetrievalOrchestrator Real Implementation** — COMPLETE (2026-08-16) — EPIC #5423 Wave A-8 Closure
  - [x] Real ANN layer integration (AdvancedVectorIndex with HNSW)
  - [x] Real Tensor layer integration (TensorFingerprintGraph)
  - [x] Real Graph layer integration (KnowledgeGraphReasoner)
  - [x] Real LLM layer integration (LLMClient)
  - [x] Timeout enforcement per layer (hard deadline with fallback)
  - [x] Concurrency controls and thread-safety (TSan-clean)
  - [x] Chaos/fault-injection testing (timeout, shard failure, partial results)
  - [x] Performance baselines locked (p95≤200ms, p99≤500ms for 4-layer chain)
  - [x] Distributed tracing with OpenTelemetry spans
  - [x] PerQueryRetrievalGuardrails enforcement
  - [x] Evidence: WAVE_A8_CLOSURE_EVIDENCE.md

## Implementation Phases

### Phase 1: Design / API Contract Freeze (COMPLETE 2026-08-06)
- [x] freeze retrieval/fusion/distributed contracts for current major line
- [x] define explicit error taxonomy for search failure classes
- [x] document contract boundaries for all 14 search components
- **Deliverables:** HybridSearch v2.0.0, DistributedHybridSearch v2.1.0, SearchResultStream v2.0.0, search_error_codes.h v1.0.0
- **Status:** COMPLETE

### Phase 2: Core Implementation Hardening (Q4 2026)
- [ ] complete hardening for hybrid/distributed merge internals
- [ ] align utility-layer behavior to bounded runtime contracts
- **Deliverables:** Enhanced distributed_hybrid_search with explicit shard-failure handling; bounded resource limits enforced across all fusion/expansion/reranking paths; cross-component resilience tests
- **Status:** IN PROGRESS

### Phase 3: Error Handling and Edge Cases (COMPLETE 2026-08-06)
- [x] standardize fail-safe behavior for shard failures, merge limits, and rerank faults
- [x] unify diagnostics across retrieval/fusion/utility incident classes
- **Deliverables:** SearchStats with degradation flags (shards_failed, partial_result, rerank_fallback); edge case tests P3-01..P3-08; conformance tests for all 14 components
- **Status:** COMPLETE

### Phase 4: Test Expansion (COMPLETE 2026-08-06)
- [x] expand focused regressions for overlap, shard-failure, and candidate-limit scenarios
- [x] extend deterministic stress fixtures for hybrid/distributed workloads
- **Deliverables:** test_search_distributed_merge_stress.cpp (SDS-01..SDS-16); test_search_edge_cases_retrieval.cpp (RET-01..RET-16); test_search_edge_cases_fusion.cpp (FUS-01..FUS-16); test_search_edge_cases_distributed.cpp (DIS-01..DIS-16); test_search_edge_cases_utility.cpp (UTL-01..UTL-16); test_search_edge_cases_analytics.cpp (ANL-01..ANL-16); test_search_integration_phase4.cpp (INT-01..INT-16). Total: 128+ test cases with CTest labels: search, phase4, stress, edge_case. Seed=42 deterministic RNG. Timeout 120s per test.
- **Status:** COMPLETE (2026-08-06)

### Phase 5: Performance Gatekeeping (COMPLETE 2026-08-06)
- [x] lock benchmark-backed release gates for search hot paths
- [x] validate p95/p99 and throughput behavior against release baselines
- **Deliverables:** bench_search_release_gates.cpp with SRCP-1..6 gates (dispatch latency, merge throughput, reranking overhead, GPU/CPU fallback, stream flush, query expansion) + SRCP-ADV-1..3 (multimodal, learning-to-rank, concurrent indexing). All gates with 10% regression tolerance. Baseline hardware: Intel Xeon + NVIDIA RTX. benchmarks/search/README.md with hardware profiles, regression thresholds, re-baseline schedule.
- **Status:** COMPLETE (2026-08-06)

### Phase 6: Documentation and Acceptance (COMPLETE 2026-08-06)
- [x] Doxygen file headers across all 14 components with maturity metadata
- [x] Comprehensive PRODUCTION_REQUIREMENTS.md
- [x] Module FINAL_ACCEPTANCE_CHECKLIST.md
- [x] Documentation governance sync
- [x] core search module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- **Deliverables:** 14 component @file headers with @version v2.0.0, @maturity_score 7-9/10, @contract_frozen true, @open_gaps list. PRODUCTION_REQUIREMENTS.md with hardware profiles (minimum/recommended/tier), SLA targets (p99 latencies, throughput, availability), data constraints (max k, result set, shards), operational procedures. FINAL_ACCEPTANCE_CHECKLIST.md with Phase 1-6 verification matrix (100+ checkboxes), all SRCP gates locked, Doxygen 99%+ coverage, sign-off section.
- **Status:** COMPLETE (2026-08-06)

## Production Readiness Checklist

- [x] core search surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] hardening tasks closed for distributed merge/utility edge paths
- [x] release benchmark stabilization complete
- [x] all 128+ phase 4 tests pass with CTest
- [x] all 6 SRCP gates pass on baseline hardware
- [x] Doxygen coverage 99%+ (14 components)
- [x] code review sign-off obtained
- [x] documentation review sign-off complete
- [x] FINAL_ACCEPTANCE_CHECKLIST.md 100% green
- [x] **Wave B CRITICAL gaps verification complete** (2026-08-17)
- [x] **Wave B HIGH severity gaps assessment complete** (2026-08-17)
- [x] **Wave B retrieval chain integration validated** (2026-08-17)

## Known Issues and Limitations

- [x] Phase 4-6 deliverables complete and verified
- [ ] SRCP benchmark baseline measurement on production hardware (pending first hardware run)
- Advanced scenarios (SRCP-ADV-1..3) pending Q1 2027 detailed integration testing
- benchmark depth should continue expanding for advanced search workflows.
- Wave B B1 work depends on upstream Wave A deployment and LLM latency prerequisites.

---

## EPIC #5423 — LayeredRetrievalOrchestrator: Phase 4 & 5 (Q3 2026)

> Phase 1 (Design), Phase 2 (Core Implementation), and Phase 3 (Error Handling) are COMPLETE.
> 41 unit tests passing. This section tracks the next two phases per Q3 2026 milestone.

### Phase 4: Integration Tests — EPIC #5423 (Target: Q3 2026)

**Status:** ⏳ Pending — implementation start gated on Q3 2026 sprint assignment.

**Scope:** Replace all gmock NiceMock layer fakes with real layer implementations; verify end-to-end retrieval quality; validate provenance correctness.

- [~] Create `tests/search/test_layered_retrieval_orchestrator_phase4.cpp` wiring ANN, Tensor, Graph, and LLM/LoRA real backends (no NiceMock) (Target: Q3 2026) — related integration harness `tests/search/test_search_integration_phase4.cpp` already exists; dedicated real-backend layered coverage is still pending
- [ ] Verify end-to-end retrieval quality with deterministic golden queries against real ANN and Graph backends; record nDCG@10 baseline (Target: Q3 2026)
- [ ] Validate provenance correctness: every result MUST carry correct layer attribution in `LayeredRetrievalResult.routing_decisions` with no silent drops (Target: Q3 2026)
- [ ] Confirm 100% of 41 existing unit tests still pass after real-backend wiring (regression gate) (Target: Q3 2026)
- [~] Deliver ≥15 new integration tests; register with CTest labels `search,phase4,integration,layered,epic5423` (Target: Q3 2026) — product feature engineering
- [~] Test timeout enforcement: a layer exceeding `LayeredRetrievalConfig.timeout_ms` MUST activate fallback; validated in ≥2 integration test cases (Target: Q3 2026) — product feature engineering

**Acceptance Criteria:**
- 41 existing tests pass (zero regression)
- ≥15 new integration tests pass
- Provenance validated across all 4 layers in golden-query fixture

### Phase 5: Performance & Hardening — EPIC #5423 (Target: Q3 2026)

**Status:** ⏳ Pending — depends on Phase 4 completion.

**Scope:** Establish latency baselines, memory profiles, stress tests, and enforce timeout semantics.

- [~] Implement `benchmarks/search/bench_layered_retrieval_phase5.cpp` with latency baselines for: ANN-only, ANN+Tensor, ANN+Tensor+Graph, all-4-layers combinations (Target: Q3 2026) — product feature engineering
- [~] Memory profile per layer configuration; document top-3 allocation hotspots; no unbounded growth under 1M-vector stress (Target: Q3 2026) — product feature engineering
- [~] Stress test ≥1M vector candidates with sustained load; confirm no OOM and p99 does not diverge (Target: Q3 2026) — product feature engineering
- [~] Enforce (not advisory) timeout semantics: implement cancellation in all 4 layer executors; test that a layer over budget triggers fallback with ≤10ms cancellation overhead (Target: Q3 2026) — product feature engineering
- [~] Integrate distributed tracing backend: all layer executors emit span with correlation ID, layer name, and latency_ms (Target: Q3 2026) — product feature engineering
- [~] Confirm SRCP-4 (GPU/CPU fallback ≤8.8ms GPU / ≤11ms CPU) gate remains green after hardening changes (Target: Q3 2026) — product feature engineering
- [~] Add `PerQueryRetrievalGuardrails` enforcement: per-query cost limit, layer-pruning on SLO breach (Target: Q3 2026) — product feature engineering

**Performance Acceptance Gate:**
- p95 ≤200ms full 4-layer chain at 10K QPS on Intel Xeon + NVIDIA RTX baseline hardware
- p99 ≤500ms under 2× overload (stress)
- Memory footprint ≤4GB at 1M vectors across all 4 layers

---

## Wave B (Q1–Q2 2027) Tracking — B1 Self-RAG Search Integration

### Scope
- [x] retrieval-controller decision hooks for selective re-retrieval
- [x] retrieval quality signals for critic feedback (Relevant/Partial/Irrelevant)
- [x] bounded retrieval refinement integration support (max 3 rounds)
- [x] search-path integration support for `InferenceEngineEnhanced` callback loops

### Validation
- [x] unit tests `SELF_RAG-SEARCH-01..08` (covered by SELF_RAG-01..12)
- [x] ALCE retrieval-quality benchmark contribution vs vanilla RAG baseline

### Acceptance Gates
- [ ] precision@k retrieval contribution ≥ 0.85 on golden-doc tests
- [ ] retrieval-path latency overhead remains within Self-RAG budget (≤ 1.5× overall baseline)
- [ ] deterministic fallback behavior under shard/backend partial failures

### Dependencies
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)
- [ ] LLM inference P95 latency < 200 ms for iterative loops
- [ ] RAG module Self-RAG controller/critic interfaces stabilized

### References
- AI tracker: `../ai/ROADMAP.md`
- RAG tracker: `../rag/ROADMAP.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`

## Breaking Changes

No breaking search contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Evidence and Verification

### Build Infrastructure
- `tests/search/CMakeLists.txt` declares `module_search_<test_stem>_focused` executables from a `test_*.cpp` glob and calls `themis_register_module_focused_test()` with `TIMEOUT 120`; it does not select the four implementation files named in the former text as a common source set.
- The file also appends `fuzzy_matcher.cpp` or layered-retrieval support sources for specific tests, and adds conditional labels. This describes the file's declarations only.
- `tests/CMakeLists.txt` auto-discovers eligible `tests/*/CMakeLists.txt` files and adds the `search` subdirectory when tests are configured. Target generation, compilation, and execution still require verification in a configured build.

### Test Evidence
- Current scoped source inventory: 16 `tests/search/test_*.cpp` files, including utility, analytics, hybrid/degradation, distributed, layered, high-cardinality, and integration-named cases.
- The two `*_integration*` filenames do not alone prove live multi-service integration; inspect each test fixture for callbacks, fakes, or simulated state.
- No build or test execution was performed for this documentation update. A previous dependency-blocker note is historical and not current run evidence.

### Audit and Documentation Evidence
- AUDIT.md: Module source verification completed; core search surfaces present and documented
- Verified compliance: Source-verifiable behavior claims (pass), Structured forward planning (pass), Historical tracking (pass), Core module docs synchronized (pass)
- Open audit findings: Three medium/low-severity items tracked (SEA-AUD-01, SEA-AUD-02, SEA-AUD-03) — distributed merge hardening, fusion diagnostics, benchmark depth

### Production Readiness Status
- [x] core search surfaces documented and source-verified (verified per AUDIT.md)
- [x] module-level security and failure behavior documented (per SECURITY.md)
- [x] benchmark mapping documented in performance expectations (per PERFORMANCE_EXPECTATIONS.md)
- [ ] remaining hardening tasks closed for distributed merge/utility edge paths (in progress per roadmap priorities)
- [x] release benchmark stabilization complete — Wave D dedicated gates delivered in `benchmarks/search/bench_search_dedicated_gates.cpp` (SE-BM-01..04) (Delivered: 2026-09-16)

### Evidence Justification
- Build/test status: not verified by execution in this update. CMake declarations and the auto-discovery path were inspected; target generation, compilation, and execution remain unverified.
- Test coverage: filenames and source-defined test cases are not evidence of compilation or passing execution.
- Benchmark status: source definitions and registrations were inspected; no measured values, baseline comparison, or gate pass was verified.
---

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D
execution model.  It does not own a primary wave deliverable but must remain
`release_critical`-green throughout all waves and must deliver Wave D
operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `search`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module — `tests/search/test_search_highcardinality_stress.cpp` (HighCardinalityDocumentIndex 500K docs, ConcurrentRankingStress, FacetFilterStress) delivered; `docs/operability/RUNBOOK_SEARCH_ENGINE.md` (5 scenarios: IndexCorruption, RankingStall, FacetExplosion, QueryTimeout, ReindexStall) delivered (Delivered: 2026-09-16)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths — `tests/integration/test_search_engine_soak.cpp` (SearchSoak_FullTextThroughput ≥5000 searches/s, SearchSoak_IndexConsistencyStability, SearchSoak_RankingReliability) delivered (Delivered: 2026-09-16)
- [x] Ensure runbook coverage for operator-critical scenarios in this module — `docs/operability/RUNBOOK_SEARCH_ENGINE.md` with 5 incident classes and log patterns (Delivered: 2026-09-16)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed — existing error handling confirmed; `[SEARCH:IndexCorruption]`, `[SEARCH:RankingStall]`, `[SEARCH:FacetExplosion]` runbook paths documented (Delivered: 2026-09-16)
- [x] Benchmark-backed p95/p99 baselines exist on representative hardware — `benchmarks/search/bench_search_dedicated_gates.cpp` (SE-BM-01..04: FTS query p95, ranking p95, facet filter p95, concurrent throughput) delivered (Delivered: 2026-09-16)
- [x] Operator-critical paths have diagnostics, alerts, and runbooks — `docs/operability/RUNBOOK_SEARCH_ENGINE.md` with 5 scenarios, log patterns, and remediation tables (Delivered: 2026-09-16)
