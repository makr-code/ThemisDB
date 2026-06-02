# api Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: api
- Generated: 2026-06-02 11:09:12
- Status: Critical Findings Present
- Total Findings: 31
- Actionable Findings (Critical + High): 5
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 4 |
| Medium | 26 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| memory | 52 |
| reliability | 18 |
| performance_patterns | 16 |
| container | 14 |
| observability | 13 |
| performance | 6 |
| concurrency | 4 |
| exception_safety | 3 |
| raii | 2 |
| audit_logging | 1 |
| legacy_duplication | 1 |
| security | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/api/geo_index_hooks.cpp | 7 | 0 | 0 | 7 | 0 |
| src/api/themisdb_grpc_service.cpp | 7 | 0 | 2 | 5 | 0 |
| src/api/graphql.cpp | 6 | 0 | 1 | 5 | 0 |
| src/api/otlp_exporter.cpp | 4 | 0 | 0 | 4 | 0 |
| src/api/graphql_ws_handler.cpp | 3 | 0 | 1 | 2 | 0 |
| src/api/grpc_server.cpp | 2 | 0 | 0 | 2 | 0 |
| src/api/ws_handler.cpp | 2 | 1 | 0 | 1 | 0 |
| src/api/federation_admin_handler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/api/tracing_middleware.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/api/geo_index_hooks.cpp
Total findings: 7

- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 541: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74

### src/api/themisdb_grpc_service.cpp
Total findings: 7

- Line 67: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: /// Kept as a local alias for backward compatibility with call sites below.
  Confidence: band=high; score=0.8
- Line 740: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
  Confidence: band=very_high; score=0.9
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: themisdb_grpc_service.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:49:01
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "api/themisdb_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 23: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->sparse_query()) + "')"
  Confidence: band=high; score=0.74
- Line 740: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
  Confidence: band=high; score=0.74

### src/api/graphql.cpp
Total findings: 6

- Line 1467: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
  Confidence: band=very_high; score=0.9
- Line 849: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultList.push_back(executeSelections(resolvedField.selections, item, context));
  Confidence: band=high; score=0.74
- Line 881: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultList.push_back(executeSelections(field.selections, item, context));
  Confidence: band=high; score=0.74
- Line 1117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_list.push_back(Value::object(std::move(field_obj)));
  Confidence: band=high; score=0.74
- Line 1186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_list.push_back(Value::object(std::move(field_obj)));
  Confidence: band=high; score=0.74
- Line 1467: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
  Confidence: band=high; score=0.74

### src/api/otlp_exporter.cpp
Total findings: 4

- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void OtlpExporter::start() {
  Confidence: band=medium; score=0.66
- Line 503: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //     "resource": {"attributes": [{"key":"service.name","value":{"stringValue":"…"}}]},
  Confidence: band=high; score=0.74
- Line 550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"key", k}, {"value", {{"stringValue", v}}}});
  Confidence: band=high; score=0.74

### src/api/graphql_ws_handler.cpp
Total findings: 3

- Line 416: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = field.arguments.find("collection");
  Confidence: band=very_high; score=0.9
- Line 134: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: GraphQLWsHandler::handleConnectionInit(const std::string& /*payload_json*/)
  Confidence: band=medium; score=0.66
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_arr.push_back(json{{"message", e.toString()}});
  Confidence: band=high; score=0.74

### src/api/grpc_server.cpp
Total findings: 2

- Line 68: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcApiServer::initialize(const GrpcServerConfig& config) {
  Confidence: band=medium; score=0.66
- Line 139: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcApiServer::start() {
  Confidence: band=medium; score=0.66

### src/api/ws_handler.cpp
Total findings: 2

- Line 113: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: const auto result = auth_->authorize(token, "cdc:subscribe");
  Confidence: band=very_high; score=0.99
- Line 97: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_hdr = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/api/federation_admin_handler.cpp
Total findings: 0


### src/api/tracing_middleware.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
