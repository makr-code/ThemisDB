# metadata Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: metadata
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 220
- Actionable Findings (Critical + High): 57
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 3 |
| High | 54 |
| Medium | 162 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 111 |
| container | 49 |
| performance | 17 |
| memory | 16 |
| exception_safety | 10 |
| reliability | 9 |
| determinism | 4 |
| observability | 2 |
| concurrency | 1 |
| distributed_consistency | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/metadata/schema_manager.cpp | 47 | 3 | 9 | 35 | 0 |
| src/metadata/information_schema.cpp | 32 | 0 | 8 | 24 | 0 |
| src/metadata/schema_version_manager.cpp | 21 | 0 | 11 | 10 | 0 |
| src/metadata/statistics_collector.cpp | 21 | 0 | 6 | 15 | 0 |
| src/metadata/column_lineage.cpp | 19 | 0 | 1 | 18 | 0 |
| src/metadata/schema_constraints.cpp | 18 | 0 | 2 | 16 | 0 |
| src/metadata/index_recommender.cpp | 17 | 0 | 2 | 15 | 0 |
| src/metadata/catalog_exporter.cpp | 15 | 0 | 11 | 4 | 0 |
| src/metadata/er_diagram_exporter.cpp | 14 | 0 | 0 | 14 | 0 |
| src/metadata/schema_audit_log.cpp | 8 | 0 | 2 | 6 | 0 |
| src/metadata/schema_consistency_checker.cpp | 6 | 0 | 1 | 5 | 0 |
| src/metadata/distributed_catalog.cpp | 2 | 0 | 1 | 0 | 1 |

## Full Scanner Findings

### src/metadata/schema_manager.cpp
Total findings: 47

- Line 176: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 199: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 222: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3630 docs(metadata): module audi... (2026-03-12) | #3297 [metadata] Wire Col
- Line 398: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["metadata"] = metadata.toJSON();
- Line 436: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["version"] = metadata.version;
- Line 437: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["capabilities"] = metadata.capabilities;
- Line 872: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find_if(schema.properties.begin(), schema.properties.end(),
  Confidence: band=very_high; score=0.9
- Line 872: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(schema.properties.begin(), schema.properties.end(),
- Line 872: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(schema.properties.begin(), schema.properties.end(),
- Line 1069: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: for = nullptr;
  Context: spdlog::debug("SchemaManager: No custom schema to delete for table '{}'", table_name);
- Line 1080: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: schema = nullptr;
  Context: spdlog::warn("SchemaManager: Failed to delete schema from storage for table '{}'", table_name);
- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: props.push_back(prop.toJSON());
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: props.push_back(prop.toJSON());
- Line 70: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idxs.push_back(idx.toJSON());
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idxs.push_back(idx.toJSON());
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: props.push_back(prop.toJSON());
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: props.push_back(prop.toJSON());
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables.push_back(schema);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: relationships.push_back(schema);
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: caps.push_back("llm");
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back("llm");
- Line 248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back("geo");
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back("gpu");
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back("grpc");
- Line 401: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables_json.push_back(table.toJSON());
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tables_json.push_back(table.toJSON());
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rels_json.push_back(rel.toJSON());
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rels_json.push_back(rel.toJSON());
- Line 521: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, PropertyInfo> property_map;
  Confidence: band=high; score=0.74
- Line 611: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: properties.push_back(prop);
  Confidence: band=high; score=0.74
- Line 640: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 679: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 688: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 697: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 706: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 1007: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.properties.push_back(prop);
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.columns.push_back(col.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.columns.push_back(col.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.columns.push_back(col.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1035: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.columns.push_back(col.get<std::string>());
- Line 1041: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.indexes.push_back(idx);
  Confidence: band=high; score=0.74
- Line 1247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idx.columns.push_back(col.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idx.columns.push_back(col.get<std::string>());
- Line 1253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idx.columns.push_back(idx.name);

### src/metadata/information_schema.cpp
Total findings: 32

- Line 308: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getTables()) {
- Line 314: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getColumns()) {
- Line 320: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getStatistics()) {
- Line 326: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getKeyColumnUsage()) {
- Line 332: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getReferentialConstraints()) {
- Line 342: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getTables()) {
- Line 350: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getColumns(table_name)) {
- Line 358: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& row : getReferentialConstraints()) {
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(col));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(col));
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rc));
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables_arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols_arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats_arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats_arr.push_back(row.toJSON());
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kcu_arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kcu_arr.push_back(row.toJSON());
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rc_arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rc_arr.push_back(row.toJSON());
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(row.toJSON());
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(row.toJSON());
- Line 358: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(row.toJSON());
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(row.toJSON());

### src/metadata/schema_version_manager.cpp
Total findings: 21

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 277: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (props_a.find(name) == props_a.end()) {
  Confidence: band=very_high; score=0.9
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(std::move(*maybe));
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, SchemaManager::PropertyInfo> props_a, props_b;
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(prop.toJSON());
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(prop.toJSON());
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(prop.toJSON());
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: added.push_back(prop.toJSON());
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: removed.push_back(prop.toJSON());
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: removed.push_back(prop.toJSON());
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(change.toJSON());
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/metadata/statistics_collector.cpp
Total findings: 21

- Line 167: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::unique_lock<std::mutex> lk(refresh_mutex_);
- Line 167: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(refresh_mutex_);
- Line 677: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.lower_bound = bj.value("lower_bound", 0.0);
- Line 677: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.lower_bound = bj.value("lower_bound", 0.0);
- Line 678: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.upper_bound = bj.value("upper_bound", 0.0);
- Line 678: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: b.upper_bound = bj.value("upper_bound", 0.0);
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buckets.push_back(b.toJSON());
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buckets.push_back(b.toJSON());
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables.push_back(name);
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_samples[col_str].push_back(val_str);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(s.toJSON());
- Line 544: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: numeric_vals.push_back(std::stod(v));
  Confidence: band=high; score=0.74
- Line 546: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 612: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buckets.push_back(bucket);
  Confidence: band=high; score=0.74
- Line 679: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hist_buckets.push_back(b);
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(s.toJSON());
  Confidence: band=high; score=0.74
- Line 706: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(s.toJSON());
- Line 747: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/metadata/column_lineage.cpp
Total findings: 19

- Line 197: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (visited.find(src) == visited.end()) {
  Confidence: band=very_high; score=0.9
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sources.push_back(src.toJSON());
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sources.push_back(src.toJSON());
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJSON());
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets_by_source_[src].push_back(entry.target_column);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: targets_by_source_[src].push_back(entry.target_column);
- Line 181: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<ColumnRef, ColumnRefHash> visited;
  Confidence: band=medium; score=0.66
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(src);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(src);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<ColumnRef, ColumnRefHash> visited;
  Confidence: band=medium; score=0.66
- Line 229: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(target);
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries_arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries_arr.push_back(e.toJSON());
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: up_arr.push_back(ref.toJSON());
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: down_arr.push_back(ref.toJSON());
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(record.toJSON());
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries_arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries_arr.push_back(e.toJSON());

### src/metadata/schema_constraints.cpp
Total findings: 18

- Line 401: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto pos = expr.find(op);
- Line 438: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (op == "=")  passes = val_num == rhs_num;
  Confidence: band=very_high; score=0.9
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(c);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(c);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, ColumnValue>& row) const
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.has_value()) violations.push_back(std::move(*v));
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.has_value()) violations.push_back(std::move(*v));
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ColumnValue> SchemaConstraints::applyDefaults(
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, ColumnValue> row) const
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c_arr.push_back(c.toJSON());
- Line 415: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_arr.push_back(c.toJSON());
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: c_arr.push_back(c.toJSON());
- Line 569: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/metadata/index_recommender.cpp
Total findings: 17

- Line 220: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing_indexes.find(tn);
- Line 408: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(persist_mutex_);
- Line 122: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void IndexRecommender::recordQuery() {
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<IndexRecommendation>> IndexRecommender::recommendAll(
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<std::string>>& existing_indexes) const
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_names.push_back(tn);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<IndexRecommendation>> result;
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ca);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ca);
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_names_to_delete.push_back(tn);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_arr.push_back(ca.toJSON());
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_arr.push_back(ca.toJSON());
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_arr.push_back(ca.toJSON());
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col_arr.push_back(ca.toJSON());
- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(ca.toJSON());
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(ca.toJSON());
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(ca.toJSON());

