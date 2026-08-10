# Temporal Database Support

**Scope:** Review of the importer-side helper `themis::importers::TemporalDatabaseSupport` in `/home/runner/work/ThemisDB/ThemisDB/include/importers/temporal_support.h` and `/home/runner/work/ThemisDB/ThemisDB/src/importers/temporal_support.cpp`.

**Review basis (repository artifacts):**
- `/home/runner/work/ThemisDB/ThemisDB/include/importers/temporal_support.h`
- `/home/runner/work/ThemisDB/ThemisDB/src/importers/temporal_support.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/include/importers/schema_inference.h`
- `/home/runner/work/ThemisDB/ThemisDB/tests/test_postgres_importer_v2.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/docs/en/importers/PRIMARY_SOURCES.md`
- `/home/runner/work/ThemisDB/ThemisDB/src/importers/README.md`
- `/home/runner/work/ThemisDB/ThemisDB/src/temporal/README.md`
- `/home/runner/work/ThemisDB/ThemisDB/src/temporal/ROADMAP.md`
- `/home/runner/work/ThemisDB/ThemisDB/src/temporal/PERFORMANCE_EXPECTATIONS.md`

**Language:** English

---

## Abstract / Zusammenfassung

This article reviews the current ThemisDB implementation of `TemporalDatabaseSupport`, a small importer-side helper for PostgreSQL-oriented temporal schema inspection and point-in-time SQL generation. The verified implementation surface is intentionally narrow: it classifies source tables as valid-time, transaction-time, or bi-temporal by matching column names against fixed hint sets, and it emits either a reconstructed `WHERE` predicate (`buildPointInTimeQuery`) or a formatted SQL:2011-style `FOR SYSTEM_TIME AS OF` string (`buildSystemTimeQuery`).

The review removes broader claims that were not backed by source artifacts, especially statements implying end-to-end PostgreSQL importer integration, automatic metadata tagging, internal field remapping, or full SQL:2011 temporal compliance. The result is a source-backed document that distinguishes this helper from the broader temporal runtime in `/home/runner/work/ThemisDB/ThemisDB/src/temporal/`, which is the canonical location for ThemisDB's production temporal subsystem.

---

## Introduction / Einleitung

Temporal support in ThemisDB exists on two different layers and they should not be conflated:

1. **Importer helper layer**: `TemporalDatabaseSupport` in the importers module inspects source-table schemas and generates SQL fragments for source-side temporal filtering.
2. **Core temporal runtime**: the dedicated temporal module under `/home/runner/work/ThemisDB/ThemisDB/src/temporal/` implements broader system-time, valid-time, snapshot, retention, CDC, and indexing behavior for ThemisDB itself.

The document under review concerns only the first layer. In repository terms, `TemporalDatabaseSupport` is not an AQL execution component and not the canonical implementation of ThemisDB's multi-model temporal runtime. Its role is narrower: it helps an importer reason about conventional temporal column names in an external relational schema.

This distinction matters for terminology and evidence quality. Claims about **AQL**, **multi-model execution**, **MVCC-backed temporal querying**, or **benchmark-backed temporal hot paths** belong to the temporal module and must be sourced from `/home/runner/work/ThemisDB/ThemisDB/src/temporal/` rather than attributed to this importer helper.

---

## Methodology / Ansatz

The review applied a source-of-truth-first methodology consistent with repository documentation governance:

- **Primary implementation truth**: public header and implementation file for `TemporalDatabaseSupport`.
- **Input contract truth**: `InferenceTableSchema` from `/home/runner/work/ThemisDB/ThemisDB/include/importers/schema_inference.h`.
- **Behavior evidence**: focused unit tests in `/home/runner/work/ThemisDB/ThemisDB/tests/test_postgres_importer_v2.cpp`.
- **Authority selection**: importer-side authority documents were chosen according to `/home/runner/work/ThemisDB/ThemisDB/docs/en/importers/PRIMARY_SOURCES.md`.
- **Context separation**: importer module documentation versus the dedicated temporal module documentation.

### Verified implementation facts

