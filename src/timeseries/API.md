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
---

# Timeseries API Contract Guide

This page helps developers select and safely call the module's C++ interfaces.
The declarations in `include/timeseries/` are the public API source of truth;
this guide does not replace their types or Doxygen contracts.

Exact model backend not verifiable from final document text alone.

## Interface map

| Need | Interface | Contract detail |
|---|---|---|
| Store, write, range query, aggregation | [`TSStore`](../../include/timeseries/tsstore.h) | `DataPoint` timestamps are milliseconds. Writes and queries return `Result` values; inspect errors rather than treating an empty query as failure. |
| Buffer single-point writes | [`TSAutoBuffer`](../../include/timeseries/ts_auto_buffer.h) | Store pointer is non-owning. `start()` enables background flushing; `stop()` joins and flushes remaining points. `push()` can return `BUFFER_FULL` or `INVALID_INPUT`. |
| Encode/decode | [`GorillaEncoder` and `GorillaDecoder`](../../include/timeseries/gorilla.h) | Timestamp/value codec; decode errors must be checked before accepting a complete chunk. |
| Retention cleanup | [`RetentionManager`](../../include/timeseries/retention.h) | Configure per-metric durations; `apply()` deletes expired data and returns a count. Async cleanup is opt-in. |
| Aggregate refresh | [`ContinuousAggregateManager`](../../include/timeseries/continuous_agg.h) | Registration, refresh, materialized reads, and watermarks are separate operations. Respect the header's synchronization requirements. |
| Tiered rollups | [`DownsamplingPipeline`](../../include/timeseries/downsampling.h) | Register policies before refresh; serialize concurrent refresh calls on one instance. |
| Remote-write payload decoding | [`PromWriteRequest`](../../include/timeseries/prometheus_remote_write.h) | Decode protobuf or Snappy+protobuf; this is parsing, not an HTTP authorization contract. |
| Encrypted chunks and key rotation | [`EncryptedChunkStore`](../../include/timeseries/encrypted_chunk_store.h), [`TsEncryptedKeyRotation`](../../include/timeseries/ts_encrypted_key_rotation.h) | Key callbacks are supplied by the caller; retain old keys while stored chunks still reference them. |
| Metrics and incidents | [`TimeSeriesMetrics`](../../include/timeseries/timeseries_metrics.h), [`TsOperatorDiagnostics`](../../include/timeseries/ts_operator_diagnostics.h) | Wire these explicitly; diagnostic state does not replace durable application logs or alerts. |

## Calling and error handling

- `TSStore` and `TSAutoBuffer` retain non-owning pointers to dependencies.
  Ensure the RocksDB objects, store, buffer, callbacks, and metrics objects
  outlive any component that uses them.
- Propagate `Result` errors from writes, queries, and decoding. In particular,
  do not retry permanent input/format errors as if they were transient
  backpressure.
- The frozen taxonomy in
  [`timeseries_api_contract.h`](../../include/timeseries/timeseries_api_contract.h)
  classifies hard, lifecycle, backpressure, and transient errors. Check the
  actual return type of the selected interface; not every API returns that
  enum.
- Range filters use millisecond bounds in `TSStore::QueryOptions`; tags are
  exact-match JSON values. Define and test limits at the caller boundary.
- `RetentionManager::apply()` is destructive. Review the policy, ensure the
  retention boundary is intended, and preserve a separate backup/recovery
  path before invoking it operationally.

## Timestamp and ordering contract

`TSStore::DataPoint::timestamp_ms`, query bounds, Gorilla timestamps, and
`late_arrival_window_ms` use signed Unix epoch milliseconds. The store does not
convert these values to nanoseconds or reject zero/negative timestamps based
only on sign.

With the default `late_arrival_window_ms == 0`, all timestamp ordering is
accepted. The current implementation also skips this check for negative values;
callers should use zero to explicitly disable it. A positive window rejects a write only when its timestamp is strictly
less than the current per-series watermark minus the window. Equality with the
watermark or cutoff is accepted; accepted older points do not move the
watermark. The watermark is in memory and is not restored when a new
`TSStore` instance is created. The storage-backed
[`TSStore ordering tests`](../../tests/test_tsstore_out_of_order.cpp) exercise
these rules; mock-only tests are not evidence of persisted-store behavior.

## Compatibility

The contract header previously declared nanosecond timestamp constants that
were not used by repository code and contradicted `TSStore`. They remain
deprecated for source compatibility but do not constrain TSStore timestamps;
use application-owned millisecond values or `kMaxPointTimestampMs` for an
open-ended range.
Persisted keys, Gorilla chunk framing, encryption envelopes, and
continuous-aggregate watermarks are separate storage compatibility surfaces.
Review the relevant serializer and readers before changing them.

## References

- [Public-header architecture](../../include/timeseries/ARCHITECTURE.md)
- [Implementation guide](IMPLEMENTATION.md)
- [Data model](MODELING.md)
- [Focused validation guide](MODULE_GUIDE.md)
