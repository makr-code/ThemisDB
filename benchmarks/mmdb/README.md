---
Author: "ThemisDB Team"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "review"
AI_Used: true
AI_Models: "GitHub Copilot (exact model backend not recorded)"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# MMDB Benchmarks (`benchmarks/mmdb/`)

This suite measures indexed storage operations over persisted benchmark records.
Fixture data is deterministic; it is not an external benchmark dataset.

## Scope

- `SecondaryIndexManager::scanEntitiesEqual` over RocksDB-backed product,
  document, and edge tables.
- Indexed product reads, category/document joins, one-hop graph lookup, and
  multi-model context assembly.
- These are component-level measurements, not end-to-end ANN, RAG, or external
  database comparison results.

## Installation

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
```

## Usage

The CTest smoke is `bench_mmdb` (registered as module `base`). It seeds one
thousand records and checks query results before recording each operation.
Database creation and fixture writes happen before timed iterations; setup warms
indexed read paths with 100 sequential and 200 deterministic pseudo-random
product/document/edge lookups. The four I/O workloads use wall-clock timing.
Vector ANN and generation workloads are excluded until they can call their
production APIs and validate quality against ground truth.

## Bezug

- Benchmark-Root: [`../README.md`](../README.md)
- Bereichsplanung: [`../ROADMAP.md`](../ROADMAP.md)
- Erweiterungen: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
- Strategischer Layer-Kontext: [`../../FUTURE_PLAN.md`](../../FUTURE_PLAN.md)
