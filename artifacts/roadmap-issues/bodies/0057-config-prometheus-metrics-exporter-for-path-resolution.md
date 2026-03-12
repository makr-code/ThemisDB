### Context

This issue implements the roadmap item 'Prometheus Metrics Exporter for Path Resolution' for the config domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Prometheus Metrics Exporter for Path Resolution

### Goal

Deliver the scoped changes for Prometheus Metrics Exporter for Path Resolution in src/config/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Prometheus Metrics Exporter for Path Resolution
**Priority:** High
**Target Version:** v1.7.0

`ConfigPathResolver::Metrics` tracks `resolution_hits`, `resolution_misses`, `legacy_fallbacks`, `cache_hits`, `cache_misses`, and `unmapped_requests` via `std::atomic<uint64_t>`. Expose these as Prometheus gauges/counters so operations teams can detect when legacy paths are still in use and monitor migration progress.

**Implementation Notes:**
- `[x]` Create `config_metrics_exporter.cpp`; register with the server's Prometheus registry at startup (`prometheus/registry.h`).
- `[x]` Metric names: `themis_config_resolution_hits_total`, `themis_config_resolution_misses_total`, `themis_config_legacy_fallbacks_total`, `themis_config_cache_hit_ratio` (derived), `themis_config_unmapped_requests_total`.
- `[x]` Add label `category` to `themis_config_legacy_fallbacks_total` using `ConfigPathResolver::inferCategory()` (already private method) to show which config category has the most legacy usage.
- `[x]` Export function called every scrape interval (pull model); read from `ConfigPathResolver::metrics()` atomics — no mutex needed.
- `[x]` Add `themis_config_cache_capacity` and `themis_config_cache_ttl_seconds` info metrics for dashboard context.

**Performance Targets:**
- Metrics scrape completes in < 1 ms (atomic reads, no cache iteration).
- Zero impact on `ConfigPathResolver::resolve()` hot path (metrics are already incremented by existing atomic ops).

---

### Acceptance Criteria

- [ ] Create `config_metrics_exporter.cpp`; register with the server's Prometheus registry at startup (`prometheus/registry.h`).
- [ ] Metric names: `themis_config_resolution_hits_total`, `themis_config_resolution_misses_total`, `themis_config_legacy_fallbacks_total`, `themis_config_cache_hit_ratio` (derived), `themis_config_unmapped_requests_total`.
- [ ] Add label `category` to `themis_config_legacy_fallbacks_total` using `ConfigPathResolver::inferCategory()` (already private method) to show which config category has the most legacy usage.
- [ ] Export function called every scrape interval (pull model); read from `ConfigPathResolver::metrics()` atomics — no mutex needed.
- [ ] Add `themis_config_cache_capacity` and `themis_config_cache_ttl_seconds` info metrics for dashboard context.
- [ ] Metrics scrape completes in < 1 ms (atomic reads, no cache iteration).
- [ ] Zero impact on `ConfigPathResolver::resolve()` hot path (metrics are already incremented by existing atomic ops).

### Relationships

- Roadmap row: #57 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#prometheus-metrics-exporter-for-path-resolution
- Source key: roadmap:57:config:v1.7.0:prometheus-metrics-exporter-for-path-resolution

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:57:config:v1.7.0:prometheus-metrics-exporter-for-path-resolution -->
<!-- roadmap-ref: row=57;module=config;target=v1.7.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#prometheus-metrics-exporter-for-path-resolution -->
