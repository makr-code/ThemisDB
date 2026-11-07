# Metadata Module Research Notes

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-11-07
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Source level:** level1 — module-local
**SOT domain:** module-behavior
**Canonical source references:** [metadata source and tests](../../src/metadata/README.md), [performance expectations](./PERFORMANCE_EXPECTATIONS.md), [schema consistency checker](./schema_consistency_checker.cpp), [metadata test registration](../../tests/metadata/CMakeLists.txt)
**Related issue / milestone:** [#7435](https://github.com/makr-code/ThemisDB/issues/7435) · v2.5.0 / v3.0.0 research and wave plan

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

## Research findings and gap analysis (2026-11-07)

The assessment below compares primary technical guidance with the current
metadata sources. External specifications are reference designs, not claims of
ThemisDB conformance or a requirement to adopt another project's data model.

| Focus | Current source evidence | Research-informed gap / implication |
|---|---|---|
| Performance and scale | `SchemaManager` discovers schema by scanning stored keys and sampling entities; cache benchmarks cover cold/warm paths and table counts through 100. Separate consistency/lineage benchmarks cover serialization and conversion helpers. | Apache Iceberg identifies metadata planning cost as a scale concern and targets planning work that does not grow with table file/partition counts. The current benchmark set does not establish scaling beyond 100 tables, discovery under entity growth, consistency-scan cost, or representative p95/p99. First collect a reproducible workload curve; evaluate incremental metadata indexing only if it improves that measured workload without unacceptable write amplification. |
| Application and usability | The module exposes schema discovery/mutation, information-schema views, statistics, version/migration helpers, consistency checks, audit history, lineage, and external/distributed catalog paths. | These are useful for introspection, migration review, data governance, and catalog integration. Sampling can omit unsampled fields; lineage tracking is in-memory; consistency checking is diagnostic and capped at 5,000 orphan keys. The roadmap should state these boundaries and test actual consumer workflows rather than equating interface presence with integrated behavior. |
| Security and resilience | Security-provider and export-policy interfaces exist separately; `CatalogExporter` does not accept an export-policy object. Export failures have explicit outcomes, and schema audit is durable. Existing tests include failure paths and security-provider concurrency. | Provider declarations do not prove enforcement at server/export call sites. Before widening use, verify caller wiring and test denied export, malformed input, partial network failure, recovery, and redaction behavior end to end. NIST SP 800-92's lifecycle guidance supports treating event generation, storage, review, and retention as one operational control rather than assuming an audit API alone supplies governance. |
| Governance and maintainability | Public contracts are split across headers; `metadata_api_contract.h` is documented as a frozen, stale artifact. `SchemaAuditLog` persists records, while lineage and the supplied snapshot store are in-memory. | Resolve the stale contract's supported/deprecated status before a major-version contract change. Define ownership, schema evolution, retention, privacy, and recovery requirements for any future durable lineage/snapshot capability. OpenLineage's versioned facet schema URLs and project-prefixed custom facets offer a useful interoperability pattern, not a replacement for ThemisDB's API contract. |
| Architecture and operations | Module code is linked into ThemisDB rather than a standalone service. Focused tests are registered in `tests/metadata/CMakeLists.txt`; benchmarks are optional and registered in `benchmarks/metadata/CMakeLists.txt`. A developer runbook exists, but deployment command coverage and the planned runbook scope have not been verified. | The current evidence supports substantial functional implementation, but not a claim of release-grade scale or operations readiness. Keep source presence, test registration, executed evidence, and release sign-off as distinct states. |

### Validation and evidence limits

- `tests/metadata/test_metadata_highcardinality_stress.cpp` exercises an
  in-process `StubMetadataTable`; its 200,000-key and concurrent cases are not
  measurements of `SchemaManager`, RocksDB, or production metadata paths.
- `tests/metadata/test_metadata_schema_churn_stress_focused.cpp` uses a
  `NullDB` test double. It is useful focused contract evidence, not a
  persistence or production-load test.
- `benchmarks/metadata/bench_metadata_cache.cpp` contains real RocksDB-backed
  cache scenarios and cache targets, but benchmark source presence is not a
  result. `bench_metadata_release_gates.cpp` benchmarks a frozen error enum;
  it does not demonstrate current exporter/runtime error-path performance.
- `benchmarks/metadata/bench_metadata_consistency_lineage_gates.cpp` uses five
  repetitions for its CPU-only helper benchmarks. These cases do not measure
  end-to-end consistency scans, lineage graph traversal, catalog HTTP
  publication, or durable recovery.
- No benchmark execution, representative-hardware baseline, caller-wiring
  audit, external catalog compatibility test, or complete deployment-runbook
  review was performed for this roadmap research. Baseline and pass/fail states
  remain unknown until their evidence artifacts are recorded.

## External technical references

1. Apache Iceberg, [Table Specification](https://iceberg.apache.org/spec/).
   Its design goals include metadata planning efficiency, serializable
   snapshots, and scalable client-side planning. Relevant as a comparative
   table-metadata design; ThemisDB is not an Iceberg implementation.
2. OpenLineage, [Facets and Extensibility](https://openlineage.io/docs/spec/facets/).
   Describes versioned schema references and namespaced custom facets for
   interoperable lineage metadata. Relevant to a future lineage-export
   contract; it does not establish that current ThemisDB lineage is durable.
3. NIST SP 800-92, [Guide to Computer Security Log Management](https://csrc.nist.gov/pubs/sp/800/92/final).
   Recommends lifecycle-level planning for log management infrastructure and
   processes. Used here as a governance/reliability reference, not a claim of
   compliance certification.
4. Google Benchmark, [User Guide: repetitions and result reporting](https://google.github.io/benchmark/user_guide.html).
   Documents repeated runs and aggregate statistics. Relevant to benchmark
   reproducibility; repetition count alone does not remove hardware or workload
   bias.
5. Siddhartha Parimi, [“A Comparative Performance & Metadata Study of Open Table
   Formats: Iceberg vs Delta vs Hudi at Scale,” Journal of Computer Science and
   Technology Studies 7(12), 513–520 (2025)](https://doi.org/10.32996/jcsts.2025.7.12.56).
   The study reports comparative benchmarking of metadata scalability,
   concurrency, and transaction behavior across open table formats. Its
   architecture and workload are not directly comparable to ThemisDB, and its
   reported results have not been independently reproduced here; it motivates
   measuring workload dimensions rather than supplying ThemisDB thresholds.

### Open assumptions and knowledge gaps

- Versioning policy identifies `v2.5.0` as the next planned minor version.
  The `v3.0.0` horizon is strategic planning only; its release date, feature
  scope, and API-break policy require release-owner confirmation.
- No representative workload distribution (table count, entity count,
  mutation rate, tenant count, or catalog-export frequency) was provided.
- No absolute p95/p99 latency objective is currently specified in
  `PERFORMANCE_EXPECTATIONS.md`; the existing ≤10% regression criterion is
  relative to a release baseline that has not been validated here.
- Whether durable lineage, durable snapshots, incremental discovery, or
  automatic repair are product requirements is unresolved. The roadmap treats
  them as evaluation/design decisions, not committed runtime capabilities.
- The security-provider and export-policy integration points need caller-level
  tracing before an end-to-end enforcement claim can be made.