| Claim | Status | Evidence |
|---|---|---|
| Temporal detection is name-based. | Verified | `detectTemporalDimensions(...)` iterates over `InferenceTableSchema::columns` and matches exact strings against fixed hint sets in `/home/runner/work/ThemisDB/ThemisDB/src/importers/temporal_support.cpp`. |
| Supported classifications are `VALID_TIME`, `TRANSACTION_TIME`, and `BI_TEMPORAL`. | Verified | Enum definition in `/home/runner/work/ThemisDB/ThemisDB/include/importers/temporal_support.h`; classification logic in `/home/runner/work/ThemisDB/ThemisDB/src/importers/temporal_support.cpp`. |
| Point-in-time reconstruction is implemented as generated SQL text, not as a native planner/executor path. | Verified | `TemporalQueryBuilder::buildPointInTimeQuery(...)` returns a `std::string` assembled with `std::ostringstream` in `/home/runner/work/ThemisDB/ThemisDB/src/importers/temporal_support.cpp`. |
| `buildSystemTimeQuery(...)` formats SQL:2011-style syntax but does not itself prove backend support. | Verified | String-formatting implementation in `/home/runner/work/ThemisDB/ThemisDB/src/importers/temporal_support.cpp`. |
| Tests cover detection and basic SQL-string generation. | Verified | `DetectValidTime`, `DetectTransactionTime`, `DetectBiTemporal`, `NoBiTemporalColumns`, `PointInTimeQueryContainsTimestamp`, and `SystemTimeQueryContainsForSystemTime` in `/home/runner/work/ThemisDB/ThemisDB/tests/test_postgres_importer_v2.cpp`. |
| Automatic tagging of imported records with `sys_period_start`. | Not evidenced in current repository review | No supporting call site or implementation was found in the reviewed importer helper artifacts. |
| Fixed ThemisDB field remapping to `_valid_from`, `_valid_to`, `_sys_from`, `_sys_to`. | Not evidenced in current repository review | No such mapping contract appears in the reviewed helper header, implementation, or cited importer tests. |
| Full SQL:2011 temporal-table compliance. | Not evidenced in current repository review | The helper emits SQL fragments and a formatted `FOR SYSTEM_TIME` string; no parser, optimizer, or executor integration is shown here. |

### Terminology normalization

To keep the document consistent with the codebase, the following vocabulary is used throughout:

- **valid-time**: business/effective time carried by source columns such as `valid_from` / `valid_to`
- **transaction-time**: system-storage time carried by source columns such as `created_at` / `updated_at`
- **bi-temporal**: simultaneous presence of valid-time and transaction-time columns in the detected source schema
- **ThemisDB temporal module**: the broader runtime in `/home/runner/work/ThemisDB/ThemisDB/src/temporal/`
- **AQL**: not part of this helper's implementation surface; avoid attributing AQL execution behavior to `TemporalDatabaseSupport`

---

## Implementation Summary

### Detection model

`TemporalDatabaseSupport` relies on exact column-name hints. The implementation recognizes the following built-in patterns:

- **valid-time start**: `valid_from`, `valid_start`, `effective_date`, `effective_from`, `start_date`, `begin_date`, `period_start`
- **valid-time end**: `valid_to`, `valid_end`, `expiry_date`, `expiration_date`, `end_date`, `finish_date`, `period_end`
- **transaction-time start**: `created_at`, `inserted_at`, `sys_period_start`, `transaction_start`, `row_created`
- **transaction-time end**: `updated_at`, `deleted_at`, `sys_period_end`, `transaction_end`, `row_updated`

The helper does **not** inspect SQL types, constraints, generated columns, triggers, or database catalog metadata. Detection is therefore conservative and schema-name driven.

### Query generation model

`buildPointInTimeQuery(...)` emits a full `SELECT * FROM <table>` statement and appends predicates for each detected temporal dimension. The generated interval semantics are source-backed and important:

- start columns use `<= <timestamp>`
- end columns use `IS NULL OR > <timestamp>`

This yields an effectively half-open interpretation of temporal intervals: `[start, end)` for both valid-time and transaction-time when end columns are present.

Example (source-backed behavior):

```cpp
using themis::importers::TemporalDatabaseSupport;

TemporalDatabaseSupport support;
auto dims = support.detectTemporalDimensions({schema});

TemporalDatabaseSupport::TemporalQueryBuilder qb;
auto sql = qb.buildPointInTimeQuery(dims[0], "2024-06-01T12:00:00Z");
```

Possible generated output:

```sql
SELECT * FROM employees
WHERE
  valid_from <= '2024-06-01T12:00:00Z'
  AND (valid_to IS NULL OR valid_to > '2024-06-01T12:00:00Z')
  AND created_at <= '2024-06-01T12:00:00Z'
```

`buildSystemTimeQuery(...)` generates:

```sql
SELECT * FROM employees
FOR SYSTEM_TIME AS OF TIMESTAMP '2024-06-01T12:00:00Z'
```

This output should be described as **formatted SQL text** rather than as proven end-to-end engine support.

---

## Evaluation / Experiments

### E1. Repository-backed behavior check

The strongest evidence for the helper today is the unit-test surface in `/home/runner/work/ThemisDB/ThemisDB/tests/test_postgres_importer_v2.cpp`:

| Capability | Evidence | Assessment |
|---|---|---|
| Detect valid-time schemas | `TEST(TemporalDatabaseSupport, DetectValidTime)` | Covered |
| Detect transaction-time schemas | `TEST(TemporalDatabaseSupport, DetectTransactionTime)` | Covered |
| Detect bi-temporal schemas | `TEST(TemporalDatabaseSupport, DetectBiTemporal)` | Covered |
| Reject non-temporal schemas | `TEST(TemporalDatabaseSupport, NoBiTemporalColumns)` | Covered |
| Emit point-in-time SQL containing table name and timestamp | `TEST(TemporalDatabaseSupport, PointInTimeQueryContainsTimestamp)` | Covered, but only as a smoke test |
| Emit `FOR SYSTEM_TIME AS OF` string | `TEST(TemporalDatabaseSupport, SystemTimeQueryContainsForSystemTime)` | Covered, but only as a formatting test |

### E2. What is *not* currently demonstrated

The current repository review did **not** uncover evidence for the following stronger claims, so they should not appear as established results in this article:

- verified invocation from the active PostgreSQL importer execution path
- benchmark measurements dedicated to `TemporalDatabaseSupport`
- parser- or executor-level support for `FOR SYSTEM_TIME AS OF`
- automatic remapping from source temporal columns into fixed ThemisDB internal field names
- automatic import-time tagging of imported records with temporal metadata

This is the central outcome of the review: the helper is real and tested, but the evidence supports a **targeted schema-detection and SQL-string-generation component**, not a complete temporal import pipeline.

### E3. Relation to the broader temporal subsystem

The broader temporal module is substantially richer and separately documented in `/home/runner/work/ThemisDB/ThemisDB/src/temporal/README.md` and `/home/runner/work/ThemisDB/ThemisDB/src/temporal/ROADMAP.md`. That module owns claims about:

- system-time and valid-time runtime behavior inside ThemisDB
- snapshots, retention, CDC, compression, and tiering
- release-gated performance expectations in `/home/runner/work/ThemisDB/ThemisDB/src/temporal/PERFORMANCE_EXPECTATIONS.md`

Those claims are valid in their own scope, but they should not be used as direct evidence that `TemporalDatabaseSupport` itself provides the same capabilities.

---

## Limitations / Known Issues

- **Name-based detection only:** non-standard column names are ignored unless the caller fills `TemporalSchema` manually.
- **No type or catalog validation:** the helper does not verify timestamp types, constraints, or trigger-driven semantics.
- **No proven importer call-path evidence in this review:** the reviewed artifacts show the helper API and its tests, but not a confirmed production importer invocation path.
- **String-based SQL generation:** `buildPointInTimeQuery(...)` and `buildSystemTimeQuery(...)` interpolate table names and timestamps directly into SQL text. Callers should therefore treat identifiers and timestamps as trusted/validated inputs or parameterize downstream execution.
- **No benchmark evidence specific to this helper:** the repository's temporal benchmark documentation covers the dedicated temporal runtime, not this importer-side helper.
- **No claim of full SQL:2011 compliance:** the helper aligns with temporal vocabulary from the standard but does not by itself demonstrate parser, optimizer, storage, or execution completeness.

---

## Conclusion

After factual review, `TemporalDatabaseSupport` should be described as a **narrow but useful importer-side temporal detection helper**. The current codebase supports three defensible claims:

1. it detects conventional valid-time, transaction-time, and bi-temporal column sets from `InferenceTableSchema::columns`;
2. it can generate source-side point-in-time SQL predicates using half-open interval semantics; and
3. it can format a SQL:2011-style `FOR SYSTEM_TIME AS OF` statement as plain SQL text.

Broader claims about AQL integration, production importer orchestration, field remapping, or full temporal-table compliance require additional code evidence and should remain outside this document until such artifacts exist.

---

## References

1. K. Kulkarni and J.-E. Michels, "Temporal Features in SQL:2011," *ACM SIGMOD Record*, vol. 41, no. 3, 2012. DOI: https://doi.org/10.1145/2380776.2380786
2. C. S. Jensen and R. T. Snodgrass, "Semantics of Time-Varying Information," *Information Systems*, vol. 21, no. 4, pp. 311-352, 1996. DOI: https://doi.org/10.1016/0306-4379(96)00014-3
3. R. T. Snodgrass, *Developing Time-Oriented Database Applications in SQL*. Morgan Kaufmann, 1999. URL: https://www.elsevier.com/books/developing-time-oriented-database-applications-in-sql/snodgrass/978-1-55860-436-6
4. PostgreSQL Global Development Group, "Range Types," PostgreSQL Documentation. URL: https://www.postgresql.org/docs/current/rangetypes.html
5. M. Fowler, "Bitemporal History," 2014. URL: https://martinfowler.com/articles/bitemporal-history.html
6. `/home/runner/work/ThemisDB/ThemisDB/include/importers/temporal_support.h` (canonical API contract reviewed 2026-08-10)
7. `/home/runner/work/ThemisDB/ThemisDB/src/importers/temporal_support.cpp` (canonical implementation reviewed 2026-08-10)
8. `/home/runner/work/ThemisDB/ThemisDB/tests/test_postgres_importer_v2.cpp` (behavior evidence reviewed 2026-08-10)
