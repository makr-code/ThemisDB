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

# Timeseries Developer Module Guide

This is the navigation and first-change guide for developers working in
`src/timeseries/`. Runtime ownership is split between the public declarations
under `include/timeseries/`, implementation files in this directory, and
focused tests under `tests/timeseries/` plus the top-level `tests/`.

Exact model backend not verifiable from final document text alone.

## Choose the right source

| Change area | Start here |
|---|---|
| Store write/query or on-disk keys | `include/timeseries/tsstore.h`, `tsstore.cpp`, then storage tests |
| Buffering/flush behavior | `include/timeseries/ts_auto_buffer.h`, `ts_auto_buffer.cpp`, `ts_auto_buffer_adaptive.cpp` |
| Compression | `include/timeseries/gorilla.h`, `gorilla.cpp`, `gorilla_simd.cpp` |
| Rollups and refresh | `include/timeseries/continuous_agg.h`, `downsampling.h`, corresponding implementation and materialization tests |
| Retention | `include/timeseries/retention.h`, `retention.cpp`, retention tests |
| Encryption/key lifecycle | `encrypted_chunk_store.cpp`, `ts_encrypted_key_rotation.cpp`, and encryption-focused hardening tests |
| Prometheus payload parsing | `include/timeseries/prometheus_remote_write.h`, `prometheus_remote_write.cpp`, `tests/test_prometheus_remote_write.cpp` |
| Metrics/incidents | `timeseries_metrics.cpp`, `ts_operator_diagnostics.cpp`, `timeseries_incident_taxonomy.cpp` |

## Safe change sequence

1. Read the relevant public header and the matching implementation before
   editing. Keep time units and pointer ownership explicit.
2. Identify persisted formats or background-thread lifetimes that the change
   can affect. Do not assume a compression or encryption option is enabled
   unless the caller attaches/configures it.
3. Add or update a focused test at the implementation boundary. The named
   contract-hardening tests include local mocks, so use storage-backed tests
   for concrete `TSStore` behavior.
4. Build and run the relevant focused target using the repository's configured
   CMake preset. The timeseries focused-test CMake file derives targets from
   `tests/timeseries/test_*.cpp`; inspect the configured build's target list
   when selecting the exact target.
5. Run the documentation metadata test when docs change:

   ```bash
   python -m unittest discover -s tests -p "test_doc_metadata_gate.py"
   ```

## Important edge cases

- The timestamp unit in concrete `TSStore` APIs is milliseconds, while the
  separate frozen contract header says nanoseconds. Ordering semantics also
  differ; see [API.md](API.md).
- Batch Gorilla chunks retain tags and metadata from the first point in each
  metric/entity group.
- Full non-blocking buffer pushes take a direct-write fallback in
  `TSStore::putDataPoint()`; callers should not assume the fallback remains
  compressed.
- Retention deletes data; no module-level undelete path is defined.
- Continuous aggregate/downsampling refresh has synchronization constraints
  documented in the public headers.

## Related documentation

- [Architecture](ARCHITECTURE.md) and [existing overview](README.md)
- [Implementation](IMPLEMENTATION.md), [API guide](API.md), and
  [data model](MODELING.md)
- [Research and evaluation](RESEARCH.md) and
  [embedding boundary](EMBEDDING.md)
- [Operations](OPERATIONS.md) and [incident runbook](RUNBOOK.md)
- [Trust model](TRUST.md) and [security scope](SECURITY.md)
- [Roadmap](ROADMAP.md), [production requirements](PRODUCTION_REQUIREMENTS.md),
  and [performance baseline](PERFORMANCE_BASELINE.md)
