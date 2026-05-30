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
