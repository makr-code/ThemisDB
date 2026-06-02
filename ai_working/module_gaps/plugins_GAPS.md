# plugins Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: plugins
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 29
- Actionable Findings (Critical + High): 4
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 4 |
| Medium | 25 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| exception_safety | 29 |
| performance_patterns | 25 |
| raii | 24 |
| performance | 17 |
| container | 15 |
| reliability | 10 |
| memory | 6 |
| legacy_duplication | 4 |
| concurrency | 2 |
| determinism | 2 |
| platform | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/plugins/plugin_manager.cpp | 14 | 0 | 1 | 13 | 0 |
| src/plugins/plugin_health_monitor.cpp | 4 | 0 | 1 | 3 | 0 |
| src/plugins/oci_registry_client.cpp | 3 | 0 | 1 | 2 | 0 |
| src/plugins/plugin_hot_plug_monitor.cpp | 3 | 0 | 0 | 3 | 0 |
| src/plugins/signed_plugin_repository.cpp | 3 | 0 | 0 | 3 | 0 |
| src/plugins/plugin_metrics.cpp | 1 | 0 | 0 | 1 | 0 |
| src/plugins/plugin_system_edition.cpp | 1 | 0 | 1 | 0 | 0 |
| src/plugins/rpc_service_registry.cpp | 0 | 0 | 0 | 0 | 0 |
| src/plugins/wasm_plugin_loader.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/plugins/plugin_manager.cpp
Total findings: 14

- Line 1261: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& name : topo_order) {
  Confidence: band=very_high; score=0.9
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.dependencies.push_back(dep.get<std::string>());
  Confidence: band=high; score=0.74
- Line 523: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!cycle_desc.empty()) cycle_desc += "; ";
  Confidence: band=high; score=0.74
- Line 525: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cycle_desc += " -> ";
  Confidence: band=high; score=0.74
- Line 822: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!dep_list.empty()) dep_list += ", ";
  Confidence: band=high; score=0.74
- Line 923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(plugin_it->second.instance.get());
  Confidence: band=high; score=0.74
- Line 936: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second.manifest);
  Confidence: band=high; score=0.74
- Line 948: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 986: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!dep_list.empty()) dep_list += ", ";
  Confidence: band=high; score=0.74
- Line 1223: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!cycle_desc.empty()) cycle_desc += "; ";
  Confidence: band=high; score=0.74
- Line 1225: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) cycle_desc += " -> ";
  Confidence: band=high; score=0.74
- Line 1239: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!missing_desc.empty()) missing_desc += "; ";
  Confidence: band=high; score=0.74
- Line 1487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents.push_back(plugin_name);
  Confidence: band=high; score=0.74
- Line 1487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents.push_back(plugin_name);
  Confidence: band=high; score=0.74

### src/plugins/plugin_health_monitor.cpp
Total findings: 4

- Line 292: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& name : names) {
  Confidence: band=very_high; score=0.9
- Line 185: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, MonitoredPlugin> PluginHealthMonitor::getAllPluginStats() const {
  Confidence: band=medium; score=0.66
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: event_callbacks_.push_back(std::move(callback));
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(n);
  Confidence: band=high; score=0.74

### src/plugins/oci_registry_client.cpp
Total findings: 3

- Line 521: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Accepted media types (OCI + Docker v2 for compatibility).
  Confidence: band=high; score=0.8
- Line 257: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> response_headers;
  Confidence: band=medium; score=0.66
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.layers.push_back(std::move(l));
  Confidence: band=high; score=0.74

### src/plugins/plugin_hot_plug_monitor.cpp
Total findings: 3

- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filename.push_back(static_cast<char>(wc));
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, fs::file_time_type> known_files;
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, fs::file_time_type> current_files;
  Confidence: band=high; score=0.74

### src/plugins/signed_plugin_repository.cpp
Total findings: 3

- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pinned_keys_.push_back(std::move(key));
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries_.push_back(entry);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e);
  Confidence: band=high; score=0.74

### src/plugins/plugin_metrics.cpp
Total findings: 1

- Line 118: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, PluginMetrics::PluginStats> PluginMetrics::getAllStats() const {
  Confidence: band=high; score=0.74

### src/plugins/plugin_system_edition.cpp
Total findings: 1

- Line 11: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * DEPRECATED - Merged into plugin_manager.cpp / plugin_manager.h
  Confidence: band=high; score=0.8

### src/plugins/rpc_service_registry.cpp
Total findings: 0


### src/plugins/wasm_plugin_loader.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