### src/metadata/catalog_exporter.cpp
Total findings: 15

- Line 30: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 138: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: db_entity["attributes"]["name"]            = config_.database_name;
- Line 139: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: db_entity["attributes"]["description"]     = "ThemisDB hybrid database";
- Line 289: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: proposal["entityType"]     = "dataset";
- Line 292: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: proposal["aspectName"]     = "datasetProperties";
- Line 295: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aspect["customProperties"]["database"]  = config_.database_name;
- Line 303: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aspect["tags"]  = json::array();
- Line 312: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: proposal["entityType"]  = "dataset";
- Line 315: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: proposal["aspectName"]  = "schemaMetadata";
- Line 318: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aspect["schemaName"]  = config_.database_name + "." + table.name;
- Line 319: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: aspect["platform"]    = "urn:li:dataPlatform:themisdb";
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(col_entity);
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* count stays at 0 */ }
- Line 262: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: count, status);
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(field);
  Confidence: band=high; score=0.74

### src/metadata/er_diagram_exporter.cpp
Total findings: 14

- Line 33: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '_';
  Confidence: band=high; score=0.74
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '_';
- Line 47: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  out += "\\<"; break;
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  out += "\\>"; break;
- Line 49: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '|':  out += "\\|"; break;
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '{':  out += "\\{"; break;
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '}':  out += "\\}"; break;
- Line 52: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 53: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: props.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: props.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idxs.push_back(std::move(i));
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: props.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: props.push_back(std::move(p));
  Confidence: band=high; score=0.74

### src/metadata/schema_audit_log.cpp
Total findings: 8

- Line 215: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& e : getHistory(table_name)) {
- Line 223: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& e : getFullHistory()) {
- Line 166: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by id (which embeds timestamp) ascending
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(SchemaAuditEntry::fromJSON(j));
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJSON());
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJSON());

### src/metadata/schema_consistency_checker.cpp
Total findings: 6

- Line 243: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(bg_mutex_);
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back(std::move(issue));
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back(std::move(issue));
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back(std::move(issue));
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(issue.toJSON());
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(issue.toJSON());

### src/metadata/distributed_catalog.cpp
Total findings: 2

- Line 94: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto entry = router_.get(
  Confidence: band=very_high; score=0.9
- Line 19: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: DistributedMetadataCatalog::DistributedMetadataCatalog(
  Confidence: band=medium; score=0.6

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
