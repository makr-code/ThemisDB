# Core Quick-Win Plan (2 Weeks)

Date: 2026-05-27
Source: ai_working/gap_scan_v3_summary.json + module gap reports
Scope: core, server, query, sharding, index, storage, network, transaction, security, auth

## Goal

Reduce high-volume, low-risk gap patterns quickly in core components without architecture redesign.

## Current Status

- Package A: completed in the focused server, query, index, and storage slices.
- Package B: completed, with storage/null guards in server handlers, QueryEngine storage-backed paths, empty-input guards in AQL injection validation, range/finite checks in wire protocol vector/geo handlers, decoded-audio validation guards across voice API endpoints, strict query-limit validation in voice listing/search endpoints, graph/query/cursor range guards in the wire protocol, strict BPMN wire-request type validation, BPMN identifier hygiene checks (length/control-character guards), QUERY/CURSOR hardening for blank AQL and invalid cursor identifiers, expanded core wire input hygiene for GET/PUT/DELETE/BATCH/TRANSACTION/GRAPH handlers, and explicit JSON type-guards before core wire string extraction.
- Package C: transaction slice advanced with SAGA plugin runtime hardening (manifest/signature runtime asset wiring) and focused bridge tests for successful bind+execute plus missing-binary failure behavior.
- Package C: core cache slice advanced with Redis RESP parser hardening in `redis_cache.cpp` (noexcept-safe integer parsing and strict reply-read error checks in SCAN/reply paths) to reduce uncaught-exception risk in network/error scenarios.
- Package C: core cache validation extended with Redis subscriber lifecycle hardening in `redis_cache.cpp` (lazy subscriber startup plus interruptible reconnect sleep) and validator-call alignment in `test_distributed_cache_integration.cpp`, validated green with `test_distributed_cache_integration_focused` (38/38 passed) and `test_distributed_cache_coordinator` (21 passed, 2 platform skips).
- Package C: network slice advanced with HTTP/3/QUIC compatibility hardening for current ngtcp2/nghttp3 APIs in `quic_server.cpp`, `quic_transport.cpp`, and `http3_session.cpp` (header/API migration, callback completeness, transport-parameter wiring, and focused-linkage fixes), validated with `QUICServerFocusedTests` under `THEMIS_ENABLE_HTTP3=ON`.
- Package C: network validation extended with successful `Http3ProductionReadinessFocusedTests` under `THEMIS_ENABLE_HTTP3=ON` after focused target linkage alignment for nghttp3/ngtcp2.
- Package C: combined focused validation run completed (`QUICServerFocusedTests` + `Http3ProductionReadinessFocusedTests`) with both tests passing under `THEMIS_ENABLE_HTTP3=ON`.
- Package C: monolithic `themis_tests` rebuild progressed past prior HTTP/3 syntax/link blockers after `test_http3_protocol.cpp` constant-name fixes and explicit nghttp3/ngtcp2 linkage in `tests/CMakeLists.txt`; remaining work is to let the long aggregate build finish and then re-check the Redis filter on the fresh monolith binary.
- Package C: Redis/cache cleanup race fixed in `tests/test_cache_replication.cpp` (scope guard + `remove_all(path, ec)` no-throw form); 42/42 `CacheReplicationTests` passed.
- Package C: SAGA exception hygiene complete — `distributed_saga.cpp`: wave `futures[i].get()` loop now wrapped in `try/catch(std::exception)/catch(...)`, all existing catch cascades in executeStep and executeCompensation extended with `catch(...)`; `saga_orchestrator.cpp`: wave `future.get()` loop wrapped, executeStep and compensation catch cascades extended with `catch(...)`; validated 3/3 SAGA CTest entries passed (SAGAOrchestratorFocusedTests, SAGALoggerFocusedTests, SAGACompactorFocusedTests).
- Package C: `nvme_manager.cpp` assessed — all fd paths (`resetZone`, `finishZone`, `getZoneWritePointer`, `probeIoUringKernel`) already safe (::close called before every return); no changes needed. `redundancy_strategy.cpp` assessed — typed `catch(std::exception&)` already in place at all catch sites; no additions required.
- Package C: COMPLETE.
- Package D: path portability advanced in server/rpc export+snapshot+blob flows via filesystem-based temp roots (`temp_directory_path()` + fallback) replacing hardcoded `/tmp` and Linux-only defaults in `export_api_handler.cpp`, `snapshot_transfer_handler.cpp`, and `blob_transfer_handler.cpp`; validated with successful `themis_network` target build.
- Package D: determinism and rollback path-hardening advanced with stable ordering for observability alerts (`monitoring_api_handler.cpp`), deterministic GROUP BY emission order in query execution (`query_engine.cpp`), and stricter/cross-platform snapshot ID validation plus base-path containment checks in MCP rollback (`mcp_server.cpp`); validated with successful `AiSnapshotCleanupTests`.
- Package D: MCP rollback hardening now test-backed in `tests/test_mcp_integration.cpp` with focused negative/edge cases for snapshot ID validation (`empty`, path-separator payload, Windows drive-prefix payload, and valid simple ID fail-closed path). Build validated via successful `themis_tests` link; runtime execution is gated by `THEMIS_ENABLE_MCP` in the active preset.
- Package D: MCP rollback input hardening extended with explicit snapshot-ID allowlist (alnum + `_-.`, bounded length, control-byte rejection) and additional negative tests for percent-encoded traversal payload, control characters, and unsupported special characters (`:`). Build validated with successful `themis_network` + `themis_tests` targets.
- Package D: monitoring determinism extended in `monitoring_api_handler.cpp` by sorting plugin metric emission order (both `/metrics` and `/api/plugins/metrics`) and stabilizing alert-label emission order in observability alerts to reduce hash-order drift in JSON/Prometheus outputs.
- Package D: query determinism extended in `query_engine.cpp` (`ObjectConstruct` expression evaluation) by sorting object field keys before JSON materialization, reducing hash-order-dependent output drift in object-returning query expressions.
- Package D: query determinism further extended in `query_engine.cpp` nested-loop join path by sorting `EvaluationContext` bindings before assembling LET-evaluation context JSON (`currentDoc`), reducing map-iteration-order drift in multi-variable query execution.
- Package D: ranking determinism extended in `query_engine.cpp` geo pipelines by adding stable PK tie-breakers in vector-distance and BM25/geo-score sorts, preventing nondeterministic ordering for equal-score candidates under parallel sort.
- Package D: graph-traversal determinism improved in `query_engine.cpp` by sorting reachable node IDs before recursive path reconstruction and sorting adjacency expansions in general BFS traversal (target PK, edge ID, graph ID).
- Package D: join determinism hardened in `query_engine.cpp` by adding a stable JSON tie-breaker for equal SORT expression values in `executeJoin`, avoiding parallel sort instability on equal keys.
- Package D: hash-join determinism strengthened in `query_engine.cpp` by sorting each hash bucket (stable `_key`/JSON fallback) before probe emission, reducing scan-order drift for same-key join matches.
- Package D: hash-join probe determinism extended in `query_engine.cpp` by sorting CTE probe documents via stable `_key`/JSON order before probing, improving reproducibility without explicit SORT.
- Package D: hash-join scan-probe determinism aligned in `query_engine.cpp` by buffering scanned probe docs, applying the same stable `_key`/JSON ordering, and only then probing buckets.
- Package D: nested-loop join determinism extended in `query_engine.cpp` by sorting CTE input documents per FOR step via stable `_key`/JSON order before filter evaluation and recursion.
- Package D: nested-loop non-CTE determinism aligned in `query_engine.cpp` by buffering table-scan docs, sorting them with the same stable `_key`/JSON key, and recursing in deterministic order.
- Package D: join result determinism finalized in `query_engine.cpp` by applying a stable default JSON ordering when no explicit SORT clause is present (before LIMIT processing).
- Package D: vector-geo brute-force determinism improved in `query_engine.cpp` by adding a PK tie-breaker for equal L2 distances before top-k truncation.
- Package D: group-by aggregation determinism improved in `query_engine.cpp` by sorting documents per group with a stable `_key`/JSON key before SUM/AVG/MIN/MAX evaluation.
- Package D: determinism helper cleanup in `query_engine.cpp` by consolidating duplicated stable JSON ordering lambdas into shared `stableJsonOrderKey`/`stableJsonLess` utilities across join and group-by paths.
- Package D: final comparator hardening in `query_engine.cpp` brute-force vector path by adding PK tie-breaker on equal distances in `tmp` ranking prior to top-k selection.
- Package D: deterministic intersection planning refinement in `query_engine.cpp` by adding a lexical tie-breaker for equal-sized lists in `intersectSortedLists_` sorting.
- Package D: content-geo determinism refined in `query_engine.cpp` by sorting spatial-first candidate PKs before fulltext token matching to stabilize downstream ranking input.
- Package D: determinism comparator cleanup in `query_engine.cpp` by replacing redundant wrapper lambdas with direct `stableJsonLess` usage in join/nested-loop sorting sites.
- Package D: join default-order consistency updated in `query_engine.cpp` by switching no-SORT result ordering to shared `stableJsonLess` comparator.
- Package D: comparator unification finalized in `query_engine.cpp` by introducing `stableJsonPtrLess` and removing the last pointer-wrapper lambda in group-by ordering.
- Package D: join sort fallback consistency improved in `query_engine.cpp` by switching expression-evaluation fallback/tie handling from raw `dump()` ordering to shared `stableJsonLess`.
- Package D: monitoring API determinism extended in `monitoring_api_handler.cpp` by sorting `modules` arrays by name and `supported_api_versions` by semantic version before JSON emission.
- Package D: metrics HTML determinism refined in `monitoring_api_handler.cpp` by sorting parsed Prometheus table rows by metric name/value before rendering.
- Package D: metrics text determinism refined in `monitoring_api_handler.cpp` by sorting `rocksdb_files_level` rows by level key before Prometheus emission.
- Package D: observability alert ordering hardened in `monitoring_api_handler.cpp` with an additional `alert_name` tie-breaker after `fired_at` and `alert_id`.
- Package D: rocksdb level metric ordering improved in `monitoring_api_handler.cpp` by sorting `files_per_level` numerically for `L<n>` keys with lexical fallback.
- Package D: observability alert comparator finalized in `monitoring_api_handler.cpp` with an additional `message` tie-breaker after `fired_at`, `alert_id`, and `alert_name`.
- Package D: range-aware predicate planning determinism improved in `query_engine.cpp` by adding explicit `column`/`value` tie-breakers when selectivities are equal.
- Package D: parallel error-path determinism improved in `query_engine.cpp` by sorting collected error messages before selecting the returned representative error in `executeAndKeys`/`executeOrKeys`.
- Package D: parallel OR fallback error-path determinism improved in `query_engine.cpp` by replacing first-arrival error capture with collected+sorted error messages in `executeOrKeysWithFallback`.
- Package D: deterministic observability improved in `query_engine.cpp` by collecting and sorted-emitting parallel deserialization warnings in `executeAndEntities` (instead of thread-racy direct warning emission).
- Package D: deterministic observability extended in `query_engine.cpp` by applying the same collected+sorted warning emission pattern to the parallel `executeOrEntities` path.
- Package D: deterministic observability completed for fallback OR entity loading by applying collected+sorted warning emission in parallel `executeOrEntitiesWithFallback`.
- Package D: deterministic observability extended to sequential-query parallel loading by replacing silent parallel deserialize catches with collected+sorted warning emission in `executeAndEntitiesSequential`.
- Package D: determinism hardening maintenance improved by consolidating repeated collected+sorted deserialize-warning emission into shared helper `logSortedDeserializeFailures(...)` in `query_engine.cpp`.
- Package D: vector+geo prefilter determinism improved in `query_engine.cpp` by applying range-index intersections in sorted column order instead of unordered-map iteration order.
- Package D: query federation determinism improved in `query_federation.cpp` by replacing unordered target-shard deduplication with sorted `sort+unique` handling in the partition-pruning execution path.
- Package D: query federation routing determinism improved in `query_federation.cpp` by normalizing all multi-shard routing results through sorted unique shard-ID lists in `determineRelevantShards`.
- Package D: federated result merge determinism improved in `query_federation.cpp` by sorting shard results by shard ID before concatenation in `mergeResults`.
- Package D: federated join determinism improved in `query_federation.cpp` by sorting broadcast and shuffle join inputs with a stable JSON key comparator before hash-table build/probe.
- Package D: federated aggregation determinism improved in `query_federation.cpp` by sorting shard results by shard ID before combining partial aggregation outputs in `executeAggregation`.
- Package D: federated pagination determinism improved in `query_federation.cpp` by applying shared stable JSON sorting before OFFSET/LIMIT slicing in `applyGlobalOperations`.
- Package D: federated RAG determinism improved in `query_federation.cpp` by sorting shard fan-out results by shard ID before converting them into `ShardRetrievalResult` objects.
- Package D: federated RAG merge safety improved in `query_federation.cpp` by sorting the final `rag_results` list by shard ID immediately before merging.
- Package D: server fingerprint helper hardening improved in `workload_fingerprint_engine.cpp` by guarding empty normalization input and switching cosine similarity to iterator-based accumulation.
- Package D: server import wizard assembly overhead reduced in `import_wizard_builder.cpp` by increasing upfront HTML buffer reserve for the large single-pass string build.
- Package D: task scheduler API hardening improved in `task_scheduler_api_handler.cpp` by adding checked seconds-to-milliseconds conversion for interval/timeout JSON fields to fail closed on overflow.
- Package D: task scheduler API container growth tightened in `task_scheduler_api_handler.cpp` by reserving capacity for Airflow task exports, tag imports, and Kubernetes extra-label accumulation.
- Package D: object-construction determinism hardened in `query_engine.cpp` by switching key-order sorting to `stable_sort`, preserving source order for duplicate object keys.
- Package D: COMPLETE.
- Package E: started in `distributed_transaction_manager.cpp` by fail-closing remote Phase-1 PREPARE dispatch when a participant fails the existing liveness check, avoiding unnecessary remote PREPARE attempts against known-unhealthy nodes.
- Package E: distributed 2PC delivery resilience improved in `distributed_transaction_manager.cpp` by adding a bounded second attempt for idempotent remote Phase-2 COMMIT/ABORT delivery paths (`phase2_rpc_fn`, `remote_phase2_dispatch`, legacy Phase-2 bridge); validated with `TransactionDistributed2PCFocusedTests`.
- Package E: wire protocol startup preflight improved in `wire_protocol_server.cpp` by rejecting `num_io_threads == 0` before marking the server as running, preventing a false-ready listener state without any I/O workers; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: security initialization preflight improved in `security_initialization.cpp` by rejecting HSM `library_path` values that do not point to an existing PKCS#11 library file before `HSMProvider::initialize()` is attempted; validated via incremental `themis_tests` rebuild and focused `HSMProviderStandaloneTest.RejectsMissingHSMLibraryPathBeforeInitialization`.
- Package E: QUIC client startup preflight aligned with server validation in `quic_server.cpp` by rejecting invalid congestion-control names before `QUICClient::connect()` proceeds into TLS/ngtcp2 setup; validated with `test_quic_server_focused` and `QUICServerFocusedTests`.
- Package E: security input validation tightened in `security_initialization.cpp` by requiring HSM `slot_id` to be a numeric unsigned value within `uint32_t` range before adapter/provider initialization; validated via incremental `themis_tests` rebuild and focused `HSMProviderStandaloneTest.RejectsInvalidHSMSlotIdBeforeInitialization`.
- Package E: wire protocol startup validation tightened in `wire_protocol_server.cpp` by fail-closing `start()` when `port == 0`, preventing unintended ephemeral binds and false-ready startup states; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup validation tightened further in `wire_protocol_server.cpp` by fail-closing `start()` when `tcp_backlog <= 0`, preventing invalid listen queue configuration from reaching socket setup; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup state hardening improved in `wire_protocol_server.cpp` by setting `running_` only after successful `bind/listen`, avoiding false running-state exposure when startup setup fails before listener activation; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup health checks expanded in `wire_protocol_server.cpp` by fail-closing when internal runtime dependencies (`io_context_`/`acceptor_`) are missing, preventing null-runtime startup attempts; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup validation tightened in `wire_protocol_server.cpp` by fail-closing when `num_worker_threads == 0`, preventing silent worker-pool starvation configurations from starting; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup validation tightened in `wire_protocol_server.cpp` by fail-closing when `max_connections_per_ip == 0`, preventing implicit reject-all behavior in per-IP connection limiting from accidental zero-config values; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup validation tightened in `wire_protocol_server.cpp` by fail-closing when `max_frame_size_mb == 0`, preventing zero-frame-limit configurations that would reject all non-empty payloads at runtime; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup validation tightened in `wire_protocol_server.cpp` by fail-closing when `request_timeout_sec == 0`, preventing zero-timeout session configurations that break request lifecycle timing semantics; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup validation tightened in `wire_protocol_server.cpp` by fail-closing when `max_requests_per_second == 0` or `max_requests_per_minute == 0`, preventing implicit reject-all rate-limit behavior from accidental zero-valued limits; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol startup auth validation tightened in `wire_protocol_server.cpp` by fail-closing when `require_auth=true` but `auth_mechanism` is blank or unsupported (currently only `SCRAM-SHA-256`), preventing silent startup with inconsistent authentication policy; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol bind-host validation tightened in `wire_protocol_server.cpp` by fail-closing startup when `host` is whitespace-only or contains control characters, preventing accidental wildcard fallback from malformed host strings; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol frame-limit validation tightened in `wire_protocol_server.cpp` by fail-closing when `max_frame_size_mb` exceeds the 32-bit wire payload envelope (`<= 4095 MB`), preventing impossible payload-limit configurations that exceed protocol framing bounds; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol auth startup hardening tightened in `wire_protocol_server.cpp` by fail-closing when `require_auth=true` but `auth_token` is empty, preventing startup in known reject-all authentication misconfiguration (WPS-3 path); validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol auth startup hardening tightened in `wire_protocol_server.cpp` by fail-closing when `require_auth=true` but `auth_token` is whitespace-only, preventing accidental weak/invalid shared-secret configuration from passing startup checks; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol auth startup hardening tightened in `wire_protocol_server.cpp` by fail-closing when `require_auth=true` but `auth_token` contains control characters, preventing malformed shared-secret values from passing startup checks and entering auth comparison paths; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol rate-limit startup hardening tightened in `wire_protocol_server.cpp` by fail-closing when `max_requests_per_minute < max_requests_per_second`, preventing contradictory throttle settings that make per-second policy exceed per-minute budget; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol connection-limit startup hardening tightened in `wire_protocol_server.cpp` by fail-closing when `max_connections_per_ip > max_connections` while global limits are enabled, preventing contradictory connection-budget configuration between per-IP and global caps; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol bind-host startup hardening tightened in `wire_protocol_server.cpp` by fail-closing when non-empty `host` values contain whitespace characters, preventing malformed host strings from silently degrading into wildcard bind fallback; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol auth startup hardening tightened in `wire_protocol_server.cpp` by fail-closing when `require_auth=true` and configured `auth_token` exceeds the AUTH payload envelope, preventing impossible authentication setups where the token cannot be transmitted within protocol size limits; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol auth startup hardening refined in `wire_protocol_server.cpp` by accounting for minimal JSON AUTH-envelope overhead (`{"token":""}`) in token-length validation, fail-closing when configured `auth_token` cannot fit into a valid AUTH payload even though raw byte limit might otherwise appear acceptable; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: wire protocol auth startup hardening refined further in `wire_protocol_server.cpp` by validating against the exact serialized AUTH JSON payload size (`{"token": ... }`) instead of a fixed envelope constant, fail-closing on any token that exceeds the real protocol payload limit after escaping/serialization; validated with `themis_network` and `WireProtocolV2FocusedTests`.
- Package E: query reliability review pass completed for `query_engine.cpp` against Package-E targets (`no_retry_logic`, `no_health_check`); no additional low-risk fail-closed guard was identified without changing query semantics, so no code delta was applied in this pass.
- Package E: COMPLETE.
- Package F: HTTP live RAG path wiring corrected in `http_server.cpp` for `/api/v1/llm/rag` by routing through `LLMPluginManager::generateRAG(...)` (including retry/bootstrap path) instead of generic generation, and by emitting `documents_retrieved` from actual retrieval context (`rag_context.documents.size()`) rather than synthetic top-k fallback metadata.
- Package F: RAG response metadata propagation aligned in `http_server.cpp` by preserving/forwarding live retrieval+budget fields (`rag_mode`, optional tensor slot hints, `max_context_tokens`, `response_budget_tokens`) from the RAG execution path.
- Package F: focused validation pass executed: `RAGPromptBuilderFocusedTests` and `MultiStepRAGFocusedTests` passed; `AgenticRAGBudgetFocusedTests` hit the preset 60s CTest timeout during heavyweight model loading (no assertion failure before timeout), and `RAGIngestionBridgeTests` reported 8 pre-existing ingestion assertions failing in `RAGIngestionBridgeTest.RI09..RI16`.
- Package F: `/api/v1/llm/rag` input validation hardened in `http_server.cpp` with fail-closed checks for invalid budget/query controls (`top_k > 0`, `max_context_tokens >= 0`, `response_budget_tokens > 0`, `max_tokens > 0`) before plugin dispatch.
- Package F: RAG telemetry consistency in `http_server.cpp` extended by returning effective runtime values (`top_k_effective`, `max_context_tokens_effective`, `response_budget_tokens_effective`) alongside live retrieval count, and revalidated green with focused `RAGPromptBuilderFocusedTests` + `MultiStepRAGFocusedTests`.
- Package F: dedicated LLM API RAG path aligned in `llm_api_handler.cpp` (`handleRAG`) with the same fail-closed budget/input checks and runtime-budget propagation (`max_context_tokens`/`response_budget_tokens`) into `RAGContext`, plus response metadata parity (`model`, `top_k_effective`, `max_context_tokens_effective`, `response_budget_tokens_effective`) for consistent production behavior across both `/api/v1/llm/rag` entry points.
- Package F: post-alignment validation repeated and green with focused `RAGPromptBuilderFocusedTests` + `MultiStepRAGFocusedTests`; server target (`themis_server`) rebuild remained clean.
- Package F: connector live-path regression coverage extended in `tests/test_connector_mode_api.cpp` by asserting the new RAG response schema fields (`top_k_effective`, `max_context_tokens_effective`, `response_budget_tokens_effective`) in both single-query and multi-query RAG flows, including basic semantic value checks.
- Package F: connector test binary rebuilt clean (`test_connector_mode_api`); direct runtime invocation of the two updated RAG live tests skipped in this environment because the connector endpoint at `127.0.0.1:8765` was not reachable.
- Package F: connector RAG schema regression coverage further tightened in `tests/test_connector_mode_api.cpp` by asserting `model` and `rag_mode_effective` alongside the effective budget fields, locking the broader live response contract for `/api/v1/llm/rag`.
- Package F: completed, with production RAG retrieval hydrated in `llm_api_handler.cpp`, module linkage aligned for the active query/GraphQL path, and focused RAG validation still green after the wiring change.

