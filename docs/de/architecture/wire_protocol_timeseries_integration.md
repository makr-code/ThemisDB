# Wire Protocol Timeseries Integration

**Status**: Implemented (Minimal MVP)  
**Date**: 2026-02-08  
**Version**: Wire Protocol v1, OpCode 0x51

## Overview

This document describes the server-side implementation of `OpCode::TIMESERIES_QUERY (0x51)` in the ThemisDB wire protocol, including integration with TSStore and ContinuousAggregateManager, as well as current limitations regarding AQL integration.

## Implementation Summary

### Wire Protocol Server Changes

1. **Constructor Updated** (`include/network/wire_protocol_server.h`, `src/network/wire_protocol_server.cpp`)
   - Added `std::shared_ptr<TSStore> ts_store_` member
   - Added `std::shared_ptr<ContinuousAggregateManager> agg_manager_` member
   - Updated constructor to accept these parameters (optional, default nullptr)

2. **OpCode Dispatch** (`src/network/wire_protocol_server.cpp::Session::handleMessage()`)
   - Implemented full OpCode dispatch switch statement
   - Routes OpCode 0x51 (TIMESERIES_QUERY) to `handleTimeseriesQuery()`
   - Validates payload size against configured maximum

3. **Handler Implementation** (`src/network/wire_protocol_server.cpp::Session::handleTimeseriesQuery()`)
   - Validates TSStore availability
   - Demonstrates integration pattern with TSStore::query() and TSStore::aggregate()
   - Shows integration with ContinuousAggregateManager for pre-computed aggregates
   - Returns wire protocol response frame

### Message Flow

```
Client                          Wire Protocol Server                    TSStore/AggManager
  |                                      |                                      |
  |-- TimeSeriesQueryRequest (0x51) --->|                                      |
  |    (protobuf payload)                |                                      |
  |                                      |-- Validate TSStore availability      |
  |                                      |                                      |
  |                                      |-- Parse payload ------------------>  |
  |                                      |   (collection, time range,           |
  |                                      |    aggregation, filters)             |
  |                                      |                                      |
  |                                      |-- Convert ns to ms timestamps        |
  |                                      |                                      |
  |                                      |-- Create QueryOptions                |
  |                                      |                                      |
  |                                      |-- ts_store_->query() ------------->  |
  |                                      |   OR ts_store_->aggregate()          |
  |                                      |                                      |
  |                                      |<-- Result<vector<DataPoint>> -------|
  |                                      |    OR Result<AggregationResult>      |
  |                                      |                                      |
  |                                      |-- Optional: use agg_manager_ ------>  |
  |                                      |   for pre-computed aggregates        |
  |                                      |                                      |
  |                                      |-- Serialize to                       |
  |                                      |   TimeSeriesQueryResponse            |
  |                                      |                                      |
  |<-- Wire frame response (0x21) ------|                                      |
  |    (protobuf payload)                |                                      |
```

### Protobuf Message Definitions

**Request** (`src/network/themis_wire_v1.proto`):
```protobuf
message TimeSeriesQueryRequest {
  string collection = 1;              // Metric name
  uint64 start_time_ns = 2;           // Start timestamp (nanoseconds)
  uint64 end_time_ns = 3;             // End timestamp (nanoseconds)
  Aggregation aggregation = 4;        // AVG, SUM, MIN, MAX, COUNT, etc.
  uint64 bucket_size_ns = 5;          // Time bucket size (nanoseconds)
  map<string, Value> filters = 6;     // Tag filters
}

enum Aggregation {
  AVG = 0; SUM = 1; MIN = 2; MAX = 3; COUNT = 4;
  STDDEV = 5; PERCENTILE_50 = 6; PERCENTILE_95 = 7; PERCENTILE_99 = 8;
}
```

**Response**:
```protobuf
message TimeSeriesQueryResponse {
  repeated TimeSeriesBucket buckets = 1;
  uint64 query_time_us = 2;
  TimeSeriesStats stats = 3;
}

message TimeSeriesBucket {
  uint64 timestamp_ns = 1;       // Bucket start time
  double value = 2;              // Aggregated value
  uint64 count = 3;              // Number of data points
  double min = 4;                // Min value in bucket (optional)
  double max = 5;                // Max value in bucket (optional)
}
```

