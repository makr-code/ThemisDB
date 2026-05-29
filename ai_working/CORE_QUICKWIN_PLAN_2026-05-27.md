# Core Quick-Win Plan (2 Weeks)

Date: 2026-05-27
Source: ai_working/gap_scan_v3_summary.json + module gap reports
Scope: core, server, query, sharding, index, storage, network, transaction, security, auth

## Goal

Reduce high-volume, low-risk gap patterns quickly in core components without architecture redesign.

## Current Status

- Package A: completed in the focused server, query, index, and storage slices.
- Package B: in progress, with storage/null guards in server handlers, QueryEngine storage-backed paths, empty-input guards in AQL injection validation, range/finite checks in wire protocol vector/geo handlers, decoded-audio validation guards across voice API endpoints, strict query-limit validation in voice listing/search endpoints, graph/query/cursor range guards in the wire protocol, strict BPMN wire-request type validation, BPMN identifier hygiene checks (length/control-character guards), QUERY/CURSOR hardening for blank AQL and invalid cursor identifiers, expanded core wire input hygiene for GET/PUT/DELETE/BATCH/TRANSACTION/GRAPH handlers, and explicit JSON type-guards before core wire string extraction.
- Package B (latest): additional VECTOR_SEARCH ingress hardening (strict type checks for k/collection, identifier guard on optional collection, bounded positive k parsing), GEO_QUERY ingress hardening (strict collection/type/limit type guards, identifier guard for collection, bounded limit range, and explicit numeric guards for bbox/center/radius fields), TimeSeries ingress bounds/type validation (aggregation range, timestamp cast/window limits, bucket-size constraints), BPMN ingress payload/variables/query guards (max payload size, request-object checks, variables object type/field-count limits, strict variable-name hygiene, bounded max_history_events, and explicit history truncation signaling), AUTH ingress hardening (payload size bound, JSON-object/type checks, username identifier hygiene), and core CRUD/BATCH/transaction/graph/cursor ingress hardening (payload size limits, JSON-object checks, and blank-identifier rejection in GET/PUT/DELETE/BATCH_GET/BATCH_PUT/TRANSACTION_*/GRAPH_TRAVERSE/CURSOR_*), plus focused WireProtocolV1 mirror tests for the new blank/object/range guards including BPMN task/process-instance blanks.
- Validation delta: unified `themis_tests` link blocker for `RedisCacheCoordinator` symbols resolved by explicit test-target source wiring; targeted CTest rerun (`ThemisWireProtocolV1Tests`, `ThemisWireDeprecatedBridgeTests`) passed and direct gtest filter `WireProtocolV1Themis*` passed 61/61.
- Validation delta (ctest carry-over): previously listed failures from `testFailure` (`LlmBenchContinuousBatchScheduler` + nine `WirePerfBenchmark*` entries) were re-run via focused CTest regex and passed 10/10.

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
- Week 2: Package C + Package D + Package E
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

## Validation Checklist

After each package:
- build target set with windows-release preset
- run focused tests for touched modules
- run gap scanner on touched modules and compare deltas
- reject changes that increase critical gaps in touched files

## Exit Criteria (2 Weeks)

- measurable drop in the top fix types: copy_overhead, uncaught_exception, null_dereference, pointer_arithmetic
- measurable drop in type_conversion/input_validation/determinism for server/query/index/storage/network
- no regression in focused module test suites

## Next Step

Start with Package A in:
- src/server/http_server.cpp
- src/query/query_engine.cpp
- src/index/secondary_index.cpp
- src/storage/rocksdb_wrapper.cpp
