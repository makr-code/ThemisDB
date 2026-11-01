---
Author: "ThemisDB Team"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/temporal/*.h; src/temporal/{README,API,ARCHITECTURE,ROADMAP,PERFORMANCE_EXPECTATIONS,SECURITY}.md; tests/temporal/CMakeLists.txt; benchmarks/temporal/CMakeLists.txt"
source: "include/temporal; src/temporal; tests/temporal; benchmarks/temporal; cmake/ModularBuild.cmake; ROADMAP.md"
source_class: "secondary"
source_weight: 60
---

> Exact model backend not verifiable from final document text alone.

# Temporal — Developer LLM Wiki

Curated source orientation for coding and review in the temporal module. This is
secondary context, not an API contract or release-status source. Prefer public
headers for signatures, implementations and focused tests for behavior, and the
root roadmap for project maturity.

## Runtime shape and contracts

Public runtime interfaces and implementation are under
[`include/temporal/`](../../include/temporal/) and
[`src/temporal/`](../../src/temporal/). The runtime namespace is
`themisdb::temporal`; its `Timestamp` is milliseconds since the Unix epoch and
`TimeRange` is half-open `[start, end)`.

The module contains system-versioned and bitemporal tables, query and join
helpers, interval indexes, aggregation, snapshots, retention, CDC, conflict
resolution, compression, migration, and cold-store/tiering interfaces. These
are separate C++ APIs; do not infer a single storage, error, or persistence
contract across them.

In particular, [`temporal_api_contract.h`](../../include/temporal/temporal_api_contract.h)
declares a distinct `themis::temporal` contract namespace, including
nanosecond-named constants and normative statements. It is not the
`themisdb::temporal` runtime contract, and its declared requirements do not prove
runtime implementation.

The module APIs do not themselves establish general SQL/AQL parser integration,
service authorization, or deployment policy. The root
[`ROADMAP.md`](../../ROADMAP.md) is authoritative for current maturity and
release readiness; code or test presence is not evidence that a gate passed.

## Persistence and lifecycle boundaries

- `TemporalSnapshotManager` captures snapshots in process memory.
- `TemporalCDC` uses a bounded in-memory replay log; `CDCPersistentLog` is a
  separate filesystem WAL API.
- Retention can remove historical versions. Verify policy and recovery needs
  before enabling enforcement.
- Cold-store interfaces expose in-memory and filesystem implementations; verify
  the concrete backend and its durability behavior before relying on it.

These are API/implementation boundaries, not deployment guarantees. See the
[module security notes](../../src/temporal/SECURITY.md) for payload exposure,
authorization, and filesystem protection considerations.

## Build, tests, and benchmarks

`cmake/ModularBuild.cmake` lists temporal implementation translation units in
the repository modular source sets. Focused test registration is in
[`tests/temporal/CMakeLists.txt`](../../tests/temporal/CMakeLists.txt): one loop
registers `test_*.cpp`; a second loop also registers `test_temporal_*.cpp` with
the `temporal` and `autogen` labels. Some matching sources therefore have two
targets. The optional `themis_temporal` link and fallback source addition are
conditional; inspect the configured target graph rather than assuming either
path is active.

The high-cardinality stress source uses local stubs for versioned-write,
conflict-resolution, and CDC examples; it does not exercise temporal runtime
classes or retention. Treat test source inventory separately from executed
results.

[`benchmarks/temporal/CMakeLists.txt`](../../benchmarks/temporal/CMakeLists.txt)
registers `bench_*.cpp` only when `THEMIS_BUILD_BENCHMARKS` is enabled. The
query and interval-tree erase sources exercise runtime classes; the dedicated
gates and release-gate sources contain stub/mock workloads, not runtime
performance evidence. Refer to
[`PERFORMANCE_EXPECTATIONS.md`](../../src/temporal/PERFORMANCE_EXPECTATIONS.md)
for case mapping and missing-result caveats.

## Doxygen reference

The current generated listing is
[`src/temporal/DOXYGEN.md`](../../src/temporal/DOXYGEN.md), derived from
[`module_doxygen_artifacts/temporal/xml/index.xml`](module_doxygen_artifacts/temporal/xml/index.xml)
with Doxygen 1.9.8 on 2026-11-01. It covers 56 C/C++ inputs across include,
source, tests, and benchmarks. Generation passed with 73 warnings, primarily
duplicate parameter documentation in implementation comments; the earlier
malformed query examples and in-body `mutex_` parameter tags were corrected.
Public headers remain the source of truth for signatures and behavior.

The direct coverage check filtered 324 source symbols and found 24 missing
symbols, 309 missing briefs, 159 missing parameter descriptions, and 183 missing
return descriptions. The checker spans public headers, implementation, tests,
and benchmarks; its unqualified-name matching and class/enum classification
produce false positives. These counts are not acceptance targets or proof that
the declarations lack usable documentation. See the
[coverage summary](module_doxygen_artifacts/temporal/direct_doxygen_coverage.md)
and [detailed report](module_doxygen_artifacts/temporal/direct_doxygen_summary.md)
for scope and examples before relying on extracted descriptive text.

## Source priority and freshness

For exact declarations use the public headers; for behavior, inspect the
corresponding implementation and focused test. Module API/architecture docs
provide navigation, while this page is an orientation layer. If statements
conflict, prefer the implementation and headers for behavior and the root
roadmap for maturity.

The Developer LLM Wiki inventory and `WIKI_STATUS.json` still describe the
2026-10-31 full-sync snapshot. This curated temporal page was added manually on
2026-11-01; it does not claim that a full wiki-source rescan was performed.
