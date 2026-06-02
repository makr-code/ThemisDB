# updates Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: updates
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 82
- Actionable Findings (Critical + High): 2
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 2 |
| Medium | 80 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 85 |
| performance_patterns | 77 |
| reliability | 31 |
| exception_safety | 22 |
| raii | 17 |
| concurrency | 10 |
| memory | 7 |
| performance | 4 |
| platform | 4 |
| audit_logging | 3 |
| determinism | 3 |
| security | 2 |
| legacy_duplication | 1 |
| type_conversion | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/updates/updates_config.cpp | 18 | 0 | 0 | 18 | 0 |
| src/updates/dependency_resolver.cpp | 14 | 0 | 0 | 14 | 0 |
| src/updates/in_place_schema_migrator.cpp | 13 | 0 | 0 | 13 | 0 |
| src/updates/delta_update_engine.cpp | 9 | 0 | 0 | 9 | 0 |
| src/updates/hot_reload_engine.cpp | 6 | 0 | 1 | 5 | 0 |
| src/updates/release_manifest.cpp | 3 | 0 | 0 | 3 | 0 |
| src/updates/canary_rollout.cpp | 2 | 0 | 1 | 1 | 0 |
| src/updates/notification_webhook.cpp | 2 | 0 | 0 | 2 | 0 |
| src/updates/preflight_health_check.cpp | 2 | 0 | 0 | 2 | 0 |
| src/updates/schema_migration.cpp | 2 | 0 | 0 | 2 | 0 |
| src/updates/schema_migration_tester.cpp | 2 | 0 | 0 | 2 | 0 |
| src/updates/update_history_logger.cpp | 2 | 0 | 0 | 2 | 0 |
| src/updates/build_verifier.cpp | 1 | 0 | 0 | 1 | 0 |
| src/updates/cluster_update_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/updates/coordinated_update_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/updates/manifest_database.cpp | 1 | 0 | 0 | 1 | 0 |
| src/updates/parallel_downloader.cpp | 1 | 0 | 0 | 1 | 0 |
| src/updates/tenant_update_scheduler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/updates/update_state_machine.cpp | 1 | 0 | 0 | 1 | 0 |
| src/updates/blue_green_deployment.cpp | 0 | 0 | 0 | 0 | 0 |
| src/updates/hardware_telemetry.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/updates/updates_config.cpp
Total findings: 18

- Line 39: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto checker = config["updates"]["checker"];
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auto_update = config["updates"]["auto_update"];
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.auto_update.schedule_days.push_back(day.as<std::string>());
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hot_reload = config["updates"]["hot_reload"];
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto notifications = config["updates"]["notifications"];
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.notifications.on_events.push_back(event.as<std::string>());
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto canary_yaml = config["updates"]["canary"];
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.canary.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto tel = config["updates"]["telemetry"];
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto checker = j["checker"];
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auto_update = j["auto_update"];
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hot_reload = j["hot_reload"];
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto notifications = j["notifications"];
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto canary_json = j["canary"];
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.canary.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto tel = j["telemetry"];
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["canary"]["stages"].push_back(stage_json);
  Confidence: band=high; score=0.74

### src/updates/dependency_resolver.cpp
Total findings: 14

- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(themis::utils::trim(cur));
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back({
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back({
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> added_edges;
  Confidence: band=medium; score=0.66
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep.package].push_back(pkg);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep.package].push_back(pkg);
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second == 0) ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(std::move(step));
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(std::move(step));
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> installed_map;
  Confidence: band=medium; score=0.66
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back({p1, p2, reason});
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back({p1, p2, reason});
  Confidence: band=high; score=0.74

### src/updates/in_place_schema_migrator.cpp
Total findings: 13

- Line 45: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_props;
  Confidence: band=high; score=0.74
- Line 51: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_props;
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> from_names;
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(p.name);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(p.name);
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_map;
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_map;
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.removed_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.modified_columns.push_back(std::move(mod));
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) cols_str += ", ";
  Confidence: band=high; score=0.74

### src/updates/delta_update_engine.cpp
Total findings: 9

- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (fd) dm.deltas.push_back(*fd);
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: registered_deltas_.push_back(manifest);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.files_fallback.push_back(fd.path);
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[h].push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[h].push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 720: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instructions.push_back(INSTR_COPY);
  Confidence: band=high; score=0.74

### src/updates/hot_reload_engine.cpp
Total findings: 6

- Line 184: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check compatibility
  Confidence: band=high; score=0.8
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("File not downloaded: " + file.path);
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rollback_points.emplace_back(rollback_id, timestamp);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (newest first)
  Confidence: band=high; score=0.74
- Line 548: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata["files"].push_back({{"path", file.path}});
  Confidence: band=high; score=0.74
- Line 548: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata["files"].push_back({{"path", file.path}});
  Confidence: band=high; score=0.74

### src/updates/release_manifest.cpp
Total findings: 3

- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_array.push_back(file.toJson());
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.files.push_back(*file);
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_array.push_back(file.toJson());
  Confidence: band=high; score=0.74

### src/updates/canary_rollout.cpp
Total findings: 2

- Line 77: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // same inputs – no inter-node communication required.
  Confidence: band=very_high; score=0.9
- Line 471: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.stages.push_back(cs);
  Confidence: band=high; score=0.74

### src/updates/notification_webhook.cpp
Total findings: 2

- Line 194: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!files_str.empty()) files_str += "\n";
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({{"title", "Files Updated"},
  Confidence: band=high; score=0.74

### src/updates/preflight_health_check.cpp
Total findings: 2

- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks_.push_back(std::move(check));
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.results.push_back(std::move(cr));
  Confidence: band=high; score=0.74

### src/updates/schema_migration.cpp
Total findings: 2

- Line 466: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) cols += ",";
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: undo_log_.push_back({op.table, idx_key, existing, had_value});
  Confidence: band=high; score=0.74

### src/updates/schema_migration_tester.cpp
Total findings: 2

- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: user_test_cases_.push_back(std::move(tc));
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.test_results.push_back(tr);
  Confidence: band=high; score=0.74

### src/updates/update_history_logger.cpp
Total findings: 2

- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(UpdateHistoryEntry::fromJson(item));
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJson());
  Confidence: band=high; score=0.74

### src/updates/build_verifier.cpp
Total findings: 1

- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
  Confidence: band=high; score=0.74

### src/updates/cluster_update_manager.cpp
Total findings: 1

- Line 56: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_statuses_.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/updates/coordinated_update_manager.cpp
Total findings: 1

- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_statuses_.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/updates/manifest_database.cpp
Total findings: 1

- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(it->key().ToString());
  Confidence: band=high; score=0.74

### src/updates/parallel_downloader.cpp
Total findings: 1

- Line 525: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(worker);
  Confidence: band=high; score=0.74

### src/updates/tenant_update_scheduler.cpp
Total findings: 1

- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/updates/update_state_machine.cpp
Total findings: 1

- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: callbacks_.push_back(std::move(cb));
  Confidence: band=high; score=0.74

### src/updates/blue_green_deployment.cpp
Total findings: 0


### src/updates/hardware_telemetry.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
