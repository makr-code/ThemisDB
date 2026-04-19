<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: ../src/updates/README.md · ../src/updates/ROADMAP.md · ../src/updates/FUTURE_ENHANCEMENTS.md -->

# Updates Module Headers - Future Enhancements

## Scope

- Public API enhancements for `include/updates/` headers
- Delta update interface (`DeltaUpdateEngine`, `DeltaManifest`, apply/generate operations)
- Canary rollout API (`CanaryDeployment`, percentage-based stage progression)
- Signature verification interface (CMS/PKCS#7 bundle verification before install)
- Rollback API (`HotReloadEngine::rollback`, pre-update state preservation)
- Multi-node coordination interface (`DistributedUpdateCoordinator`)

## Design Constraints

- [ ] Delta apply is atomic (all-or-nothing); partial application leaves filesystem unchanged
- [ ] Canary rollout is percentage-based (1 → 5 → 25 → 100); no arbitrary node targeting in public API
- [ ] Signature verification MUST block install if the CMS/PKCS#7 check fails; no `force` bypass in public API
- [ ] Rollback API preserves the exact pre-update filesystem state via backup snapshot
- [ ] `isSafePath` is checked on all bundle-supplied paths before any filesystem write

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `DeltaUpdateEngine::applyDelta(DeltaManifest)` | Update pipeline | Atomic; returns list of updated files |
| `CanaryDeployment::deploy()` | Ops tooling | Returns `std::future`; progress via callback |
| `HotReloadEngine::rollback(rollback_id)` | Admin API | Restores pre-update state |
| `DistributedUpdateCoordinator::updateCluster(version)` | Cluster manager | Rolling update with health checks |
| `UpdateVerifier::verify()` | Post-update gate | Runs smoke + integration + perf tests |

## Planned Header Additions

### distributed_update_coordinator.h
**Priority:** High
**Target Version:** v1.7.0

Header for coordinating updates across distributed ThemisDB clusters.

**Proposed Interface:**
```cpp
namespace themis {
namespace updates {

/**
 * @brief Coordinate updates across all nodes in a cluster
 */
class DistributedUpdateCoordinator {
public:
    struct Config {
        bool enable_rolling_updates = true;
        size_t max_parallel_updates = 1;
        std::chrono::seconds health_check_timeout{30};
        bool rollback_on_failure = true;
    };

    struct ClusterUpdateProgress {
        size_t total_nodes;
        size_t nodes_updated;
        size_t nodes_failed;
        std::string current_node;
        std::string status;
        std::vector<std::string> errors;
    };

    /**
     * @brief Construct coordinator with Raft manager
     */
    DistributedUpdateCoordinator(
        std::shared_ptr<RaftManager> raft,
        const Config& config = {}
    );

    /**
     * @brief Update entire cluster to target version
     * @param version Target version
     * @return Future that resolves when update completes
     */
    std::future<Result<void>> updateCluster(const std::string& version);

    /**
     * @brief Set progress callback
     */
    void setProgressCallback(
        std::function<void(const ClusterUpdateProgress&)> callback
    );

    /**
     * @brief Cancel ongoing cluster update
     */
    void cancelUpdate();

    /**
     * @brief Get current update status
     */
    ClusterUpdateProgress getProgress() const;
};

} // namespace updates
} // namespace themis
```

**Use Cases:**
- Zero-downtime cluster-wide upgrades
- Coordinated schema migrations across nodes
- Automatic health validation after updates

---

### delta_update_engine.h
**Priority:** High
**Target Version:** v1.6.0

Header for binary delta/patch-based updates to reduce download sizes.

**Proposed Interface:**
```cpp
namespace themis {
namespace updates {

/**
 * @brief Binary delta patches for efficient updates
 */
class DeltaUpdateEngine {
public:
    enum class Algorithm {
        BSDIFF,      // Best compression ratio
        XDELTA3,     // Fast, good compression
        VCDIFF,      // RFC 3284 compliant
        ZSTD_DICT    // Dictionary-based
    };

    struct DeltaManifest {
        std::string from_version;
        std::string to_version;
        std::vector<FileDelta> deltas;
        uint64_t total_patch_size;
        uint64_t total_full_size;

        struct FileDelta {
            std::string path;
            std::string base_hash;
            std::string target_hash;
            std::string patch_url;
            uint64_t patch_size;
            uint64_t target_size;
            Algorithm algorithm;
        };
    };

    /**
     * @brief Find delta update between versions
     * @return Delta manifest if available
     */
    std::optional<DeltaManifest> findDelta(
        const std::string& from_version,
        const std::string& to_version
    );

    /**
     * @brief Apply delta update
     * @param delta Delta manifest
     * @return Result with list of updated files
     */
    Result<std::vector<std::string>> applyDelta(const DeltaManifest& delta);

    /**
     * @brief Generate delta between two releases
     * @param base_release Base release
     * @param target_release Target release
     * @param algorithm Delta algorithm to use
     * @return Generated delta manifest
     */
    Result<DeltaManifest> generateDelta(
        const ReleaseManifest& base_release,
        const ReleaseManifest& target_release,
        Algorithm algorithm = Algorithm::BSDIFF
    );
};

} // namespace updates
} // namespace themis
```

**Expected Benefits:**
- 70-90% reduction in download size
- Faster updates for low-bandwidth connections
- Reduced server bandwidth costs

---

### schema_migration.h ✅ IMPLEMENTED (v1.7.0)
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Released — `include/updates/schema_migration.h`, `src/updates/schema_migration.cpp`

Header for automatic schema migration with online DDL support.

**Proposed Interface:**
```cpp
namespace themis {
namespace updates {

/**
 * @brief Automatic schema migration framework
 */
class SchemaMigrator {
public:
    struct ColumnDef {
        std::string name;
        std::string type;
        bool nullable = true;
        std::string default_value;
        std::string comment;
    };

    struct IndexDef {
        std::string name;
        std::vector<std::string> columns;
        bool unique = false;
        bool build_online = true;
    };

    /**
     * @brief Create migration for specific version
     */
    explicit SchemaMigrator(const std::string& version);

    /**
     * @brief Add column to table
     */
    SchemaMigrator& addColumn(
        const std::string& table,
        const ColumnDef& column
    );

    /**
     * @brief Drop column from table
     */
    SchemaMigrator& dropColumn(
        const std::string& table,
        const std::string& column,
        std::chrono::hours grace_period = std::chrono::hours(0)
    );

    /**
     * @brief Rename column
     */
    SchemaMigrator& renameColumn(
        const std::string& table,
        const std::string& old_name,
        const std::string& new_name
    );

    /**
     * @brief Add index
     */
    SchemaMigrator& addIndex(
        const std::string& table,
        const IndexDef& index
    );

    /**
     * @brief Drop index
     */
    SchemaMigrator& dropIndex(
        const std::string& table,
        const std::string& index_name
    );

    /**
     * @brief Add custom migration logic
     */
    SchemaMigrator& addCustomMigration(
        std::function<bool(MigrationContext&)> migration
    );

    /**
     * @brief Apply migration
     */
    Result<void> apply(IStorageEngine& storage);

    /**
     * @brief Rollback migration
     */
    Result<void> rollback();

    /**
     * @brief Get migration SQL (for inspection)
     */
    std::vector<std::string> toSQL() const;
};

} // namespace updates
} // namespace themis
```

**Key Features:**
- Zero-downtime schema changes
- Automatic backfill for new columns
- Online index building
- Rollback capability

---

### canary_deployment.h
**Priority:** Medium
**Target Version:** v1.7.0

Header for gradual rollout with automatic monitoring and rollback.

**Proposed Interface:**
```cpp
namespace themis {
namespace updates {

/**
 * @brief Canary deployment with progressive rollout
 */
class CanaryDeployment {
public:
    struct Stage {
        int percentage;             // 1%, 5%, 25%, 100%
        std::chrono::seconds duration;

        // Thresholds for automatic rollback
        double max_error_rate = 0.05;       // 5%
        std::chrono::milliseconds max_p99_latency{500};
        double max_memory_increase = 0.20;  // 20%
    };

    struct Metrics {
        double error_rate;
        std::chrono::milliseconds p99_latency;
        double memory_usage_gb;
        double cpu_usage_percent;
    };

    /**
     * @brief Configure canary deployment
     */
    CanaryDeployment& setVersion(const std::string& version);
    CanaryDeployment& setStages(const std::vector<Stage>& stages);

    /**
     * @brief Set metrics provider
     */
    CanaryDeployment& setMetricsProvider(
        std::function<Metrics()> provider
    );

    /**
     * @brief Start canary deployment
     */
    std::future<Result<void>> deploy();

    /**
     * @brief Set stage completion callback
     */
    void onStageComplete(std::function<void(const Stage&)> callback);

    /**
     * @brief Set rollback callback
     */
    void onRollback(std::function<void(const std::string& reason)> callback);

    /**
     * @brief Force rollback
     */
    void forceRollback();

    /**
     * @brief Get current deployment status
     */
    std::string getStatus() const;
};

} // namespace updates
} // namespace themis
```

**Monitoring Integration:**
```cpp
canary.setMetricsProvider([]() {
    return CanaryDeployment::Metrics{
        .error_rate = prometheus->getErrorRate(),
        .p99_latency = prometheus->getP99Latency(),
        .memory_usage_gb = prometheus->getMemoryUsage(),
        .cpu_usage_percent = prometheus->getCPUUsage()
    };
});
```

---

### dependency_resolver.h
**Priority:** Medium
**Target Version:** v1.6.0

Header for automatic resolution of update dependencies.

**Proposed Interface:**
```cpp
namespace themis {
namespace updates {

/**
 * @brief Resolve update dependencies with topological sorting
 */
class DependencyResolver {
public:
    struct Dependency {
        std::string package;
        std::string version_constraint;     // ">=1.4.0,<2.0.0"
        bool optional = false;
        std::vector<std::string> conflicts;
    };

    struct UpdateStep {
        std::string package;
        std::string from_version;
        std::string to_version;
        std::vector<std::string> dependencies;
    };

    struct Resolution {
        bool success;
        std::string error_message;
        std::vector<UpdateStep> steps;  // Ordered by dependencies
    };

    /**
     * @brief Add dependency for a version
     */
    void addDependency(
        const std::string& version,
        const Dependency& dependency
    );

    /**
     * @brief Resolve dependencies for target version
     * @param target_version Target version to update to
     * @param current_versions Currently installed versions
     * @return Ordered list of update steps
     */
    Resolution resolve(
        const std::string& target_version,
        const std::map<std::string, std::string>& current_versions
    );

    /**
     * @brief Detect dependency conflicts
     */
    std::vector<Conflict> detectConflicts(
        const std::map<std::string, std::string>& versions
    );
};

} // namespace updates
} // namespace themis
```

**Usage:**
```cpp
DependencyResolver resolver;

// Define dependencies
resolver.addDependency("1.5.0", {
    .package = "themis-storage",
    .version_constraint = ">=1.4.0,<2.0.0"
});

// Resolve
auto resolution = resolver.resolve("1.5.0", current_versions);
if (resolution.success) {
    for (const auto& step : resolution.steps) {
        applyUpdate(step);
    }
}
```

---

### update_verifier.h
**Priority:** Medium
**Target Version:** v1.6.0

Header for automated testing before applying updates.

**Proposed Interface:**
```cpp
namespace themis {
namespace updates {

/**
 * @brief Automated update verification
 */
class UpdateVerifier {
public:
    enum class TestType {
        SMOKE,          // Basic functionality tests
        INTEGRATION,    // API/endpoint tests
        PERFORMANCE,    // Regression tests
        SCHEMA          // Schema compatibility tests
    };

    struct TestResult {
        std::string test_name;
        TestType type;
        bool passed;
        std::string error_message;
        std::chrono::milliseconds duration;
    };

    struct VerificationResult {
        bool success;
        std::vector<TestResult> test_results;
        std::string summary;
    };

    /**
     * @brief Add smoke test
     */
    void addSmokeTest(
        const std::string& name,
        std::function<bool()> test
    );

    /**
     * @brief Add integration test
     */
    void addIntegrationTest(
        const std::string& name,
        std::function<bool()> test
    );

    /**
     * @brief Add performance test
     */
    void addPerformanceTest(
        const std::string& name,
        std::function<bool()> test
    );

    /**
     * @brief Run all verification tests
     */
    VerificationResult verify();

    /**
     * @brief Run specific test type
     */
    VerificationResult verifyType(TestType type);
};

} // namespace updates
} // namespace themis
```

**Example Tests:**
```cpp
UpdateVerifier verifier;

// Smoke tests
verifier.addSmokeTest("database_connect", []() {
    return db->isHealthy();
});

// Integration tests
verifier.addIntegrationTest("api_health", []() {
    auto resp = httpGet("/health");
    return resp.status == 200;
});

// Performance tests
verifier.addPerformanceTest("query_latency", []() {
    auto start = now();
    executeQuery("SELECT * FROM users LIMIT 100");
    return (now() - start) < 100ms;
});

// Verify after update
auto result = verifier.verify();
if (!result.success) {
    rollbackUpdate();
}
```

---

## API Improvements

### Enhanced DownloadResult
**Priority:** Low
**Target Version:** v1.6.0

Add more detailed download statistics.

**Current:**
```cpp
struct DownloadResult {
    bool success;
    std::string error_message;
    std::string download_path;
    ReleaseManifest manifest;
};
```

**Proposed:**
```cpp
struct DownloadResult {
    bool success;
    std::string error_message;
    std::string download_path;
    ReleaseManifest manifest;

    // New fields
    std::chrono::milliseconds download_duration;
    uint64_t bytes_downloaded;
    double average_speed_mbps;
    int retry_count;
    std::vector<std::string> warnings;
};
```

---

### Enhanced ReloadResult
**Priority:** Low
**Target Version:** v1.6.0

Add more detailed reload information.

**Current:**
```cpp
struct ReloadResult {
    bool success;
    std::string error_message;
    std::vector<std::string> files_updated;
    std::string rollback_id;
};
```

**Proposed:**
```cpp
struct ReloadResult {
    bool success;
    std::string error_message;
    std::vector<std::string> files_updated;
    std::string rollback_id;

    // New fields
    std::chrono::milliseconds reload_duration;
    std::vector<FileUpdateResult> file_results;
    std::string previous_version;
    std::string new_version;
    bool schema_migration_applied;
};
```

---

### Async Update API
**Priority:** Medium
**Target Version:** v1.7.0

Add asynchronous update operations with futures.

**Proposed Addition:**
```cpp
class HotReloadEngine {
public:
    // Existing synchronous API
    DownloadResult downloadRelease(const std::string& version);
    ReloadResult applyHotReload(const std::string& version);

    // New asynchronous API
    std::future<DownloadResult> downloadReleaseAsync(const std::string& version);
    std::future<ReloadResult> applyHotReloadAsync(const std::string& version);

    // Cancel ongoing async operation
    void cancelAsyncOperation();
};
```

**Usage:**
```cpp
// Start async download
auto download_future = engine->downloadReleaseAsync("1.5.0");

// Do other work...

// Wait for completion
auto download_result = download_future.get();
```

---

## Type Safety Improvements

### Strongly-Typed Version
**Priority:** Low
**Target Version:** v1.6.0

Replace string version with strongly-typed Version class.

**Proposed:**
```cpp
namespace themis {
namespace updates {

/**
 * @brief Strongly-typed version number
 */
class Version {
public:
    /**
     * @brief Parse version from string
     */
    static std::optional<Version> parse(const std::string& version_str);

    /**
     * @brief Construct version
     */
    Version(int major, int minor, int patch, std::string prerelease = "");

    /**
     * @brief Comparison operators
     */
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
    bool operator==(const Version& other) const;
    bool operator<=(const Version& other) const;
    bool operator>=(const Version& other) const;

    /**
     * @brief Convert to string
     */
    std::string toString() const;

    /**
     * @brief Check if compatible upgrade
     */
    bool isCompatibleUpgradeFrom(const Version& from) const;

    int major() const;
    int minor() const;
    int patch() const;
    std::string prerelease() const;

private:
    int major_;
    int minor_;
    int patch_;
    std::string prerelease_;
};

} // namespace updates
} // namespace themis
```

**Benefits:**
- Type-safe version comparisons
- Semantic version parsing
- Clearer API contracts

---

### Strongly-Typed Platform/Architecture
**Priority:** Low
**Target Version:** v1.6.0

Replace string platform/architecture with enums.

**Proposed:**
```cpp
enum class Platform {
    WINDOWS,
    LINUX,
    MACOS,
    BSD,
    UNKNOWN
};

enum class Architecture {
    X64,
    X86,
    ARM64,
    ARM32,
    UNKNOWN
};

// Update ReleaseFile
struct ReleaseFile {
    // Replace strings
    Platform platform;
    Architecture architecture;
};
```

---

## Documentation Improvements

### Doxygen Enhancement
**Priority:** Medium
**Target Version:** v1.6.0

Add comprehensive Doxygen documentation for all public APIs.

**Example:**
```cpp
/**
 * @brief Hot-reload engine for zero-downtime updates
 *
 * The HotReloadEngine provides capabilities for downloading, verifying,
 * and applying updates to a running ThemisDB instance without downtime.
 *
 * @section features Features
 * - Resume-capable downloads from GitHub releases
 * - Atomic file replacement with fsync guarantees
 * - Automatic backup before updates
 * - CMS/PKCS#7 signature verification
 * - Progress callback system
 * - Dry-run mode for testing
 * - Rollback capability
 *
 * @section thread_safety Thread Safety
 * - Not thread-safe for concurrent updates
 * - Uses filesystem locks to prevent parallel updates
 * - Read operations are thread-safe
 *
 * @section example Example Usage
 * @code
 * HotReloadEngine engine(manifest_db, update_checker, config);
 * auto result = engine.downloadRelease("1.5.0");
 * if (result.success) {
 *     engine.applyHotReload("1.5.0");
 * }
 * @endcode
 *
 * @see ManifestDatabase
 * @see ReleaseManifest
 */
class HotReloadEngine {
    // ...
};
```

---

## Backwards Compatibility

### Deprecation Warnings
**Priority:** High
**Target Version:** v1.6.0

Add deprecation warnings for APIs that will be removed.

**Proposed:**
```cpp
// Mark deprecated APIs
[[deprecated("Use applyHotReloadAsync instead")]]
ReloadResult applyHotReload(const std::string& version);

// Add migration guide in comments
/**
 * @deprecated Use applyHotReloadAsync instead
 * @migration
 * Old code:
 *   auto result = engine->applyHotReload("1.5.0");
 *
 * New code:
 *   auto future = engine->applyHotReloadAsync("1.5.0");
 *   auto result = future.get();
 */
```

---

## Build System Integration

### CMake Targets
**Priority:** Low
**Target Version:** v1.6.0

Add granular CMake targets for header-only components.

**Proposed:**
```cmake
# Header-only targets
add_library(themis-updates-headers INTERFACE)
target_include_directories(themis-updates-headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Component targets
add_library(themis-updates-hot-reload)
add_library(themis-updates-manifest-db)
add_library(themis-updates-delta)

# Aggregate target
add_library(themis-updates)
target_link_libraries(themis-updates
    themis-updates-hot-reload
    themis-updates-manifest-db
    themis-updates-delta
)
```

---

## Testing Infrastructure

### Mock Headers
**Priority:** Medium
**Target Version:** v1.6.0

Add mock implementations for testing.

**Proposed:**
```cpp
namespace themis {
namespace updates {
namespace testing {

/**
 * @brief Mock hot-reload engine for testing
 */
class MockHotReloadEngine : public IHotReloadEngine {
public:
    MOCK_METHOD(DownloadResult, downloadRelease, (const std::string&), (override));
    MOCK_METHOD(ReloadResult, applyHotReload, (const std::string&, bool), (override));
    MOCK_METHOD(bool, rollback, (const std::string&), (override));
};

} // namespace testing
} // namespace updates
} // namespace themis
```

---

*Last Updated: April 2026*
*Module Version: v1.5.x*
*Next Review: v1.6.0 Release*

## Test Strategy

- Unit tests: `DeltaUpdateEngine::applyDelta` — inject mid-apply failure and verify no partial writes persist
- Unit tests: `CanaryDeployment` stage progression — mock metrics exceeding thresholds trigger automatic rollback
- Integration tests: full download → verify signature → apply delta → post-update smoke tests pass
- Negative tests: bundle with invalid CMS signature is rejected before any file is written
- Negative tests: `isSafePath` blocks path traversal (`../`) in bundle-supplied file entries
- Regression tests: rollback restores all files to byte-identical pre-update state

## Performance Targets

- `DeltaUpdateEngine::applyDelta` throughput: ≤ 10 s per 100 MB of target files
- Signature verification (CMS/PKCS#7 verify call): ≤ 50 ms for a standard bundle
- `HotReloadEngine::rollback` initiation (trigger to first file restore): ≤ 1 s
- `CanaryDeployment` stage transition (metrics evaluation + traffic shift): ≤ 5 s per stage
- `DistributedUpdateCoordinator::updateCluster` per-node health check: ≤ 30 s timeout

## Security / Reliability

- All update bundles MUST be verified via CMS/PKCS#7 signature before any file is installed; failure is non-bypassable in public API
- `isSafePath` is called on every bundle-supplied file path to block directory traversal attacks
- Rollback operations are audit-logged with operator identity, timestamp, from-version, and to-version
- Delta patches are validated (hash check) after application before old files are removed from backup
- `DistributedUpdateCoordinator` uses rolling updates to maintain quorum availability; at most `max_parallel_updates` nodes updated simultaneously
