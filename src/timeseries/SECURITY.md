# Security - Timeseries Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the timeseries module focuses on deterministic ingest and retention behavior, explicit encrypted-chunk lifecycle handling, bounded remote-write ingest behavior, and observable failure signaling across flush and query paths.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| silent data-loss in ingest/flush paths | explicit ingest and adaptive flush outcomes |
| opaque encrypted chunk/key-rotation failure | diagnosable encrypted chunk and key-rotation behavior |
| malformed or abusive remote-write payloads | explicit remote-write validation and rejection behavior |
| hidden retention/downsampling corruption | observable lifecycle and query-stage failures |

## Implemented Security Controls

- ingest, flush, and query paths expose explicit results.
- encrypted chunk and rotation behavior remains observable.
- remote-write faults remain explicit and bounded.
- retention and aggregation lifecycle failures are diagnosable.

## Security Follow-ups

- broaden fault-injection coverage for encrypted chunk and key-rotation edge cases.
- deepen stress coverage for concurrent adaptive flush and remote-write pressure.
- tighten diagnostics taxonomy across ingest, retention, and remote-write incidents.

## Sourcecode Verification (Module: timeseries/security)

- Verified files:
  - src/timeseries/tsstore.cpp
  - src/timeseries/ts_auto_buffer.cpp
  - src/timeseries/ts_auto_buffer_adaptive.cpp
  - src/timeseries/encrypted_chunk_store.cpp
  - src/timeseries/ts_encrypted_key_rotation.cpp
  - src/timeseries/prometheus_remote_write.cpp
  - src/timeseries/retention.cpp
- Verified controls:
  - explicit ingest/flush and query fault signaling
  - observable encrypted chunk lifecycle behavior
  - deterministic remote-write validation behavior