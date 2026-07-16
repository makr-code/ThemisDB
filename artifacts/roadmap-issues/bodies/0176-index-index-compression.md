### Context

This issue implements the roadmap item 'Index Compression' for the index domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Index Compression

### Goal

Deliver the scoped changes for Index Compression in src/index/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Index Compression
**Priority:** Medium  
**Target Version:** v1.7.0

Reduce index storage footprint via compression.

**Techniques:**
- **Delta Encoding**: Store differences between adjacent keys
- **Prefix Compression**: Share common key prefixes
- **Bloom Filters**: Reduce false lookups (already in RocksDB)
- **Dictionary Encoding**: Map frequent strings to small integers
- **Run-Length Encoding**: Compress repeated values

**Example:**
```cpp
// Without compression
idx:users:country:USA:pk1
idx:users:country:USA:pk2
idx:users:country:USA:pk3

// With prefix compression
idx:users:country:USA:[pk1, pk2, pk3]

// 60% size reduction typical
```

**API:**
```cpp
SecondaryIndexManager::Config config;
config.enable_compression = true;
config.compression_algorithm = CompressionAlgorithm::ZSTD;
config.compression_level = 3;  // Balance: speed vs ratio

SecondaryIndexManager sim(db, config);
```

---

### Acceptance Criteria

- [ ] **Delta Encoding**: Store differences between adjacent keys
- [ ] **Prefix Compression**: Share common key prefixes
- [ ] **Bloom Filters**: Reduce false lookups (already in RocksDB)
- [ ] **Dictionary Encoding**: Map frequent strings to small integers
- [ ] **Run-Length Encoding**: Compress repeated values

### Relationships

- Roadmap row: #176 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/index/FUTURE_ENHANCEMENTS.md#index-compression
- Source key: roadmap:176:index:v1.7.0:index-compression

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:176:index:v1.7.0:index-compression -->
<!-- roadmap-ref: row=176;module=index;target=v1.7.0 -->
<!-- roadmap-detail: src/index/FUTURE_ENHANCEMENTS.md#index-compression -->
