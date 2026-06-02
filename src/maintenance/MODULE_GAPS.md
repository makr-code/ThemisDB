# maintenance Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: maintenance
- Generated: 2026-06-02 11:09:13
- Status: Findings Present
- Total Findings: 9
- Actionable Findings (Critical + High): 0
- Affected Files: 2

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 0 |
| Medium | 9 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 9 |
| container | 4 |
| raii | 2 |
| security | 2 |
| reliability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/maintenance/database_maintenance_orchestrator.cpp | 9 | 0 | 0 | 9 | 0 |
| src/maintenance/maintenance_registry.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/maintenance/database_maintenance_orchestrator.cpp
Total findings: 9

- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(job);
  Confidence: band=high; score=0.74
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.module_signals.push_back(std::move(sig));
  Confidence: band=high; score=0.74
- Line 716: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string>
  Confidence: band=high; score=0.74
- Line 720: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> result;
  Confidence: band=high; score=0.74
- Line 1388: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<MaintenanceTaskType, std::size_t> taskIndex;
  Confidence: band=high; score=0.74
- Line 1425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74

### src/maintenance/maintenance_registry.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
