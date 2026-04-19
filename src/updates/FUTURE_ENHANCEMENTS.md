<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Updates Module - Future Enhancements

- Zero-downtime hot-reload via atomic file replacement with fsync and all-or-nothing semantics (`HotReloadEngine`)
- CMS/PKCS#7 signature validation and X.509 certificate chain verification before applying any binary update
- Binary delta updates to reduce download bandwidth; resume-capable downloads from GitHub releases
- Canary rollout and blue/green deployment for staged, node-by-node production updates
- In-place schema migration (additive changes without data copy) with full rollback via versioned restore points
- Multi-node coordinated updates with replication-safe sequencing (`CoordinatedUpdateManager`)
- Pre-flight health checks: disk space, memory, and dependency version verification before applying any update

## Design Constraints

- [ ] Every update bundle must carry a valid CMS/PKCS#7 signature; unsigned bundles are rejected before any file is written to disk
- [ ] Atomic file replacement must use `rename(2)` (POSIX) or `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` (Windows) after `fsync`
- [ ] Rollback restore points must be created before any file modification; update must abort if backup creation fails
- [ ] `isSafePath` must be called on every path extracted from an update bundle to prevent path traversal attacks
- [ ] Canary rollout fraction is configurable; promotion to 100% requires explicit operator approval or automated health gate pass
- [ ] Schema migrations must be idempotent: re-running the same migration version must produce the same result without error
- [ ] Concurrent update prevention must use a filesystem lock; cross-node coordination handled by `CoordinatedUpdateManager`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IHotReloadEngine` | Update orchestrator, CLI | apply, rollback, listRollbackPoints, dryRun |
| `ISignatureValidator` | `HotReloadEngine`, `DeltaUpdateEngine` | CMS/PKCS#7 + X.509 chain verification; fail-closed |
| `IDeltaUpdateEngine` | Update orchestrator | generate, apply, verify (SHA-256 hash check post-apply) |
| `ISchemaMigrator` | Schema migration framework | apply, rollback, getVersion, getHistory; idempotent |
| `ICoordinatedUpdateManager` | Multi-node update sequencing | Transport-agnostic via injected callbacks; replication-safe |
| `INotificationWebhook` | Update event system | Slack/PagerDuty HTTP POST with injectable `HttpSendFunc` |
| `IHealthCheck` | Pre-flight check system | Disk space, memory, dependency version; completes in ≤ 2 s |

## Planned Features

### `ManifestDatabase`: Delete Associated Files on Entry Removal
**Priority:** Medium
**Target Version:** v1.8.0

`manifest_database.cpp` line 479: "TODO: Delete associated files from registry". When a manifest entry is removed, the associated binary files are not cleaned up from the registry directory, causing accumulation of orphaned files.

**Implementation Notes:**
- `[x]` In `ManifestDatabase::deleteManifest()`, after removing the RocksDB manifest record, enumerate associated file paths from the entry metadata and call `std::filesystem::remove()` for each.
- `[x]` Guard against race: delete files only after the RocksDB entry is committed; use a tombstone key during the deletion window.
- `[x]` Add test: insert manifest entry with 3 associated files, remove entry, verify all 3 files are deleted.

---


**Priority:** High
**Target Version:** v1.7.0

Coordinate updates across all nodes in a ThemisDB cluster with Raft consensus.

**Features:**
- Raft-based consensus for cluster-wide updates
- Rolling updates (update one node at a time)
- Automatic health checks before/after updates
- Abort on failure with automatic rollback
- Version skew protection (max 1 minor version difference)
- Leader election for update coordination

**API:**
```cpp
ClusterUpdateManager cluster_updates(raft_manager);

// Initiate cluster-wide update
ClusterUpdateResult result = cluster_updates.updateCluster("1.7.0", {
    .rolling = true,                    // Rolling update
    .max_unavailable = 1,               // Max nodes down at once
    .health_check_timeout = 30s,        // Health check timeout
    .rollback_on_failure = true,        // Auto-rollback on failure
    .parallel_updates = false           // Sequential updates
});

// Monitor progress
cluster_updates.setProgressCallback([](const ClusterUpdateProgress& progress) {
    std::cout << "Updated: " << progress.nodes_updated << "/" << progress.total_nodes << "\n";
    std::cout << "Current node: " << progress.current_node << "\n";
    std::cout << "Status: " << progress.status << "\n";
});

