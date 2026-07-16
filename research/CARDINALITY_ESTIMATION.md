# Cardinality Estimation in ThemisDB

**Module:** `include/importers/schema_inference.h` / `src/importers/schema_inference.cpp`
**Version:** 2.1.0
**Status:** ✅ Implemented
**Last Updated:** 2026-05-14
**Authors:** ThemisDB Research Team

---

## Abstract

Cardinality estimation—determining how many rows satisfy a given predicate or foreign-key relationship—is a foundational step for query optimisation, import planning, and data-quality assessment in database systems. ThemisDB implements cardinality estimation as the third of three algorithms in `SchemaInferenceEngine` (the other two being implicit-relationship discovery and semantic-type detection), exposing three metrics per foreign-key relationship: average fan-out (`one_to_many_ratio`), referential integrity fraction (`selectivity`), and a 95 % confidence interval computed via a simplified Wilson score approximation.

This document describes the algorithms, their mathematical foundations, the integration with `AdaptiveImportOptimizer`, measurement evidence from the test suite, and the known limitations of the current approach. All claims are tied to concrete source artifacts.

---

## Introduction

Query optimisers in relational and multi-model database systems rely on statistics to estimate result cardinalities and choose efficient execution plans. Errors in those estimates propagate directly into sub-optimal join orders, excessive memory allocation, and degraded throughput [3].

ThemisDB is a multi-model database with an AQL-based query layer and distributed operation modes (see [README.md](../README.md) and [ARCHITECTURE.md](../ARCHITECTURE.md)). As part of its PostgreSQL importer pipeline (version 2.1+), ThemisDB collects column-level statistics and feeds them into:

1. **`SchemaInferenceEngine::estimateCardinalities`** – produces `CardinalityEstimate` structs per discovered FK relationship.
2. **`AdaptiveImportOptimizer::optimizeImportPlan`** – uses `ColumnStatistics.avg_length` (and a tunable `batch_multiplier_`) to set initial batch sizes, with runtime adjustment via `adaptBatchSize()`.
3. **`QueryOptimizer` / `OptimizerCostModel`** – uses `StatisticsCollector` cardinality fallback when the secondary index reports zero matches (see `src/query/query_optimizer.cpp`).

The remainder of this document details the mathematical model, the implementation, experiment evidence, and limitations.

---

## Methodology

### M1 – Evidence model

All claims in this document are grounded in at least one of the following artifact classes:

- Public API headers: `include/importers/schema_inference.h`, `include/importers/adaptive_import.h`
- Implementation files: `src/importers/schema_inference.cpp`, `src/importers/adaptive_import.cpp`
- Test coverage: `tests/test_postgres_importer_v2.cpp` (tests `CardinalityEstimateFromStats`, `CardinalityEstimateNoStats`)
- External literature cited with DOI or URL

### M2 – Terminology

| Term | Definition in ThemisDB |
|------|------------------------|
| `one_to_many_ratio` | `total_rows / distinct_count(FK_column)` – average number of child rows per parent |
| `selectivity` | `(total_rows − null_count) / total_rows` – fraction of child rows with a non-null FK reference |
| Confidence interval | 95 % interval computed via a simplified Wilson score approximation on `selectivity` |
| `batch_multiplier_` | Runtime-adjusted scalar (initial 1.0) applied to the base batch size by `adaptBatchSize()` |

### M3 – Claim validation policy

Statements qualified as "the implementation" are only made when the described behaviour can be traced to a named source file and function. Hypothetical or aspirational behaviour is labelled explicitly.

---

## Metrics Computed

### 1 – One-to-Many Ratio

**Formula** (`src/importers/schema_inference.cpp`, function `estimateCardinalities`):

```
one_to_many_ratio = total_rows / distinct_count(FK_column)
```

**Interpretation:**

- `≈ 1.0` → likely 1:1 relationship (or unique FK column)
- `> 1.0` → 1:N relationship; value is the average number of child rows per parent
- `distinct_count == 0` → defaults to `1.0` (no division by zero)

This ratio corresponds to the arithmetic mean children-per-parent (i.e., mean group size). The source code labels this a "Harmonic Mean estimator" (`src/importers/schema_inference.cpp`, comment on line 261), following the framing in the referenced literature [1]; however, the formula `total_rows / distinct_count` is mathematically an arithmetic mean. The label is preserved here for traceability to the implementation comment.

### 2 – Selectivity

**Formula** (`src/importers/schema_inference.cpp`, function `estimateCardinalities`):

