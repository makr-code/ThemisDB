### Context

This issue implements the roadmap item 'Compressed Replication' for the replication domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Compressed Replication

### Goal

Deliver the scoped changes for Compressed Replication in src/replication/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Compressed Replication
**Priority:** Medium  
**Target Version:** v1.6.0

Compress replication streams to reduce bandwidth usage, especially for cross-region replication.

**Features:**
- Multiple compression algorithms (LZ4, Zstd, Snappy)
- Adaptive compression based on data characteristics
- Configurable compression level
- Compression statistics and monitoring

**Architecture:**
```cpp
class CompressedReplicationStream {
public:
    enum CompressionAlgorithm {
        NONE,
        LZ4,        // Fast, moderate compression
        ZSTD,       // Best compression ratio
        SNAPPY,     // Very fast, low compression
        AUTO        // Automatically select based on data
    };
    
    struct CompressionConfig {
        CompressionAlgorithm algorithm = AUTO;
        int compression_level = 3;  // 1-9
        bool adaptive = true;
        uint32_t min_batch_size = 1024;  // Only compress batches >= 1KB
    };
    
    CompressedReplicationStream(
        const std::string& endpoint,
        const CompressionConfig& config
    );
    
    // Send compressed batch
    bool sendBatch(const std::vector<WALEntry>& entries);
    
    // Get compression statistics
    struct CompressionStats {
        uint64_t bytes_uncompressed;
        uint64_t bytes_compressed;
        double compression_ratio;
        std::string algorithm_used;
    };
    CompressionStats getStats() const;
};

// Example: Cross-region replication with compression
CompressionConfig comp_config;
comp_config.algorithm = CompressionAlgorithm::ZSTD;
comp_config.compression_level = 6;

CompressedReplicationStream stream("eu-west-1:7000", comp_config);
stream.sendBatch(wal_entries);

auto stats = stream.getStats();
std::cout << "Saved " << (stats.bytes_uncompressed - stats.bytes_compressed) 
          << " bytes (" << stats.compression_ratio << "x)" << std::endl;
```

**Compression Ratios (typical):**
- JSON documents: 5-10x with Zstd
- Binary data: 1.5-3x
- Already compressed data: ~1x (minimal benefit)

**Network Bandwidth Savings:**
- Cross-region: 80-90% reduction
- Local network: 60-80% reduction

---

### Acceptance Criteria

- [ ] Multiple compression algorithms (LZ4, Zstd, Snappy)
- [ ] Adaptive compression based on data characteristics
- [ ] Configurable compression level
- [ ] Compression statistics and monitoring
- [ ] JSON documents: 5-10x with Zstd
- [ ] Binary data: 1.5-3x
- [ ] Already compressed data: ~1x (minimal benefit)
- [ ] Cross-region: 80-90% reduction
- [ ] Local network: 60-80% reduction

### Relationships

- Roadmap row: #197 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/replication/FUTURE_ENHANCEMENTS.md#compressed-replication
- Source key: roadmap:197:replication:v1.6.0:compressed-replication

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:197:replication:v1.6.0:compressed-replication -->
<!-- roadmap-ref: row=197;module=replication;target=v1.6.0 -->
<!-- roadmap-detail: src/replication/FUTURE_ENHANCEMENTS.md#compressed-replication -->