// Wait for completion
if (result.wait()) {
    LOG_INFO("Cluster updated successfully");
} else {
    LOG_ERROR("Cluster update failed: {}", result.error_message);
}
```

**Rolling Update Procedure:**
```
1. Elect update coordinator (Raft leader)
2. For each node (excluding leader):
   a. Drain connections
   b. Download and verify update
   c. Backup current version
   d. Apply update
   e. Restart node
   f. Health check
   g. Rejoin cluster
3. Update leader last
4. Verify cluster health
```

**Use Cases:**
- Zero-downtime cluster upgrades
- Coordinated schema migrations
- Automatic failover during updates

---


## Distributed Cluster Updates ✅ IMPLEMENTED (v1.7.0)
<!-- anchor: distributed-cluster-updates -->
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Released — `include/updates/cluster_update_manager.h`, `src/updates/cluster_update_manager.cpp`

Coordinate updates across all nodes in a ThemisDB cluster with Raft consensus.

**Features:**
- ✅ Rolling (sequential) updates — non-leaders first, leader(s) last
- ✅ Automatic health checks after each node update (`NodeHealthCheckFunc` callback)
- ✅ Injected rollback via `NodeRollbackFunc` callback when `rollback_on_failure=true`
- ✅ Version skew protection — leader node is always updated last
- ✅ Transport-agnostic design via `NodeUpdateFunc` / `NodeHealthCheckFunc` / `NodeRollbackFunc` callbacks
- ✅ Incremental `ClusterUpdateProgress` callbacks for monitoring
- ✅ Cancellation support via `cancelUpdate()`

**API:**
```cpp
ClusterUpdateManager::Config cfg;
cfg.nodes = {
    { "node-a", "host-a:6543", false, "1.6.0" },
    { "node-b", "host-b:6543", false, "1.6.0" },
    { "node-c", "host-c:6543", true,  "1.6.0" },  // Raft leader — updated last
};
cfg.default_options.rollback_on_failure  = true;
cfg.default_options.health_check_timeout = std::chrono::seconds{30};

ClusterUpdateManager cluster_updates(cfg);

// Inject per-node update logic (e.g. gRPC RPC call).
cluster_updates.setNodeUpdateFunc(
    [](const ClusterNode& node, const std::string& version,
       const ClusterUpdateOptions& opts) {
        return my_rpc.updateNode(node.node_id, version);
    });

// Optional: inject per-node health check.
cluster_updates.setNodeHealthCheckFunc(
    [](const ClusterNode& node, std::chrono::seconds timeout) {
        return my_rpc.healthCheck(node.node_id, timeout);
    });

// Optional: inject per-node rollback (called when rollback_on_failure=true).
cluster_updates.setNodeRollbackFunc(
    [](const ClusterNode& node, const std::string& applied_version) {
        return my_rpc.rollbackNode(node.node_id, applied_version);
    });

// Monitor progress.
cluster_updates.setProgressCallback([](const ClusterUpdateProgress& p) {
    std::cout << "Updated: " << p.nodes_updated << "/" << p.total_nodes << "\n";
    std::cout << "Current node: " << p.current_node << "\n";
    std::cout << "Status: " << p.status << "\n";
});

// Initiate cluster-wide update.
ClusterUpdateResult result = cluster_updates.updateCluster("1.7.0");
if (result.success) {
    LOG_INFO("Cluster updated successfully");
} else {
    LOG_ERROR("Cluster update failed: {}", result.error_message);
}
```

**Rolling Update Procedure:**
```
1. Sort nodes: non-leader nodes first, leader(s) last
2. For each node in order:
   a. Mark DRAINING  — emit progress
   b. Invoke NodeUpdateFunc (→ APPLYING)
   c. Record applied_version; invoke NodeHealthCheckFunc (→ HEALTH_CHECK)
   d. On pass: REJOINING → COMPLETED
   e. On fail (rollback_on_failure=true):
      - Invoke NodeRollbackFunc(node, applied_version)
      - Mark ROLLED_BACK; abort remaining nodes
