---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/timeseries/tsstore.h; include/timeseries/ts_auto_buffer.h; include/timeseries/continuous_agg.h; include/timeseries/hypertable.h"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries public headers

This directory declares the C++ APIs for timeseries storage and related
components. The headers are authoritative for signatures, ownership,
configuration, and thread-safety notes; implementation details are in
[`../../src/timeseries/`](../../src/timeseries/).

## TSStore quick start

`TSStore` requires a live, non-owned RocksDB `TransactionDB`. The column-family
handle may be null. With Gorilla configured, `putDataPoint()` may be accepted
by an attached auto-buffer; otherwise it writes an individual point. A
Gorilla-configured `putDataPoints()` or `putBatch()` writes grouped compressed
chunks.

```cpp
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"

rocksdb::TransactionDB* db = /* application-owned database */;
rocksdb::ColumnFamilyHandle* cf = nullptr;

themis::TSStore::Config config;
config.compression = themis::TSStore::CompressionType::Gorilla;
themis::TSStore store(db, cf, config);

themis::TSStore::DataPoint point{
    "cpu_usage", "srv-01", timestamp_ms, 0.72, {{"env", "prod"}}, {}
};
auto write_result = store.putDataPoint(point);

themis::TSStore::QueryOptions query;
query.metric = "cpu_usage";
query.entity = "srv-01";
query.from_timestamp_ms = start_ms;
query.to_timestamp_ms = end_ms;
auto points = store.query(query);
```

Timestamps and inclusive query bounds are signed Unix epoch milliseconds.
Check each returned `Result` with `ok()` and handle `error()`; an empty
successful query is distinct from a failed query.

To batch single-point calls through a `TSAutoBuffer`, construct it after the
store, attach it, and start it. The buffer and store have non-owning references
to one another's dependencies; stop the buffer before destroying the store.

```cpp
themis::TSAutoBuffer buffer(&store);
store.setAutoBuffer(&buffer);
buffer.start();
// Calls to store.putDataPoint(...) may be buffered.
buffer.stop();
```

`TSStore::TSRow` contains metric, entity, timestamp, and value only.
`putBatch(std::span<const TSRow>)` returns
`Result<TSStore::BatchWriteResult>`; invalid rows are reported in the result,
while a RocksDB write failure is returned as an error. See `tsstore.h` for
partial-validation and lifetime details.

## Header inventory

| Area | Headers |
|---|---|
| Store and partitioning | `timeseries.h`, `tsstore.h`, `hypertable.h` |
| Buffer and flush | `ts_auto_buffer.h`, `ts_auto_buffer_adaptive.h`, `adaptive_flush_controller.h` |
| Compression | `gorilla.h`, `gorilla_simd.h`, `compression_selector.h` |
| Query and derived data | `aggregates.h`, `aggregate_scheduler.h`, `continuous_agg.h`, `downsampling.h`, `gap_fill.h`, `query_optimizer.h`, `ts_stream_cursor.h` |
| Retention and encryption | `retention.h`, `encrypted_chunk_store.h`, `ts_encrypted_key_rotation.h` |
| Integration and diagnostics | `prometheus_remote_write.h`, `timeseries_metrics.h`, `timeseries_api_contract.h`, `timeseries_incident_taxonomy.h`, `ts_edge_case_handler.h`, `ts_operator_diagnostics.h` |
| Anomaly detection | `anomaly_detection.h` |

These are API declarations, not a guarantee that every helper is wired into
every application. In particular:

- `TimeSeriesStore` in `timeseries.h` is a simpler separate API; it is not the
  same class as `TSStore`.
- `compression_selector.h` declares selection helpers. `TSStore::Config`
  exposes `None` and `Gorilla`; do not infer automatic selector integration
  from the presence of that header.
- `Hypertable::listChunks()` enumerates column-family names matching the
  hypertable chunk prefix, skips names whose time range cannot be parsed, and
  sorts the results by start time. Its row and size fields are RocksDB
  estimates; returned `cf_handle` values are null, and `is_compressed` is
  currently false rather than tracked manual-compression metadata. Do not treat
  those fields as exact counts, usable handles, or compression-state evidence.
- `prometheus_remote_write.h` declares payload decoding. HTTP routes,
  authentication, response codes, and transport policy belong to an integrating
  server and are not defined by this parser header.
- `TsStreamCursor` is declared in the `themis::timeseries` namespace; consult
  its header for its paged cursor interface and ownership constraints.
- Concurrent use restrictions differ by component. Check the relevant header
  rather than assuming module-wide thread safety.

## Build and tests

The public headers are part of the repository include tree. Focused tests are
registered from `tests/timeseries/CMakeLists.txt`; the implementation and test
documentation are linked from the module [source README](../../src/timeseries/README.md).
Target names depend on the source stems and are generated by CMake. No build or
test result is asserted here.