```
selectivity = (total_rows - null_count) / total_rows
```

**Interpretation:**

- `≈ 1.0` → nearly all rows have a valid FK reference (tight referential integrity)
- `< 0.9` → high NULL rate; potential data-quality issue
- `total_rows == 0` → defaults to `1.0`

The `selectivity` value is also reused by `QueryOptimizer` as a cardinality fallback: when the secondary index reports zero matches and column statistics are available, the optimizer estimates the result count as `selectivity × row_count` (`src/query/query_optimizer.cpp`, function `chooseOrderForAndQuery`).

### 3 – 95 % Confidence Interval (Simplified Wilson Score)

The standard Wald interval `p̂ ± z·√(p̂(1−p̂)/n)` breaks down for extreme values of `p̂` (near 0 or 1) and small `n`. The full Wilson score interval is:

```
centre = (p̂ + z²/2n) / (1 + z²/n)
margin = (z / (1 + z²/n)) · √(p̂(1−p̂)/n + z²/(4n²))
CI     = [centre − margin, centre + margin],   z = 1.96 for 95 %
```

ThemisDB uses the simplified Wald-form approximation for performance while accepting the known breakdown for extreme `p̂` and small `n`:

```cpp
// src/importers/schema_inference.cpp
double margin = n > 0 ? z * std::sqrt(p * (1.0 - p) / n) : 0.1;
confidence_interval = {
    std::max(0.0, p - margin),
    std::min(1.0, p + margin)
};
```

The two-element vector `[lower, upper]` is clamped to `[0, 1]`.

---

## Integration with `ColumnStatistics`

The `ColumnStatistics` struct (defined in `include/importers/schema_inference.h`) carries the raw counts consumed by all three estimators:

```cpp
struct ColumnStatistics {
    std::string column_name;
    std::string table_name;
    size_t total_rows{0};
    size_t null_count{0};
    size_t distinct_count{0};
    double avg_length{0.0};   // average serialised byte length
    double min_value{0.0};
    double max_value{0.0};
};
```

Statistics are populated externally (e.g., by sampling the source system) and passed to `estimateCardinalities` and `optimizeImportPlan` as a `std::map<std::string, ColumnStatistics>` keyed by `"table.column"`. For PostgreSQL sources, a representative sampling query is:

```sql
SELECT
    n_distinct,
    null_frac,
    avg_width
FROM pg_stats
WHERE tablename = $1 AND attname = $2;
```

---

## Example Output

Given a relationship `orders.user_id → users.id` with 1 000 total rows, 100 distinct user IDs, and 10 NULL entries (as exercised in `tests/test_postgres_importer_v2.cpp::CardinalityEstimateFromStats`):

```cpp
CardinalityEstimate est;
est.relationship_id     = "orders.user_id -> users.id";
est.one_to_many_ratio   = 10.0;    // 1000 / 100
est.selectivity         = 0.99;    // (1000 - 10) / 1000
est.confidence_interval = {0.9845, 0.9955};  // ≈ z*sqrt(p*(1-p)/n) margin
```

The unit test asserts `one_to_many_ratio > 0`, `selectivity ∈ [0, 1]`, and `confidence_interval.size() == 2`.

---

## Adaptive Batch Sizing

`AdaptiveImportOptimizer::optimizeImportPlan` (`src/importers/adaptive_import.cpp`) sets initial per-table batch sizes based on the **average serialised row width** of the primary-key column(s), not directly on the fan-out ratio:

```cpp
// Base batch: inversely proportional to avg_length, clamped to [100, 10 000]
batch = max(100, min(10000, 1000.0 / (avg_length / 64.0)));
// Applied multiplier (runtime-tuned):
plan.batch_sizes[table] = batch * batch_multiplier_;
```

The `batch_multiplier_` is adjusted at runtime by `adaptBatchSize()`:

- **Back-off** (multiply by 0.75): when `memory_utilization > 80 %` or `cpu_utilization > 90 %`
- **Scale-up** (multiply by 1.25): when both `memory_utilization < 50 %` and `cpu_utilization < 50 %`
- Scalar is clamped to `[0.1, 4.0]`

Tables with no FK dependencies are flagged as `parallel_candidates`, allowing parallel import without FK-ordering constraints.

---

## Evaluation

### E1 – Unit test coverage (`tests/test_postgres_importer_v2.cpp`)

