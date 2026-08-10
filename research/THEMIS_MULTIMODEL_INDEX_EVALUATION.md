# ThemisDB Multi-Model Index Evaluation: Repository-Grounded Architecture, Evidence, and Validation Gaps

**Status**: Reviewed draft  
**Version**: 0.2  
**Last Updated**: 2026-08-10  
**Target Venue**: arXiv / systems-internal technical review  
**Language**: English

---

## Abstract

This document reviews the current ThemisDB index subsystem against the repository's canonical implementation sources instead of treating roadmap targets as measured results. The reviewed codebase contains a production-facing index surface centered on `IndexManager` for secondary, vector, and graph indexes, with adjacent components for spatial indexing, full-text indexing, learned indexes, Matryoshka-based truncation, adaptive index recommendations, and hot/warm/cold tier migration. The current evidence base is strong for interface shape, correctness properties, tenant isolation, lifecycle behavior, and selected single-node latency thresholds. It is weaker for publication-grade performance claims such as GPU throughput multipliers, ANN recall curves, VRAM reduction factors, or end-to-end cross-model benchmark results. This revision therefore removes unmeasured claims, normalizes terminology, and reframes evaluation around source-verifiable artifacts: module documentation, public headers, and focused tests.

---

## 1. Introduction

### 1.1 Problem Context

ThemisDB aims to support multi-model workloads that combine several access patterns in one codebase: ordered lookups and range scans, approximate nearest-neighbor retrieval, graph traversal, geospatial filtering, and full-text search. Such workloads require different indexing strategies, different failure semantics, and different operational controls. A technically credible evaluation must therefore distinguish between:

1. implemented index-related components,
2. behaviors that are validated by tests or repository artifacts, and
3. performance or scalability claims that still require dedicated benchmark evidence.

### 1.2 Why the Review Was Necessary

The previous draft mixed three different evidence classes:

- implementation facts from headers and source files,
- acceptance thresholds from focused tests,
- planned or expected benchmark outcomes.

That made several statements stronger than the current repository state supports. In particular, the draft presented target recall, throughput, p99 latency, and dataset-scale claims as if they were already backed by committed benchmark artifacts.

### 1.3 Goal of This Revision

This revised article has a narrower and more defensible goal:

- describe the current ThemisDB index surface using repository-grounded terminology,
- retain only claims that can be tied to code, tests, or module documentation,
- identify where the present evidence stops,
- provide a review-ready structure: problem -> approach -> evaluation -> limitations -> conclusion.

---

## 2. Methodology / Review Approach

### 2.1 Source-of-Truth Domain

This review is scoped to the documentation-governance domain **module behavior and implementation status**. The primary sources used for this document are:

- `src/index/ROADMAP.md`
- `src/index/ARCHITECTURE.md`
- `include/index/index_manager.h`
- `include/index/README.md`
- `tests/db/test_index_performance.cpp`
- `tests/test_multi_tenant_index.cpp`
- `tests/test_adaptive_index.cpp`
- `tests/test_inverted_index.cpp`
- `tests/geo/test_spatial_index.cpp`
- `tests/index/test_spatial_correctness_integration.cpp`
- `tests/index/test_tiered_index_migration.cpp`
- `tests/index/test_learned_index.cpp`
- `tests/index/test_matryoshka_truncation.cpp`
- `tests/test_cross_module_index_matryoshka.cpp`

### 2.2 Review Rules

Only statements satisfying at least one of the following rules are retained as central claims:

1. the behavior is defined in a public header or module architecture document,
2. the behavior is validated by a focused test,
3. the statement is explicitly marked as a roadmap target or open validation gap.

Claims without one of those anchors were either removed or rewritten as open work.

### 2.3 Terminology Normalization

This document uses the following terminology consistently:

