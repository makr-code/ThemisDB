# DuckDB: An Embeddable Analytical Database

**Metadaten:**
- Author(en): Mark Raasveldt, Hannes Mühleisen
- Konferenz/Journal: ACM SIGMOD International Conference on Management of Data (SIGMOD 2019)
- Jahr: 2019
- Link: [ACM DL](https://dl.acm.org/doi/10.1145/3299869.3320212) · [PDF](https://www.cidrdb.org/cidr2020/papers/p21-raasveldt-cidr20.pdf)
- Zitierweise: `raasveldt2019duckdb`
- Tags: `relational`, `olap`, `columnar`, `vectorized-execution`, `embedded-database`, `analytics`
- ThemisDB-Versionen: Research influence on query engine design; planned adoption v2.x
- Status: [x] Partially Implemented (columnar export path) · [ ] Full vectorized execution planned

## 📋 Executive Summary

DuckDB is an embeddable in-process analytical database engine built for OLAP workloads. It combines vectorized execution (batch-at-a-time), columnar storage, and an adaptive radix tree index into a single library with no external dependencies. DuckDB consistently outperforms row-oriented databases (PostgreSQL, SQLite) by 10–100× on analytical workloads while preserving full ACID compliance. Its architecture is directly relevant to ThemisDB's analytical query path and columnar export/import pipeline.

## 🎯 Key Findings

- **Vectorized execution model**: Process tuples in batches (vectors) of 1024 elements, drastically reducing per-tuple overhead vs. row-at-a-time Volcano model.
- **Columnar storage format**: Stores data in column-oriented chunks; achieves 3–10× better cache utilization for aggregation-heavy queries.
- **In-process embedding**: No client–server protocol overhead; queries execute in the same process as the application — ideal for ThemisDB's embedded analytical mode.
- **Apache Arrow integration**: DuckDB natively reads/writes Arrow format; zero-copy data exchange with ThemisDB's Arrow exporter.
- **Parallel query execution**: Exploits multi-core parallelism via morsel-driven scheduling (similar to HyPer/Umbra).
- **Full SQL compliance**: Window functions, CTEs, ASOF joins, full ANSI SQL — no SQL dialect gaps for analytical queries.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Query execution engine → `src/query/`
- [x] Columnar export → `src/exporters/` (Apache Arrow, Parquet)
- [ ] Vectorized execution path → `src/query/vectorized/` *(planned v2.x)*
- [ ] Embedded analytical mode → ThemisDB Lite edition *(planned)*

### What Was Adopted?

1. **Apache Arrow output format**: ThemisDB's `ArrowExporter` and `ArrowStreamExporter` follow DuckDB's practice of first-class Arrow output for analytics.
2. **Columnar chunk layout**: ThemisDB's columnar storage layer organizes data in 1024-element chunks, mirroring DuckDB's `DataChunk` abstraction.
3. **Morsel-driven parallelism concept**: The scheduling principle informed ThemisDB's query pipeline thread pool design.

### How Was It Adapted?

| DuckDB Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Pure in-process embedding | Optional embedded mode + client/server | ThemisDB serves concurrent clients; pure embedded insufficient |
| Columnar-first storage | Hybrid row+columnar storage | OLTP workloads require row access; OLAP path uses columnar projection |
| Monolithic engine binary | Modular build (CMake options) | ThemisDB has 39 modules; single binary not appropriate |
| DuckDB SQL dialect | SQL + AQL hybrid | ThemisDB extends SQL with graph traversal (AQL) |

### Performance Impact

| Metric | DuckDB Claim | ThemisDB Target | Status |
|--------|-------------|-----------------|--------|
| TPC-H Q1 (scale 1) | ~40 ms | <200 ms (without JIT) | ⏳ Planned |
| Columnar scan throughput | ~8 GB/s | ≥3 GB/s | ⏳ Benchmarking |
| Arrow export throughput | ~1.2 GB/s | ≥800 MB/s | ⏳ Benchmarking |

## ⚠️ Limitations & Open Questions

- DuckDB's vectorized execution engine requires significant refactoring of ThemisDB's current row-at-a-time executor.
  - ThemisDB solution: Phased approach — add vectorized scan operators first, then aggregation operators.
- DuckDB lacks multi-tenant isolation; ThemisDB requires per-tenant data isolation.
  - ThemisDB solution: Namespace-partitioned columnar chunks with tenant-aware scan filters.
- DuckDB's embedded model conflicts with ThemisDB's distributed sharding architecture.
  - ThemisDB solution: Vectorized execution only in single-node analytical path; distributed queries use distributed planner.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written
- [ ] Benchmark executed (TPC-H scale factor 1)
- [ ] Documentation updated
- [ ] Module README linked
- [ ] implementation_influence index updated

## 📚 Related Work

- [Neumann (2011) — HyPer/Umbra query compilation](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)
- [Apache Arrow specification](https://arrow.apache.org/docs/)
- [Kleppmann (2017) — Designing Data-Intensive Applications](https://dataintensive.net/)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
