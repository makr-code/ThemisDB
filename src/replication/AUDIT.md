---
Author: ThemisDB Contributors
Created: 2026-05-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - ../../include/replication/replication_manager.h
  - ../../include/replication/async_wal_shipper.h
  - replication_manager.cpp
  - async_wal_shipper.cpp
  - logical_replication.cpp
  - ../../tests/CMakeLists.txt
  - ../../tests/replication/CMakeLists.txt
  - ../../benchmarks/replication/CMakeLists.txt
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication documentation/source review

This is a source inventory, not a test, build, security, or production-readiness
attestation. Earlier audit snapshots in this file's history and related gap reports are
not current evidence.

## Confirmed inventory (2026-10-31)

- `include/replication/` contains 18 public headers.
- `src/replication/` contains 13 implementation `.cpp` files.
- The public surfaces include manager/Raft-v2, logical replication and slots, async WAL
  shipping and lag alerts, placement, conflict resolution, event streams, policy,
  observability, tiering, schema CDC, and additional declared interfaces.
- The root `tests/CMakeLists.txt` has explicit replication test registrations.
  `tests/replication/CMakeLists.txt` also contains module-local registration logic; its
  inclusion by every active configure path was not established in this review.
- `benchmarks/replication/CMakeLists.txt` registers `bench_*.cpp` files when
  `THEMIS_BUILD_BENCHMARKS` is enabled.

## Gaps requiring follow-up

- **TODO:** Determine whether `kafka_change_stream.h` and
  `multi_master_replication.h` are implemented or wired outside `src/replication/`.
- **TODO:** Verify the supported YAML/configuration mapping for the module's C++
  `ReplicationConfig`, `WalShippingConfig`, and `SLOThresholds`. The server-side
  `shipper_enabled` configuration path is a separate sharding WAL shipper path.
- **TODO:** Reconcile the historical gap counts and completion claims in
  `MODULE_GAPS.md`, `MODULE_GAPS_BATCH4.md`, and older roadmap evidence with current
  code and a reproducible scanner run.
- **TODO:** Execute the relevant configured build and focused tests in a separate
  validation task. No build, test, or benchmark was run for this documentation review.

## References

- [Module overview](README.md) · [API](API.md) · [Architecture](ARCHITECTURE.md)
- [Module roadmap/backlog](ROADMAP.md) · [Future enhancements](FUTURE_ENHANCEMENTS.md)
