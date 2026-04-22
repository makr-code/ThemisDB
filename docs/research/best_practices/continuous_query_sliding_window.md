# Continuous Query Language: Sliding Window and Standing Query Patterns

**Metadaten:**
- Source: Stanford STREAM Project (Arasu et al., 2006); Apache Flink Documentation (2024); ARCADE System (arXiv 2509.19757)
- URL: https://infolab.stanford.edu/stream/ · https://nightlies.apache.org/flink/flink-docs-stable/docs/dev/datastream/operators/windows/ · https://arxiv.org/html/2509.19757v1
- Tags: `streaming`, `continuous-query`, `sliding-window`, `standing-query`, `real-time`, `event-processing`, `cql`
- ThemisDB-Versionen: Planned v2.0.0+
- Status: [x] Identified | [ ] Partially Adopted | [ ] Fully Adopted

## 📋 Summary

A **Continuous Query** (also called a *standing query*) is registered once and evaluated continuously as new data arrives, delivering incremental result updates rather than a one-shot answer. Sliding windows define the temporal or count-based scope of data visible to each evaluation cycle. This pattern is the backbone of real-time monitoring, IoT aggregation, anomaly detection, and stream-join workloads.

ThemisDB's query engine (Phases 1–7) handles ad-hoc queries. Phase 8 (planned v2.0.0) extends the engine with a `ContinuousQueryEngine` that implements the window, standing-query lifecycle, and push-based delivery patterns described here.

## 🎯 Core Principles

- **Standing query lifecycle**: A continuous query is registered (`CREATE CONTINUOUS QUERY`), maintained in a query registry, evaluated on each window tick or new-event trigger, and deregistered (`DROP CONTINUOUS QUERY`). Its lifecycle is independent of client connections — results are buffered until consumed.
- **Window types**: Three canonical window types cover almost all streaming use cases: (1) *time-based sliding* (`RANGE INTERVAL T`), (2) *count-based sliding* (`ROWS N`), (3) *tumbling / landmark* (non-overlapping; special case of time-based with stride = size).
- **Delta propagation**: On each evaluation, only the *delta* (newly added or expired tuples) is processed, not the full window contents. This keeps per-tick CPU proportional to the arrival rate, not the window size.
- **Watermark discipline**: Out-of-order events are handled by a configurable `allowed_lateness_ms`. Tuples arriving after the watermark has passed are either dropped or trigger a late-data correction update.
- **Result delivery modes**: Three modes mirror CQL's Istream/Dstream/Rstream: `DELTA` (only changes), `SNAPSHOT` (full current result), and `CHANGES` (CDC-style `+/-` rows). The client selects the mode at registration time.
- **Backpressure**: Continuous query result channels apply backpressure when the consumer is slow; the engine signals the scheduler to reduce evaluation frequency rather than drop results.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/query/continuous_query/` — `ContinuousQueryEngine`, `ContinuousQueryRegistry`, `ContinuousQueryPlanner`, `WindowSpec` *(planned v2.0.0)*
- `src/query/aql_parser.cpp` — `CREATE CONTINUOUS QUERY` / `DROP CONTINUOUS QUERY` / `SHOW CONTINUOUS QUERIES` DDL extensions *(planned)*
- `src/analytics/streaming_window.cpp` — reused for sliding window synopsis maintenance
- `src/timeseries/aggregate_scheduler.cpp` — tick-based evaluation scheduling
- `src/timeseries/continuous_agg.cpp` — watermark and incremental refresh, extended with generic `allowed_lateness_ms`
- `src/cdc/` — Istream/Dstream events feed continuous query input queues
- `src/server/http_server.cpp` / wire protocol — Server-Sent Events (SSE) and WebSocket push delivery of incremental results

### What Was Adopted?

**Window specification grammar** (planned AQL extension):

```sql
-- Time-based sliding window: aggregate last 5 minutes of sensor readings
CREATE CONTINUOUS QUERY cq_avg_temp
ON sensor_readings
WINDOW (TYPE TIME RANGE INTERVAL '5' MINUTE SLIDE INTERVAL '1' MINUTE)
RESULT_MODE DELTA
BEGIN
  SELECT sensor_id, AVG(temperature) AS avg_temp
  FROM __window__
  GROUP BY sensor_id
END;
```

```sql
-- Count-based sliding window: last 100 events per device
CREATE CONTINUOUS QUERY cq_last100_errors
ON error_events
WINDOW (TYPE COUNT ROWS 100 SLIDE 10 PARTITION BY device_id)
RESULT_MODE SNAPSHOT
BEGIN
  SELECT device_id, COUNT(*) AS error_count, MAX(severity) AS max_severity
  FROM __window__
  WHERE severity >= 3
  GROUP BY device_id