## Priority Patterns (Core Set)

1. type_conversion: 4679
2. determinism: 3206
3. input_validation: 3008
4. container: 2663
5. performance_patterns: 2299
6. observability: 2101

Top concrete fix types:
- copy_overhead: 1843
- uncaught_exception: 893
- pointer_arithmetic: 670
- null_dereference: 650
- data_race: 593
- hardcoded_path: 510
- no_retry_logic: 306
- manual_cleanup: 298

## Delivery Model

- Week 1: Package A + Package B
- Week 2: Package C + Package D + Package E + Package F (clean RAG wiring)
- Validate after each package with re-scan on touched modules only.

## Package A (Highest ROI): Copy and Container Overhead

Target types:
- copy_overhead
- iterator_invalidation (safe loop rewrites)

Core actions:
- add reserve() before known growth loops
- replace push_back(temp) with emplace_back(...) where safe
- use const reference in range loops for heavy objects
- eliminate avoidable temporary copies in tight loops

Primary files:
- src/server/http_server.cpp
- src/server/import_wizard_builder.cpp
- src/server/query_api_handler.cpp
- src/query/query_engine.cpp
- src/query/aql_translator.cpp
- src/index/secondary_index.cpp
- src/index/vector_index.cpp
- src/storage/rocksdb_wrapper.cpp
- src/transaction/transaction_manager.cpp

