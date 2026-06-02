# api Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: api
- Generated: 2026-06-02 11:55:47
- Status: Critical Findings Present
- Total Findings: 131
- Actionable Findings (Critical + High): 86
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 7 |
| High | 79 |
| Medium | 45 |
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
| src/api/graphql.cpp | 63 | 2 | 48 | 13 | 0 |
| src/api/themisdb_grpc_service.cpp | 27 | 3 | 19 | 5 | 0 |
| src/api/geo_index_hooks.cpp | 16 | 0 | 3 | 13 | 0 |
| src/api/otlp_exporter.cpp | 11 | 0 | 4 | 7 | 0 |
| src/api/graphql_ws_handler.cpp | 6 | 0 | 3 | 3 | 0 |
| src/api/grpc_server.cpp | 3 | 1 | 0 | 2 | 0 |
| src/api/tracing_middleware.cpp | 2 | 0 | 2 | 0 | 0 |
| src/api/ws_handler.cpp | 2 | 1 | 0 | 1 | 0 |
| src/api/federation_admin_handler.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/api/graphql.cpp
Total findings: 63

- Line 37: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.document = cached_plan->parsed_document;
- Line 1664: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: docField.description = "The new document state (null for DELETED events)";
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4453 fix(graphql): resolve varia... (2026-04-07) | #4200 [WIP] Implement Gra
- Line 716: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Executor::Result Executor::execute(const Document &document, const ExecutionContext &context,
- Line 1451: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1452: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docQuery.arguments["id"]         = {"ID", true, false, nullptr};
- Line 1459: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docsQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1460: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docsQuery.arguments["limit"]      = {"Int", false, false, nullptr};
- Line 1461: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docsQuery.arguments["offset"]     = {"Int", false, false, nullptr};
- Line 1467: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
- Line 1467: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
  Confidence: band=very_high; score=0.9
- Line 1472: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlQuery.arguments["query"]     = {"String", true, false, nullptr};
- Line 1473: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlQuery.arguments["variables"] = {"JSON", false, false, nullptr};
- Line 1499: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vectorQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1500: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vectorQuery.arguments["vector"]     = {"Float", true, true, nullptr};
- Line 1501: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vectorQuery.arguments["k"]          = {"Int", false, false, nullptr};
- Line 1508: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graphQuery.arguments["startNode"] = {"ID", true, false, nullptr};
- Line 1509: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graphQuery.arguments["depth"]     = {"Int", false, false, nullptr};
- Line 1510: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graphQuery.arguments["direction"] = {"String", false, false, nullptr};
- Line 1518: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["series"] = {"String", true, false, nullptr};
- Line 1519: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["from"]   = {"String", true, false, nullptr};
- Line 1520: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["to"]     = {"String", true, false, nullptr};
- Line 1521: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["limit"]  = {"Int", false, false, nullptr};
- Line 1529: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsLatestQuery.arguments["series"] = {"String", true, false, nullptr};
- Line 1530: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsLatestQuery.arguments["count"]  = {"Int", false, false, nullptr};
- Line 1538: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1539: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["center"]     = {"GeoPointInput", true, false, nullptr};
- Line 1540: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["radiusKm"]   = {"Float", true, false, nullptr};
- Line 1541: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["limit"]      = {"Int", false, false, nullptr};
- Line 1557: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createDoc.arguments["collection"] = {"String", true, false, nullptr};
- Line 1558: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createDoc.arguments["input"]      = {"DocumentInput", true, false, nullptr};
- Line 1565: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: updateDoc.arguments["collection"] = {"String", true, false, nullptr};
- Line 1566: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: updateDoc.arguments["id"]         = {"ID", true, false, nullptr};
- Line 1567: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: updateDoc.arguments["input"]      = {"DocumentInput", true, false, nullptr};
- Line 1574: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: deleteDoc.arguments["collection"] = {"String", true, false, nullptr};
- Line 1575: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: deleteDoc.arguments["id"]         = {"ID", true, false, nullptr};
- Line 1582: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["source"]     = {"ID", true, false, nullptr};
- Line 1583: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["target"]     = {"ID", true, false, nullptr};
- Line 1584: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["type"]       = {"String", true, false, nullptr};
- Line 1585: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["properties"] = {"JSON", false, false, nullptr};
- Line 1593: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["series"]    = {"String", true, false, nullptr};
- Line 1594: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["timestamp"] = {"String", true, false, nullptr};
- Line 1595: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["value"]     = {"Float", true, false, nullptr};
- Line 1596: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["tags"]      = {"JSON", false, false, nullptr};
- Line 1610: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlMut.arguments["query"]     = {"String", true, false, nullptr};
- Line 1611: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlMut.arguments["variables"] = {"JSON", false, false, nullptr};
- Line 1687: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: onChangeField.arguments["collection"] = {"String", true, false, nullptr};
- Line 1688: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: onChangeField.arguments["filter"]     = {"ChangeFilter", false, false, nullptr};
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: list.push_back(*valResult);
- Line 639: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\n';
- Line 642: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\r';
- Line 645: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\t';
- Line 648: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '"';
- Line 651: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\\';
- Line 849: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultList.push_back(executeSelections(resolvedField.selections, item, context));
  Confidence: band=high; score=0.74
- Line 850: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resultList.push_back(executeSelections(resolvedField.selections, item, context));
- Line 881: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultList.push_back(executeSelections(field.selections, item, context));
  Confidence: band=high; score=0.74
- Line 882: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resultList.push_back(executeSelections(field.selections, item, context));
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

### src/api/themisdb_grpc_service.cpp
Total findings: 27

- Line 549: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 588: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 642: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 67: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: /// Kept as a local alias for backward compatibility with call sites below.
  Confidence: band=high; score=0.8
- Line 294: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: err->set_message("document not found or delete failed");
- Line 391: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (req->query().empty()) {
- Line 409: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = aql_engine_->execute(req->query());
- Line 436: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 441: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 446: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 451: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 456: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 469: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (req->query().empty()) {
- Line 480: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = aql_engine_->execute(req->query());
- Line 492: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (ctx->IsCancelled()) {
- Line 638: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
- Line 718: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (req->query().empty() || req->collection().empty()) {
- Line 731: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
- Line 740: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
- Line 740: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
  Confidence: band=very_high; score=0.9
- Line 743: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = aql_engine_->execute(aql);
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

### src/api/geo_index_hooks.cpp
Total findings: 16

- Line 505: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: JSON = nullptr;
  Context: THEMIS_WARN("Geo hook atomic delete JSON parse error for {}:{}: {}", table, pk, e.what());
- Line 508: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_WARN("Geo hook atomic delete error for {}:{}: {}", table, pk, e.what());
- Line 576: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_WARN("Geo hook delete error for {}:{}: {}", table, pk, e.what());
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
- Line 281: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 497: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 541: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);

### src/api/otlp_exporter.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4208 feat(api/otlp): exponential... (2026-03-15) | #4219 feat(api): wire Tra
- Line 43: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buf->append(ptr, size * nmemb);
- Line 306: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(queue_mutex_);
- Line 426: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
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
- Line 516: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resource_attrs.push_back({{"key", key}, {"value", {{"stringValue", value}}}});
- Line 541: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: json status;
- Line 550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"key", k}, {"value", {{"stringValue", v}}}});
  Confidence: band=high; score=0.74
- Line 551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attrs.push_back({{"key", k}, {"value", {{"stringValue", v}}}});

### src/api/graphql_ws_handler.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4310 [High Priority] Implement G... (2026-03-17) | #4200 [WIP] Implement Gra
- Line 252: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!alive->load(std::memory_order_acquire)) {
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
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_arr.push_back(json{{"message", e.toString()}});

### src/api/grpc_server.cpp
Total findings: 3

- Line 204: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 68: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcApiServer::initialize(const GrpcServerConfig& config) {
  Confidence: band=medium; score=0.66
- Line 139: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcApiServer::start() {
  Confidence: band=medium; score=0.66

### src/api/tracing_middleware.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4219 feat(api): wire TracingMidd... (2026-03-14) | #3546 docs(api): sync api
- Line 95: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.attributes["http.status_code"] = std::to_string(http_status);

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
Total findings: 1

- Line 69: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {"status",         "success"}};

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
