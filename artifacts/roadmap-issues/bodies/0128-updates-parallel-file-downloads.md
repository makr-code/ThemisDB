### Context

This issue implements the roadmap item 'Parallel File Downloads' for the updates domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Parallel File Downloads

### Goal

Deliver the scoped changes for Parallel File Downloads in src/updates/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Parallel File Downloads
**Priority:** High  
**Target Version:** v1.6.0

Download multiple files concurrently to reduce update time.

**Features:**
- Configurable concurrency level
- Bandwidth throttling
- Priority queue for critical files
- Resume support per file

**Implementation:**
```cpp
ParallelDownloader downloader;
downloader.setConcurrency(4);           // 4 parallel downloads
downloader.setBandwidthLimit(100_MB);   // 100 MB/s total

// Download manifest files
std::vector<DownloadTask> tasks;
for (const auto& file : manifest.files) {
    tasks.push_back({
        .url = file.download_url,
        .dest = config.download_directory + "/" + file.path,
        .expected_hash = file.sha256_hash,
        .priority = file.type == "executable" ? 10 : 1
    });
}

auto results = downloader.downloadAll(tasks);
```

**Expected Improvement:** 3-5x faster downloads (network bound)

---

### Acceptance Criteria

- [ ] Configurable concurrency level
- [ ] Bandwidth throttling
- [ ] Priority queue for critical files
- [ ] Resume support per file

### Relationships

- Roadmap row: #128 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/updates/FUTURE_ENHANCEMENTS.md#parallel-file-downloads
- Source key: roadmap:128:updates:v1.6.0:parallel-file-downloads

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:128:updates:v1.6.0:parallel-file-downloads -->
<!-- roadmap-ref: row=128;module=updates;target=v1.6.0 -->
<!-- roadmap-detail: src/updates/FUTURE_ENHANCEMENTS.md#parallel-file-downloads -->