3. Emit final ClusterUpdateProgress
```

**Use Cases:**
- Zero-downtime cluster upgrades
- Coordinated schema migrations
- Automatic failover during updates

---

### Binary Delta Patches ✅ IMPLEMENTED (v1.6.0)
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Released — `include/updates/delta_update_engine.h`, `src/updates/delta_update_engine.cpp`

Reduce download size by applying binary diffs instead of full file replacement.

**Features:**
- ✅ Binary diff generation (bsdiff/xdelta3 — fallback to ZSTD_DICT; VCDIFF pure-C++ implementation)
- ✅ Patch verification with checksums (SHA-256 base_hash / target_hash in FileDelta)
- ✅ Fallback to full download if patch fails (per-file fallback in `DeltaApplyResult::files_fallback`)
- ✅ Automatic patch generation in CI/CD (`generatePatch()` API; `.github/workflows/02-feature-modules_storage_binary-delta-patches-ci.yml`)
- ✅ Compression-friendly delta encoding (ZSTD_DICT dictionary compression + VCDIFF RFC 3284)

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

**References:**
- [13] C. Percival, "Naive Differences of Executable Code," Technical Report, http://www.daemonology.net/bsdiff/, 2003.
- [14] J. Mogul *et al.*, "Delta Encoding in HTTP," IETF RFC 3229, January 2002.
- [15] D. Korn and K.-P. Vo, "VCDIFF: An Open Encoding for Merging, Differencing, and Compression," IETF RFC 3284, June 2002.

### Automatic Schema Migration Framework ✅ IMPLEMENTED (v1.7.0)
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Released — `include/updates/schema_migration.h`, `src/updates/schema_migration.cpp`

Automated schema migration with online DDL (zero-downtime schema changes).

**Features:**
- ✅ Schema versioning and tracking
- ✅ Online DDL (background schema changes)
- ✅ Automatic backfill for new columns
- ✅ Index rebuilding without downtime
- ✅ Dual-write during migration
- ✅ Rollback capability for schema changes

**Migration DSL:**
```cpp
SchemaMigration migration("1.5.0");

// Add column
migration.addColumn("users", {
    .name = "phone_number",
    .type = "VARCHAR(20)",
    .nullable = true,
    .default_value = "NULL"
});

// Rename column
migration.renameColumn("users", "email", "email_address");

// Add index (online)
migration.addIndex("users", {
    .name = "idx_email",
    .columns = {"email_address"},
    .unique = false,
    .build_online = true  // Build in background
});

// Drop column (after grace period)
migration.dropColumn("users", "old_column", {
    .grace_period = std::chrono::hours(24 * 7)  // 7 days
});

// Custom migration logic
migration.addCustomMigration([](MigrationContext& ctx) {
    // Migrate data manually
    auto it = ctx.storage->createIterator("users");
    while (it->valid()) {
        auto data = it->value();
        // Transform data
        ctx.storage->put(it->key(), transformed_data);
        it->next();
    }
    return true;
});

// Apply migration
auto result = migration.apply(storage_engine);
```

**Online DDL Algorithm:**
```
1. Create shadow table with new schema
2. Start dual-write (write to both tables)
3. Background copy old table to shadow table
4. Verify data consistency
5. Atomic swap (rename shadow → main)
6. Drop old table
```

**Rollback Strategy:**
```cpp
// Automatic rollback if migration fails
migration.setRollbackStrategy(RollbackStrategy::AUTOMATIC);

// Manual rollback
if (!migration_result.success) {
    migration.rollback();
}
```

---

### Canary Deployments ✅ IMPLEMENTED (v1.7.0)
**Priority:** Medium
**Target Version:** v1.7.0
**Status:** ✅ Released — `include/updates/canary_rollout.h`, `src/updates/canary_rollout.cpp`

Gradual rollout of updates with automatic rollback on errors.

**Features:**
- Progressive rollout (1% → 5% → 25% → 100%)
- Automatic monitoring of error rates
- Rollback if error rate exceeds threshold
- A/B testing for updates
- Traffic splitting for canary nodes

**Configuration:**
```cpp
CanaryDeployment canary;
canary.setVersion("1.5.0");
canary.setStages({
    {.percentage = 1,   .duration = std::chrono::hours(1)},
    {.percentage = 5,   .duration = std::chrono::hours(2)},
    {.percentage = 25,  .duration = std::chrono::hours(6)},
    {.percentage = 100, .duration = std::chrono::hours(0)}
});

