---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/timeseries/tsstore.h; include/timeseries/ts_auto_buffer.h; include/timeseries/continuous_agg.h; include/timeseries/downsampling.h; include/timeseries/retention.h; include/timeseries/prometheus_remote_write.h"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries module features

This page lists capabilities declared by the current timeseries interfaces.
It does not imply a performance level, deployment configuration, or that every
capability is enabled in a particular application.

| Capability | Interface | Boundary |
|---|---|---|
| Point writes, batch writes, range queries, and aggregate results | `TSStore` | Signed millisecond timestamps; the selected write method and configuration determine whether points are stored individually or in Gorilla batches. |
| Optional buffering and adaptive flush controls | `TSAutoBuffer`, `FlushController`, `AdaptiveFlushController` | Explicitly constructed/attached components with caller-managed lifetimes. |
| Gorilla encoding and decoding | `GorillaEncoder`, `GorillaDecoder`, `GorillaSIMDDecoder` | Codec interfaces; benchmark labels are not runtime throughput guarantees. |
| Continuous aggregates and tiered downsampling | `ContinuousAggMaterializationEngine`, `ContinuousAggregateManager`, `DownsamplingPipeline` | Registration and refresh are explicit operations; consult the headers for synchronization and watermark contracts. |
| Retention | `RetentionManager` | Cleanup is destructive; an undelete operation is not defined by this API. |
| Encrypted chunks and key rotation | `EncryptedChunkStore`, `TsEncryptedKeyRotation` | Key lookup and any required durable audit sink are supplied by the caller. |
| Prometheus remote-write decoding | `PromWriteRequest` | Payload parsing only; HTTP routes, authorization, and transport belong to the integrating server. |
| Gap filling, anomaly detection, metrics, and diagnostics | Corresponding `gap_fill.h`, `anomaly_detection.h`, `timeseries_metrics.h`, and `ts_operator_diagnostics.h` APIs | Availability and integration are component-specific. |

For signatures and configuration, use the
[public-header overview](../../include/timeseries/README.md) and its linked
headers. For storage formats and write behavior, see
[MODELING.md](MODELING.md) and [IMPLEMENTATION.md](IMPLEMENTATION.md).
