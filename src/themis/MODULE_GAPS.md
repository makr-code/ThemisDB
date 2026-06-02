# themis Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: themis
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 35
- Actionable Findings (Critical + High): 3
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 2 |
| Medium | 32 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 39 |
| raii | 39 |
| performance_patterns | 29 |
| reliability | 24 |
| platform | 20 |
| exception_safety | 9 |
| input_validation | 5 |
| audit_logging | 4 |
| memory | 4 |
| observability | 3 |
| legacy_duplication | 2 |
| performance | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/themis/module_dependency_resolver.cpp | 13 | 0 | 0 | 13 | 0 |
| src/themis/wire_protocol_server.cpp | 8 | 0 | 1 | 7 | 0 |
| src/themis/module_loader.cpp | 6 | 0 | 1 | 5 | 0 |
| src/themis/build_info.cpp | 3 | 0 | 0 | 3 | 0 |
| src/themis/edition_manager.cpp | 2 | 0 | 0 | 2 | 0 |
| src/themis/module_loader_linux.cpp | 2 | 0 | 0 | 2 | 0 |
| src/themis/license_info.cpp | 1 | 1 | 0 | 0 | 0 |
| include/themis/network/wire_protocol_server.hpp | 0 | 0 | 0 | 0 | 0 |
| include/themis/network/wire_protocol_v2.hpp | 0 | 0 | 0 | 0 | 0 |
| src/themis/module_hash_verifier.cpp | 0 | 0 | 0 | 0 | 0 |
| src/themis/module_loader_win32.cpp | 0 | 0 | 0 | 0 | 0 |
| src/themis/module_signature_verifier.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/themis/module_dependency_resolver.cpp
Total findings: 13

- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> visited;
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: precheck.missingRequired.push_back(n);
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: closure.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> inDegree;
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> dependents;
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.missingRequired.push_back(dep.name);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycleNodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cycleNodes.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/themis/wire_protocol_server.cpp
Total findings: 8

- Line 19: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // (themis::network namespace) for backward compatibility during the v1.7.0
  Confidence: band=high; score=0.8
- Line 188: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static uint32_t crc32Compute(const uint8_t* data, std::size_t len) {
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(protoValueToJson(entry));
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '?';
  Confidence: band=high; score=0.74
- Line 1541: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: void WireProtocolServer::start() {
  Confidence: band=medium; score=0.66
- Line 1575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sessions_to_close.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/themis/module_loader.cpp
Total findings: 6

- Line 986: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ABI compatibility
  Confidence: band=high; score=0.8
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(module);
  Confidence: band=high; score=0.74
- Line 771: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ModuleMetadata ModuleLoader::extractMetadataFromHandle(void* handle) {
  Confidence: band=high; score=0.74
- Line 940: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quarantined.push_back(path);
  Confidence: band=high; score=0.74
- Line 1137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: module.healthChecks.push_back(healthResult);
  Confidence: band=high; score=0.74
- Line 1273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: snapshot.emplace_back(mod.name, mod.path);
  Confidence: band=high; score=0.74

### src/themis/build_info.cpp
Total findings: 3

- Line 873: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 873: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74
- Line 884: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mod.name);
  Confidence: band=high; score=0.74

### src/themis/edition_manager.cpp
Total findings: 2

- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(feat);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(feat);
  Confidence: band=high; score=0.74

### src/themis/module_loader_linux.cpp
Total findings: 2

- Line 306: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!metadata.empty()) metadata += "; ";
  Confidence: band=high; score=0.74

### src/themis/license_info.cpp
Total findings: 1

- Line 198: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: email
  Context: oss << "  Contact Email:      " << license.contact_email << "\n";
  Confidence: band=very_high; score=0.92

### include/themis/network/wire_protocol_server.hpp
Total findings: 0


### include/themis/network/wire_protocol_v2.hpp
Total findings: 0


### src/themis/module_hash_verifier.cpp
Total findings: 0


### src/themis/module_loader_win32.cpp
Total findings: 0


### src/themis/module_signature_verifier.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
