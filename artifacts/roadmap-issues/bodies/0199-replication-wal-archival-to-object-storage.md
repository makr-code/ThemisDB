### Context

This issue implements the roadmap item 'WAL Archival to Object Storage' for the replication domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: WAL Archival to Object Storage

### Goal

Deliver the scoped changes for WAL Archival to Object Storage in src/replication/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### WAL Archival to Object Storage
**Priority:** Medium  
**Target Version:** v1.6.0

Archive old WAL segments to cloud object storage (S3, GCS, Azure Blob) for long-term retention and cost optimization.

**Features:**
- Automatic archival of old segments
- Configurable retention policy
- On-demand retrieval for PITR
- Encryption at rest in object storage
- Lifecycle management (transition to glacier/cold storage)

**Architecture:**
```cpp
class WALArchivalManager {
public:
    struct ArchivalConfig {
        std::string storage_type;  // "s3", "gcs", "azure"
        std::string bucket_name;
        std::string prefix;
        
        // Archival policy
        uint32_t archive_after_segments = 100;
        uint32_t local_retention_segments = 10;
        bool compress_before_upload = true;
        bool encrypt_at_rest = true;
        
        // Lifecycle
        uint32_t transition_to_cold_after_days = 90;
        uint32_t delete_after_days = 365;
    };
    
    explicit WALArchivalManager(const ArchivalConfig& config);
    
    // Archive old segments
    void archiveSegments(const std::vector<std::string>& segment_paths);
    
    // Retrieve archived segment
    std::optional<std::vector<uint8_t>> retrieveSegment(uint64_t segment_id);
    
    // List archived segments
    struct ArchivedSegment {
        uint64_t segment_id;
        uint64_t start_sequence;
        uint64_t end_sequence;
        uint64_t size_bytes;
        std::chrono::system_clock::time_point archived_at;
        std::string storage_tier;  // "standard", "cold", "glacier"
    };
    std::vector<ArchivedSegment> listArchived() const;
};

// Example: Archive to S3
ArchivalConfig s3_config;
s3_config.storage_type = "s3";
s3_config.bucket_name = "themisdb-wal-archive";
s3_config.prefix = "prod-cluster/";
s3_config.archive_after_segments = 50;
s3_config.compress_before_upload = true;
s3_config.transition_to_cold_after_days = 30;

WALArchivalManager archival(s3_config);

// Background archival process
while (true) {
    auto old_segments = wal_manager.getArchivedSegments();
    archival.archiveSegments(old_segments);
    std::this_thread::sleep_for(std::chrono::hours(1));
}

// Point-in-time recovery from archive
auto segment = archival.retrieveSegment(12345);
if (segment) {
    wal_manager.restoreSegment(12345, *segment);
}
```

**Cost Savings:**
- 90% reduction in local storage costs
- S3 Standard: $0.023/GB/month
- S3 Glacier: $0.004/GB/month
- S3 Deep Archive: $0.00099/GB/month

---

### Acceptance Criteria

- [ ] Automatic archival of old segments
- [ ] Configurable retention policy
- [ ] On-demand retrieval for PITR
- [ ] Encryption at rest in object storage
- [ ] Lifecycle management (transition to glacier/cold storage)
- [ ] 90% reduction in local storage costs
- [ ] S3 Standard: $0.023/GB/month
- [ ] S3 Glacier: $0.004/GB/month
- [ ] S3 Deep Archive: $0.00099/GB/month

### Relationships

- Roadmap row: #199 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/replication/FUTURE_ENHANCEMENTS.md#wal-archival-to-object-storage
- Source key: roadmap:199:replication:v1.6.0:wal-archival-to-object-storage

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:199:replication:v1.6.0:wal-archival-to-object-storage -->
<!-- roadmap-ref: row=199;module=replication;target=v1.6.0 -->
<!-- roadmap-detail: src/replication/FUTURE_ENHANCEMENTS.md#wal-archival-to-object-storage -->
