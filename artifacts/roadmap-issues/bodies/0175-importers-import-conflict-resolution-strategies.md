### Context

This issue implements the roadmap item 'Import Conflict Resolution Strategies' for the importers domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Import Conflict Resolution Strategies

### Goal

Deliver the scoped changes for Import Conflict Resolution Strategies in src/importers/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Import Conflict Resolution Strategies
**Priority:** Medium
**Target Version:** v1.7.0

Add an `ImportConflictResolver` that handles documents where the target collection already contains a document with the same key. Currently the PostgreSQL importer silently overwrites or errors on conflict; operators need explicit control to support upsert, merge, and skip workflows.

**Implementation Notes:**
- Add `conflict_resolver.cpp` with four strategies: `SKIP` (do not overwrite existing), `OVERWRITE` (replace existing document entirely), `MERGE` (merge fields: incoming fields win unless the existing field is listed in `protected_fields`), and `ERROR` (abort the batch on first conflict).
- Strategy is configured per import job via `ImportConfig::conflict_strategy`; default is `OVERWRITE` for backward compatibility.
- MERGE strategy uses a configurable `merge_depth` (default 1, meaning top-level fields only; set to -1 for deep recursive merge) to avoid unexpected behavior with nested objects.
- Emit `importers_conflicts_total` Prometheus counter with label `strategy` and `outcome=skipped|overwritten|merged|error` for operator visibility.

**Performance Targets:**
- Conflict resolution overhead ≤ 5 % of import throughput for SKIP and OVERWRITE strategies (one extra key-existence check per document).
- MERGE strategy overhead ≤ 15 % compared to OVERWRITE for documents with ≤ 100 fields.

---

### Acceptance Criteria

- [ ] Add `conflict_resolver.cpp` with four strategies: `SKIP` (do not overwrite existing), `OVERWRITE` (replace existing document entirely), `MERGE` (merge fields: incoming fields win unless the existing field is listed in `protected_fields`), and `ERROR` (abort the batch on first conflict).
- [ ] Strategy is configured per import job via `ImportConfig::conflict_strategy`; default is `OVERWRITE` for backward compatibility.
- [ ] MERGE strategy uses a configurable `merge_depth` (default 1, meaning top-level fields only; set to -1 for deep recursive merge) to avoid unexpected behavior with nested objects.
- [ ] Emit `importers_conflicts_total` Prometheus counter with label `strategy` and `outcome=skipped|overwritten|merged|error` for operator visibility.
- [ ] Conflict resolution overhead ≤ 5 % of import throughput for SKIP and OVERWRITE strategies (one extra key-existence check per document).
- [ ] MERGE strategy overhead ≤ 15 % compared to OVERWRITE for documents with ≤ 100 fields.

### Relationships

- Roadmap row: #175 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/importers/FUTURE_ENHANCEMENTS.md#import-conflict-resolution-strategies
- Source key: roadmap:175:importers:v1.7.0:import-conflict-resolution-strategies

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:175:importers:v1.7.0:import-conflict-resolution-strategies -->
<!-- roadmap-ref: row=175;module=importers;target=v1.7.0 -->
<!-- roadmap-detail: src/importers/FUTURE_ENHANCEMENTS.md#import-conflict-resolution-strategies -->
