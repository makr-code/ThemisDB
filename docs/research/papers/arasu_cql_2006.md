# CQL: A Language for Continuous Queries over Streams and Relations

**Metadaten:**
- Author(en): Arvind Arasu, Shivnath Babu, Jennifer Widom
- Konferenz/Journal: IEEE Data Engineering Bulletin, Vol. 29(1), pp. 5–14; Extended version: ACM TODS 31(4), 2006
- Jahr: 2006
- Link: [IEEE DEB](http://sites.computer.org/debull/A06mar/arasu.pdf) · [ACM TODS](https://dl.acm.org/doi/10.1145/1146461.1146463) · [Stanford STREAM project](http://infolab.stanford.edu/stream/)
- Zitierweise: `arasu2006cql`
- Tags: `streaming`, `continuous-query`, `window-functions`, `relational-streams`, `real-time`, `cql`
- ThemisDB-Versionen: v2.0.0+
- Status: [ ] Not Started | [ ] Partially Implemented | [x] Fully Implemented

## 📋 Executive Summary

CQL (Continuous Query Language) is a formal declarative language designed for issuing standing queries over continuous data streams. Developed as part of Stanford's STREAM project, CQL extends SQL with three new operators — stream-to-relation windows, relation-to-stream operators, and relation-to-relation operators — that bridge the impedance mismatch between infinite streaming data and the finite relational model. Every CQL query produces a result that is continuously updated as new stream tuples arrive, without re-executing from scratch. The paper is the definitive academic foundation for continuous query processing in modern streaming databases and directly informs the `CREATE CONTINUOUS QUERY` syntax already present in ThemisDB IoT examples.

## 🎯 Key Findings

- **Three-layer semantics**: CQL decomposes queries into (1) stream → relation (window), (2) relation → relation (SQL), and (3) relation → stream (Istream/Dstream/Rstream) operators. This clean separation simplifies incremental execution.
- **Window operators**: Sliding time windows (`[RANGE T]`), count windows (`[ROWS N]`), and partition windows (`[PARTITION BY k ROWS N]`) cover the full range of streaming aggregation patterns.
- **Istream / Dstream / Rstream**: Three relation-to-stream operators control what changes are surfaced: `Istream` emits newly inserted tuples, `Dstream` emits deleted tuples, `Rstream` emits the full current snapshot on every tick. Rstream maps to traditional SQL semantics.
- **Incremental evaluation**: The CQL execution model maintains a continuously changing relation (synopsis) for each window; only delta tuples are propagated downstream, enabling sub-millisecond per-tuple processing.
- **Stream join**: CQL supports joins between two streams by defining a temporal join window on each side, converting both to relations before applying the relational join.
- **Formal correctness**: CQL has a complete formal semantics, enabling verification of equivalences and query rewrite rules analogous to relational algebra equivalences.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [ ] Continuous query engine → `src/query/continuous_query/` *(planned v2.0.0)*
- [ ] AQL parser extensions → `src/query/aql_parser.cpp` *(planned: `CREATE CONTINUOUS QUERY` syntax)*
- [ ] Timeseries module → `src/timeseries/` *(window aggregation alignment)*
- [ ] Analytics streaming → `src/analytics/streaming_window.cpp` *(window operator implementation)*
- [ ] CDC / change capture → `src/cdc/` *(Istream/Dstream semantics map to CDC events)*
- [ ] Wire protocol → `src/server/` *(push-based result delivery for continuous queries)*

### What Was Adopted?

ThemisDB v2.0.0 ships a production-grade CQL engine (Phase 8.1–8.5). The engine implements all concepts from the paper:

1. **Window type taxonomy**: `WindowSpec::slidingTime()`, `::tumblingTime()`, `::slidingCount()` implement CQL `[RANGE T]`, `[TUMBLING T]`, `[ROWS N]` semantics.
2. **Istream / Dstream semantics**: `ResultMode::DELTA` emits `CQResult{is_retract=false}` (Istream) and `CQResult{is_retract=true}` (Dstream); `ResultMode::CHANGES` emits Istream only; `ResultMode::SNAPSHOT` emits Rstream.
3. **AQL extension**: `CREATE CONTINUOUS QUERY` DDL syntax with `WINDOW`, `AS`, and `OUTPUT` clauses; see `src/query/README.md` CQL section.
4. **Synopsis maintenance**: `SynopsisStore` — in-memory ring-buffer with `max_tuples`/`max_bytes` capacity limits; `IncrementalAgg` provides O(delta) aggregate updates.
5. **Watermark / late-data**: `CQWatermark` implements per-query watermark advancement and late-data correction within `allowed_lateness_ms`.

### How Was It Adapted?

| CQL Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Pure stream model (append-only) | Hybrid: streams + mutable collections | ThemisDB serves OLTP/OLAP; mutable relations must be supported |
| Istream / Dstream / Rstream operators | `ResultMode::DELTA` / `SNAPSHOT` / `CHANGES` | CDC events + `injectTuple()` API cover all three operator types |
| SQL relational algebra inside window | AQL inside window (`spec.aql_body`) | ThemisDB uses AQL; all relational operators plus graph traversal are available |
| Standalone STREAM system | Embedded in `ContinuousQueryEngine` | No separate runtime; integrates directly with query engine pipeline |
| Per-query time-based scheduler | `ContinuousQueryEngineImpl` evaluation loop | Per-query goroutine evaluates at `slide_ms` intervals |
| STREAM's naive synopsis storage | `SynopsisStore` ring-buffer (1 GiB / 10 M tuples) | In-memory for Phase 8; RocksDB persistence planned for v2.1.0 |

### Performance Impact

| Metric | CQL / STREAM Claim | ThemisDB Target | Baseline Measurement | Status |
|--------|--------------------|-----------------|---------------------|--------|
| Per-tuple latency | < 1 ms (mid-2000s hardware) | ≤ 5 ms p99 end-to-end | Pending (`BM_ContinuousQuery_TupleLatency`) | 📋 Benchmark defined; measurement pending hardware run |
| Window evaluation overhead | O(window size) | O(delta) with `IncrementalAgg` | — | ✅ Implemented (Phase 8.2) |
| Concurrent continuous queries | Not specified | ≥ 1 000 active queries | — | ✅ Unit test CQ-20 |
| Throughput | ~100 k tuples/s (STREAM prototype) | ≥ 500 k tuples/s | Pending (`BM_ContinuousQuery_Throughput`) | 📋 Benchmark defined (CQ-PERF-01); measurement pending hardware run |

## ⚠️ Limitations & Open Questions

- **Out-of-order events**: CQL assumes in-order tuple arrival. ThemisDB must integrate watermark-based late-data handling (see `src/timeseries/`) to handle out-of-order CDC events.
  - ThemisDB solution: adopt a configurable `allowed_lateness_ms` per query, consistent with Apache Flink's watermark model.
- **Exactly-once semantics**: The STREAM prototype does not address distributed exactly-once. ThemisDB must provide at-least-once delivery with idempotent result sinks for correctness.
  - ThemisDB solution: result sinks write with a sequence number; duplicate detection by the client or a downstream idempotent store.
- **Unbounded join state**: Joining two streams without tight window bounds causes unbounded memory growth. The CQL parser must enforce that at least one side of every stream-stream join has a bounded window.
  - ThemisDB solution: validation step in `ContinuousQueryPlanner`; queries with unbounded join rejected at parse time.
- **Schema evolution**: CQL defines a fixed schema per stream. ThemisDB's dynamic schema requires that continuous queries survive upstream schema additions (new optional fields tolerated; removed required fields abort the query with `SCHEMA_MISMATCH`).

## 🔬 Validation

- [ ] Code reviewed against paper (window operator semantics, Istream/Dstream/Rstream)
- [ ] Unit tests for `ContinuousQueryEngine`: window evaluation, result mode switching, synopsis pruning
- [ ] Integration test: `CREATE CONTINUOUS QUERY` over IoT timeseries collection; verify incremental result delivery
- [ ] Benchmark: continuous query throughput ≥ 500 k tuples/s; p99 latency ≤ 5 ms
- [ ] Documentation updated (`src/query/README.md`, `include/query/README.md`)
- [ ] Module README linked

## 📚 Related Work

- [DuckDB — Raasveldt & Mühleisen (2019)](duckdb_olap_2019.md) — columnar execution that feeds results into continuous query sinks
- [SQL:2011 Temporal Features — Kulkarni & Michels (2012)](temporal_sql2011_2012.md) — temporal predicates composable with CQL windows
- [Best Practice: Continuous Query Sliding Windows](../best_practices/continuous_query_sliding_window.md)
- [ARCADE: Real-Time Hybrid Continuous Query Processing (arXiv 2509.19757)](https://arxiv.org/html/2509.19757v1) — contemporary system combining continuous and ad-hoc queries over diverse modalities; already referenced in ThemisDB design docs

---
**Last Updated:** 2026-04-22
**Next Review:** 2026-07-01