- **multi-model**: multiple index and query styles in one repository/module surface.
- **IndexManager façade**: the DI-enabled public manager covering **secondary**, **vector**, and **graph** index creation and lookup APIs; it is not itself the sole entry point for every adjacent index utility in the repository.
- **adjacent index components**: `AdaptiveIndexManager`, `TieredIndexManager`, `SpatialIndexManager`, `InvertedIndex`, `LearnedIndex`, `MatryoshkaTruncatedIndex`, and related helpers.
- **tenant-scoped index name**: the prefix form `tenant:<tenant_id>:<index_name>` verified in `IndexManager::makeTenantIndexName()`.
- **performance evidence**: thresholds or measurements encoded in committed tests, not aspirational benchmark tables.

---

## 3. Current ThemisDB Index Surface

### 3.1 Module Status

The current index roadmap describes a production-facing subsystem with active hardening work still in progress. `src/index/ROADMAP.md` reports existing runtime support across vector, secondary, spatial, and graph indexing, but it also marks benchmark stabilization, backend parity, diagnostics consistency, and parts of GPU acceleration as ongoing work. The same roadmap centers current delivery on **ANN Frontdoor** rollout and associated hardening gates rather than on a closed, fully benchmarked nine-family evaluation baseline.

### 3.2 Architectural Shape

The strongest source-backed architectural statement is the following:

> ThemisDB exposes a unified `IndexManager` interface for secondary, vector, and graph indexes, while additional index-related capabilities are implemented as adjacent managers or utilities rather than as one monolithic universal manager.

This matters because it is more precise than the previous claim that all index families are uniformly accessed through one façade.

### 3.3 Repository-Grounded Component Map

| Component | Canonical evidence | What can be claimed safely |
|---|---|---|
| `IndexManager` | `include/index/index_manager.h` | DI-enabled façade for secondary/vector/graph APIs; tenant-scoped create/get/drop; index statistics export |
| `AdaptiveIndexManager` | `include/index/adaptive_index.h`, `tests/test_adaptive_index.cpp` | query-pattern tracking, selectivity analysis, cache-aware suggestion generation |
| `TieredIndexManager` | `include/index/tiered_index_manager.h`, `tests/index/test_tiered_index_migration.cpp` | HOT/WARM/COLD registration, migration policy, callback-based import/export, failure propagation |
| `SpatialIndexManager` | `tests/geo/test_spatial_index.cpp`, `tests/index/test_spatial_correctness_integration.cpp` | create/insert/update plus intersects/within/contains/nearby query behavior and integration-level correctness coverage |
| `InvertedIndex` | `tests/test_inverted_index.cpp` | tokenization, config persistence, document indexing, ranking basics, phrase/full-text helpers |
| `LearnedIndex` | `tests/index/test_learned_index.cpp` | train/lookup/range correctness across multiple numeric key types |
| `MatryoshkaTruncatedIndex` | `tests/index/test_matryoshka_truncation.cpp`, `tests/test_cross_module_index_matryoshka.cpp` | truncation semantics, wrapper behavior, and cross-module retrieval sanity |

### 3.4 Multi-Tenancy and Isolation

`IndexManager::makeTenantIndexName()` constructs the tenant-prefixed namespace `tenant:<tenant_id>:<index_name>`. The corresponding focused tests validate:

- distinct key generation across tenants,
- rejection of empty tenant identifiers for mutating tenant-scoped APIs,
- not-found behavior for unknown tenant-scoped indexes,
- separator-injection rejection for `:` in tenant or index names.

These tests support a claim of **logical namespace isolation at the index-name level**. They do **not** by themselves establish physical tenant isolation, process isolation, or a full security certification boundary.

---

## 4. Evaluation / Experiments

### 4.1 What the Current Repository Actually Evaluates

The repository already contains meaningful focused tests, but they are not equivalent to a complete paper-grade benchmark campaign. The strongest review-ready position is therefore to treat the current evidence as **implementation validation plus limited local performance thresholds**.