// Set monitoring thresholds
canary.setErrorRateThreshold(0.05);  // 5% error rate
canary.setLatencyThreshold(std::chrono::milliseconds(500));  // 500ms p99 latency

// Start canary deployment
auto result = canary.deploy();

// Monitor progress
canary.onStageComplete([](const CanaryDeploymentStage& stage) {
    LOG_INFO("Stage {} complete: {}% of nodes updated",
             stage.stage_number, stage.percentage);
});

canary.onRollback([](const std::string& reason) {
    LOG_ERROR("Canary deployment rolled back: {}", reason);
    notifyAdmins("Canary rollback: " + reason);
});
```

**Metrics to Monitor:**
- Error rate (HTTP 5xx, exceptions)
- Latency (p50, p95, p99)
- Memory usage
- CPU usage
- Disk I/O
- Custom metrics (query errors, transaction failures)

---

### Dependency Resolution Engine ✅ IMPLEMENTED (v1.6.0)
**Priority:** Medium
**Target Version:** v1.6.0
**Status:** ✅ Released — `include/updates/dependency_resolver.h`, `src/updates/dependency_resolver.cpp`

Automatic resolution of update dependencies with topological sorting.

**Features:**
- ✅ Dependency graph construction
- ✅ Topological sort for correct order
- ✅ Cycle detection
- ✅ Minimum version constraints
- ✅ Conflict resolution
- ✅ Automatic backfill of missing dependencies

**Dependency Format:**
```cpp
struct Dependency {
    std::string package;                // "themis-storage"
    std::string version_constraint;     // ">=1.4.0,<2.0.0"
    bool optional = false;
    std::vector<std::string> conflicts; // Conflicting packages
};
```

**Usage:**
```cpp
DependencyResolver resolver;

// Add dependencies for version 1.5.0
resolver.addDependency("1.5.0", {
    .package = "themis-storage",
    .version_constraint = ">=1.4.0,<2.0.0"
});

resolver.addDependency("1.5.0", {
    .package = "themis-query",
    .version_constraint = ">=1.4.5"
});

// Resolve dependencies
auto resolution = resolver.resolve("1.5.0", current_versions);
if (resolution.success) {
    LOG_INFO("Update plan:");
    for (const auto& step : resolution.steps) {
        LOG_INFO("  {} {} -> {}", step.package, step.from_version, step.to_version);
    }

    // Execute update plan
    for (const auto& step : resolution.steps) {
        engine->applyHotReload(step.to_version);
    }
} else {
    LOG_ERROR("Dependency resolution failed: {}", resolution.error_message);
}
```

**Conflict Resolution:**
```cpp
// Detect conflicts
auto conflicts = resolver.detectConflicts({
    {"themis-storage", "1.5.0"},
    {"themis-query", "1.4.0"}  // Requires themis-storage >= 1.5.1
});

if (!conflicts.empty()) {
    LOG_ERROR("Dependency conflicts:");
    for (const auto& conflict : conflicts) {
        LOG_ERROR("  {} conflicts with {}", conflict.package1, conflict.package2);
    }
}
```

---

### Update Verification Test Suite
**Priority:** Medium
**Target Version:** v1.6.0

Automated testing before applying updates to production.

**Features:**
- Smoke tests (basic functionality)
- Integration tests (API endpoints)
- Performance regression tests
- Schema compatibility tests
- Automatic rollback on test failure

**Test Suite:**
```cpp
UpdateVerifier verifier;

// Add smoke tests
verifier.addSmokeTest("database_connect", []() {
    auto db = connectToDatabase();
    return db && db->isHealthy();
});

verifier.addSmokeTest("execute_query", []() {
    auto result = executeQuery("SELECT 1");
    return result && result->rowCount() == 1;
});

// Add integration tests
verifier.addIntegrationTest("api_health_check", []() {
    auto response = httpGet("http://localhost:8080/health");
    return response.status_code == 200;
});

