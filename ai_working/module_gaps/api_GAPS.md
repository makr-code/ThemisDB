# api Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: api
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 173
- Actionable Findings (Critical + High): 99
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 9 |
| High | 90 |
| Medium | 74 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| memory | 55 |
| container | 40 |
| reliability | 22 |
| performance_patterns | 16 |
| observability | 13 |
| security | 9 |
| performance | 6 |
| concurrency | 4 |
| exception_safety | 3 |
| raii | 3 |
| audit_logging | 1 |
| legacy_duplication | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/api/graphql.cpp | 89 | 3 | 53 | 33 | 0 |
| src/api/themisdb_grpc_service.cpp | 31 | 3 | 22 | 6 | 0 |
| src/api/geo_index_hooks.cpp | 22 | 0 | 3 | 19 | 0 |
| src/api/otlp_exporter.cpp | 13 | 0 | 4 | 9 | 0 |
| src/api/graphql_ws_handler.cpp | 7 | 1 | 3 | 3 | 0 |
| src/api/grpc_server.cpp | 5 | 1 | 2 | 2 | 0 |
| src/api/federation_admin_handler.cpp | 3 | 0 | 2 | 1 | 0 |
| src/api/ws_handler.cpp | 2 | 1 | 0 | 1 | 0 |
| src/api/tracing_middleware.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/api/graphql.cpp
Total findings: 89

