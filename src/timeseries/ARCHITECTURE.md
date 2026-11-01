---
Author: "ThemisDB Team"
Created: "2026-09-09"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/timeseries/tsstore.h; include/timeseries/ts_auto_buffer.h; src/timeseries/tsstore.cpp; src/timeseries/ts_auto_buffer.cpp"
---

> Exact model backend not verifiable from final document text alone.

# Architecture - Timeseries Module

<!-- Source inventory reviewed: 2026-10-31; implementation contracts remain header-specific -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PHASE_6_ACCEPTANCE_CHECKLIST.md · PERFORMANCE_BASELINE.md · OPERATOR_GUIDE.md -->

## Overview

The timeseries module composes high-frequency ingest and storage behavior, compression and adaptive flush behavior, aggregation and query optimization behavior, and retention/encryption/remote-write lifecycle behavior into a bounded subsystem.

## Main Execution Planes

1. Ingest and storage plane
- timeseries API, TSStore, hypertable, and chunk storage behavior

2. Compression and query plane
- Gorilla codec, adaptive flush, query optimization, and range/downsampling behavior

3. Lifecycle and integration plane
- retention, aggregation scheduling, encryption, metrics, and remote-write behavior

## Contract areas

| Contract area | Source of contract |
|---|---|
| ingest and storage | `include/timeseries/tsstore.h`, `ts_auto_buffer.h` |
| query and aggregation | `query_optimizer.h`, `continuous_agg.h`, `downsampling.h` |
| retention and encryption | `retention.h`, `encrypted_chunk_store.h`, `ts_encrypted_key_rotation.h` |
| integrations and diagnostics | `prometheus_remote_write.h`, `timeseries_metrics.h`, `timeseries_incident_taxonomy.h` |

## Failure semantics

Error types and failure handling are component-specific. Check the return type,
documented error cases, and implementation of the operation being changed;
this architecture overview makes no blanket guarantee that every failure is
reported, retried, or recoverable.

## Source inventory checked

The following verification list is a partial inventory from the earlier
2026-09-09 review, not evidence that tests or builds passed:

- Verified files:
  - src/timeseries/timeseries.cpp
  - src/timeseries/tsstore.cpp
  - src/timeseries/gorilla.cpp
  - src/timeseries/ts_auto_buffer.cpp
  - src/timeseries/ts_auto_buffer_adaptive.cpp
  - src/timeseries/continuous_agg.cpp
  - src/timeseries/downsampling.cpp
  - src/timeseries/query_optimizer.cpp
  - src/timeseries/retention.cpp
  - src/timeseries/encrypted_chunk_store.cpp
  - src/timeseries/prometheus_remote_write.cpp
- Verified architecture claims:
  - ingest/storage + compression/query + lifecycle/integration plane split
  - explicit failure boundaries for flush, query, retention, and remote-write faults
  - module-local ownership of timeseries behavior

## Data and dependency boundaries

- `TSStore` receives a non-owning RocksDB `TransactionDB` pointer and an
  optional column-family handle. Its individual-point and Gorilla batch
  formats are implemented in `tsstore.cpp`.
- With Gorilla configured, an attached `TSAutoBuffer` can accept and batch
  single-point writes. The buffer holds a non-owning pointer to `TSStore`, and
  the store keeps a non-owning pointer to the buffer. Callers must manage
  lifetimes and stop background work before destroying dependencies.
- Encryption is optional and is attached to `TSStore` through
  `EncryptedChunkStore`; key providers and audit callbacks are supplied by the
  caller.
- The module includes a payload parser for Prometheus remote-write. The parser
  does not establish an HTTP route, server authorization, or transport policy.
- This document does not assert server integration, module-wide concurrency
  guarantees, MVCC behavior, or a specific locking strategy. Consult each
  component's public header and implementation for those contracts.

## Related documentation

- [Public-header architecture](../../include/timeseries/ARCHITECTURE.md)
- [Implementation guide](IMPLEMENTATION.md)
- [Operations](OPERATIONS.md) and [trust boundaries](TRUST.md)