## TSStore Integration

The handler integrates with TSStore through the following APIs:

### Query API
```cpp
TSStore::QueryOptions query_opts;
query_opts.metric = collection;
query_opts.from_timestamp_ms = start_time_ns / 1000000;  // Convert ns to ms
query_opts.to_timestamp_ms = end_time_ns / 1000000;
query_opts.limit = 1000;
query_opts.tag_filter = filters;  // Optional tag-based filtering

auto result = ts_store_->query(query_opts);
```

### Aggregation API
```cpp
auto agg_result = ts_store_->aggregate(query_opts);
// Returns: AggregationResult with min, max, avg, sum, count
```

### Continuous Aggregates (Optional)
If `agg_manager_` is configured, the handler can leverage pre-computed continuous aggregates for better performance on large time ranges:
```cpp
if (use_aggregates && agg_manager_) {
    // Use materialized aggregates stored by ContinuousAggregateManager
    auto agg_result = ts_store_->aggregate(query_opts);
}
```

## Current Implementation Status

### ✅ Implemented
- [x] TSStore and ContinuousAggregateManager added to WireProtocolServer constructor
- [x] OpCode dispatch switch statement in Session::handleMessage()
- [x] OpCode 0x51 routing to handleTimeseriesQuery()
- [x] Integration pattern with TSStore::query() and TSStore::aggregate()
- [x] Integration pattern with ContinuousAggregateManager
- [x] Error handling for missing TSStore
- [x] Wire frame response structure
- [x] Minimal test coverage (API/linkage verification)

### 🔄 Partial / TODO
- [ ] **Full protobuf deserialization** - Current implementation uses placeholder values
- [ ] **Complete response serialization** - Need to serialize TSStore results to TimeSeriesQueryResponse protobuf
- [ ] **Timestamp conversion** - ns↔ms conversion needs to be tested thoroughly
- [ ] **Tag filter mapping** - Map protobuf filters to TSStore tag_filter JSON
- [ ] **Bucket aggregation** - Implement time bucketing logic for downsampled results
- [ ] **Authentication/authorization** - Verify user has permission to query metric
- [ ] **Comprehensive integration tests** - End-to-end tests with real socket connections

## AQL Integration Gaps

### Current Behavior

**Timeseries data is NOT directly queryable via AQL.** The current implementation stores timeseries data in a specialized format optimized for range scans:

**Storage Key Format**:
- Single points: `ts:{metric}:{entity}:{timestamp_ms}`
- Compressed chunks: `tsc:{metric}:{entity}:{first_ts}:{last_ts}` (Gorilla codec)

**AQL Limitations**:
1. **No automatic timeseries syntax** - AQL does not have native `TIMESERIES` or `AGGREGATE OVER TIME` clauses
2. **No collection abstraction** - Timeseries metrics are stored with a `ts:` or `tsc:` prefix, not as AQL collections
3. **No time-based indexing** - Secondary indices don't support timeseries-specific optimizations

### Workarounds (Generic Document AQL)

Timeseries data CAN be queried via AQL by treating it as generic documents:

**Example 1: Range query (inefficient)**
```aql
FOR doc IN documents
  FILTER doc._key STARTS_WITH "ts:cpu_usage:server01:"
  FILTER doc._key >= "ts:cpu_usage:server01:1704067200000"
  FILTER doc._key <= "ts:cpu_usage:server01:1704153600000"
  RETURN doc
```

**Example 2: Using document properties**
```aql
FOR doc IN documents
  FILTER doc.metric == "cpu_usage"
  FILTER doc.entity == "server01"
  FILTER doc.timestamp_ms >= 1704067200000
  FILTER doc.timestamp_ms <= 1704153600000
  SORT doc.timestamp_ms ASC
  LIMIT 1000
  RETURN doc
```

