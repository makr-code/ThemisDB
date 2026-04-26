# CQL: A Language for Continuous Queries over Streams and Relations

**Metadaten:**
- Author(en): Arvind Arasu, Shivnath Babu, Jennifer Widom
- Konferenz/Journal: IEEE Data Engineering Bulletin, Vol. 29(1), pp. 5–14; Extended version: ACM TODS 31(4), 2006
- Jahr: 2006
- Link: [IEEE DEB](http://sites.computer.org/debull/A06mar/arasu.pdf) · [ACM TODS](https://dl.acm.org/doi/10.1145/1146461.1146463) · [Stanford STREAM project](http://infolab.stanford.edu/stream/)
- Zitierweise: `arasu2006cql`
- Tags: `streaming`, `continuous-query`, `window-functions`, `relational-streams`, `real-time`, `cql`
- ThemisDB-Versionen: Planned v2.0.0+
- Status: [x] Not Started | [ ] Partially Implemented | [ ] Fully Implemented

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

ThemisDB's IoT example (`examples/09_iot_sensor_network/`) already uses a `CREATE CONTINUOUS QUERY` syntax that directly mirrors CQL's standing query concept. The planned Phase 8 of the query module (see `src/query/ROADMAP.md`) formalises this into a production-grade CQL engine:

1. **Window type taxonomy**: Time-based (`RANGE`), count-based (`ROWS`), and tumbling windows map directly to ThemisDB's `WindowSpec` in `src/analytics/streaming_window.cpp`.
2. **Istream semantics**: ThemisDB's CDC module already produces insert/delete events that align with CQL's `Istream`/`Dstream` operators.
3. **AQL extension plan**: `CREATE CONTINUOUS QUERY <name> ON <source> [WINDOW ...] BEGIN <AQL_SELECT> END` follows the CQL standing query declaration syntax.
4. **Incremental aggregation**: The watermark-based incremental refresh in `src/timeseries/continuous_agg.cpp` corresponds to CQL's synopsis maintenance model.

### How Was It Adapted?

| CQL Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Pure stream model (append-only) | Hybrid: streams + mutable collections | ThemisDB serves OLTP/OLAP; mutable relations must be supported |
| Istream / Dstream / Rstream operators | CDC events + continuous query result modes | ThemisDB's CDC module already captures `INSERT`/`DELETE`/`UPDATE`; the `RESULT_MODE` query option selects delta vs. snapshot delivery |
| SQL relational algebra inside window | AQL inside window | ThemisDB uses AQL instead of raw SQL; all relational operators plus graph traversal are available |
| Standalone STREAM system | Embedded in AQL query engine | No separate runtime; CQL plan nodes plug into the existing `QueryExecutor` pipeline |
| Per-query time-based scheduler | Shared `AggregateScheduler` from timeseries | Reuses `src/timeseries/aggregate_scheduler.cpp` to trigger window evaluation |
| STREAM's naive synopsis storage | RocksDB-backed ring buffer per window | Persistent state survives node restarts; WAL protects against mid-window crashes |

### Performance Impact

| Metric | CQL / STREAM Claim | ThemisDB Target | Status |
|--------|--------------------|-----------------|--------|
| Per-tuple latency | < 1 ms (mid-2000s hardware) | ≤ 5 ms p99 end-to-end | ⏳ Planned |
| Window evaluation overhead | O(window size) | O(delta) with synopsis | ⏳ Planned |
| Concurrent continuous queries | Not specified | ≥ 1 000 active queries | ⏳ Planned |
| Throughput | ~100 k tuples/s (STREAM prototype) | ≥ 500 k tuples/s | ⏳ Planned |

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
