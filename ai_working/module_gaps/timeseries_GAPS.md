# timeseries Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: timeseries
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 39
- Actionable Findings (Critical + High): 3
- Affected Files: 20

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 3 |
| Medium | 36 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 31 |
| performance_patterns | 30 |
| container | 20 |
| exception_safety | 20 |
| memory | 16 |
| determinism | 8 |
| performance | 7 |
| platform | 7 |
| concurrency | 3 |
| legacy_duplication | 3 |
| raii | 3 |
| input_validation | 2 |
| uninitialized | 2 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/timeseries/tsstore.cpp | 10 | 0 | 1 | 9 | 0 |
| src/timeseries/anomaly_detection.cpp | 8 | 0 | 2 | 6 | 0 |
| src/timeseries/hypertable.cpp | 5 | 0 | 0 | 5 | 0 |
| src/timeseries/aggregates.cpp | 4 | 0 | 0 | 4 | 0 |
| src/timeseries/gap_fill.cpp | 4 | 0 | 0 | 4 | 0 |
| src/timeseries/adaptive_flush_controller.cpp | 2 | 0 | 0 | 2 | 0 |
| src/timeseries/continuous_agg.cpp | 2 | 0 | 0 | 2 | 0 |
| src/timeseries/aggregate_scheduler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/timeseries/compression_selector.cpp | 1 | 0 | 0 | 1 | 0 |
| src/timeseries/gorilla.cpp | 1 | 0 | 0 | 1 | 0 |
| src/timeseries/gorilla_simd.cpp | 1 | 0 | 0 | 1 | 0 |
| src/timeseries/encrypted_chunk_store.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/prometheus_remote_write.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/retention.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/timeseries.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/timeseries_metrics.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/ts_auto_buffer.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/ts_auto_buffer_adaptive.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/ts_encrypted_key_rotation.cpp | 0 | 0 | 0 | 0 | 0 |
| src/timeseries/ts_stream_cursor.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/timeseries/tsstore.cpp
Total findings: 10

- Line 407: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& point : points) {
  Confidence: band=very_high; score=0.9
- Line 258: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // 2. Sorted by timestamp
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grouped[group_key].push_back(point);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp for Gorilla efficiency
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timestamps.push_back(p.timestamp_ms);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp for Gorilla efficiency
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.row_errors.emplace_back(idx,
  Confidence: band=high; score=0.74
- Line 661: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: TSStore::query(const QueryOptions& options) const {
  Confidence: band=high; score=0.74
- Line 844: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(dp);
  Confidence: band=high; score=0.74
- Line 859: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort results by timestamp (mixed raw + compressed may be out of order)
  Confidence: band=high; score=0.74

### src/timeseries/anomaly_detection.cpp
Total findings: 8

- Line 66: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = score_map.find(p.timestamp_ms);
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = merged.find(ap.timestamp_ms);
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int64_t, double> score_map;
  Confidence: band=medium; score=0.66
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ap);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ap);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ap);
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int64_t, AnomalyPoint> merged;
  Confidence: band=medium; score=0.66
- Line 174: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Collect and sort by timestamp
  Confidence: band=high; score=0.74

### src/timeseries/hypertable.cpp
Total findings: 5

- Line 95: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::pair<int64_t, std::string>>> chunk_batches;
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunk_batches[chunk_name].emplace_back(timestamp, data);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_to_scan.push_back(getChunkName(t));
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(timestamp, data);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp
  Confidence: band=high; score=0.74

### src/timeseries/aggregates.cpp
Total findings: 4

- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: window_data[window_start].push_back(values[i]);
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.timestamps.push_back(window_ts);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: window_values.push_back(values[j]);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: window_values.push_back(values[j]);
  Confidence: band=high; score=0.74

### src/timeseries/gap_fill.cpp
Total findings: 4

- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*exact);
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*exact);
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*exact);
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ts.push_back(t);
  Confidence: band=high; score=0.74

### src/timeseries/adaptive_flush_controller.cpp
Total findings: 2

- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffer_.push_back(p);
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(std::move(buffer_.front()));
  Confidence: band=high; score=0.74

### src/timeseries/continuous_agg.cpp
Total findings: 2

- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partial_results.push_back(shard_query_(s, cfg, from_ms, to_ms));
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.has_value()) result.push_back(std::move(*s));
  Confidence: band=high; score=0.74

### src/timeseries/aggregate_scheduler.cpp
Total findings: 1

- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(agg);
  Confidence: band=high; score=0.74

### src/timeseries/compression_selector.cpp
Total findings: 1

- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deltas.push_back(points[i].timestamp_ms - points[i - 1].timestamp_ms);
  Confidence: band=high; score=0.74

### src/timeseries/gorilla.cpp
Total findings: 1

- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf_.push_back(static_cast<uint8_t>(v & 0x7FUL) | 0x80U);
  Confidence: band=high; score=0.74

### src/timeseries/gorilla_simd.cpp
Total findings: 1

- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(dods_buf[b], bits_to_dbl_simd(xors_buf[b]));
  Confidence: band=high; score=0.74

### src/timeseries/encrypted_chunk_store.cpp
Total findings: 0


### src/timeseries/prometheus_remote_write.cpp
Total findings: 0


### src/timeseries/retention.cpp
Total findings: 0


### src/timeseries/timeseries.cpp
Total findings: 0


### src/timeseries/timeseries_metrics.cpp
Total findings: 0


### src/timeseries/ts_auto_buffer.cpp
Total findings: 0


### src/timeseries/ts_auto_buffer_adaptive.cpp
Total findings: 0


### src/timeseries/ts_encrypted_key_rotation.cpp
Total findings: 0


### src/timeseries/ts_stream_cursor.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
