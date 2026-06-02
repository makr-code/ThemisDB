# rpc_grpc Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: rpc_grpc
- Generated: 2026-06-02 11:09:13
- Status: Findings Present
- Total Findings: 2
- Actionable Findings (Critical + High): 0
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 0 |
| Medium | 2 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| audit_logging | 9 |
| performance | 3 |
| reliability | 3 |
| raii | 2 |
| determinism | 1 |
| exception_safety | 1 |
| memory | 1 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/rpc_grpc/grpc_plugin.cpp | 2 | 0 | 0 | 2 | 0 |

## Full Scanner Findings

### src/rpc_grpc/grpc_plugin.cpp
Total findings: 2

- Line 289: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint64_t> reqs, errs, lats;
  Confidence: band=medium; score=0.66
- Line 475: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GRPCPlugin::initialize(const char* config_json) {
  Confidence: band=medium; score=0.66

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
