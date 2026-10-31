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

## Timestamp contract mismatch

There is a material mismatch that callers must not paper over:

1. `TSStore::DataPoint` and its storage key use milliseconds.
2. `timeseries_api_contract.h` describes Unix nanoseconds and strict
   increasing timestamps.
3. In `TSStore::putDataPoint()`, a zero late-arrival window skips the
   watermark check; a positive window rejects only values older than
   `watermark - window`, and equal timestamps pass that comparison.

Until the public contract and concrete implementation are reconciled, follow
the units declared by the exact method being called and add tests against that
implementation. Do not infer server endpoint units or ordering behavior from
the mock-only contract tests.

## Compatibility

The contract header describes the current v1.x line as stable and calls for a
major-version bump, migration notes, and a changelog entry for breaking
changes. Persisted keys, Gorilla chunk framing, encryption envelopes, and
continuous-aggregate watermarks are also storage compatibility surfaces.
Review the relevant serializer and readers before changing them.

## References

- [Public-header architecture](../../include/timeseries/ARCHITECTURE.md)
- [Implementation guide](IMPLEMENTATION.md)
- [Data model](MODELING.md)
- [Focused validation guide](MODULE_GUIDE.md)