- Line 36: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.document = cached_plan->parsed_document;
- Line 840: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator fieldIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto fieldIt    = obj.find(resolvedField.name);
- Line 1663: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: docField.description = "The new document state (null for DELETED events)";
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 715: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Executor::Result Executor::execute(const Document &document, const ExecutionContext &context,
- Line 839: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto &obj = parent->asObject();
- Line 869: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto &obj = parent->asObject();
- Line 937: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return it != types_.end() ? &it->second : nullptr;
- Line 937: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != types_.end() ? &it->second : nullptr;
- Line 1151: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: type_obj["name"]        = Value::string(type->name);
- Line 1152: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: type_obj["description"] = Value::string(type->description);
- Line 1450: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1451: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docQuery.arguments["id"]         = {"ID", true, false, nullptr};
- Line 1458: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docsQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1459: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docsQuery.arguments["limit"]      = {"Int", false, false, nullptr};
- Line 1460: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: docsQuery.arguments["offset"]     = {"Int", false, false, nullptr};
- Line 1466: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
- Line 1466: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
  Confidence: band=very_high; score=0.9
- Line 1471: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlQuery.arguments["query"]     = {"String", true, false, nullptr};
- Line 1472: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlQuery.arguments["variables"] = {"JSON", false, false, nullptr};
- Line 1498: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vectorQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1499: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vectorQuery.arguments["vector"]     = {"Float", true, true, nullptr};
- Line 1500: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vectorQuery.arguments["k"]          = {"Int", false, false, nullptr};
- Line 1507: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graphQuery.arguments["startNode"] = {"ID", true, false, nullptr};
- Line 1508: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graphQuery.arguments["depth"]     = {"Int", false, false, nullptr};
- Line 1509: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: graphQuery.arguments["direction"] = {"String", false, false, nullptr};
- Line 1517: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["series"] = {"String", true, false, nullptr};
- Line 1518: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["from"]   = {"String", true, false, nullptr};
- Line 1519: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["to"]     = {"String", true, false, nullptr};
- Line 1520: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsRangeQuery.arguments["limit"]  = {"Int", false, false, nullptr};
- Line 1528: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsLatestQuery.arguments["series"] = {"String", true, false, nullptr};
- Line 1529: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tsLatestQuery.arguments["count"]  = {"Int", false, false, nullptr};
- Line 1537: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["collection"] = {"String", true, false, nullptr};
- Line 1538: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["center"]     = {"GeoPointInput", true, false, nullptr};
- Line 1539: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["radiusKm"]   = {"Float", true, false, nullptr};
- Line 1540: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: geoQuery.arguments["limit"]      = {"Int", false, false, nullptr};
- Line 1556: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createDoc.arguments["collection"] = {"String", true, false, nullptr};
- Line 1557: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createDoc.arguments["input"]      = {"DocumentInput", true, false, nullptr};
- Line 1564: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: updateDoc.arguments["collection"] = {"String", true, false, nullptr};
- Line 1565: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: updateDoc.arguments["id"]         = {"ID", true, false, nullptr};
- Line 1566: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: updateDoc.arguments["input"]      = {"DocumentInput", true, false, nullptr};
- Line 1573: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: deleteDoc.arguments["collection"] = {"String", true, false, nullptr};
- Line 1574: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: deleteDoc.arguments["id"]         = {"ID", true, false, nullptr};
- Line 1581: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["source"]     = {"ID", true, false, nullptr};
- Line 1582: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["target"]     = {"ID", true, false, nullptr};
- Line 1583: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["type"]       = {"String", true, false, nullptr};
- Line 1584: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: createEdge.arguments["properties"] = {"JSON", false, false, nullptr};
- Line 1592: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["series"]    = {"String", true, false, nullptr};
- Line 1593: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["timestamp"] = {"String", true, false, nullptr};
- Line 1594: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["value"]     = {"Float", true, false, nullptr};
- Line 1595: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: insertTsPoint.arguments["tags"]      = {"JSON", false, false, nullptr};
- Line 1609: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlMut.arguments["query"]     = {"String", true, false, nullptr};
- Line 1610: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aqlMut.arguments["variables"] = {"JSON", false, false, nullptr};
- Line 1686: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: onChangeField.arguments["collection"] = {"String", true, false, nullptr};
- Line 1687: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: onChangeField.arguments["filter"]     = {"ChangeFilter", false, false, nullptr};
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.document.operations.push_back(std::move(*opResult));
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(convertToParseError(opResult.error()));
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: op.variables.push_back(std::move(*varDefResult));
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: op.selections.push_back(std::move(*fieldResult));
- Line 330: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: field.selections.push_back(std::move(*nestedFieldResult));
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: list.push_back(*valResult);
- Line 638: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\n';
- Line 641: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\r';
- Line 644: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\t';
- Line 647: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '"';
- Line 650: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\\';
- Line 848: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultList.push_back(executeSelections(resolvedField.selections, item, context));
  Confidence: band=high; score=0.74
- Line 849: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resultList.push_back(executeSelections(resolvedField.selections, item, context));
- Line 880: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: resultList.push_back(executeSelections(field.selections, item, context));
  Confidence: band=high; score=0.74
- Line 881: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resultList.push_back(executeSelections(field.selections, item, context));
- Line 1116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_list.push_back(Value::object(std::move(field_obj)));
  Confidence: band=high; score=0.74
- Line 1117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: field_list.push_back(Value::object(std::move(field_obj)));
- Line 1121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: type_list.push_back(Value::object(std::move(type_obj)));
- Line 1185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_list.push_back(Value::object(std::move(field_obj)));
  Confidence: band=high; score=0.74
- Line 1186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: field_list.push_back(Value::object(std::move(field_obj)));
- Line 1260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geoPointInputType.fields.push_back(latInputField);
- Line 1266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geoPointInputType.fields.push_back(lonInputField);
- Line 1466: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
  Confidence: band=high; score=0.74
- Line 1483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryType.fields.push_back(apiVersionField);
- Line 1492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryType.fields.push_back(schemaVersionField);
- Line 1530: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryType.fields.push_back(tsLatestQuery);
- Line 1541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryType.fields.push_back(geoQuery);
- Line 1634: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changeFilterInput.fields.push_back(filterTypeField);
- Line 1647: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changeEventType.fields.push_back(seqField);
- Line 1653: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changeEventType.fields.push_back(evtTypeField);
- Line 1659: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changeEventType.fields.push_back(keyField);
- Line 1665: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changeEventType.fields.push_back(docField);
- Line 1671: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: changeEventType.fields.push_back(tsField);

### src/api/themisdb_grpc_service.cpp
Total findings: 31

- Line 548: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 587: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 641: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 66: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: /// Kept as a local alias for backward compatibility with call sites below.
  Confidence: band=high; score=0.8
- Line 293: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: err->set_message("document not found or delete failed");
- Line 390: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (req->query().empty()) {
- Line 408: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = aql_engine_->execute(req->query());
- Line 429: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 435: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 440: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 445: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 450: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 455: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: row->set_data(*result);
- Line 468: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (req->query().empty()) {
- Line 479: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = aql_engine_->execute(req->query());
- Line 599: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto hits = vector_index_->search(embedding, k, nullptr);
- Line 599: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto hits = vector_index_->search(embedding, k, nullptr);
- Line 637: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
- Line 717: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (req->query().empty() || req->collection().empty()) {
- Line 730: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
- Line 739: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
- Line 739: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
  Confidence: band=very_high; score=0.9
- Line 742: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = aql_engine_->execute(aql);
- Line 898: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: themisdb_grpc_service.cpp | Version: 0.0.15
  Confidence: band=high; score=0.74
- Line 9: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "api/themisdb_grpc_service.h"
  Confidence: band=high; score=0.74
- Line 22: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: // This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: err->set_message("document not found or delete failed");
- Line 666: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->sparse_query()) + "')"
  Confidence: band=high; score=0.74
- Line 739: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
  Confidence: band=high; score=0.74

### src/api/geo_index_hooks.cpp
Total findings: 22

- Line 504: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: JSON = nullptr;
  Context: THEMIS_WARN("Geo hook atomic delete JSON parse error for {}:{}: {}", table, pk, e.what());
- Line 507: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_WARN("Geo hook atomic delete error for {}:{}: {}", table, pk, e.what());
- Line 575: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: error = nullptr;
  Context: THEMIS_WARN("Geo hook delete error for {}:{}: {}", table, pk, e.what());
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(byte);
- Line 179: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
- Line 280: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(byte);
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
- Line 388: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(byte);
- Line 496: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);
- Line 540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(byte);
  Confidence: band=high; score=0.74
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(byte);
- Line 550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
  Confidence: band=high; score=0.74
- Line 551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
- Line 568: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: table, pk, status.message);

### src/api/otlp_exporter.cpp
Total findings: 13

- Line 42: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: buf->append(ptr, size * nmemb);
- Line 42: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buf->append(ptr, size * nmemb);
- Line 305: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(queue_mutex_);
- Line 425: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
- Line 98: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void OtlpExporter::start() {
  Confidence: band=medium; score=0.66
- Line 502: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: //     "resource": {"attributes": [{"key":"service.name","value":{"stringValue":"…"}}]},
  Confidence: band=high; score=0.74
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: resource_attrs.push_back({{"key", key}, {"value", {{"stringValue", value}}}});
- Line 540: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: json status;
- Line 549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attrs.push_back({{"key", k}, {"value", {{"stringValue", v}}}});
  Confidence: band=high; score=0.74
- Line 550: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attrs.push_back({{"key", k}, {"value", {{"stringValue", v}}}});
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: span_array.push_back(span_obj);

### src/api/graphql_ws_handler.cpp
Total findings: 7

- Line 305: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = subscriptions_.find(id);
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cdc_handle = changefeed_->subscribe(std::move(f),
- Line 251: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!alive->load(std::memory_order_acquire)) {
- Line 415: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto it = field.arguments.find("collection");
  Confidence: band=very_high; score=0.9
- Line 133: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: GraphQLWsHandler::handleConnectionInit(const std::string& /*payload_json*/)
  Confidence: band=medium; score=0.66
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors_arr.push_back(json{{"message", e.toString()}});
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors_arr.push_back(json{{"message", e.toString()}});

### src/api/grpc_server.cpp
Total findings: 5

- Line 203: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 270: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("GrpcApiServer: cannot open file: " + path);
- Line 316: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 67: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcApiServer::initialize(const GrpcServerConfig& config) {
  Confidence: band=medium; score=0.66
- Line 138: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GrpcApiServer::start() {
  Confidence: band=medium; score=0.66

### src/api/federation_admin_handler.cpp
Total findings: 3

- Line 29: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("FederationAdminHandler: coordinator must not be null");
- Line 58: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("DP budget exhausted");
- Line 68: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {"status",         "success"}};

### src/api/ws_handler.cpp
Total findings: 2

- Line 112: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authorize" without audit log
  Context: const auto result = auth_->authorize(token, "cdc:subscribe");
  Confidence: band=very_high; score=0.99
- Line 96: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto auth_hdr = req[http::field::authorization];
  Confidence: band=high; score=0.74

### src/api/tracing_middleware.cpp
Total findings: 1

- Line 94: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: span.attributes["http.status_code"] = std::to_string(http_status);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