| Test | Input | Expected | Observed |
|------|-------|----------|----------|
| `CardinalityEstimateFromStats` | 1 000 rows, 100 distinct, 10 nulls | `ratio > 0`, `selectivity ∈ [0,1]`, CI size = 2 | Pass |
| `CardinalityEstimateNoStats` | No statistics supplied | Non-empty estimate list (defaults to `ratio=1.0`) | Pass |

These tests run as part of the `test_postgres_importer_v2` target in the CMake build system.

### E2 – Query optimiser integration (`tests/test_query_optimizer_statistics.cpp`)

The `StatisticsImproveCardinality` test inserts 500 rows into an `inventory` table (10 % `SKU001`, 90 % `OTHER`), runs `StatisticsCollector::collectStats`, and confirms that a `QueryOptimizer` instance wired to real statistics produces a more selective predicate order than one operating without statistics. This validates the end-to-end path from `ColumnStatistics.selectivity` through `QueryOptimizer::chooseOrderForAndQuery`.

### E3 – Known accuracy bounds

The simplified Wilson approximation is theoretically accurate when `n·p̂ ≥ 10` and `n·(1−p̂) ≥ 10` (see Limitations). For `n ≤ 30` or extreme `p̂` the CI may under-cover. No micro-benchmark comparing estimation error against ground-truth cardinalities has been published for this module yet; adding such a benchmark is tracked in the import-module roadmap.

---

## Limitations

- **Stale statistics**: Statistics must be collected before import. If the source data changes after collection, estimates degrade silently. Callers should re-collect statistics when significant data mutations occur.
- **Uniform distribution assumption**: The `one_to_many_ratio` estimator assumes FK values are approximately uniformly distributed across parent rows. Highly skewed distributions (e.g., Zipf) produce inflated average fan-out estimates, which may cause under-batching for the majority of parent rows.
- **Wilson CI for small tables**: The simplified Wald-form approximation used (`z·√(p̂(1−p̂)/n)`) is unreliable when both `n` is small and `p̂` is extreme (near 0 or 1); validity depends on the joint condition `n·p̂ ≥ 10` and `n·(1−p̂) ≥ 10`. For very small tables or near-boundary selectivities the exact Clopper-Pearson interval or the full Wilson formula should be applied.
- **Batch sizing decoupled from fan-out**: The current `AdaptiveImportOptimizer` derives initial batch sizes from `avg_length`, not directly from `one_to_many_ratio`. Integrating fan-out into the batch-size heuristic is a planned enhancement (see `include/importers/adaptive_import.h` documentation).
- **No cross-table join cardinality**: The estimator operates per FK column independently. Multi-join selectivity is not modelled; the `QueryOptimizer` and `OptimizerCostModel` handle join selectivity separately via `estimateJoinSelectivity` in `src/query/optimizer_cost_model.cpp`.

---

## References

1. Flajolet, P., Fusy, É., Gandouet, O., & Meunier, F. (2007). *HyperLogLog: the analysis of a near-optimal cardinality estimation algorithm*. AOFA 2007, pp. 137–156. <https://algo.inria.fr/flajolet/Publications/FlFuGaMe07.pdf> *(Representative reference for distinct-count estimation; the ThemisDB source comment attributes the "Harmonic Mean estimator" label to "Li et al., 2016" without a full citation.)*
2. Ioannidis, Y. E. & Kang, Y. (1991). *Randomized algorithms for optimizing large join queries*. ACM SIGMOD Record, 20(2), 312–321. <https://doi.org/10.1145/115790.115853>
3. Leis, V., Gubichev, A., Mirchev, A., Boncz, P., Kemper, A., & Neumann, T. (2015). *How good are query optimizers, really?* VLDB, 9(3), 204–215. <https://doi.org/10.14778/2850583.2850594>
4. Wilson, E. B. (1927). *Probable inference, the law of succession, and statistical inference*. Journal of the American Statistical Association, 22(158), 209–212. <https://doi.org/10.2307/2276774>
5. Pavlo, A., Angulo, G., Arulraj, J., Lin, H., Lin, J., Ma, L., … Zhang, T. (2017). *Self-driving database management systems*. CIDR 2017. <https://www.cidrdb.org/cidr2017/papers/p42-pavlo-cidr17.pdf>
6. Selinger, P. G., Astrahan, M. M., Chamberlin, D. D., Lorie, R. A., & Price, T. G. (1979). *Access path selection in a relational database management system*. ACM SIGMOD, pp. 23–34. <https://doi.org/10.1145/582095.582099>