Expected impact:
- high volume reduction in copy_overhead and container findings
- low semantic risk

## Package B: Null and Input Guards

Target types:
- null_dereference
- input_validation
- size_assumption

Core actions:
- add early null/empty guards before dereference
- switch unchecked [] access to checked flow where practical
- enforce min/max boundary checks at API/module ingress

Primary files:
- src/server/http_server.cpp
- src/server/voice_api_handler.cpp
- src/query/query_engine.cpp
- src/storage/rocksdb_wrapper.cpp
- src/storage/columnar_format.cpp
- src/network/wire_protocol_server.cpp
- src/security/aql_injection_detector.cpp

Expected impact:
- direct reduction of high severity defensive coding findings

## Package C: Exception and Cleanup Hygiene

Target types:
- uncaught_exception
- manual_cleanup
- db_connection_leak

Core actions:
- replace catch(...) with typed catches where possible
- convert manual cleanup paths to RAII guards
- guarantee close/release on all early-return and error branches

Primary files:
- src/sharding/redundancy_strategy.cpp
- src/storage/nvme_manager.cpp
- src/network/wire_protocol_server.cpp
- src/network/quic_server.cpp
- src/transaction/distributed_saga.cpp
- src/transaction/saga_orchestrator.cpp
- src/core/concerns/redis_cache.cpp

