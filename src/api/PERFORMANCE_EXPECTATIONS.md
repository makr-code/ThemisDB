# PERFORMANCE_EXPECTATIONS - src/api

## Scope

- Module: src/api
- This file defines measurable API module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark file:
  - benchmarks/bench_api_endpoints.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| API-1 | GraphQL parse simple uncached path remains within release baseline budget | BM_GraphQL_Parse_Simple_Uncached |
| API-2 | GraphQL parse simple cached path remains within release baseline budget | BM_GraphQL_Parse_Simple_Cached |
| API-3 | GraphQL parse complex path remains within release baseline budget | BM_GraphQL_Parse_Complex_Uncached |
| API-4 | GraphQL invalid-query rejection path remains bounded | BM_GraphQL_Parse_Invalid |
| API-5 | GraphQL execution path remains within release baseline budget | BM_GraphQL_Execute_MockResolver |
| API-6 | correlation-id generation and header-check overhead remain bounded | BM_CorrelationId_Generate_UUIDv4, BM_CorrelationId_Header_Check |
| API-7 | JSON serialization path remains within release baseline budget | BM_Json_Serialize_SingleDocument, BM_Json_Serialize_PaginatedList |
| API-8 | JSON deserialization and HTTP message build paths remain bounded | BM_Json_Deserialize_PutRequest, BM_HttpMessage_Build_GetRequest, BM_HttpMessage_Build_PostRequest |
| API-9 | API endpoint hot path remains within release baseline budget | BM_HttpServer_Health_Endpoint, BM_HttpServer_Documents_List_Persistent |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | parser/execute/serialization path p99 <= release threshold | p99 from mapped bench_api_endpoints cases |
| AG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: api/performance)

- Verified benchmark source:
  - benchmarks/bench_api_endpoints.cpp
- Verified mapping surfaces:
  - GraphQL parse/execute benchmark cases
  - correlation and serialization benchmark cases
  - endpoint hot-path benchmark cases
- Result:
  - Referenced benchmark cases exist in current benchmark source.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.