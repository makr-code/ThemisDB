# Temporal Features in SQL:2011

**Metadaten:**
- Author(en): Krishna Kulkarni, Jan-Eike Michels
- Konferenz/Journal: ACM SIGMOD Record, Vol. 41, No. 3
- Jahr: 2012
- Link: [ACM DL](https://dl.acm.org/doi/10.1145/2380776.2380786)
- Zitierweise: `kulkarni2012temporal`
- Tags: `temporal`, `sql-standard`, `bi-temporal`, `valid-time`, `transaction-time`, `timeline`
- ThemisDB-Versionen: v1.x+ (`src/temporal/`)
- Status: [x] Partially Implemented · [ ] Full SQL:2011 compliance planned

## 📋 Executive Summary

This paper describes the temporal extensions standardized in SQL:2011 — the first international standard to include native temporal database features. It covers period types, temporal predicates (OVERLAPS, CONTAINS, PRECEDES, etc.), system-versioned tables (`FOR SYSTEM_TIME AS OF`), application-time tables, and bi-temporal tables. ThemisDB's temporal module is designed around these semantics as the compliance target.

## 🎯 Key Findings

- **Period data type**: A pair of timestamps `[start, end)` stored as a first-class column constraint — `PERIOD FOR period_name(col_start, col_end)`.
- **SYSTEM_TIME**: Automatically maintained by the database engine; tracks when rows were current in the database (transaction time).
- **APPLICATION_TIME**: User-managed; tracks when facts were true in the real world (valid time).
- **Bi-temporal tables**: Combine both dimensions — independently queryable via `FOR SYSTEM_TIME AS OF` and `FOR VALID_TIME AS OF` clauses.
- **Temporal predicates**: `OVERLAPS`, `CONTAINS`, `PRECEDES`, `SUCCEEDS`, `IMMEDIATELY PRECEDES/SUCCEEDS`, `EQUALS` — all operate on periods.
- **Temporal DML**: `UPDATE ... FOR PORTION OF period` and `DELETE ... FOR PORTION OF period` — split existing rows to maintain temporal integrity.
- **Sequenced uniqueness**: Unique constraints qualified by time period — no two rows with the same business key can have overlapping periods.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Temporal module → `src/temporal/`
- [x] Query engine → `src/query/` (temporal predicates)
- [x] Storage → `src/storage/` (system-versioned row storage)
- [ ] SQL parser → temporal `FOR SYSTEM_TIME AS OF` syntax *(partial)*

### What Was Adopted?

1. **SYSTEM_TIME versioning**: ThemisDB's MVCC layer stores row version intervals; `FOR SYSTEM_TIME AS OF <timestamp>` queries read from the appropriate snapshot.
2. **Period storage**: Temporal columns stored as paired `(valid_from, valid_to)` timestamps with inclusive/exclusive semantics matching SQL:2011.
3. **Temporal predicates**: `OVERLAPS`, `CONTAINS`, `PRECEDES` predicates implemented as index-accelerated range comparisons.

### How Was It Adapted?

| SQL:2011 Feature | ThemisDB Adaptation | Rationale |
|---|---|---|
| Pure SQL syntax extensions | AQL temporal extensions + SQL compat | ThemisDB has its own AQL query language alongside SQL |
| `PERIOD FOR` column constraint | `temporal_period` field metadata flag | Stored in schema metadata layer |
| Sequenced unique constraint | Temporal index with overlap check | Implemented via interval tree index |
| `UPDATE FOR PORTION OF period` | Temporal split-update operation | Splits affected row into up-to-3 segments (before, overlap, after) |

### Performance Impact

| Metric | SQL:2011 Target | ThemisDB Result | Status |
|--------|----------------|-----------------|--------|
| Point-in-time lookup | O(log n) | O(log n) via MVCC snapshot | ✅ Achieved |
| Temporal range scan | O(k log n) | O(k log n) via interval tree | ✅ Achieved |
| Temporal join | O(n log n) | O(n log n) merge-sort join | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- `FOR VALID_TIME AS OF` (application time) query syntax not yet fully parsed.
  - ThemisDB solution: Priority roadmap item; parser extension needed.
- Sequenced unique constraint enforcement across partitions is complex in distributed mode.
  - ThemisDB solution: Single-partition temporal uniqueness first; cross-shard deferred.
- SQL:2011 does not cover temporal aggregation (e.g., `SUM(...) OVER valid-time period`).
  - ThemisDB solution: Extended temporal aggregation following Böhlen et al. research.

## 🔬 Validation

- [x] Code reviewed against paper
- [x] Unit tests written
- [ ] Benchmark executed
- [x] Documentation updated
- [ ] Module README linked
- [ ] implementation_influence index updated

## 📚 Related Work

- Snodgrass (1995) — *Developing Time-Oriented Database Applications in SQL* (Morgan Kaufmann)
- Johnston & Weis (2010) — *Managing Time in Relational Databases* (Morgan Kaufmann)
- Böhlen & Jensen (1996) — *The Consensus Glossary of Temporal Database Concepts*
- [Temporal module README](../../../src/temporal/README.md)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
