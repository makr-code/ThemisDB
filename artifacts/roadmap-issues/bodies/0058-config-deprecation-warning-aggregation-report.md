### Context

This issue implements the roadmap item 'Deprecation Warning Aggregation Report' for the config domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Deprecation Warning Aggregation Report

### Goal

Deliver the scoped changes for Deprecation Warning Aggregation Report in src/config/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Deprecation Warning Aggregation Report
**Priority:** High
**Target Version:** v1.7.0

Currently, each call to `resolve()` with a legacy path emits an individual log warning. For high-traffic deployments this floods logs. Replace per-call warnings with a background aggregation thread that batches and periodically reports which legacy paths are still in active use and how frequently.

**Implementation Notes:**
- `[x]` Add `DeprecationAggregator` class to `config_path_resolver.cpp`; maintained as a static singleton alongside `metrics_`.
- `[x]` `DeprecationAggregator` stores a `std::unordered_map<std::string, uint64_t> usage_counts_` (key = legacy path); updated atomically on each `legacy_fallback` increment.
- `[x]` Background reporter thread (or timer via `std::jthread`) fires every `aggregation_interval_s` (default: 300 s); logs a structured summary: `"[CONFIG] Legacy path report: {path: 'config/lora_training_config.yaml', hits: 4821, removal_date: '2026-06-01', guide: 'https://...'}"`.
- `[x]` Expose `ConfigPathResolver::deprecationReport()` returning a `std::vector<DeprecationEntry>` for use by admin CLI tooling.
- `[x]` Suppressed from per-call log warnings once aggregator is active (controlled by `setCachingEnabled`-style flag `setAggregationEnabled(bool)`).

**Performance Targets:**
- Aggregator map update: single atomic increment, < 50 ns overhead on `resolve()` hot path.
- Report generation for 60 legacy paths completes in < 1 ms (in-memory map iteration).

---

### Acceptance Criteria

- [ ] Add `DeprecationAggregator` class to `config_path_resolver.cpp`; maintained as a static singleton alongside `metrics_`.
- [ ] `DeprecationAggregator` stores a `std::unordered_map<std::string, uint64_t> usage_counts_` (key = legacy path); updated atomically on each `legacy_fallback` increment.
- [ ] Background reporter thread (or timer via `std::jthread`) fires every `aggregation_interval_s` (default: 300 s); logs a structured summary: `"[CONFIG] Legacy path report: {path: 'config/lora_training_config.yaml', hits: 4821, removal_date: '2026-06-01', guide: 'https://...'}"`.
- [ ] Expose `ConfigPathResolver::deprecationReport()` returning a `std::vector<DeprecationEntry>` for use by admin CLI tooling.
- [ ] Suppressed from per-call log warnings once aggregator is active (controlled by `setCachingEnabled`-style flag `setAggregationEnabled(bool)`).
- [ ] Aggregator map update: single atomic increment, < 50 ns overhead on `resolve()` hot path.
- [ ] Report generation for 60 legacy paths completes in < 1 ms (in-memory map iteration).

### Relationships

- Roadmap row: #58 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#deprecation-warning-aggregation-report
- Source key: roadmap:58:config:v1.7.0:deprecation-warning-aggregation-report

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:58:config:v1.7.0:deprecation-warning-aggregation-report -->
<!-- roadmap-ref: row=58;module=config;target=v1.7.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#deprecation-warning-aggregation-report -->
