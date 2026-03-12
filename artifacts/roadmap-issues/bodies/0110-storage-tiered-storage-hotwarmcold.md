### Context

This issue implements the roadmap item 'Tiered Storage (Hot/Warm/Cold)' for the storage domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Tiered Storage (Hot/Warm/Cold)

### Goal

Deliver the scoped changes for Tiered Storage (Hot/Warm/Cold) in src/storage/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Tiered Storage (Hot/Warm/Cold)
**Priority:** High  
**Target Version:** v1.6.0

Automatic data migration based on access patterns.

**Tiers:**
- **Hot**: NVMe SSDs (frequent access)
- **Warm**: SATA SSDs (moderate access)
- **Cold**: Object storage (rare access, archival)

**Policies:**
- Age-based: Move data older than N days to warm/cold
- Access-based: Move rarely accessed data to cold tier
- Size-based: Move large blobs to object storage

**Configuration:**
```cpp
TieredStorageConfig config;
config.hot_tier_path = "/nvme/data";
config.warm_tier_path = "/sata/data";
config.cold_tier_backend = "s3://archive-bucket";
config.hot_to_warm_days = 30;
config.warm_to_cold_days = 90;

TieredStorageManager tiered(config);
```

---

### Acceptance Criteria

- [ ] **Hot**: NVMe SSDs (frequent access)
- [ ] **Warm**: SATA SSDs (moderate access)
- [ ] **Cold**: Object storage (rare access, archival)
- [ ] Age-based: Move data older than N days to warm/cold
- [ ] Access-based: Move rarely accessed data to cold tier
- [ ] Size-based: Move large blobs to object storage

### Relationships

- Roadmap row: #110 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#tiered-storage-hotwarmcold
- Source key: roadmap:110:storage:v1.6.0:tiered-storage-hotwarmcold

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:110:storage:v1.6.0:tiered-storage-hotwarmcold -->
<!-- roadmap-ref: row=110;module=storage;target=v1.6.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#tiered-storage-hotwarmcold -->
