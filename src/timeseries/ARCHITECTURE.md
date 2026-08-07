# Architecture - Timeseries Module

<!-- Status: current | validated: 2026-08-07 -->
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

## Core Contracts

| Contract | Behavior |
|---|---|
| ingest contract | deterministic write, batch, and flush behavior |
| query contract | bounded range, aggregation, and downsampling behavior |
| lifecycle contract | explicit retention and encryption transition behavior |
| integration contract | observable remote-write and metrics behavior |

## Failure Semantics

- ingest, compression, and flush faults are explicit.
- query and downsampling failures remain diagnosable and non-silent.
- retention and key-rotation lifecycle faults remain observable.
- remote-write and encrypted chunk failures surface deterministic outcomes.

## Sourcecode Verification (Module: timeseries/architecture)

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