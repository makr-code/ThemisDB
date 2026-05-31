# projects Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: projects
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 50
- Actionable Findings (Critical + High): 16
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 14 |
| Medium | 34 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 20 |
| container | 19 |
| security | 4 |
| exception_safety | 3 |
| determinism | 2 |
| memory | 1 |
| reliability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/projects/project_diff.cpp | 28 | 1 | 7 | 20 | 0 |
| src/projects/project_versioning.cpp | 10 | 0 | 3 | 7 | 0 |
| src/projects/collaboration_manager.cpp | 4 | 1 | 1 | 2 | 0 |
| src/projects/project_template.cpp | 4 | 0 | 1 | 3 | 0 |
| src/projects/in_memory_project_audit_log.cpp | 3 | 0 | 1 | 2 | 0 |
| src/projects/project_lifecycle.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/projects/project_diff.cpp
Total findings: 28

- Line 199: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = ours_index.find(their_entry.field_path);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 52: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : entries) arr.push_back(e.toJson());
  Confidence: band=very_high; score=0.9
- Line 59: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : j["entries"])
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, doc] : from_map) {
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, doc] : to_map) {
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = ours_index.find(their_entry.field_path);
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& e : entries) arr.push_back(e.toJson());
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back(DeltaEntry::fromJson(e));
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back(DeltaEntry::fromJson(e));
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ds.entries.push_back(DeltaEntry::fromJson(e));
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({path + "/" + key, DeltaType::REMOVED, val, nullptr});
  Confidence: band=high; score=0.74
- Line 87: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({path + "/" + key, DeltaType::REMOVED, val, nullptr});
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({child_path, DeltaType::ADDED, nullptr, val});
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({child_path, DeltaType::ADDED, nullptr, val});
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({path, DeltaType::MODIFIED, from, to});
- Line 137: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, json> m;
  Confidence: band=medium; score=0.66
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back({"/" + id, DeltaType::REMOVED, doc, nullptr});
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back({"/" + id, DeltaType::REMOVED, doc, nullptr});
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ds.entries.push_back({"/" + id, DeltaType::REMOVED, doc, nullptr});
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back({"/" + id, DeltaType::ADDED, nullptr, doc});
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ds.entries.push_back({"/" + id, DeltaType::ADDED, nullptr, doc});
- Line 194: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const DeltaEntry*> ours_index;
  Confidence: band=medium; score=0.66
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.applied.entries.push_back(their_entry);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.applied.entries.push_back(their_entry);
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.applied.entries.push_back(their_entry);
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.entries.push_back(their_entry);

### src/projects/project_versioning.cpp
Total findings: 10

- Line 101: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix(prefix, [&](std::string_view key, std::string_view) {
- Line 211: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix(prefix, [&](std::string_view key, std::string_view) {
- Line 259: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: storage_->del("snap_data:" + snap_uuid);
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.emplace_back(key);
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: content_array.push_back(json::parse(val));
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: content_array.push_back(json::parse(val));
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: content_array.push_back(val);
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: content_array.push_back(val);
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: content_array.push_back(val);
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: content_array.push_back(val);

### src/projects/collaboration_manager.cpp
Total findings: 4

- Line 159: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = locks_.find(composite);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(c);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(c);

### src/projects/project_template.cpp
Total findings: 4

- Line 251: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->del("template_obj:" + project_id + ":" + type + ":" + n);
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created.push_back(*name);
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created.push_back(*name);
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: created.push_back(*name);

### src/projects/in_memory_project_audit_log.cpp
Total findings: 3

- Line 72: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::vector<ProjectAuditEntry> InMemoryProjectAuditLog::query(
- Line 64: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e);
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e);

### src/projects/project_lifecycle.cpp
Total findings: 1

- Line 243: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix(prefix, [&](std::string_view, std::string_view val) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
