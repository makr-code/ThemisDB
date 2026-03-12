### Context

This issue implements the roadmap item 'Binary Delta Patches' for the updates domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Binary Delta Patches

### Goal

Deliver the scoped changes for Binary Delta Patches in src/updates/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Binary Delta Patches
**Priority:** High  
**Target Version:** v1.6.0

Reduce download size by applying binary diffs instead of full file replacement.

**Features:**
- Binary diff generation (bsdiff/xdelta3)
- Patch verification with checksums
- Fallback to full download if patch fails
- Automatic patch generation in CI/CD
- Compression-friendly delta encoding

**Algorithms:**
```cpp
enum class PatchAlgorithm {
    BSDIFF,     // Best compression, slower
    XDELTA3,    // Fast, good compression
    VCDIFF,     // HTTP-friendly (RFC 3284)
    ZSTD_DICT   // Dictionary-based compression
};
```

**Delta Manifest:**
```cpp
struct DeltaManifest {
    std::string from_version;           // "1.4.0"
    std::string to_version;             // "1.5.0"
    std::vector<FileDelta> deltas;
    
    struct FileDelta {
        std::string path;
        std::string base_hash;          // SHA-256 of base file
        std::string target_hash;        // SHA-256 of target file
        std::string patch_url;          // Download URL for patch
        uint64_t patch_size;            // Patch size
        uint64_t target_size;           // Final file size
        PatchAlgorithm algorithm;
    };
};
```

**Usage:**
```cpp
DeltaUpdateEngine delta_engine;

// Check for delta update
auto delta = delta_engine.findDelta("1.4.0", "1.5.0");
if (delta) {
    LOG_INFO("Delta update available: {} -> {}", delta->from_version, delta->to_version);
    LOG_INFO("Download size: {} MB (vs {} MB full)", 
             delta->total_patch_size / 1024 / 1024,
             delta->total_full_size / 1024 / 1024);
    
    // Apply delta update
    auto result = delta_engine.applyDelta(*delta);
    if (!result.success) {
        LOG_WARN("Delta update failed, falling back to full update");
        engine->applyHotReload("1.5.0");
    }
} else {
    // No delta available, use full update
    engine->applyHotReload("1.5.0");
}
```

**Expected Savings:** 70-90% bandwidth reduction for typical updates

---

### Acceptance Criteria

- [ ] Binary diff generation (bsdiff/xdelta3)
- [ ] Patch verification with checksums
- [ ] Fallback to full download if patch fails
- [ ] Automatic patch generation in CI/CD
- [ ] Compression-friendly delta encoding

### Relationships

- Roadmap row: #126 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/updates/FUTURE_ENHANCEMENTS.md#binary-delta-patches
- Source key: roadmap:126:updates:v1.6.0:binary-delta-patches

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:126:updates:v1.6.0:binary-delta-patches -->
<!-- roadmap-ref: row=126;module=updates;target=v1.6.0 -->
<!-- roadmap-detail: src/updates/FUTURE_ENHANCEMENTS.md#binary-delta-patches -->
