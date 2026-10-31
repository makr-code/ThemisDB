# Metadata Implementation Map

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
**Canonical source references:** [metadata implementation](./), [public metadata headers](../../include/metadata/), [focused test registration](../../tests/metadata/CMakeLists.txt)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Code map

| Concern | Implementation | Public contract / key test |
|---|---|---|
| Schema discovery, cache, overrides | [`schema_manager.cpp`](./schema_manager.cpp) | [`schema_manager.h`](../../include/metadata/schema_manager.h), [`test_schema_manager.cpp`](../../tests/metadata/test_schema_manager.cpp) |
| Statistics and refresh | [`statistics_collector.cpp`](./statistics_collector.cpp) | [`statistics_collector.h`](../../include/metadata/statistics_collector.h), [`test_statistics_collector.cpp`](../../tests/metadata/test_statistics_collector.cpp), [`test_statistics_auto_refresh.cpp`](../../tests/metadata/test_statistics_auto_refresh.cpp) |
| SQL-style metadata views | [`information_schema.cpp`](./information_schema.cpp) | [`information_schema.h`](../../include/metadata/information_schema.h) |
| Constraints and consistency | [`schema_constraints.cpp`](./schema_constraints.cpp), [`schema_consistency_checker.cpp`](./schema_consistency_checker.cpp) | [`schema_constraints.h`](../../include/metadata/schema_constraints.h), [`schema_consistency_checker.h`](../../include/metadata/schema_consistency_checker.h) |
| Versions, diff, and migration helpers | [`schema_version_manager.cpp`](./schema_version_manager.cpp) | [`schema_version_manager.h`](../../include/metadata/schema_version_manager.h), [`test_schema_version_manager.cpp`](../../tests/metadata/test_schema_version_manager.cpp), [`test_schema_migration_script.cpp`](../../tests/metadata/test_schema_migration_script.cpp) |
| Durable schema audit | [`schema_audit_log.cpp`](./schema_audit_log.cpp) | [`schema_audit_log.h`](../../include/metadata/schema_audit_log.h), [`test_schema_audit_log.cpp`](../../tests/metadata/test_schema_audit_log.cpp) |
| Column lineage | [`column_lineage.cpp`](./column_lineage.cpp) | [`column_lineage.h`](../../include/metadata/column_lineage.h), [`test_column_lineage.cpp`](../../tests/metadata/test_column_lineage.cpp) |
| External catalog integration | [`catalog_exporter.cpp`](./catalog_exporter.cpp) | [`catalog_exporter.h`](../../include/metadata/catalog_exporter.h), [`test_catalog_exporter.cpp`](../../tests/metadata/test_catalog_exporter.cpp) |
| Standalone export-policy objects | Header-only [`imetadata_export_policy.h`](../../include/metadata/imetadata_export_policy.h) | [`test_metadata_export_policy.cpp`](../../tests/metadata/test_metadata_export_policy.cpp); not accepted by `CatalogExporter`'s current API |
| Distributed schema catalog | [`distributed_catalog.cpp`](./distributed_catalog.cpp) | [`distributed_catalog.h`](../../include/metadata/distributed_catalog.h), [`test_distributed_catalog.cpp`](../../tests/metadata/test_distributed_catalog.cpp) |
| Index recommendations and ER export | [`index_recommender.cpp`](./index_recommender.cpp), [`er_diagram_exporter.cpp`](./er_diagram_exporter.cpp) | [`index_recommender.h`](../../include/metadata/index_recommender.h), [`er_diagram_exporter.h`](../../include/metadata/er_diagram_exporter.h) |

## Change workflow

1. Locate the authoritative interface in `include/metadata/` and its owning
   implementation in this directory. Keep public contract, error behavior,
   ownership, and thread-safety statements aligned.
2. Find the target under
   [`tests/metadata/CMakeLists.txt`](../../tests/metadata/CMakeLists.txt).
   Focused targets are registered through `themis_register_module_focused_test`;
   use the repository's configured build directory and target names in that
   registration rather than guessing a test binary name.
3. Add a regression for the changed edge case. Keep external HTTP behavior
   deterministic by using `CatalogExporter::setHttpPostForTesting()`.
4. Run the focused target/test, then relevant related module coverage. Report
   exactly what ran; source inspection alone is not test evidence.
5. Update the smallest relevant module document and this guide only when the
   implementation map or developer workflow changes.

## Concurrency and lifetime checks

- `SchemaManager` synchronizes cache access with a shared mutex; its mutations
  and cache refresh must preserve the existing lock discipline.
- `StatisticsCollector` owns an optional refresh thread; stop it through the
  public lifecycle API or let destruction stop it. A metrics-hook pointer is
  non-owning and its owner must outlive the collector's use of it.
- `SchemaVersionManager` and `SchemaConstraints` require external
  synchronization when shared across threads.
- `SchemaConsistencyChecker::runCheck()` is thread-safe, but starting and
  stopping its background worker are not safe to race against each other.
- `DistributedMetadataCatalog` holds a non-owning reference to its router.

See [API.md](./API.md) for contract details and [RUNBOOK.md](./RUNBOOK.md) for
source-backed troubleshooting steps.