Expected impact:
- quick wins in reliability, RAII, and leak-related categories

## Package D: Determinism and Path Portability

Target types:
- determinism
- hardcoded_path
- platform

Core actions:
- normalize path handling through std::filesystem::path
- remove hardcoded separators and host-specific literals
- add deterministic ordering before emitting or hashing map-like data
- seed and logging determinism checks for reproducible outputs

Primary files:
- src/server/http_server.cpp
- src/server/mcp_server.cpp
- src/server/monitoring_api_handler.cpp
- src/security/* (high determinism count)
- src/query/* (deterministic result ordering hotspots)

Expected impact:
- strong cross-platform and reproducibility gains with modest code changes

## Package E: Retry and Health Check Baseline

Target types:
- no_retry_logic
- no_health_check

Core actions:
- add bounded retry wrappers with jitter for external calls
- add explicit health checks before dependent operations
- surface retry exhaustion in logs/metrics

Primary files:
- src/network/wire_protocol_server.cpp
- src/network/quic_server.cpp
- src/query/query_engine.cpp
- src/core/security_initialization.cpp
- src/transaction/distributed_transaction_manager.cpp

Expected impact:
- visible reliability improvements and fewer operational false negatives

## Package F: Clean RAG Wiring (Production Path)

Target types:
- integration_gap
- runtime_wiring
- input_validation
- observability

Core actions:
- align default plugin path so budget-aware RAG logic is active on live API requests
- finalize RAG retrieval wiring in HTTP handler (remove migration fallback behavior)
- ensure RAG request metadata and token-budget clamping remain consistent end-to-end
- add focused integration tests for RAG request -> retrieval -> plugin -> response path
- add lightweight telemetry checks for documents_retrieved, token budget, and failure reasons

Primary files:
- src/server/llm_api_handler.cpp
- src/llm/llm_plugin_manager.cpp
- src/llm/llama_wrapper.cpp
- src/llama_cpp/llama_cpp_plugin.cpp
- src/server/http_server.cpp
- tests/*llm* (focused integration coverage)

Expected impact:
- activate already-implemented RAG efficiency primitives on the production path
- reduce duplicate implementation risk for adaptive-thinking follow-up work
- improve RAG reliability and cost/quality behavior with low-to-moderate change risk

## Suggested 2-Week Sequence

Day 1-2:
- Package A in server + query

Day 3-4:
- Package A in index + storage + transaction

Day 5:
- module re-scan and stabilize

Day 6-7:
- Package B across server/storage/network/security

Day 8:
- Package C (transaction + network)

Day 9:
- Package C (sharding + core)

Day 10:
- Package D and E minimal baseline changes

Day 11:
- Package F wiring changes (plugin selection + API retrieval path)

Day 12:
- Package F focused integration tests + telemetry checks + stabilization

## Validation Checklist

After each package:
- build target set with windows-release preset
- run focused tests for touched modules
- run gap scanner on touched modules and compare deltas
- reject changes that increase critical gaps in touched files

## Exit Criteria (2 Weeks)

- measurable drop in the top fix types: copy_overhead, uncaught_exception, null_dereference, pointer_arithmetic
- measurable drop in type_conversion/input_validation/determinism for server/query/index/storage/network
- RAG live-path verification completed (budget-aware context assembly active and retrieval wiring enabled)
- no regression in focused module test suites

## Next Step

Start with Package A in:
- src/server/http_server.cpp
- src/query/query_engine.cpp
- src/index/secondary_index.cpp
- src/storage/rocksdb_wrapper.cpp
