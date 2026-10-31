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

# Timeseries Data Model

This document describes the module's persisted and derived data shapes for
developers changing ingestion, queries, compression, or lifecycle behavior.
The authoritative declarations are the public headers and storage
implementation, not this summary.

Exact model backend not verifiable from final document text alone.

## Point and series identity

`TSStore::DataPoint` in `include/timeseries/tsstore.h` contains:

| Field | Meaning |
|---|---|
| `metric` | Metric/series family name |
| `entity` | Entity key that distinguishes a series within a metric |
| `timestamp_ms` | Timestamp in milliseconds since the Unix epoch |
| `value` | Numeric `double` sample |
| `tags` | JSON values used by exact-match query filters |
| `metadata` | Additional JSON metadata |

The concrete key builder in `src/timeseries/tsstore.cpp` uses
`ts:{metric}:{entity}:{timestamp_ms}` for individual points. Gorilla batches use
the `tsc:` prefix and include the metric, entity, and first/last timestamps in
the key. The stored value for an individual point is its JSON representation;
a batch value contains the codec marker, count, tags/metadata, and binary
Gorilla data.

## Storage forms and fidelity

- A single `putDataPoint()` call is buffered only when Gorilla compression is
  configured and a `TSAutoBuffer` is attached. Otherwise it writes an
  individual point.
- `putDataPoints()` groups by metric/entity, sorts each group by timestamp, and
  writes Gorilla-encoded chunks. The chunk metadata takes tags and metadata
  from the first point in a group, so callers should not batch unlike metadata
  under one metric/entity and expect per-point metadata to survive the chunk
  representation.
- `include/timeseries/gorilla.h` documents delta-of-delta timestamp encoding
  and XOR value encoding. New encoded chunks have a versioned prefix; decoders
  also accept headerless legacy chunks.
- When `EncryptedChunkStore` is attached, the compressed chunk is encrypted
  after compression. Encryption is a storage envelope around the codec data,
  not a different time-series value model.

## Queries and derived data

`TSStore::QueryOptions` identifies a metric, optional entity, inclusive
millisecond bounds, a maximum result count, and an optional JSON tag filter.
The tag filter compares a supplied value exactly. Aggregation results contain
min, max, average, sum, count, and first/last timestamps.

Continuous aggregates and downsampling add derived metrics rather than
replacing the raw point schema. `DownsamplingPolicy` orders tiers from finest
to coarsest; each tier consumes the prior tier's data. Continuous-aggregate
watermarks are persisted by `ContinuousAggWatermarkStore` under the
`wm:cagg:` key prefix. Consumers must account for refresh lag and the
watermark rather than assuming a policy is fully materialized.

## Time-unit and ordering caveat

The concrete `TSStore` and Gorilla interfaces use milliseconds. The separate
frozen contract header describes nanoseconds, and its mock contract cases
expect strict ordering. In the implementation, a zero
`late_arrival_window_ms` disables the watermark check; with a positive window,
the check rejects timestamps strictly older than the watermark minus that
window. Equal timestamps are not rejected by that comparison. Do not convert
units or infer one API's ordering semantics from another without checking its
header and implementation.

## No native embedding representation

The timeseries point model has scalar numeric values, tags, and JSON metadata.
The module does not define an embedding dimension, vector-distance contract,
embedding model, or vector index. If an application stores vector-like data in
metadata, that is opaque application data and is not searchable or validated
as an embedding by this module.

## References

- [TSStore](../../include/timeseries/tsstore.h) and
  [its implementation](tsstore.cpp)
- [Gorilla codec](../../include/timeseries/gorilla.h)
- [Encryption envelope](../../include/timeseries/encrypted_chunk_store.h)
- [Continuous aggregates](../../include/timeseries/continuous_agg.h)
- [Downsampling tiers](../../include/timeseries/downsampling.h)
- [Canonical module architecture](ARCHITECTURE.md)
