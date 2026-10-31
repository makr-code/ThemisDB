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

# Timeseries Implementation Guide

This guide is for developers changing the internal runtime under
`src/timeseries/`. Public declarations live in `include/timeseries/`; changes
to their behavior must be checked against the public headers, the frozen
contract, and focused tests.

Exact model backend not verifiable from final document text alone.

## Write path

`TSStore::putDataPoint()` validates non-empty metric and entity names. If a
Gorilla-configured store has an attached `TSAutoBuffer`, a successful buffer
push delays persistence until flush. A full buffer falls through to a direct
individual RocksDB write. Without that configuration, the point is written as
JSON under the `ts:` key prefix.

`TSStore::putDataPoints()` groups points by metric/entity, sorts each group by
timestamp, encodes timestamp/value pairs with `GorillaEncoder`, and writes the
chunk records in a RocksDB `WriteBatch`. The chunk metadata uses the first
point's tags and metadata; this is a data-model constraint when batching.
Compression and encryption are composed compress-then-encrypt when an
`EncryptedChunkStore` is attached.

`TSAutoBuffer` can flush on configured size/time thresholds or on explicit
`flush()`/`flushFor()`. Its optional WAL snapshot methods persist and restore
buffer state; after restore, the caller must flush or start the buffer to send
the restored points to `TSStore`. Remove the snapshot only after the
application has verified that replay is complete.

## Read and derived-data path

Queries scan the time-series key range and apply optional entity, limit, and
exact JSON tag filters. Gorilla chunks are decoded on read; encrypted chunks
must be decrypted first. `TsStreamCursor` wraps paged range reads for consumers
that should not materialize an entire result at once.

Continuous aggregate refresh reads its configured range, writes derived
points, and persists watermarks. Downsampling tiers are ordered fine-to-coarse
and consume the preceding tier. Refresh calls on the same
`DownsamplingPipeline` instance are documented as not thread-safe; callers
must serialize them. Aggregate manager methods likewise require external
synchronization where the header states they are not thread-safe.

## Lifecycle and integration paths

- `RetentionManager` applies per-metric age limits and can run a background
  cleanup loop. A zero or absent per-metric duration means ignore that metric.
  Deletion is destructive; audit entries and counters are observability, not a
  restore mechanism.
- `PromWriteRequest` decodes raw or Snappy-compressed Prometheus protobuf
  payloads. The parser validates wire bounds; HTTP authentication, transport,
  and endpoint policy belong to the server integration, not this decoder.
- `EncryptedChunkStore` obtains current and historical keys from caller
  callbacks. The key provider and audit logger are external dependencies.
  `TsEncryptedKeyRotation` scans encrypted Gorilla chunks and replaces a
  stale-key value before the old value can be discarded.

## Invariants and known contract gap

Keep timestamp units explicit at each interface: `TSStore::DataPoint` uses
milliseconds, while `include/timeseries/timeseries_api_contract.h` states
nanoseconds. The contract also describes strict monotonicity, but
`TSStore::putDataPoint()` only checks ordering when `late_arrival_window_ms`
is positive and its watermark helper accepts equality. The mock-based
contract-hardening tests do not establish concrete `TSStore` behavior for
these cases. Reconcile this mismatch before changing either contract.

Do not silently change chunk key formats, serialized JSON fields, Gorilla
headers, encryption envelope fields, or watermark keys: existing stored data
and readers depend on them. Add focused round-trip or migration tests for any
format change.

## Focused validation

- `tests/timeseries/test_timeseries_contract_hardening_focused.cpp` covers the
  named contract edge cases, but includes mock helpers.
- `tests/timeseries/test_continuous_agg_materialization.cpp` covers aggregate
  refresh and watermark behavior.
- `tests/timeseries/test_timeseries_error_path_phase3.cpp` covers incident
  classification and concurrent incident emission.
- `tests/test_tsstore.cpp`, `tests/test_tsstore_batch.cpp`, and
  `tests/test_tsstore_out_of_order.cpp` exercise concrete storage paths.

See [MODULE_GUIDE.md](MODULE_GUIDE.md) for focused build and documentation
validation.

## References

- [TSStore implementation](tsstore.cpp) and
  [public declaration](../../include/timeseries/tsstore.h)
- [API contract](../../include/timeseries/timeseries_api_contract.h)
- [Auto-buffer implementation](ts_auto_buffer.cpp) and
  [interface](../../include/timeseries/ts_auto_buffer.h)
- [Gorilla implementation](gorilla.cpp)
- [Retention implementation](retention.cpp)
- [Module tests](../../tests/timeseries/)
