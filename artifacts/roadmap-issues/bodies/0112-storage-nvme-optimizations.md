### Context

This issue implements the roadmap item 'NVMe Optimizations' for the storage domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: NVMe Optimizations

### Goal

Deliver the scoped changes for NVMe Optimizations in src/storage/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### NVMe Optimizations
**Priority:** High  
**Target Version:** v1.6.0

Leverage NVMe-specific features for better performance.

**Optimizations:**
- **io_uring**: Linux async I/O framework
- **Multi-queue**: Parallel I/O submission
- **Zone namespaces (ZNS)**: Direct control over flash management
- **Direct I/O**: Bypass page cache for predictable latency

**Expected Improvement:** 30-50% lower latency, 2x throughput

---

### Acceptance Criteria

- [ ] **io_uring**: Linux async I/O framework
- [ ] **Multi-queue**: Parallel I/O submission
- [ ] **Zone namespaces (ZNS)**: Direct control over flash management
- [ ] **Direct I/O**: Bypass page cache for predictable latency

### Relationships

- Roadmap row: #112 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#nvme-optimizations
- Source key: roadmap:112:storage:v1.6.0:nvme-optimizations

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:112:storage:v1.6.0:nvme-optimizations -->
<!-- roadmap-ref: row=112;module=storage;target=v1.6.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#nvme-optimizations -->
