# projects Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: projects
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 18
- Actionable Findings (Critical + High): 1
- Affected Files: 5

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 1 |
| Medium | 17 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 16 |
| container | 8 |
| exception_safety | 3 |
| determinism | 2 |
| memory | 1 |
| reliability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/projects/project_diff.cpp | 12 | 0 | 1 | 11 | 0 |
| src/projects/project_template.cpp | 2 | 0 | 0 | 2 | 0 |
| src/projects/project_versioning.cpp | 2 | 0 | 0 | 2 | 0 |
| src/projects/collaboration_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/projects/in_memory_project_audit_log.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/projects/project_diff.cpp
Total findings: 12

- Line 197: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = ours_index.find(their_entry.field_path);
  Confidence: band=very_high; score=0.9
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back(DeltaEntry::fromJson(e));
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back(DeltaEntry::fromJson(e));
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({path + "/" + key, DeltaType::REMOVED, val, nullptr});
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({child_path, DeltaType::ADDED, nullptr, val});
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, json> m;
  Confidence: band=medium; score=0.66
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back({"/" + id, DeltaType::REMOVED, doc, nullptr});
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back({"/" + id, DeltaType::REMOVED, doc, nullptr});
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ds.entries.push_back({"/" + id, DeltaType::ADDED, nullptr, doc});
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const DeltaEntry*> ours_index;
  Confidence: band=medium; score=0.66
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.applied.entries.push_back(their_entry);
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.applied.entries.push_back(their_entry);
  Confidence: band=high; score=0.74

### src/projects/project_template.cpp
Total findings: 2

- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created.push_back(*name);
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created.push_back(*name);
  Confidence: band=high; score=0.74

### src/projects/project_versioning.cpp
Total findings: 2

- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.emplace_back(key);
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: content_array.push_back(json::parse(val));
  Confidence: band=high; score=0.74

### src/projects/collaboration_manager.cpp
Total findings: 1

- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(c);
  Confidence: band=high; score=0.74

### src/projects/in_memory_project_audit_log.cpp
Total findings: 1

- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
