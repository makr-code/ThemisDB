---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Temporal focused tests

This directory contains 20 `test_*.cpp` sources covering temporal tables,
queries, indexes, aggregation, snapshots, retention, CDC, conflict handling,
compression, migration, tiers, graph behavior, and version compatibility. The
source inventory is not a test result or a coverage percentage.

## CMake registration

[`CMakeLists.txt`](CMakeLists.txt) glob-registers every `test_*.cpp` as a
`module_temporal_<stem>_focused` target and a focused CTest entry with a
120-second timeout. A second loop registers each of the 15 `test_temporal_*.cpp`
sources again as an `_autofocused` target/CTest entry with the `temporal` and
`autogen` labels. These are separate registrations of the same source files,
not additional test source files.

Each target links `TEST_LIBS`, `themis_core`, `spdlog::spdlog`, and
`Threads::Threads`; it links `themis_temporal` only if that CMake target exists.
When it does not, CMake conditionally adds `src/index/temporal_graph.cpp` if
present. Confirm the configured target graph before relying on the optional
link path.

## Test areas

- `test_bi_temporal.cpp`, `test_system_versioned_table.cpp`,
  `test_temporal_query_engine.cpp` — version and query behavior.
- `test_interval_tree_index.cpp`, `test_temporal_index.cpp`,
  `test_temporal_aggregation.cpp`, `test_temporal_aggregation_property.cpp`,
  `test_temporal_aggregator.cpp` — indexing and aggregation.
- `test_snapshot_manager.cpp`, `test_retention_manager.cpp` — snapshot and
  retention lifecycle behavior.
- `test_temporal_highcardinality_stress.cpp` — stub-based versioned-write,
  conflict-resolution, and CDC examples; it does not exercise temporal runtime
  classes or retention behavior.
- `test_temporal_cdc.cpp`, `test_temporal_conflict_resolver.cpp`,
  `test_temporal_compressor.cpp`, `test_temporal_migrator.cpp`,
  `test_temporal_cold_store.cpp`, `test_temporal_tier_manager.cpp` — event,
  conflict, history, and tier behavior.
- `test_temporal_contract_hardening_focused.cpp`,
  `test_temporal_graph.cpp`, `test_temporal_v18_v19.cpp` — contract-focused,
  graph, and compatibility tests.

No build or test run was performed for this documentation update. Use the
repository's canonical build setup and inspect CTest's generated test list for
the selected configuration.
