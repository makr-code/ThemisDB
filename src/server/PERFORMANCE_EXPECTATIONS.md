# PERFORMANCE_EXPECTATIONS - src/server

<!-- Status: current | validated: 2026-08-17 -->

## Scope

- Module: src/server
- This file defines measurable server module performance expectations for release gating.

## Phase 5 Hardening Gates (Q3 2026)

| Gate ID | Source | Description |
|---|---|---|
| SVR-01..SVR-08 | `benchmarks/server/bench_server_hotpaths.cpp` | 8 benchmark release gates covering server hot-paths; Wave-7 baselines (read p99≤200µs, write ≥80k ops/s) confirmed with no regressions from retry/timeout logic |
| WSR-01..WSR-16 | `tests/server/test_server_phase5_hardening.cpp` | P5-S01 wire-protocol retry latency gates (exponential backoff, budget cap, jitter paths) |
| HST-01..HST-12 | `tests/server/test_server_phase5_hardening.cpp` | P5-S02 HTTP timeout and graceful-shutdown timing gates (deadline enforcement, drain ordering) |

These gates supplement the baseline SRVP-1..SRVP-4 / SRVG-1..SRVG-3 expectations below.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_api_endpoints.cpp
  - benchmarks/bench_stream_protocol.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| SRVP-1 | GraphQL parse and mock-execution helper paths remain bounded | BM_GraphQL_Parse_Simple_Uncached, BM_GraphQL_Parse_Simple_Cached, BM_GraphQL_Parse_Complex_Uncached, BM_GraphQL_Parse_Invalid, BM_GraphQL_Execute_MockResolver |
| SRVP-2 | request-building and JSON serialization helper paths remain bounded | BM_Json_Serialize_SingleDocument, BM_Json_Serialize_PaginatedList, BM_Json_Deserialize_PutRequest, BM_HttpMessage_Build_GetRequest, BM_HttpMessage_Build_PostRequest |
| SRVP-3 | health and persistent document-list endpoint paths remain bounded | BM_HttpServer_Health_Endpoint, BM_HttpServer_Documents_List_Persistent |
| SRVP-4 | stream protocol framing, compression, metrics, and buffer-pool helper paths remain bounded | BM_StreamProtocol_FrameHeaderBuild, BM_StreamProtocol_LZ4Roundtrip, BM_StreamProtocol_MetricsSnapshot, BM_StreamProtocol_BufferPoolRoundtrip |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| SRVG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| SRVG-2 | mapped server hot-path p99 <= release threshold | p99 from mapped server benchmark cases |
| SRVG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional server benchmark scenarios are introduced.

## Sourcecode Verification (Module: server/performance)

- Verified benchmark sources:
  - benchmarks/bench_api_endpoints.cpp
  - benchmarks/bench_stream_protocol.cpp
- Verified mapping surfaces:
  - GraphQL parse and mock-execution behavior
  - HTTP message, JSON, and selected endpoint behavior
  - stream protocol framing, compression, metrics, and buffer-pool behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.