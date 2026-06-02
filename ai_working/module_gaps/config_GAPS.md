# config Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: config
- Generated: 2026-06-02 11:09:12
- Status: High-Priority Findings Present
- Total Findings: 19
- Actionable Findings (Critical + High): 6
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 6 |
| Medium | 13 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| exception_safety | 45 |
| reliability | 31 |
| raii | 26 |
| legacy_duplication | 12 |
| performance_patterns | 11 |
| container | 8 |
| platform | 8 |
| performance | 6 |
| audit_logging | 5 |
| security | 5 |
| concurrency | 3 |
| determinism | 3 |
| input_validation | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/config/config_schema_validator.cpp | 6 | 0 | 1 | 5 | 0 |
| src/config/config_encrypted_store.cpp | 5 | 0 | 0 | 5 | 0 |
| src/config/config_path_resolver.cpp | 5 | 0 | 2 | 3 | 0 |
| src/config/config_metrics_exporter.cpp | 3 | 0 | 3 | 0 | 0 |
| src/config/config_audit_log.cpp | 0 | 0 | 0 | 0 | 0 |
| src/config/config_file_watcher.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/config/config_schema_validator.cpp
Total findings: 6

- Line 386: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (v == ref) {
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(yamlNodeToJsonImpl(child));
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '/';
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_refs.push_back(ref);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_keys.push_back(k);
  Confidence: band=high; score=0.74

### src/config/config_encrypted_store.cpp
Total findings: 5

- Line 46: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += have2 ? kB64Chars[(triple >> 6) & 0x3F] : '=';
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ConfigEncryptedBlob> new_store;
  Confidence: band=medium; score=0.66
- Line 287: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ConfigEncryptedBlob> new_store;
  Confidence: band=medium; score=0.66

### src/config/config_path_resolver.cpp
Total findings: 5

- Line 314: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Deprecated/Backup Files
  Confidence: band=high; score=0.8
- Line 694: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ── Deprecated/Backup Files ───────────────────────────────────────────────
  Confidence: band=high; score=0.8
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1414: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& ConfigPathResolver::legacyPathMappings() {
  Confidence: band=high; score=0.74
- Line 1608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: categories.push_back(entry.first);
  Confidence: band=high; score=0.74

### src/config/config_metrics_exporter.cpp
Total findings: 3

- Line 196: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // New-path hits (backward compatibility)
  Confidence: band=high; score=0.8
- Line 208: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Cache hits/misses (backward compatibility)
  Confidence: band=high; score=0.8
- Line 332: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Gauges use *_current naming; *_aggregate variants provide counter-like totals, while *_total aliases are preserved for backward compatibility (non-counter gauges; planned deprecation in v1.9.0).
  Confidence: band=high; score=0.8

### src/config/config_audit_log.cpp
Total findings: 0


### src/config/config_file_watcher.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
