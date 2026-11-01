---
Author: ThemisDB Contributors
Created: 2026-08-18
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: security
Canonical_References:
  - ../../include/replication/replication_manager.h
  - ../../include/replication/raft_v2.h
  - ../../include/replication/replication_slot.h
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/lag_alert_manager.h
  - ../../tests/CMakeLists.txt
  - ../../tests/replication/
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication production requirements: evidence boundary

This page is a draft checklist of topics that deployment owners should review. It is
not a validated production standard or proof that the implementation enforces each
item. Statements previously phrased as mandatory implementation guarantees are not
repeated here without source or owner evidence.

## What the public API confirms

- `ReplicationConfig` declares election/heartbeat/replication timing, WAL, quorum,
  failover, read preference, TLS-path, and `require_mtls` fields.
- `AsyncWalShipper` declares a bounded queue, `false` on a full queue, configurable lag
  threshold, injected ship handler, and `stop()`.
- `LagAlertManager` declares configurable alert/critical/failover thresholds and
  caller-invoked evaluation.
- Slot APIs declare state persistence and LSN progress operations.
- `RaftV2ClusterConfig` exposes joint-consensus membership and quorum methods.

Declaration presence does not prove that a value is parsed from production config,
validated at startup, or honored in all integrations. A user-facing config example
for the server-side sharding WAL shipper is distinct from these module APIs.

## Deployment-owner checklist (verify per deployment)

- [ ] Confirm peer authentication, transport encryption, credential storage, and
  rotation for the concrete transport handler.
- [ ] Confirm caller authorization for member changes, promotion/failover, and slot
  controls.
- [ ] Select and validate timeout, lag, queue, and resource limits for topology and
  deployment conditions; API defaults are not automatically production values.
- [ ] Define how queue-full, transport failure, lag alerts, and quorum loss affect
  caller-visible write success.
- [ ] Validate slot persistence, WAL retention, restart/replay, and recovery procedures.
- [ ] Confirm monitoring names, labels, endpoint exposure, and alert ownership.
- [ ] Identify the exact configured build/test targets and collect execution evidence.

## Open verification items

- **TODO:** Identify a source-backed lock hierarchy per component. The earlier global
  three-level “Manager → Resource → I/O” rule and lock timing budgets are not
  substantiated by the reviewed public APIs as universal requirements.
- **TODO:** Locate and confirm the claimed focused lock-ordering test; the former
  `test_replication_lock_ordering_focused.cpp` path is not in the current
  `tests/replication/` inventory.
- **TODO:** Verify `file_io_timeout_ms` and other timeout fields against all blocking
  I/O paths; a struct field alone does not establish a bounded operation.
- **TODO:** Have the security/operations owners approve deployment requirements and
  escalation procedures.

See [SECURITY.md](SECURITY.md), [OPERATIONS.md](OPERATIONS.md), and
[AUDIT.md](AUDIT.md) for the corresponding source gaps. No production audit, build, or
test run is claimed for this update.
