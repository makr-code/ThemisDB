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
- Package C: network slice advanced with HTTP/3/QUIC compatibility hardening for current ngtcp2/nghttp3 APIs in `quic_server.cpp`, `quic_transport.cpp`, and `http3_session.cpp` (header/API migration, callback completeness, transport-parameter wiring, and focused-linkage fixes), validated with `QUICServerFocusedTests` under `THEMIS_ENABLE_HTTP3=ON`.
- Package C: network validation extended with successful `Http3ProductionReadinessFocusedTests` under `THEMIS_ENABLE_HTTP3=ON` after focused target linkage alignment for nghttp3/ngtcp2.
- Package C: combined focused validation run completed (`QUICServerFocusedTests` + `Http3ProductionReadinessFocusedTests`) with both tests passing under `THEMIS_ENABLE_HTTP3=ON`.

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
