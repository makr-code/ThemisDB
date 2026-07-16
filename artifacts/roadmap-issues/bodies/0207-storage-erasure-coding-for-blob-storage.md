### Context

This issue implements the roadmap item 'Erasure Coding for Blob Storage' for the storage domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Erasure Coding for Blob Storage

### Goal

Deliver the scoped changes for Erasure Coding for Blob Storage in src/storage/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Erasure Coding for Blob Storage
**Priority:** Medium  
**Target Version:** v1.7.0

Space-efficient redundancy using erasure codes (e.g., Reed-Solomon).

**Encoding Schemes:**
- **RS(10,4)**: 10 data + 4 parity blocks (40% overhead vs 200% for mirroring)
- **RS(6,3)**: 6 data + 3 parity blocks (50% overhead)
- **RS(4,2)**: 4 data + 2 parity blocks (50% overhead, faster)

**Benefits:**
- 50-70% storage savings vs mirroring
- Survives multiple node failures
- Configurable fault tolerance

**Example:**
```cpp
ErasureCodingConfig config;
config.data_blocks = 10;
config.parity_blocks = 4;

ErasureCodingBackend backend(config);
backend.put("blob-123", data);  // Automatically encodes and distributes

// Survives loss of up to 4 blocks
auto result = backend.get("blob-123");  // Reconstructs from available blocks
```

---

### Acceptance Criteria

- [ ] **RS(10,4)**: 10 data + 4 parity blocks (40% overhead vs 200% for mirroring)
- [ ] **RS(6,3)**: 6 data + 3 parity blocks (50% overhead)
- [ ] **RS(4,2)**: 4 data + 2 parity blocks (50% overhead, faster)
- [ ] 50-70% storage savings vs mirroring
- [ ] Survives multiple node failures
- [ ] Configurable fault tolerance

### Relationships

- Roadmap row: #207 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#erasure-coding-for-blob-storage
- Source key: roadmap:207:storage:v1.7.0:erasure-coding-for-blob-storage

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:207:storage:v1.7.0:erasure-coding-for-blob-storage -->
<!-- roadmap-ref: row=207;module=storage;target=v1.7.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#erasure-coding-for-blob-storage -->