// Add performance tests
verifier.addPerformanceTest("query_latency", []() {
    auto start = std::chrono::high_resolution_clock::now();
    executeQuery("SELECT * FROM users LIMIT 100");
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    return duration.count() < 100;  // Less than 100ms
});

// Run verification after update
auto update_result = engine->applyHotReload("1.5.0");
if (update_result.success) {
    auto verify_result = verifier.verify();
    if (!verify_result.success) {
        LOG_ERROR("Verification failed: {}", verify_result.error_message);
        LOG_INFO("Rolling back update");
        engine->rollback(update_result.rollback_id);
    }
}
```

---

### Multi-Tenant Update Scheduling ✅ Implemented (v1.8.0, Issue #262)
**Priority:** Low
**Target Version:** v1.8.0
**Status:** Implemented

Per-tenant update schedules and maintenance windows.

**Features:**
- [x] Tenant-specific maintenance windows
- [x] Update blackout periods
- [x] Priority tiers (critical, normal, low)
- [x] Tenant consent for updates
- [x] Rollback per tenant

**Implementation files:**
- `include/updates/tenant_update_scheduler.h`
- `src/updates/tenant_update_scheduler.cpp`
- Tests: 37 focused tests in `tests/test_multi_tenant_update_scheduling.cpp`
- CI: `.github/workflows/multi-tenant-update-scheduling-ci.yml`

**Configuration:**
```cpp
TenantUpdateScheduler scheduler;

// Configure tenant maintenance windows
scheduler.setMaintenanceWindow("tenant-123", {
    .days = {"Saturday", "Sunday"},
    .time_range = {"02:00", "06:00"},
    .timezone = "America/New_York"
});

scheduler.setMaintenanceWindow("tenant-456", {
    .days = {"Daily"},
    .time_range = {"23:00", "05:00"},
    .timezone = "Europe/London"
});

// Set update policy
scheduler.setUpdatePolicy("tenant-123", {
    .auto_update = false,           // Require manual approval
    .critical_auto_update = true,   // Auto-apply critical updates
    .notification_lead_time = std::chrono::hours(24)
});

// Check if update can be applied now
if (scheduler.canUpdateNow("tenant-123")) {
    engine->applyHotReload("1.5.0");
} else {
    auto next_window = scheduler.getNextMaintenanceWindow("tenant-123");
    LOG_INFO("Next maintenance window: {}", next_window);
}
```

---

## Performance Optimizations

### Parallel File Downloads ✅ Implemented (v1.6.0, Issue #128)
**Priority:** High
**Target Version:** v1.6.0
**Status:** Implemented

Download multiple files concurrently to reduce update time.

**Features:**
- [x] Configurable concurrency level (`setConcurrency(n)`)
- [x] Bandwidth throttling (`setBandwidthLimit(bps)` – token-bucket)
- [x] Priority queue for critical files (`DownloadTask::priority`)
- [x] Resume support per file (`DownloadTask::enable_resume` + HTTP Range)

**Implementation files:**
- `include/updates/parallel_downloader.h`
- `src/updates/parallel_downloader.cpp`
- Tests: 29 focused tests in `tests/test_parallel_file_downloads.cpp`
- CI: `.github/workflows/parallel-file-downloads-ci.yml`

**Usage:**
```cpp
ParallelDownloader downloader;
downloader.setConcurrency(4);                         // 4 parallel downloads
downloader.setBandwidthLimit(100ULL * 1024 * 1024);   // 100 MB/s total

// Download manifest files
std::vector<DownloadTask> tasks;
for (const auto& file : manifest.files) {
    tasks.push_back({
        .url           = file.download_url,
        .dest          = config.download_directory + "/" + file.path,
        .expected_hash = file.sha256_hash,
        .priority      = file.type == "executable" ? 10 : 1
    });
}

