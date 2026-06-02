# utils Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: utils
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 164
- Actionable Findings (Critical + High): 33
- Affected Files: 44

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 33 |
| Medium | 119 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 115 |
| container | 94 |
| raii | 64 |
| platform | 58 |
| exception_safety | 32 |
| observability | 31 |
| performance | 30 |
| reliability | 25 |
| memory | 22 |
| legacy_duplication | 14 |
| concurrency | 11 |
| determinism | 9 |
| input_validation | 7 |
| audit_logging | 5 |
| uninitialized | 5 |
| security | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/utils/geo/ewkb.cpp | 22 | 0 | 0 | 22 | 0 |
| src/utils/memory/pool_allocator.cpp | 12 | 0 | 12 | 0 | 0 |
| src/utils/audit_logger.cpp | 10 | 0 | 2 | 7 | 1 |
| src/utils/pii_detector.cpp | 10 | 0 | 0 | 10 | 0 |
| src/utils/capability_auto_generator.cpp | 8 | 0 | 0 | 7 | 1 |
| src/utils/error_registry.cpp | 8 | 0 | 1 | 6 | 1 |
| src/utils/http_client_pool.cpp | 7 | 0 | 5 | 2 | 0 |
| src/utils/pii_detection_engine.cpp | 7 | 0 | 0 | 7 | 0 |
| src/utils/input_validator.cpp | 6 | 0 | 0 | 6 | 0 |
| src/utils/regex_detection_engine.cpp | 6 | 0 | 0 | 6 | 0 |
| src/utils/self_awareness.cpp | 5 | 0 | 0 | 5 | 0 |
| src/utils/tracing.cpp | 5 | 0 | 0 | 5 | 0 |
| src/utils/bloom_filter.cpp | 4 | 0 | 0 | 1 | 3 |
| src/utils/consistent_hash.cpp | 4 | 0 | 0 | 4 | 0 |
| src/utils/hkdf_cache.cpp | 4 | 0 | 4 | 0 | 0 |
| src/utils/logger.cpp | 4 | 0 | 0 | 1 | 3 |
| src/utils/pki_client.cpp | 4 | 0 | 3 | 1 | 0 |
| src/utils/stopwords.cpp | 4 | 0 | 0 | 4 | 0 |
| src/utils/build_info.cpp | 3 | 0 | 0 | 3 | 0 |
| src/utils/cursor.cpp | 3 | 0 | 0 | 3 | 0 |
| src/utils/grpc_channel_pool.cpp | 3 | 0 | 3 | 0 | 0 |
| src/utils/lek_manager.cpp | 3 | 0 | 1 | 2 | 0 |
| src/utils/pii_pseudonymizer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/utils/sampled_logger.cpp | 3 | 0 | 0 | 0 | 3 |
| src/utils/utils_adapters.cpp | 3 | 0 | 1 | 2 | 0 |
| src/utils/ner_detection_engine.cpp | 2 | 0 | 0 | 2 | 0 |
| src/utils/saga_logger.cpp | 2 | 0 | 0 | 2 | 0 |
| src/utils/checksum_utils.cpp | 1 | 0 | 1 | 0 | 0 |
| src/utils/compression_metrics.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/normalizer.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/pii_stream_scanner.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/rate_limiter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/retention_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/serialization.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/thread_pool_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/update_checker.cpp | 1 | 0 | 0 | 1 | 0 |
| src/utils/boost_throw_exception.cpp | 0 | 0 | 0 | 0 | 0 |
| src/utils/cron_parser.cpp | 0 | 0 | 0 | 0 | 0 |
| src/utils/hkdf_helper.cpp | 0 | 0 | 0 | 0 | 0 |
| src/utils/lz4_codec.cpp | 0 | 0 | 0 | 0 | 0 |
| src/utils/runtime_license_gate.cpp | 0 | 0 | 0 | 0 | 0 |
| src/utils/simd_distance.cpp | 0 | 0 | 0 | 0 | 0 |
| src/utils/timestamp_utils.cpp | 0 | 0 | 0 | 0 | 0 |
| src/utils/zstd_codec.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/utils/geo/ewkb.cpp
Total findings: 22

- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.push_back(c);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(bytes[i]);
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.coords.emplace_back(x, y, z);
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.rings[r].emplace_back(x, y, z);
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.geometries.push_back(parseGeometryFromPtr(ptr));
  Confidence: band=high; score=0.74
- Line 651: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.geometries.push_back(parseGeoJSONGeomImpl(member, depth - 1));
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: geom.coords.push_back(parseCoordinateToken(token));
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: merged += ",";
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: merged += ",";
  Confidence: band=high; score=0.74
- Line 743: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords.push_back(parseCoordinateToken(token));
  Confidence: band=high; score=0.74
- Line 840: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 851: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: coords_arr.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 864: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: line_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 879: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 879: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 896: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 896: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 896: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring_coords.push_back({c.x, c.y, c.getZ()});
  Confidence: band=high; score=0.74
- Line 910: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: members.push_back(json::parse(toGeoJSON(sub)));
  Confidence: band=high; score=0.74

### src/utils/memory/pool_allocator.cpp
Total findings: 12

- Line 230: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> BuddyAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 378: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* allocate() {
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: bool deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 449: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* allocate() {
  Confidence: band=very_high; score=0.9
- Line 473: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: bool deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 492: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 527: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> SlabAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 623: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 661: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> StackAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
  Confidence: band=very_high; score=0.9
- Line 823: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: Result<void> PoolAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9

### src/utils/audit_logger.cpp
Total findings: 10

- Line 763: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // This method is kept for API compatibility
  Confidence: band=high; score=0.8
- Line 1133: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (baseline.avg_frequency_seconds == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 505: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = state["last_timestamp_ms"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 794: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 843: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 941: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = record["ts"].get<uint64_t>();
  Confidence: band=high; score=0.74
- Line 1317: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = record["ts"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 1407: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ms = record["ts"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 642: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: formatted_message = formatAsSyslog(event, event_type);
  Confidence: band=medium; score=0.6

### src/utils/pii_detector.cpp
Total findings: 10

- Line 29: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized.push_back(static_cast<char>(std::tolower(ch)));
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> PIIDetector::detectInJson(
  Confidence: band=medium; score=0.66
- Line 153: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>> result;
  Confidence: band=medium; score=0.66
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: enabled.push_back(engine->getName());
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.at("engines").push_back(engine->getMetadata());
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto settings = config["global_settings"];
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: json_node.push_back(item_json);
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<PIIFinding>>& findings) const {
  Confidence: band=medium; score=0.66
- Line 508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path_it->second.push_back(std::move(finding));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduplicated.push_back(curr);
  Confidence: band=high; score=0.74

### src/utils/capability_auto_generator.cpp
Total findings: 8

- Line 55: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto analysis = root["rocksdb_analysis"];
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto audit = root["audit"];
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto security = root["security"];
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto output = root["output"];
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.data_types.push_back(type);
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.keywords.push_back(sorted_keywords[i].first);
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: capability.domains.push_back(domain);
  Confidence: band=high; score=0.74
- Line 535: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::ofstream log(config_.audit_log_path, std::ios::app);
  Confidence: band=medium; score=0.6

### src/utils/error_registry.cpp
Total findings: 8

- Line 890: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
  Confidence: band=very_high; score=0.9
- Line 890: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "2. Optimize query (add indexes, reduce data scanned)\n"
  Confidence: band=high; score=0.74
- Line 1721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(errors_.at(code));
  Confidence: band=high; score=0.74
- Line 1736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1748: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: categories.push_back(category);
  Confidence: band=high; score=0.74
- Line 1761: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result["errors"].push_back(pair.second.toJSON());
  Confidence: band=high; score=0.74
- Line 140: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",
  Confidence: band=medium; score=0.6

### src/utils/http_client_pool.cpp
Total findings: 7

- Line 247: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 298: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& stripe : stripes_) {
  Confidence: band=very_high; score=0.9
- Line 330: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < stripes_.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: io_threads_.emplace_back([this]() {
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stripe->connections.push_back(pooled);
  Confidence: band=high; score=0.74

### src/utils/pii_detection_engine.cpp
Total findings: 7

- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hash_bytes.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (to_mask > 0) { out.push_back('*'); --to_mask; }
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig_node = config["signature"];
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig_node = config["signature"];
  Confidence: band=high; score=0.74

### src/utils/input_validator.cpp
Total findings: 6

- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!isAsciiControl(c)) out.push_back(c);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto min_len = prop["minLength"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto max_len = prop["maxLength"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = schema["properties"].begin();
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool InputValidator::validateAQLQuery(const std::string& query) const {
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  result += "&amp;";  break;
  Confidence: band=high; score=0.74

### src/utils/regex_detection_engine.cpp
Total findings: 6

- Line 67: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = config["signature"];
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto settings = config["settings"];
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back(finding);
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flag_strings.push_back(flag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flag_strings.push_back(flag.get<std::string>());
  Confidence: band=high; score=0.74
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern.field_hints.push_back(hint.get<std::string>());
  Confidence: band=high; score=0.74

### src/utils/self_awareness.cpp
Total findings: 5

- Line 41: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sa = root["self_awareness"];
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto thresh = sa["thresholds"];
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto snaps = sa["snapshots"];
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Collect snapshot files sorted by name (which encodes timestamp)
  Confidence: band=high; score=0.74
- Line 597: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path());
  Confidence: band=high; score=0.74

### src/utils/tracing.cpp
Total findings: 5

- Line 170: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void Baggage::inject(std::map<std::string, std::string>& headers) {
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void Baggage::extract(const std::map<std::string, std::string>& headers) {
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: {"service.name", serviceName},
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: {"service.version", "0.1.0"}
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::string headerValue(const std::map<std::string, std::string>& headers,
  Confidence: band=high; score=0.74

### src/utils/bloom_filter.cpp
Total findings: 4

- Line 101: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: BloomFilter::clear()
  Context: void BloomFilter::clear() {
  Confidence: band=medium; score=0.56
- Line 28: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double ln2 = std::log(2.0);
  Confidence: band=medium; score=0.6
- Line 29: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return static_cast<size_t>(std::ceil(-static_cast<double>(n) * std::log(p) / (ln2 * ln2)));
  Confidence: band=medium; score=0.6
- Line 35: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: size_t k = static_cast<size_t>(std::ceil(static_cast<double>(bits) / static_cast<double>(n) * std::log(2.0)));
  Confidence: band=medium; score=0.6

### src/utils/consistent_hash.cpp
Total findings: 4

- Line 75: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(node);
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::nodeCount()
  Context: size_t ConsistentHashRing::nodeCount() const {
  Confidence: band=medium; score=0.56

### src/utils/hkdf_cache.cpp
Total findings: 4

- Line 158: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 165: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& shard : impl_->shards) {
  Confidence: band=very_high; score=0.9
- Line 217: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: size_t ikm_end = raw_key.find('\x00');
  Confidence: band=very_high; score=0.9

### src/utils/logger.cpp
Total findings: 4

- Line 45: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 95: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6
- Line 123: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6
- Line 153: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: spdlog::set_default_logger(logger_);
  Confidence: band=medium; score=0.6

### src/utils/pki_client.cpp
Total findings: 4

- Line 599: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY signing (preferred) instead of deprecated RSA_sign API.
  Confidence: band=high; score=0.8
- Line 607: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use PKCS#1 v1.5 padding for compatibility with RSA_sign
  Confidence: band=high; score=0.8
- Line 819: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use EVP_PKEY verification instead of deprecated RSA_verify
  Confidence: band=high; score=0.8
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back((uint8_t)((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74

### src/utils/stopwords.cpp
Total findings: 4

- Line 16: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static std::unordered_set<std::string> make_set(std::initializer_list<const char*> list) {
  Confidence: band=medium; score=0.66
- Line 17: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> s;
  Confidence: band=medium; score=0.66
- Line 45: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> Stopwords::merge(const std::unordered_set<std::string>& base,
  Confidence: band=medium; score=0.66
- Line 47: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> out = base;
  Confidence: band=medium; score=0.66

### src/utils/build_info.cpp
Total findings: 3

- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 857: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74

### src/utils/cursor.cpp
Total findings: 3

- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(base64_chars[(val >> valb) & 0x3F]);
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: output.push_back(char((val >> valb) & 0xFF));
  Confidence: band=high; score=0.74

### src/utils/grpc_channel_pool.cpp
Total findings: 3

- Line 143: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [target, pool] : target_pools_) {
  Confidence: band=very_high; score=0.9

### src/utils/lek_manager.cpp
Total findings: 3

- Line 366: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& expired_date : to_revoke) {
  Confidence: band=very_high; score=0.9
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_revoke.push_back(cached_date);
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_revoke.push_back(cached_date);
  Confidence: band=high; score=0.74

### src/utils/pii_pseudonymizer.cpp
Total findings: 3

- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created_uuids.push_back(pii_uuid);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created_uuids.push_back(pii_uuid);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto encrypted_json = mapping["original_value_encrypted"];
  Confidence: band=high; score=0.74

### src/utils/sampled_logger.cpp
Total findings: 3

- Line 58: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: bool SampledLogger::should_log(Logger::Level level, const char* file, int line) {
  Confidence: band=medium; score=0.6
- Line 99: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void SampledLogger::log(Logger::Level level, const std::string& msg,
  Confidence: band=medium; score=0.6
- Line 102: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!should_log(level, file, line)) {
  Confidence: band=medium; score=0.6

### src/utils/utils_adapters.cpp
Total findings: 3

- Line 171: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
  Confidence: band=very_high; score=0.9
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.categories.push_back(cat);
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AuditCursor HashChainAuditLogAdapter::query(const AuditQuery& query) const {
  Confidence: band=high; score=0.74

### src/utils/ner_detection_engine.cpp
Total findings: 2

- Line 66: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sig = config["signature"];
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > first) value += ' ';
  Confidence: band=high; score=0.74

### src/utils/saga_logger.cpp
Total findings: 2

- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_array.push_back(entry);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(step);
  Confidence: band=high; score=0.74

### src/utils/checksum_utils.cpp
Total findings: 1

- Line 66: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // The deprecated signature is kept for backward-compatible callers that still
  Confidence: band=high; score=0.8

### src/utils/compression_metrics.cpp
Total findings: 1

- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: methods.push_back(pair.first);
  Confidence: band=high; score=0.74

### src/utils/normalizer.cpp
Total findings: 1

- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (is2(c, d, 0xC3, 0xA4)) { out.push_back('a'); i += 2; continue; } // ä
  Confidence: band=high; score=0.74

### src/utils/pii_stream_scanner.cpp
Total findings: 1

- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(f));
  Confidence: band=high; score=0.74

### src/utils/rate_limiter.cpp
Total findings: 1

- Line 73: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Context: void RateLimiter::reset() {
  Confidence: band=medium; score=0.56

### src/utils/retention_manager.cpp
Total findings: 1

- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(policy);
  Confidence: band=high; score=0.74

### src/utils/serialization.cpp
Total findings: 1

- Line 35: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffer_.push_back((value >> (i * 8)) & 0xFF);
  Confidence: band=high; score=0.74

### src/utils/thread_pool_manager.cpp
Total findings: 1

- Line 27: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back([this]() { workerLoop(); });
  Confidence: band=high; score=0.74

### src/utils/update_checker.cpp
Total findings: 1

- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: releases.push_back(*release);
  Confidence: band=high; score=0.74

### src/utils/boost_throw_exception.cpp
Total findings: 0


### src/utils/cron_parser.cpp
Total findings: 0


### src/utils/hkdf_helper.cpp
Total findings: 0


### src/utils/lz4_codec.cpp
Total findings: 0


### src/utils/runtime_license_gate.cpp
Total findings: 0


### src/utils/simd_distance.cpp
Total findings: 0


### src/utils/timestamp_utils.cpp
Total findings: 0


### src/utils/zstd_codec.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
