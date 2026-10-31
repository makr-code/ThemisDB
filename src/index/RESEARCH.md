---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_Sources:
  - "src/index/README.md"
  - "src/index/ARCHITECTURE.md"
  - "src/index/ROADMAP.md"
  - "include/index/ann_frontdoor.h"
  - "include/index/ann_index.h"
  - "tests/index/test_ann_frontdoor.cpp"
  - "tests/index/test_ann_cpu_parity.cpp"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index research notes

## Purpose and reader

This note records the measurable questions and evidence path for engineers
evaluating retrieval methods in `src/index/`. It is for index maintainers,
benchmark authors, and reviewers deciding whether a backend or routing change
is supported by evidence. It is not a claim that every backend has passed
production or hardware validation.

## Research scope

The module indexes caller-supplied vectors and other persisted data. It does
not own text embedding generation. Research in scope is limited to vector
distance/search, ANN candidate retrieval, routing, distributed candidate
merging, and index maintenance. Query planning, embedding model quality, and
hardware driver correctness belong to their owning modules or deployment
environment.

`AnnFrontdoor` exposes HNSW, ScaNN, DiskANN, distributed, and flat strategy
labels. `planRetrieval()` considers scope/backend availability, dataset size,
hot/cold tier, and shard-awareness. The actual configured thresholds are in
[`ann_frontdoor.h`](../../include/index/ann_frontdoor.h); do not treat a
strategy label as proof that a matching implementation is installed. For
example, `AnnFrontdoor::bruteForceSearch()` currently returns an empty result
set rather than implementing a storage scan
([`ann_frontdoor.cpp`](ann_frontdoor.cpp)).

## Questions and method

| Question | Comparison method | Required evidence |
|---|---|---|
| Does ANN preserve useful neighbors? | Compare returned IDs/ranks to an exact flat baseline for the same vectors, metric, query, and `k`. | Recall@k or rank overlap, plus candidate count and distance ordering. |
| Does a routing change honor its contract? | Exercise scope-specific/global backends, hot/cold tiers, and shard-aware fan-out with controlled backends. | Chosen strategy, route reason, fallback fields, shard outcome counts, and deterministic expected candidates. |
| Is a distance/backend change numerically compatible? | Compare identical inputs and metric with a stated tolerance; include ties and invalid values. | Per-query distances, ordering, error cases, and CPU/GPU parity results where hardware exists. |
| Is an optimization operationally acceptable? | Run mapped benchmarks against the same release baseline and representative dataset/workload. | Benchmark manifest, configuration, hardware/backend, latency distribution, throughput, memory, and recall. |

Use the deterministic fixtures in
[`test_ann_cpu_parity.cpp`](../../tests/index/test_ann_cpu_parity.cpp) for
cardinality, ascending distance order, non-negative output, and CPU/flat
fixture comparisons. Routing and failure fixtures are in
[`test_ann_frontdoor.cpp`](../../tests/index/test_ann_frontdoor.cpp). These
tests establish only the behaviors they exercise; stub backends do not measure
real backend recall or throughput.

## Assumptions and limitations

- Distances are interpreted as smaller-is-nearer by the ANN result contract.
  Metric-specific definitions and supported metrics remain backend-specific.
- The frontdoor's `estimated_recall` values are source constants described as
  informational estimates, not service-level guarantees or current benchmark
  results. Do not use them to approve a recall target.
- Recall, latency, and resource use depend on index parameters, corpus,
  filters, backend build flags, and hardware. Report those inputs with results.
- Distributed shard-cost metadata currently uses fixed values in the routing
  implementation; measured shard cost/relevance/freshness injection is not
  demonstrated by that implementation.
- Hardware-only GPU parity remains pending for representative devices. CPU
  tests and compile-time backend availability are not evidence of GPU parity.

## Validation and next action

For code changes, first add or update a deterministic test that isolates the
method under study; then run the focused index test and the corresponding
benchmark on representative hardware. The test target names are registered by
[`tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt). Record the
backend, dataset, metric, parameters, and baseline rather than reporting
unqualified claims such as “faster” or “better.”

Canonical context: [module overview](README.md),
[architecture](ARCHITECTURE.md), and
[performance expectations](PERFORMANCE_EXPECTATIONS.md). The roadmap's GPU
hardware gate remains pending in
[`ROADMAP.md`](ROADMAP.md).
