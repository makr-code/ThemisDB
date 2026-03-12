### Context

This issue implements the roadmap item '`CapabilityAutoGenerator`: Persist Schedule and Document Count State' for the utils domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `CapabilityAutoGenerator`: Persist Schedule and Document Count State

### Goal

Deliver the scoped changes for `CapabilityAutoGenerator`: Persist Schedule and Document Count State in src/utils/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `CapabilityAutoGenerator`: Persist Schedule and Document Count State
**Priority:** Medium
**Target Version:** v1.8.0

`capability_auto_generator.cpp` has 3 TODOs:
- Line 193: "Check last update time and compare with schedule interval" — schedule checking is a no-op; updates run every invocation regardless of interval.
- Line 373: "Store previous document count somewhere" — document count delta cannot be computed without persistence.
- Line 435: "Implement YAML serialization and file writing" — generated capabilities are not persisted to disk.

**Implementation Notes:**
- `[ ]` Use a small RocksDB key (`utils_capgen_state`) to persist `last_run_timestamp` and `last_document_count`; load on construction.
- `[ ]` At line 193: compare `now - last_run_timestamp` against `config_.schedule_interval_s`; skip regeneration if within interval.
- `[ ]` At line 373: persist the current `document_count` to the state key after each successful run.
- `[ ]` At line 435: serialize the generated `CapabilitySet` to YAML using `yaml-cpp` and atomically write via `ConfigPathResolver::resolveWritable()`.

---

### Acceptance Criteria

- [ ] Line 193: "Check last update time and compare with schedule interval" — schedule checking is a no-op; updates run every invocation regardless of interval.
- [ ] Line 373: "Store previous document count somewhere" — document count delta cannot be computed without persistence.
- [ ] Line 435: "Implement YAML serialization and file writing" — generated capabilities are not persisted to disk.
- [ ] Use a small RocksDB key (`utils_capgen_state`) to persist `last_run_timestamp` and `last_document_count`; load on construction.
- [ ] At line 193: compare `now - last_run_timestamp` against `config_.schedule_interval_s`; skip regeneration if within interval.
- [ ] At line 373: persist the current `document_count` to the state key after each successful run.
- [ ] At line 435: serialize the generated `CapabilitySet` to YAML using `yaml-cpp` and atomically write via `ConfigPathResolver::resolveWritable()`.

### Relationships

- Roadmap row: #217 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/utils/FUTURE_ENHANCEMENTS.md#capabilityautogenerator-persist-schedule-and-document-count-state
- Source key: roadmap:217:utils:v1.8.0:capabilityautogenerator-persist-schedule-and-document-count-state

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:217:utils:v1.8.0:capabilityautogenerator-persist-schedule-and-document-count-state -->
<!-- roadmap-ref: row=217;module=utils;target=v1.8.0 -->
<!-- roadmap-detail: src/utils/FUTURE_ENHANCEMENTS.md#capabilityautogenerator-persist-schedule-and-document-count-state -->