auto results = downloader.downloadAll(tasks);
```

**Expected Improvement:** 3-5x faster downloads (network bound)

---

### Incremental Manifest Updates
**Priority:** Medium
**Target Version:** v1.7.0

Only download changed parts of manifests to reduce overhead.

**Features:**
- Manifest versioning with ETags
- Partial manifest updates
- Content-based diffing
- Compression-friendly format

**Protocol:**
```
1. Client sends last known manifest hash
2. Server compares with current manifest
3. Server sends only changed entries (delta)
4. Client merges delta with cached manifest
```

**Expected Improvement:** 90% reduction in manifest download size

---

### Background Verification
**Priority:** Medium
**Target Version:** v1.6.0

Verify downloaded files in background while downloading remaining files.

**Features:**
- Pipeline: Download → Verify → Apply
- Overlapping I/O and CPU
- Early failure detection
- Resource-aware scheduling

**Expected Improvement:** 20-30% faster overall update time

---

### Smart Rollback Points
**Priority:** Low
**Target Version:** v1.8.0

Optimize rollback storage with deduplication and compression.

**Features:**
- Hardlinks for unchanged files
- Delta storage for changed files
- Compression for rollback archives
- Automatic cleanup based on age/space

**Space Savings:** 80-90% reduction in rollback storage

---

## Refactoring Opportunities

### Separate Download and Apply Logic
**Priority:** Medium
**Target Version:** v1.7.0

Split HotReloadEngine into separate download and apply components.

**Proposed Structure:**
```cpp
class DownloadEngine {
    Result<DownloadedRelease> download(const std::string& version);
    Result<void> verify(const DownloadedRelease& release);
};

class ApplyEngine {
    Result<ApplyResult> apply(const DownloadedRelease& release);
    Result<void> rollback(const std::string& rollback_id);
};

class HotReloadOrchestrator {
    DownloadEngine downloader_;
    ApplyEngine applier_;

    Result<void> updateToVersion(const std::string& version) {
        auto downloaded = downloader_.download(version);
        auto verified = downloader_.verify(*downloaded);
        auto applied = applier_.apply(*downloaded);
        return applied;
    }
};
```

**Benefits:**
- Easier testing (mock download, test apply logic)
- Reusable components
- Better separation of concerns

---

### Plugin-Based Migration System
**Priority:** Medium
**Target Version:** v1.7.0

Allow custom migration strategies via plugin API.

**Plugin Interface:**
```cpp
class IMigrationPlugin {
public:
    virtual ~IMigrationPlugin() = default;

    virtual std::string name() const = 0;
    virtual std::string version() const = 0;