⚠️ **Performance Warning**: Both approaches are significantly slower than using the native wire protocol TIMESERIES_QUERY (OpCode 0x51) or HTTP REST API (`/ts/query`), which leverage RocksDB range scans and Gorilla compression.

### Recommended Approach

**For timeseries operations, use:**
1. **Wire Protocol** - `OpCode::TIMESERIES_QUERY (0x51)` - Best performance
2. **HTTP REST API** - `/ts/query`, `/ts/aggregate` - Easy to use
3. **AQL (fallback)** - Generic document queries - Slow, not optimized

### Future Enhancements

To properly integrate timeseries with AQL, the following would be needed:

1. **Timeseries collection type**
   ```aql
   CREATE COLLECTION metrics TYPE TIMESERIES (
     metric STRING,
     entity STRING,
     timestamp TIMESTAMP,
     value DOUBLE,
     tags OBJECT
   )
   ```

2. **Time-based aggregation syntax**
   ```aql
   FOR doc IN metrics
     FILTER doc.metric == "cpu_usage"
     FILTER doc.timestamp >= @start AND doc.timestamp <= @end
     COLLECT bucket = DATE_BUCKET(doc.timestamp, "1h")
     AGGREGATE avg_value = AVG(doc.value), count = COUNT(1)
     SORT bucket ASC
     RETURN { time: bucket, avg: avg_value, count: count }
   ```

3. **Optimized execution** - Query optimizer recognizes timeseries patterns and routes to TSStore instead of generic document scan

**Status**: Not planned for v1.x - Would be considered for v2.0 if there's significant demand

## Testing

Minimal tests added in `tests/test_wire_protocol_integration.cpp`:
- `WireProtocolIntegration.ServerInstantiationWithTSStore` - Verifies API changes compile
- `WireProtocolIntegration.TimeseriesQueryFlowDocumentation` - Documents expected message flow

**To run**:
```bash
cd build
ctest -R WireProtocolIntegration -V
```

**Future test needs**:
- End-to-end socket connection test
- Protobuf serialization/deserialization test
- TSStore integration test with real data
- Performance benchmark vs HTTP REST API

## Client Usage

All native clients (Python, TypeScript, Java, Rust) already expose the TIMESERIES_QUERY opcode:

**Python** (`clients/python/themis/themis_native.py`):
```python
from themis_native import ThemisNativeClient

client = ThemisNativeClient("localhost", 18765)
response = client.timeseries_query(
    collection="cpu_usage",
    start_time_ns=1704067200000000000,
    end_time_ns=1704153600000000000,
    aggregation="avg",
    bucket_size_ns=3600000000000  # 1 hour
)
```

**TypeScript** (`clients/typescript/src/themis-client.ts`):
```typescript
const client = new ThemisClient("localhost", 18765);
const response = await client.timeseriesQuery({
  collection: "cpu_usage",
  startTimeNs: 1704067200000000000n,
  endTimeNs: 1704153600000000000n,
  aggregation: Aggregation.AVG,
  bucketSizeNs: 3600000000000n
});
```

⚠️ **Note**: With this server-side implementation, these client methods will now work instead of returning "not implemented" errors. However, full protobuf serialization/deserialization is still needed for production use.

## References

- Wire Protocol Specification: `docs/de/architecture/wire_protocol_v1.md`
- TSStore API: `include/timeseries/tsstore.h`
- Continuous Aggregates: `include/timeseries/continuous_agg.h`
- Protobuf Definitions: `src/network/themis_wire_v1.proto`
- Client Implementations: `clients/*/`

## Changelog

### 2026-02-08 - Initial Implementation (MVP)
- Added TSStore/ContinuousAggregateManager to WireProtocolServer
- Implemented OpCode dispatch for 0x51
- Created handleTimeseriesQuery() with integration pattern
- Added minimal test coverage
- Documented AQL integration gaps

---

**Next Steps**:
1. Complete protobuf parsing/serialization
2. Add comprehensive integration tests
3. Performance benchmarks
4. Consider AQL integration for v2.0