END;
```

```sql
-- Tumbling window: 1-minute non-overlapping buckets
CREATE CONTINUOUS QUERY cq_minute_agg
ON telemetry
WINDOW (TYPE TIME RANGE INTERVAL '1' MINUTE SLIDE INTERVAL '1' MINUTE)
RESULT_MODE SNAPSHOT
BEGIN
  SELECT bucket_start(), metric_name, SUM(value) AS total
  FROM __window__
  GROUP BY metric_name
END;
```

**Standing query lifecycle** (planned C++ API):

```cpp
// ContinuousQueryEngine (planned: include/query/continuous_query_engine.h)
class ContinuousQueryEngine {
public:
    ContinuousQueryHandle registerQuery(ContinuousQuerySpec spec);
    void dropQuery(const std::string& query_name);
    std::vector<ContinuousQueryInfo> listQueries() const;
    // Result delivery: consumer calls next() on the returned ResultStream
    ResultStreamPtr subscribe(const std::string& query_name, ResultMode mode);
};
```

**Delta propagation model** — on each window tick:
1. Compute `added_tuples = new_arrivals ∩ [window_open, window_close]`
2. Compute `expired_tuples = synopsis ∩ [old_window_open, new_window_open)`
3. Apply incremental aggregation update: `agg_new = agg_old + Σ(added) - Σ(expired)`
4. Emit delta or snapshot according to `RESULT_MODE`

**Watermark handling**:
- Each continuous query declares `ALLOWED_LATENESS INTERVAL '500' MILLISECOND`
- Tuples arriving within the lateness bound trigger a retroactive synopsis update and a correction delta emission
- Tuples arriving after the lateness bound are counted in a `late_dropped_events` Prometheus counter

### Deviations & Rationale

| Best Practice | ThemisDB Deviation | Rationale |
|---|---|---|
| Pure stream model (no mutable state) | Continuous queries may join against mutable ThemisDB collections | ThemisDB is a hybrid OLTP+streaming system; dimension table lookups are necessary |
| Dedicated streaming runtime | Embedded in existing `QueryExecutor` pipeline | Avoids a second execution runtime; consistent error handling, security, and observability |
| Fixed schema per stream | Dynamic schema with optional fields | ThemisDB collections evolve; new optional fields are tolerated, missing required fields abort the query |
| Client holds query socket open | Queries survive client disconnection | Results buffered in a durable result queue (max `result_buffer_ms`); client reconnects and catches up |

## ⚠️ Trade-offs & Limitations

- **Memory footprint per window**: A 24-hour sliding window over a 10 k event/s stream holds up to 864 M synopsis entries. Mandatory `MAX_WINDOW_SIZE` limit (default: 10 M tuples or 1 GB, whichever is smaller) enforced at registration time.
- **Cross-shard continuous queries**: When the source collection is sharded, each shard runs a local window sub-query; partial aggregates are merged by a coordinator node. This adds one network round-trip per evaluation tick and may cause slightly inconsistent cross-shard snapshots (eventual consistency window ≤ one tick interval).
- **Late data and exactly-once**: Late-data corrections produce additional delta emissions. Downstream consumers must be idempotent or accept occasional duplicate result rows in `DELTA` mode.
- **AQL function side effects**: UDFs called inside a continuous query body must be pure (no external side effects). Impure UDFs are rejected at registration time with `IMPURE_UDF_IN_CONTINUOUS_QUERY` error.

## 🔬 Validation

- [ ] Code reviewed against CQL paper and Flink window semantics
- [ ] Unit tests: `WindowSpec` construction, delta computation, synopsis expiry, watermark advancement
- [ ] Integration test: end-to-end `CREATE CONTINUOUS QUERY` → event injection → SSE result verification
- [ ] Late-data test: inject events after watermark; verify correction delta emitted within one tick
- [ ] Performance: ≥ 500 k tuples/s throughput; ≤ 5 ms p99 per-tuple latency; ≤ 1 µs window tick overhead for empty windows
- [ ] Memory guard: verify `MAX_WINDOW_SIZE` enforcement triggers `WINDOW_OVERFLOW` error
- [ ] Documentation updated (`src/query/README.md`, module ROADMAP, API reference)

## 📚 Related

- [CQL Paper — Arasu, Babu & Widom (2006)](../papers/arasu_cql_2006.md)
- [SQL:2011 Temporal Features](../papers/temporal_sql2011_2012.md) — temporal predicates composable with sliding windows
- [DuckDB OLAP Paper](../papers/duckdb_olap_2019.md) — columnar execution for window aggregation
- [Best Practice: Exponential Backoff](exponential_backoff_retry.md) — retry policy for continuous query evaluation failures
- [Best Practice: Token Bucket Rate Limiting](token_bucket_rate_limiting.md) — backpressure enforcement on result delivery

---
**Last Updated:** 2026-04-22
