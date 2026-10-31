# Metadata Module Research Notes

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Source level:** level1 — module-local
**SOT domain:** module-behavior
**Canonical source references:** [metadata source and tests](../../src/metadata/README.md), [performance expectations](./PERFORMANCE_EXPECTATIONS.md), [schema consistency checker](./schema_consistency_checker.cpp), [metadata test registration](../../tests/metadata/CMakeLists.txt)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Purpose and evidence boundary

These notes summarize research questions that can be answered from the metadata
module's current C++ interfaces, implementations, tests, and module roadmap.
They are not a product-status or release-readiness statement. Use the source and
focused tests linked below to verify claims before changing behavior.

## Current research surface

| Question | Existing evidence | Useful next check |
|---|---|---|
| How is schema discovered? | [`SchemaManager`](../../include/metadata/schema_manager.h) scans RocksDB key prefixes, samples entities for properties, reads available index metadata, and caches schema views. | Exercise cache refresh, mixed field types, and key-prefix edge cases in [`test_schema_manager.cpp`](../../tests/metadata/test_schema_manager.cpp). |
| How are changes checked and explained? | [`SchemaConsistencyChecker`](../../include/metadata/schema_consistency_checker.h) checks orphan keys, stale statistics, and missing constraint definitions when those collaborators are supplied. | Compare issue results and diagnostics in [`test_schema_consistency_checker.cpp`](../../tests/metadata/test_schema_consistency_checker.cpp) and [`test_schema_consistency_edge_cases_focused.cpp`](../../tests/metadata/test_schema_consistency_edge_cases_focused.cpp). |
| What provenance is retained? | [`ColumnLineageTracker`](../../include/metadata/column_lineage.h) records in-memory derivation entries and traverses upstream/downstream column relationships. | Check multi-source and diamond-DAG behavior in [`test_column_lineage.cpp`](../../tests/metadata/test_column_lineage.cpp). |
| What leaves the module? | [`CatalogExporter`](../../include/metadata/catalog_exporter.h) builds Apache Atlas or DataHub payloads and reports publish outcomes. | Use the injected HTTP test double in [`test_catalog_exporter.cpp`](../../tests/metadata/test_catalog_exporter.cpp); these tests do not require a live catalog. |
| Which stress evidence is representative? | The focused tests include schema churn, security-provider concurrency, and metadata-specific hardening cases. | Distinguish real module behavior tests from the in-process stubs in [`test_metadata_highcardinality_stress.cpp`](../../tests/metadata/test_metadata_highcardinality_stress.cpp). |

## Evidence limits and open questions

- [`ROADMAP.md`](./ROADMAP.md) identifies broader concurrent-access stress and
  benchmark re-baselining as work in progress. Do not treat focused tests as a
  substitute for long-duration production soak evidence.
- The module's [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md)
  maps four target groups to cases in
  [`benchmarks/metadata/bench_metadata_cache.cpp`](../../benchmarks/metadata/bench_metadata_cache.cpp).
  Its prior `benchmarks/bench_metadata_cache.cpp` reference was stale. The
  mapped cases are present in the nested source; case presence does not establish
  that a benchmark run occurred or that any target was met.
- The source-verified case mappings are:

  | Expectation | Benchmark cases |
  |---|---|
  | METP-1: cold discovery scaling | `BM_MetadataCache_ColdScan`, `BM_MetadataCache_RocksDBScan_Direct` |
  | METP-2: warm retrieval and cache hit/miss | `BM_MetadataCache_WarmHit`, `BM_MetadataCache_HitRate_Hit`, `BM_MetadataCache_HitRate_Miss` |
  | METP-3: table/database lookups | `BM_MetadataCache_GetTable_Hit`, `BM_MetadataCache_GetTable_Miss`, `BM_MetadataCache_GetDatabaseMetadata` |
  | METP-4: maintenance/adaptation/concurrency | `BM_MetadataCache_RefreshCache`, `BM_MetadataCache_TTLVariants`, `BM_MetadataCache_AdaptiveTTL`, `BM_MetadataCache_ConcurrentReads` |

- The same expectations define METG-1 as regression ≤10% against the release
  baseline, METG-2 as hot-path p99 within the configured release threshold, and
  METG-3 as completeness of mapped cases in the release manifest. These are
  release targets, not evidence of a current run or passing gates. The configured
  p99 threshold is not specified in the expectations page. No benchmark run,
  measurement output, baseline, p99 value, or release manifest was validated for
  this documentation change; no performance result is implied.
- [`SchemaConsistencyChecker`](../../include/metadata/schema_consistency_checker.h)
  scans at most 5,000 keys for orphan prefixes. Stale-statistics and
  missing-constraint checks run only when their respective collaborators are
  supplied. See the implementation in
  [`schema_consistency_checker.cpp`](./schema_consistency_checker.cpp).
- [`ROADMAP.md`](./ROADMAP.md) is internally inconsistent about the operator
  runbook: its planned-features entry marks
  [`docs/operability/RUNBOOK_METADATA_STORE.md`](../../docs/operability/RUNBOOK_METADATA_STORE.md)
  delivered, while its known-issues section says the runbook is not yet written.
  [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) also lists it as future
  work. Reconcile these status claims; do not repeat either as settled fact.
  The existing runbook file is not proof that its CLI commands are implemented.
- Current interfaces do not establish that every production server access path
  is wired to the optional security providers or that in-memory lineage is
  durably persisted. Confirm integration at the caller before making those
  claims.

## Research workflow

1. Start with [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) and classify the question
   as schema, consistency, lineage, statistics, versioning, or export.
2. Read the corresponding public header and implementation before treating
   module-local summaries as behavioral evidence.
3. Find the focused test registration in
   [`tests/metadata/CMakeLists.txt`](../../tests/metadata/CMakeLists.txt), then
   inspect the test assertions for edge cases and test doubles.
4. Record any mismatch between implementation, tests, and module planning in
   the issue or review notes. Do not silently turn roadmap aspirations into
   implemented guarantees.