| Evidence file | Evaluation focus | Verified behavior | Boundary of evidence |
|---|---|---|---|
| `tests/db/test_index_performance.cpp` | secondary-index creation, range queries, lookup throughput, moderate-scale inserts | concrete timing thresholds for selected local runs | not a full benchmark suite; does not prove p99 at 100k+ scale |
| `tests/test_multi_tenant_index.cpp` | tenant namespace isolation | prefix correctness and invalid-input rejection | does not prove physical isolation |
| `tests/test_adaptive_index.cpp` | adaptive recommendation mechanics | tracker aggregation, selectivity analysis, thread-safety, cache-aware suggestion logic | does not prove workload-level latency reduction |
| `tests/test_inverted_index.cpp` | full-text indexing behavior | tokenization, config persistence, search ranking basics, phrase helpers | does not prove full BM25 parity against external search engines |
| `tests/geo/test_spatial_index.cpp`, `tests/index/test_spatial_correctness_integration.cpp` | spatial correctness | create/insert/update plus intersects/within/contains/nearby behavior, KNN/Z-range coverage, and integration-level correctness gates | no committed spatial benchmark artifact reviewed here |
| `tests/index/test_tiered_index_migration.cpp` | lifecycle migration | registration, promotion/demotion, callback execution, failure handling | no measured object-store/NVMe timing study |
| `tests/index/test_learned_index.cpp` | learned-index correctness | training, point lookup, range lookup, serialization guards | no committed speedup study versus B-tree in this review |
| `tests/index/test_matryoshka_truncation.cpp`, `tests/test_cross_module_index_matryoshka.cpp` | truncation-based ANN workflow | truncation semantics, wrapper behavior, and multi-stage retrieval sanity | no committed recall/QPS curve for publication use |

### 4.2 Secondary-Index Performance Claims That Survive Review

The most concrete performance evidence in the current repository comes from `tests/db/test_index_performance.cpp`. The test names and comments required special care because some names overstate what the code actually executes.

| Test | Current checked-in workload | Current asserted threshold | Safe interpretation |
|---|---|---|---|
| `Creation_MediumDataset` | 1,500 inserted items | `< 15,000 ms` on non-Windows | moderate local creation threshold |
| `RangeQuery_PerformanceSLA` | 15,000 inserted items | single measured range query `< 200 ms` | targeted local latency check, not p99 |
| `RangeQuery_VariableSelectivity` | 8,000 inserted items | thresholds from `50 ms` to `220 ms` depending on range width | local selectivity-sensitive timing check |
| `Lookup_ThroughputMeasurement` | 8,000 items, 800 lookups | `> 400 ops/sec` | minimum local lookup-throughput threshold |
| `LargeDataset_100kItems` | **20,000** inserted items | query `< 400 ms`, memory delta `< 1024 MB` | moderate-scale validation; the test name does not justify a 100k claim |

Accordingly, the previous claims of:

- sub-100 ms p99 range queries,
- verified 100,000+ document scale,
- fully benchmarked sustained concurrent load

have been removed from this document because the reviewed test artifact does not support them as written.

### 4.3 Current Evidence for Non-Secondary Components

For the remaining index-related components, the repository currently provides stronger correctness coverage than benchmark evidence:

- **Adaptive advisor**: validated for pattern aggregation, selectivity estimation, and cache-aware scoring mechanics, but not yet for measured end-to-end workload improvement.
- **Tiered migration**: validated for policy transitions and callback failure handling, but not for production storage-latency envelopes.
- **Learned index**: validated for training and lookup correctness across integer and floating-point keys, but not for a committed speedup comparison against a conventional index baseline.
- **Matryoshka truncation**: validated for multi-stage retrieval sanity and wrapper correctness, but not for repository-backed recall/QPS trade-off curves.
- **Spatial indexing**: validated for core query semantics, but not for a standalone reproducible experiment section with throughput or dataset-size reporting.
- **Full-text indexing**: validated for tokenization and ranking basics, but not for external baseline comparisons.

### 4.4 Reproducibility Status

The present repository supports source-level reproducibility of interfaces and focused tests, but it does **not** yet provide a clean, publication-ready artifact bundle for all performance claims discussed in the earlier draft. In practical terms:

- code paths and tests are inspectable,
- evaluation intent is partially encoded in focused tests,
- some benchmark and rollout work remains open in `src/index/ROADMAP.md`,
- fresh GPU/ANN benchmark artifacts are not embedded in this paper revision.

