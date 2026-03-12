### Context

This issue implements the roadmap item 'Raft Snapshot Compaction and Log Truncation' for the sharding domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: [ ] Raft Snapshot Compaction and Log Truncation

### Goal

Deliver the scoped changes for Raft Snapshot Compaction and Log Truncation in src/sharding/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### [ ] Raft Snapshot Compaction and Log Truncation
**Priority:** High
**Target Version:** v0.9.0

Implement automated Raft log snapshot compaction in `raft_log.cpp` and `raft_wal_integration.cpp` to prevent unbounded WAL growth. Snapshots are compressed with ZSTD (`utils/zstd_codec.cpp`) and stored via `metadata_snapshot.cpp`, then transferred to lagging replicas via `wal_shipper.cpp`.

**Implementation Notes:**
- Add `RaftSnapshotManager` to `raft_log.cpp` that triggers compaction when log size exceeds a configurable threshold (default 512 MB); store snapshot index and term in `raft_state.cpp`.
- Use `utils/zstd_codec.cpp` for snapshot compression; target compression ratio >3× for typical metadata payloads.
- `wal_shipper.cpp` must support chunked snapshot transfer with checksums to tolerate network interruption during lagging-replica catch-up.
- `paxos_snapshot.cpp` and `paxos_wal.cpp` must receive equivalent snapshot compaction support for parity with the Raft path.

**Performance Targets:**
- Snapshot creation time for 1 GB Raft state: <10 s.
- Compressed snapshot size: <35% of uncompressed (ZSTD level 3).
- Lagging replica catch-up via snapshot transfer: >200 MB/s on 10 GbE LAN.

---

### Acceptance Criteria

- [ ] Add `RaftSnapshotManager` to `raft_log.cpp` that triggers compaction when log size exceeds a configurable threshold (default 512 MB); store snapshot index and term in `raft_state.cpp`.
- [ ] Use `utils/zstd_codec.cpp` for snapshot compression; target compression ratio >3× for typical metadata payloads.
- [ ] `wal_shipper.cpp` must support chunked snapshot transfer with checksums to tolerate network interruption during lagging-replica catch-up.
- [ ] `paxos_snapshot.cpp` and `paxos_wal.cpp` must receive equivalent snapshot compaction support for parity with the Raft path.
- [ ] Snapshot creation time for 1 GB Raft state: <10 s.
- [ ] Compressed snapshot size: <35% of uncompressed (ZSTD level 3).
- [ ] Lagging replica catch-up via snapshot transfer: >200 MB/s on 10 GbE LAN.

### Relationships

- Roadmap row: #108 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/sharding/FUTURE_ENHANCEMENTS.md#-raft-snapshot-compaction-and-log-truncation
- Source key: roadmap:108:sharding:v1.6.0:raft-snapshot-compaction-and-log-truncation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:108:sharding:v1.6.0:raft-snapshot-compaction-and-log-truncation -->
<!-- roadmap-ref: row=108;module=sharding;target=v1.6.0 -->
<!-- roadmap-detail: src/sharding/FUTURE_ENHANCEMENTS.md#-raft-snapshot-compaction-and-log-truncation -->
