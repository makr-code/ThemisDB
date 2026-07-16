# Temporal Database Support

**Module:** `include/importers/temporal_support.h` / `src/importers/temporal_support.cpp`
**Version:** 2.3.0
**Status:** ✅ Implemented

---

## Overview

The `TemporalDatabaseSupport` module detects and queries **temporal dimensions** in
PostgreSQL schemas, enabling point-in-time queries and bi-temporal data modelling as
specified by **ISO/IEC 9075-2:2011 SQL:2011**.

Many PostgreSQL applications implement ad-hoc temporal patterns using timestamp columns
without formally declaring them as temporal tables.  This module automates detection
and generates the correct SQL predicates for point-in-time retrieval.

---

## Scientific Foundations

| Standard / Paper | Relevance |
|------------------|-----------|
| ISO/IEC 9075-2:2011 SQL:2011, Part 2: Foundation | Temporal table specification |
| Snodgrass, R. T. (1999) *Developing Time-Oriented Database Applications in SQL* | Temporal SQL query patterns |
| Jensen & Snodgrass (1996) *Semantics of Time-Varying Information* | Bi-temporal data model theory |
| PostgreSQL `tstzrange` / `temporal_tables` extension | Implementation reference |

---

## Temporal Models

### Valid Time (Business Time)

Represents **when a fact was true** in the real world, independent of when it was stored.

Common column name patterns:
```
valid_from / valid_start / effective_date / effective_from / start_date / period_start
valid_to   / valid_end   / expiry_date   / expiration_date / end_date  / period_end
```

**SQL:2011 syntax (aspirational):**
```sql
SELECT * FROM contracts FOR VALID_TIME AS OF DATE '2024-06-01';
```

**ThemisDB point-in-time query (generated):**
```sql
SELECT * FROM contracts
WHERE valid_from <= '2024-06-01'
  AND (valid_to IS NULL OR valid_to > '2024-06-01')
```

### Transaction Time (System Time)

Represents **when a fact was stored** in the database, enabling audit trails and
regulatory reconstructions.

Common column name patterns:
```
created_at / inserted_at / sys_period_start / transaction_start / row_created
updated_at / deleted_at  / sys_period_end   / transaction_end   / row_updated
```

**SQL:2011 syntax:**
```sql
SELECT * FROM contracts FOR SYSTEM_TIME AS OF TIMESTAMP '2024-06-01 12:00:00';
```

### Bi-Temporal

A table has **both** valid-time and transaction-time columns.  This is the most powerful
temporal model: it allows answering questions like *"What did we believe on 2024-01-01
about what was true on 2023-07-01?"*

---

## Detection Algorithm

```
for each column name in table:
    if name ∈ VALID_TIME_FROM_HINTS  → mark valid_from_column
    if name ∈ VALID_TIME_TO_HINTS    → mark valid_to_column
    if name ∈ TX_TIME_FROM_HINTS     → mark transaction_from_column
    if name ∈ TX_TIME_TO_HINTS       → mark transaction_to_column

if has_valid AND has_tx  → BI_TEMPORAL
if has_valid             → VALID_TIME
if has_tx                → TRANSACTION_TIME
```

The hint sets are conservative (only well-known patterns), avoiding false positives.

---

## Point-in-Time Query Builder

### `buildPointInTimeQuery`

Generates a SQL `WHERE` clause that reconstructs the state of the table at the given
timestamp, combining valid-time and transaction-time predicates as appropriate.

```cpp
TemporalDatabaseSupport::TemporalQueryBuilder qb;

// Detect temporal dimensions
auto dims = TemporalDatabaseSupport::detectTemporalDimensions({schema});
// dims[0].temporal_model == TemporalModel::BI_TEMPORAL

// Build query
auto sql = qb.buildPointInTimeQuery(dims[0], "2024-06-01T12:00:00Z");
// SELECT * FROM employees
// WHERE valid_from <= '2024-06-01T12:00:00Z'
//   AND (valid_to IS NULL OR valid_to > '2024-06-01T12:00:00Z')
//   AND created_at <= '2024-06-01T12:00:00Z'
//   AND (updated_at IS NULL OR updated_at > '2024-06-01T12:00:00Z')
```

### `buildSystemTimeQuery`

Generates the SQL:2011 `FOR SYSTEM_TIME AS OF` syntax for databases that natively support it:

```sql
SELECT * FROM employees FOR SYSTEM_TIME AS OF TIMESTAMP '2024-06-01T12:00:00Z'
```

---

## Integration with Import Pipeline

During a PostgreSQL import, `TemporalDatabaseSupport` is used to:

1. **Detect temporal dimensions** in the source schema before import begins.
2. **Tag imported records** with import-time metadata (`sys_period_start`).
3. **Filter historical snapshots**: only import records valid at a specific point in time.

```
Source PostgreSQL (bi-temporal)
         │
         ├── detectTemporalDimensions()
         │        └── [valid_from, valid_to, created_at, updated_at]
         │
         ├── buildPointInTimeQuery("2024-01-01")
         │        └── WHERE valid_from <= ? AND (valid_to IS NULL OR valid_to > ?)
         │                 AND created_at <= ? AND (updated_at IS NULL OR ...)
         │
         └── Import filtered rows → ThemisDB collection
```

---

## ThemisDB Schema Mapping

| PostgreSQL Temporal Column | ThemisDB Field | Type |
|---------------------------|----------------|------|
| `valid_from` | `_valid_from` | `datetime` |
| `valid_to` | `_valid_to` | `datetime` (nullable) |
| `created_at` | `_sys_from` | `datetime` |
| `updated_at` | `_sys_to` | `datetime` (nullable) |

---

## Limitations

- Auto-detection is name-based; columns with non-standard names (e.g., `t_begin`) will
  not be detected.  Use `TemporalSchema` to specify columns manually.
- `buildSystemTimeQuery` produces SQL:2011 syntax that PostgreSQL 15 does not natively
  support; use `buildPointInTimeQuery` for compatibility.
- Open-ended intervals use `NULL` as the sentinel for "now / forever"; ensure the target
  storage layer handles `NULL` comparisons correctly.

---

## References

- ISO/IEC 9075-2:2011. *Information technology – Database languages – SQL – Part 2: Foundation*. International Organization for Standardization.
- Snodgrass, R. T. (1999). *Developing Time-Oriented Database Applications in SQL*. Morgan Kaufmann.
- Jensen, C. S. & Snodgrass, R. T. (1996). Semantics of Time-Varying Information. *Information Systems*, 21(4), 311–352.