The paper is therefore review-ready as a **repository-grounded technical evaluation**, not yet as a final experimental systems paper with closed quantitative claims.

---

## 5. Limitations / Known Issues

### 5.1 Evidence Limitations

1. **No checked-in benchmark tables for GPU ANN claims**: this review found no committed artifact in the cited paper draft that justifies concrete GPU throughput, recall, or VRAM reduction numbers.
2. **Secondary-index scale claims were overstated**: the reviewed performance test labeled `LargeDataset_100kItems` currently inserts 20,000 items, not 100,000.
3. **Cross-model evaluation remains incomplete**: the earlier draft described compound graph/vector/geo experiments, but the current article cannot present them as completed measurements without corresponding benchmark artifacts.

### 5.2 Module-State Limitations

The roadmap still marks several index tasks as active work, including:

- backend parity and deterministic fallback hardening,
- benchmark stabilization for vector search, rebuild, spatial, and quantization paths,
- diagnostics consistency for lifecycle and distributed incidents,
- further GPU backend work,
- broader ANN rollout hardening around `AnnFrontdoor`.

These roadmap items do not negate the implemented functionality, but they do mean the document should avoid blanket wording such as "fully benchmarked" or "complete production proof" for the entire index surface.

### 5.3 Interpretation Limits

- Tenant prefix tests support logical namespace isolation, not a full tenancy security proof.
- Learned-index tests support correctness, not a published asymptotic or empirical speedup result for the current repository revision.
- Inverted-index tests support BM25-style ranking behavior in local scenarios, not competitive benchmarking against specialized search engines.
- Spatial tests support geometric query behavior, not global geospatial performance claims.

---

## 6. Conclusion

ThemisDB currently exposes a broad and technically interesting multi-model index subsystem, but the evidence is uneven across architecture, correctness, and performance. The safest review-ready conclusion is:

1. the repository clearly implements a DI-based `IndexManager` façade plus several adjacent index components,
2. focused tests already validate important behaviors such as tenant-prefixed naming, adaptive recommendation mechanics, learned-index correctness, spatial query semantics, full-text indexing behavior, and tier migration logic,
3. several quantitative claims from the earlier draft were ahead of the committed evidence and have therefore been removed or downgraded to open validation work.

In short, the codebase supports a strong architecture-and-correctness narrative today. A final publication-grade performance narrative still requires dedicated benchmark artifacts aligned with the current roadmap.

---

## References

[1] Lu, J., Holubová, I., and Catania, B. (2019). *Multi-model Databases: A New Journey to Handle the Variety of Data*. ACM Computing Surveys, 52(3). https://doi.org/10.1145/3323214  
[2] ArangoDB Documentation. https://www.arangodb.com/docs/  
[3] OrientDB Documentation. https://orientdb.org/docs/  
[4] Malkov, Y. A., and Yashunin, D. A. (2020). *Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs*. IEEE TPAMI, 42(4), 824-836. https://doi.org/10.1109/TPAMI.2018.2889473  
[5] Jégou, H., Douze, M., and Schmid, C. (2011). *Product Quantization for Nearest Neighbor Search*. IEEE TPAMI, 33(1), 117-128. https://doi.org/10.1109/TPAMI.2010.57  
[6] Kraska, T., Beutel, A., Chi, E. H., Dean, J., and Polyzotis, N. (2018). *The Case for Learned Index Structures*. ACM SIGMOD. https://arxiv.org/abs/1712.01208  
[7] Ferragina, P., and Vinciguerra, G. (2020). *The PGM-index: a fully-dynamic compressed learned index with provable worst-case bounds*. PVLDB. https://doi.org/10.14778/3389133.3389135  
[8] Kusupati, A., Bhatt, G., Rege, A., et al. (2022). *Matryoshka Representation Learning*. NeurIPS. https://arxiv.org/abs/2205.13147  
[9] Guttman, A. (1984). *R-trees: A Dynamic Index Structure for Spatial Searching*. ACM SIGMOD. https://doi.org/10.1145/602259.602266  
[10] RocksDB Documentation. https://rocksdb.org/  