    virtual bool canHandle(const Migration& migration) = 0;
    virtual Result<void> apply(const Migration& migration) = 0;
    virtual Result<void> rollback(const Migration& migration) = 0;
};
```

**Benefits:**
- Custom migration strategies per application
- Third-party migration tools
- Domain-specific migrations

---

### Unified Update Configuration
**Priority:** Low
**Target Version:** v1.8.0

Merge UpdatesConfig with HotReloadEngine::Config for consistency.

**Proposed:**
```cpp
struct UnifiedUpdateConfig {
    // All settings in one place
    struct Checker { ... };
    struct AutoUpdate { ... };
    struct HotReload { ... };
    struct Notifications { ... };
    struct Advanced { ... };
};
```

---

## Known Issues

### Issue #1: No Verification of Available Disk Space
**Severity:** Medium
**Reported:** v1.5.0

HotReloadEngine doesn't check available disk space before downloading.

**Workaround:** Manually check disk space before update

**Fix:** Add disk space check before download

**Planned Fix:** v1.6.0

---

### Issue #2: Rollback Points Not Cleaned Automatically
**Severity:** Low
**Reported:** v1.5.0

Old rollback points accumulate, consuming disk space.

**Workaround:** Manually call `cleanRollbackPoints()`

**Fix:** Add background cleanup job with configurable retention

**Planned Fix:** v1.6.0

---

### Issue #3: No Progress Resumption After Process Restart
**Severity:** Medium
**Reported:** v1.5.1

If process crashes during update, must restart from beginning.

**Workaround:** Use filesystem locks to detect interrupted updates

**Fix:** Add update state persistence to resume interrupted updates

**Planned Fix:** v1.6.1

---

### Issue #4: Signature Verification Blocks Main Thread
**Severity:** Low
**Reported:** v1.5.0

Large file signature verification can block for seconds.

**Workaround:** Use dry-run mode to verify before actual update

**Fix:** Move signature verification to background thread pool

**Planned Fix:** v1.6.0

---

### Issue #5: No Rate Limiting for GitHub API
**Severity:** Low
**Reported:** v1.5.2

Frequent update checks can hit GitHub API rate limits.

**Workaround:** Increase check interval

**Fix:** Add exponential backoff and rate limit handling

**Planned Fix:** v1.6.0

---

## Research Areas

### Zero-Copy Update Application
**Focus:** Minimize memory copies during file replacement

Explore:
- Memory-mapped I/O for large files
- Direct kernel I/O (O_DIRECT)
- Copy-on-write filesystems (Btrfs, ZFS)
- Reflink support for instant copies

**Research Questions:**
- Can we leverage filesystem features for instant updates?
- What's the performance gain vs compatibility cost?

**References:**
- [1] A. Bellard, "QEMU, a Fast and Portable Dynamic Translator," *USENIX Annual Technical Conference*, 2005. (memory-mapped file replacement)
- [2] T. Ts'o and A. Dilger, "Ext4 File System," *Proceedings of Linux Symposium*, 2009. (reflink / CoW semantics)
- [3] M. Rosenblum and J. K. Ousterhout, "The Design and Implementation of a Log-Structured File System," *ACM Trans. Comput. Syst.*, vol. 10, no. 1, pp. 26–52, 1992.

---

### Blockchain-Based Update Verification
**Focus:** Decentralized update verification

Explore:
- Blockchain-based manifest registry
- Distributed signature verification
- Merkle tree for file integrity
- Smart contracts for update policies

**Research Questions:**
- Can we eliminate central authority for updates?
- What's the performance impact of blockchain verification?

**References:**
- [4] S. Nakamoto, "Bitcoin: A Peer-to-Peer Electronic Cash System," 2008. (Merkle tree integrity)
- [5] N. Szabo, "Smart Contracts," *Extropy*, no. 16, 1994.
- [6] G. Wood, "Ethereum: A Secure Decentralised Generalised Transaction Ledger," *Ethereum Project Yellow Paper*, vol. 151, pp. 1–32, 2014.

---

### Machine Learning for Update Scheduling
**Focus:** Optimal update timing based on historical data

Explore:
- Predict low-traffic periods
- Learn tenant usage patterns
- Minimize user impact
- Adaptive maintenance windows

**Research Questions:**
- Can ML improve update success rates?
- What data do we need to collect?

**References:**
- [7] A. Krause and D. Golovin, "Submodular Function Maximization," *Tractability: Practical Approaches to Hard Problems*, 2014. (optimal scheduling under constraints)
- [8] J. Dean and S. Ghemawat, "MapReduce: Simplified Data Processing on Large Clusters," *Commun. ACM*, vol. 51, no. 1, pp. 107–113, 2008. (workload characterization)
- [9] D. Silver *et al.*, "Mastering the Game of Go with Deep Neural Networks and Tree Search," *Nature*, vol. 529, pp. 484–489, 2016. (reinforcement learning for sequential decisions)

---

### Content-Addressable Update System
**Focus:** Deduplicate files across versions

Explore:
- Content-addressable storage (CAS) for binaries
- Automatic deduplication across versions
- Hash-based file retrieval
- Reduced storage footprint

**Research Questions:**
- Can we reduce storage by 80%+ with CAS?
- How to handle file permissions and metadata?

**References:**
- [10] C. Loki, "Content Addressable Storage," *Linux Journal*, 2003.
- [11] S. Quinlan and S. Dorward, "Venti: A New Approach to Archival Storage," *USENIX Conference on File and Storage Technologies*, 2002.
- [12] A. Muthitacharoen, B. Chen, and D. Mazières, "A Low-Bandwidth Network File System," *Proc. 18th ACM Symp. on Operating Systems Principles (SOSP)*, pp. 174–187, 2001. (chunk-level deduplication)

## Migration Paths

### v1.5.x → v1.6.x: Parallel Downloads
**Breaking Changes:** None (additive)

**New APIs:**
```cpp
ParallelDownloader downloader;
downloader.setConcurrency(4);
```

**Migration Steps:**
1. Update to v1.6.0
2. Configure parallelism (optional)
3. Enjoy faster downloads

**Timeline:** Immediate adoption possible

---

### v1.6.x → v1.7.x: Automatic Schema Migrations
**Breaking Changes:** Schema migration format changes

**Old Format:**
```cpp
// Manual SQL scripts
executeSQL("ALTER TABLE users ADD COLUMN phone VARCHAR(20)");
```

**New Format:**
```cpp
SchemaMigration migration("1.7.0");
migration.addColumn("users", {...});
migration.apply(storage_engine);
```

**Migration Steps:**
1. Convert existing SQL scripts to new DSL
2. Test migrations in staging
3. Deploy to production

**Timeline:** 3 months gradual adoption

---

### v1.7.x → v2.0.x: Distributed Updates
**Breaking Changes:** Update API redesigned for cluster support

**Old API:**
```cpp
engine->applyHotReload("1.7.0");  // Single node
```

**New API:**
```cpp
// Backward compatible (single node)
engine->applyHotReload("2.0.0");

