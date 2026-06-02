# cdc Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: cdc
- Generated: 2026-06-02 11:09:12
- Status: High-Priority Findings Present
- Total Findings: 27
- Actionable Findings (Critical + High): 1
- Affected Files: 13

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 1 |
| Medium | 26 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 86 |
| performance_patterns | 23 |
| container | 20 |
| exception_safety | 19 |
| raii | 15 |
| platform | 9 |
| legacy_duplication | 6 |
| memory | 6 |
| concurrency | 5 |
| audit_logging | 4 |
| determinism | 3 |
| performance | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/cdc/consumer_group.cpp | 6 | 0 | 0 | 6 | 0 |
| src/cdc/cross_collection_stream.cpp | 5 | 0 | 0 | 5 | 0 |
| src/cdc/ws_transport.cpp | 4 | 0 | 0 | 4 | 0 |
| src/cdc/delivery_tracker.cpp | 3 | 0 | 0 | 3 | 0 |
| src/cdc/tenant_buffer_manager.cpp | 3 | 0 | 0 | 3 | 0 |
| src/cdc/cdc_ws_handler.cpp | 2 | 0 | 0 | 2 | 0 |
| src/cdc/changefeed.cpp | 2 | 0 | 1 | 1 | 0 |
| src/cdc/cdc_materialized_view.cpp | 1 | 0 | 0 | 1 | 0 |
| src/cdc/outbox.cpp | 1 | 0 | 0 | 1 | 0 |
| src/cdc/cdc_admin.cpp | 0 | 0 | 0 | 0 | 0 |
| src/cdc/changefeed_buffer.cpp | 0 | 0 | 0 | 0 | 0 |
| src/cdc/dead_letter_queue.cpp | 0 | 0 | 0 | 0 | 0 |
| src/cdc/kafka_cdc_producer.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/cdc/consumer_group.cpp
Total findings: 6

- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(std::move(gid));
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ev));
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(changefeed.getEvent(seq));
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_records.push_back({ev.sequence, now, 1});
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consumer_inflight.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consumer_inflight.push_back(std::move(rec));
  Confidence: band=high; score=0.74

### src/cdc/cross_collection_stream.cpp
Total findings: 5

- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Comparator: sort by (timestamp_ms ASC, collection ASC, sequence ASC).
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Changefeed*> feeds_snapshot;
  Confidence: band=medium; score=0.66
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_events.push_back({name, std::move(ev)});
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Changefeed*> feeds_snapshot;
  Confidence: band=medium; score=0.66

### src/cdc/ws_transport.cpp
Total findings: 4

- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_items.push_back({sid, sub_id, std::move(opts)});
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_items.push_back({sid, sub_id, std::move(opts)});
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({item.session_id, item.sub_id, std::move(events)});
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overflow_sessions.push_back(result.session_id);
  Confidence: band=high; score=0.74

### src/cdc/delivery_tracker.cpp
Total findings: 3

- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_expire.push_back(seq);
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(stats));
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: consumer_ids.push_back(id);
  Confidence: band=high; score=0.74

### src/cdc/tenant_buffer_manager.cpp
Total findings: 3

- Line 254: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, TenantStats> TenantBufferManager::getAllTenantStats() const {
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, TenantStats> all_stats;
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tenants.push_back(tenant_id);
  Confidence: band=high; score=0.74

### src/cdc/cdc_ws_handler.cpp
Total findings: 2

- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.push_back(pending.frame);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frames.push_back(pending.frame);
  Confidence: band=high; score=0.74

### src/cdc/changefeed.cpp
Total findings: 2

- Line 39: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // compatibility with existing deployments.
  Confidence: band=high; score=0.8
- Line 1167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.push_back(entry);
  Confidence: band=high; score=0.74

### src/cdc/cdc_materialized_view.cpp
Total findings: 1

- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: records.push_back(std::move(rec));
  Confidence: band=high; score=0.74

### src/cdc/outbox.cpp
Total findings: 1

- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rec));
  Confidence: band=high; score=0.74

### src/cdc/cdc_admin.cpp
Total findings: 0


### src/cdc/changefeed_buffer.cpp
Total findings: 0


### src/cdc/dead_letter_queue.cpp
Total findings: 0


### src/cdc/kafka_cdc_producer.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
