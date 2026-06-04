# projects Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: projects
- Generated: 2026-06-04 08:50:22
- Status: High-Priority Findings Present
- Total Findings: 9
- Actionable Findings (Critical + High): 4
- Affected Files: 5

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 4 |
| Medium | 3 |
| Low | 2 |

## Category Summary

| Category | Count |
|---|---:|
| resource_leaked_in_exception | 3 |
| module_doc_linkset_drift | 2 |
| unordered_container_iter | 2 |
| copy_overhead | 1 |
| nested_loop_find | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| projects/project_diff.cpp | 5 | 0 | 3 | 2 | 0 |
| projects/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| projects/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| projects/collaboration_manager.cpp | 1 | 0 | 1 | 0 | 0 |
| projects/project_template.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### projects/project_diff.cpp
Total findings: 5

- Line 31: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 42: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 197: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = ours_index.find(their_entry.field_path);
- Line 135: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, json> m;
- Line 192: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, const DeltaEntry*> ours_index;

### projects/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### projects/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### projects/collaboration_manager.cpp
Total findings: 1

- Line 38: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### projects/project_template.cpp
Total findings: 1

- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: created.push_back(*name);

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