// New (cluster-wide)
cluster_updates->updateCluster("2.0.0");
```

**Migration Steps:**
1. Update to v2.0.0
2. Existing single-node code works unchanged
3. Optionally adopt cluster updates

**Automated Migration Tool:** `scripts/migrate_updates_v2.sh`

**Timeline:** 12 months deprecation period for old API

---

## Community Contributions Welcome

We welcome contributions in the following areas:

### High-Impact, Beginner-Friendly
- [ ] Add disk space check before downloads
- [ ] Automatic rollback point cleanup
- [ ] Better error messages and logging
- [ ] Update status dashboard/web UI

### Medium Complexity
- [ ] Parallel file downloads
- [ ] Delta/binary patches (bsdiff/xdelta3)
- [ ] Background verification during downloads
- [ ] Resume interrupted updates

### Advanced Topics
- [ ] Distributed cluster updates
- [ ] Automatic schema migration framework
- [ ] Canary deployments with monitoring
- [ ] Machine learning for update scheduling

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for update improvements? We'd love to hear from you:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 📊 Update experiences: Share your update stories in discussions

---

*Last Updated: April 2026*
*Module Version: v1.8.0*
*Next Review: v2.0.0 Release*

---

## Test Strategy

- Unit test coverage ≥ 80% for `HotReloadEngine`, `DeltaUpdateEngine`, `InPlaceSchemaMigrator`, and `CoordinatedUpdateManager`
- Integration tests: full update cycle (download → validate CMS signature → apply delta → atomic install → SHA-256 hash verify → health check pass)
- Rollback integration test: corrupt the installed binary post-update and verify automatic rollback restores the original file with matching SHA-256
- Security tests: tampered bundle (invalid CMS signature) and path traversal in bundle path must both be rejected before any write to disk
- Schema migration idempotency test: apply the same migration version twice and verify second run is a no-op with version unchanged
- Canary rollout test: verify that ≤ configured fraction of nodes are updated; all remaining nodes are unchanged until explicit promotion

## Performance Targets

- Delta update apply time ≤ 10 s for a 100 MB binary delta on NVMe storage (excluding download time)
- CMS/PKCS#7 signature verification ≤ 50 ms for a 2-certificate chain on commodity hardware without HSM
- Atomic file replacement (fsync + rename/MoveFileExW) ≤ 500 ms for a 200 MB binary on NVMe
- Hot-reload engine restart latency (stop → apply → start) ≤ 5 s for a service with ≤ 1,000 open connections
- In-place schema migration (additive, metadata-only) ≤ 100 ms for tables with ≤ 10 million rows
- Pre-flight health check completion ≤ 2 s including disk space, memory headroom, and dependency version checks

## Security / Reliability

- All hot-reload paths must validate CMS/PKCS#7 signature against the embedded X.509 trust anchor before writing any file to disk
- `isSafePath` guard must be applied to every path extracted from an update bundle; path traversal attempts must be logged and the entire bundle rejected
- Rollback restore points must include a SHA-256 manifest of all replaced files; restore aborts if any file's checksum does not match the manifest
- Update bundles are signed with hardware-backed HSM keys; the public trust anchor is embedded in the binary and cannot be overridden at runtime
- Filesystem lock must prevent concurrent `HotReloadEngine` invocations on the same node; failed lock acquisition returns `UpdateError::ALREADY_IN_PROGRESS`
- Pre-flight disk space check must confirm ≥ 2× the bundle size of free space is available before starting download to prevent mid-install space exhaustion
