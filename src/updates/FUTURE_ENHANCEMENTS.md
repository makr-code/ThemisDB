# Updates Module - Future Enhancements

## Planned Features

### Distributed Cluster Updates
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

### Automatic Schema Migration Framework
**Priority:** High  
**Target Version:** v1.7.0

Automated schema migration with online DDL (zero-downtime schema changes).

**Features:**
- Schema versioning and tracking
- Online DDL (background schema changes)
- Automatic backfill for new columns
- Index rebuilding without downtime
- Dual-write during migration
- Rollback capability for schema changes

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

### Canary Deployments
**Priority:** Medium  
**Target Version:** v1.7.0

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
canary.setLatencyThreshold(500ms);    // 500ms p99 latency

// Start canary deployment
auto result = canary.deploy();

// Monitor progress
canary.onStageComplete([](const CanaryStage& stage) {
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

### Dependency Resolution Engine
**Priority:** Medium  
**Target Version:** v1.6.0

Automatic resolution of update dependencies with topological sorting.

**Features:**
- Dependency graph construction
- Topological sort for correct order
- Cycle detection
- Minimum version constraints
- Conflict resolution
- Automatic backfill of missing dependencies

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

### Multi-Tenant Update Scheduling
**Priority:** Low  
**Target Version:** v1.8.0

Per-tenant update schedules and maintenance windows.

**Features:**
- Tenant-specific maintenance windows
- Update blackout periods
- Priority tiers (critical, normal, low)
- Tenant consent for updates
- Rollback per tenant

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

---

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

*Last Updated: February 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.6.0 Release*
