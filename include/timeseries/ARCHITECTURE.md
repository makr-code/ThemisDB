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
Canonical_References: "include/timeseries/; src/timeseries/; tests/timeseries/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries public-header architecture

`include/timeseries/` declares the module's public C++ interfaces. The
implementation is under [`../../src/timeseries/`](../../src/timeseries/);
module-focused tests are under [`../../tests/timeseries/`](../../tests/timeseries/).
Use the declarations and implementation for actual behavior rather than
inferring it from a header's filename or from this inventory.

## Header groups

| Group | Headers |
|---|---|
| Store and partitioning | `timeseries.h`, `tsstore.h`, `hypertable.h` |
| Buffer and flush | `ts_auto_buffer.h`, `ts_auto_buffer_adaptive.h`, `adaptive_flush_controller.h` |
| Compression | `gorilla.h`, `gorilla_simd.h`, `compression_selector.h` |
| Query and derived data | `aggregates.h`, `aggregate_scheduler.h`, `continuous_agg.h`, `downsampling.h`, `gap_fill.h`, `query_optimizer.h`, `ts_stream_cursor.h` |
| Retention and encryption | `retention.h`, `encrypted_chunk_store.h`, `ts_encrypted_key_rotation.h` |
| Integration and diagnostics | `prometheus_remote_write.h`, `timeseries_metrics.h`, `timeseries_api_contract.h`, `timeseries_incident_taxonomy.h`, `ts_edge_case_handler.h`, `ts_operator_diagnostics.h` |
| Anomaly detection | `anomaly_detection.h` |

The inventory contains 26 headers at the time of this review. A header's
presence does not establish that its API is used by every runtime path.

## Contract boundaries

- `TSStore` owns the timeseries point and query contracts. It accepts a
  non-owned RocksDB `TransactionDB`; see `tsstore.h` for storage formats,
  timestamp rules, and error results.
- `putDataPoint()` is an individual write unless an auto-buffer is attached and
  accepts the point. Gorilla batch encoding is used by batch methods when
  configured.
- The auto-buffer holds a non-owning pointer to `TSStore`; callers must order
  object lifetimes and stop background work before destroying dependencies.
- `timeseries.h` declares a separate, simpler `TimeSeriesStore` interface.
  Do not treat it as an alias for `TSStore`.
- The remote-write header parses request payloads; it does not define an HTTP
  endpoint or network security policy.
- Thread-safety and ownership are component-specific and are documented in
  the individual headers. This directory does not promise module-wide
  thread-safety or API stability.

## Related documentation

- [Public API overview](README.md)
- [Canonical implementation architecture](../../src/timeseries/ARCHITECTURE.md)
- [Public-header planning notes](ROADMAP.md) and
  [future enhancements](FUTURE_ENHANCEMENTS.md)
