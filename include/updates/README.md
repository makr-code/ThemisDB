> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# ThemisDB Updates Module - Header Files

This directory contains the public header files for ThemisDB's Updates module, which provides comprehensive update and migration capabilities including hot-reload, version management, and schema evolution.

## Header Files Overview

### Core Components

#### hot_reload_engine.h
**Purpose:** Hot-reload engine for zero-downtime updates with atomic file replacement and rollback support.

**Key Classes:**
- `HotReloadEngine` - Main engine for downloading and applying updates
- `DownloadResult` - Result of download operations
- `ReloadResult` - Result of hot-reload operations with rollback ID
- `VerificationResult` - Result of manifest verification

**Key Features:**
- Resume-capable downloads from GitHub releases
- Atomic file replacement with fsync guarantees
- Automatic backup before updates
- CMS/PKCS#7 signature verification
- Progress callback system
- Dry-run mode for testing
- Rollback capability with multiple restore points

**Main APIs:**
```cpp
class HotReloadEngine {
public:
    struct Config {
        std::string download_directory;
        std::string backup_directory;
        std::string install_directory;
        bool verify_signatures;
        bool create_backup;
        bool dry_run;
    };

    // Core operations
    DownloadResult downloadRelease(const std::string& version);
    ReloadResult applyHotReload(const std::string& version, bool verify_only = false);
    bool rollback(const std::string& rollback_id);

    // Verification
    VerificationResult verifyRelease(const ReleaseManifest& manifest);
    bool isCompatibleUpgrade(const std::string& current, const std::string& target);

    // Management
    std::vector<std::pair<std::string, std::string>> listRollbackPoints() const;
    void cleanRollbackPoints(size_t keep_count = 3);
    void setProgressCallback(std::function<void(int, const std::string&)> callback);
};
```

**Usage Example:**
```cpp
#include "updates/hot_reload_engine.h"

// Configure engine
HotReloadEngine::Config config;
config.download_directory = "/tmp/themis_updates";
config.verify_signatures = true;
config.create_backup = true;

// Create engine
auto engine = std::make_unique<HotReloadEngine>(
    manifest_db,
    update_checker,
    config
);

// Download and apply update
auto download = engine->downloadRelease("1.5.0");
if (download.success) {
    auto reload = engine->applyHotReload("1.5.0");
    if (!reload.success) {
        engine->rollback(reload.rollback_id);
    }
}
```

**Thread Safety:**
- Not thread-safe for concurrent updates
- Uses filesystem locks to prevent parallel updates
- Read operations (listRollbackPoints) are thread-safe

---

#### manifest_database.h
**Purpose:** RocksDB-backed database for storing and retrieving release manifests with signature caching.

**Key Classes:**
- `ManifestDatabase` - Persistent storage for release manifests

**Key Features:**
- RocksDB storage with column families
- Signature verification caching
- Download cache to avoid re-downloads
- Version sorting and comparison
- File registry for individual file tracking

**Column Families:**
- `release_manifests` - version → ReleaseManifest (JSON)
- `file_registry` - path:version → ReleaseFile (JSON)
- `signature_cache` - hash → verification result
- `download_cache` - version:file → local path

**Main APIs:**
```cpp
class ManifestDatabase {
public:
    ManifestDatabase(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<acceleration::PluginSecurityVerifier> verifier
    );

    // Manifest operations
    bool storeManifest(const ReleaseManifest& manifest);
    std::optional<ReleaseManifest> getManifest(const std::string& version);
    std::optional<ReleaseManifest> getLatestManifest();
    std::vector<std::string> listVersions() const;
    bool deleteManifest(const std::string& version);

    // Verification
    bool verifyManifest(const ReleaseManifest& manifest);
    bool verifyFile(const std::string& path, const std::string& version);

    // File registry
    std::optional<ReleaseFile> getFile(const std::string& path, const std::string& version);
    bool storeFile(const ReleaseFile& file, const std::string& version);

    // Caching
    void cacheSignatureVerification(const std::string& hash, bool verified, const std::string& cert);
    std::optional<bool> getCachedSignatureVerification(const std::string& hash);
    void cacheDownload(const std::string& version, const std::string& filename, const std::string& path);
    std::optional<std::string> getCachedDownload(const std::string& version, const std::string& filename);
};
```

**Usage Example:**
```cpp
#include "updates/manifest_database.h"

// Create database
auto manifest_db = std::make_shared<ManifestDatabase>(
    rocksdb_wrapper,
    security_verifier
);

// Store manifest
ReleaseManifest manifest;
manifest.version = "1.5.0";
manifest.files = {...};
manifest_db->storeManifest(manifest);

// Retrieve latest version
auto latest = manifest_db->getLatestManifest();
if (latest) {
    std::cout << "Latest: " << latest->version << "\n";
}

// Check cached signature
auto cached = manifest_db->getCachedSignatureVerification(hash);
if (cached && *cached) {
    // Skip verification
}
```

**Thread Safety:**
- Thread-safe for all operations (RocksDB handles concurrency)
- Multiple readers and writers supported
- Uses RocksDB transactions for atomic updates

---

#### release_manifest.h
**Purpose:** Data structures representing release manifests with all files, signatures, and metadata.

**Key Structures:**
- `ReleaseManifest` - Complete release information
- `ReleaseFile` - Individual file information

**Key Features:**
- JSON serialization/deserialization
- SHA-256 hash calculation
- Digital signature support
- Platform and architecture tracking
- Dependency management
- Upgrade path validation

**Data Structures:**
```cpp
struct ReleaseFile {
    // Identity
    std::string path;                 // "bin/themis_server"
    std::string type;                 // "executable", "library", "config", "data"

    // Integrity
    std::string sha256_hash;
    uint64_t size_bytes;
    std::string file_signature;

    // Platform
    std::string platform;             // "windows", "linux", "macos"
    std::string architecture;         // "x64", "arm64"
    std::string permissions;          // "0755" (Unix)

    // Download
    std::string download_url;
    json metadata;

    // Serialization
    json toJson() const;
    static std::optional<ReleaseFile> fromJson(const json& j);
};

struct ReleaseManifest {
    // Release Info
    std::string version;
    std::string tag_name;
    std::string release_notes;
    std::chrono::system_clock::time_point release_date;
    bool is_critical;

    // Files
    std::vector<ReleaseFile> files;

    // Signatures
    std::string manifest_hash;
    std::string signature;
    std::string signing_certificate;
    std::string timestamp_token;

    // Build Info
    std::string build_commit;
    std::string build_date;
    std::string compiler_version;

    // Dependencies
    std::vector<std::string> dependencies;
    std::string min_upgrade_from;

    // Schema
    int schema_version;

    // Serialization
    json toJson() const;
    static std::optional<ReleaseManifest> fromJson(const json& j);
    std::string calculateHash() const;
};
```

**Usage Example:**
```cpp
#include "updates/release_manifest.h"

// Create manifest
ReleaseManifest manifest;
manifest.version = "1.5.0";
manifest.tag_name = "v1.5.0";
manifest.is_critical = false;

// Add file
ReleaseFile file;
file.path = "bin/themis_server";
file.type = "executable";
file.sha256_hash = "abc123...";
file.size_bytes = 10485760;
file.platform = "linux";
file.architecture = "x64";
file.permissions = "0755";
manifest.files.push_back(file);

// Calculate hash
manifest.manifest_hash = manifest.calculateHash();

// Serialize to JSON
json j = manifest.toJson();
std::string json_str = j.dump(2);

// Parse from JSON
auto parsed = ReleaseManifest::fromJson(j);
if (parsed) {
    std::cout << "Version: " << parsed->version << "\n";
}
```

---

#### updates_config.h
**Purpose:** Configuration system for update checker, auto-update, hot-reload, and notification settings.

**Key Structures:**
- `UpdatesConfig` - Comprehensive configuration for all update subsystems
- `CheckerConfig` - Update checker settings
- `AutoUpdateConfig` - Automatic update settings
- `HotReloadConfig` - Hot-reload settings
- `NotificationConfig` - Notification settings

**Key Features:**
- YAML and JSON support
- Hierarchical configuration
- Scheduled updates
- Notification webhooks
- Flexible download settings

**Configuration Structure:**
```cpp
struct UpdatesConfig {
    struct CheckerConfig {
        bool enabled;
        std::chrono::seconds check_interval;
        std::string github_owner;
        std::string github_repo;
        std::string github_api_url;
        std::string github_api_token;
        std::string proxy_url;
    } checker;

    struct AutoUpdateConfig {
        bool enabled;
        bool critical_only;
        bool require_approval;
        std::chrono::seconds approval_timeout;
        bool scheduled;
        std::string schedule_time;
        std::vector<std::string> schedule_days;
    } auto_update;

    struct HotReloadConfig {
        bool enabled;
        std::string download_directory;
        std::string backup_directory;
        std::string install_directory;
        bool verify_signatures;
        bool create_backup;
        int keep_rollback_points;
        int download_timeout_seconds;
        int max_retries;
        int retry_delay_seconds;
    } hot_reload;

    struct NotificationConfig {
        bool enabled;
        std::vector<std::string> on_events;
        std::string webhook_url;
        std::string email_to;
    } notifications;

    // Load/save
    static UpdatesConfig loadFromYaml(const std::string& yaml_path);
    static UpdatesConfig fromJson(const json& j);
    json toJson() const;
    void saveToYaml(const std::string& yaml_path) const;
};
```

**Usage Example:**
```cpp
#include "updates/updates_config.h"

// Load from YAML
auto config = UpdatesConfig::loadFromYaml("/etc/themisdb/updates.yaml");

// Configure update checker
config.checker.enabled = true;
config.checker.check_interval = std::chrono::seconds(3600);

// Configure auto-update
config.auto_update.enabled = false;
config.auto_update.critical_only = true;
config.auto_update.scheduled = true;
config.auto_update.schedule_time = "02:00";

// Configure hot-reload
config.hot_reload.enabled = true;
config.hot_reload.verify_signatures = true;
config.hot_reload.keep_rollback_points = 3;

// Configure notifications
config.notifications.enabled = true;
config.notifications.webhook_url = "https://hooks.slack.com/...";

// Save configuration
config.saveToYaml("/etc/themisdb/updates.yaml");
```

**YAML Format:**
```yaml
checker:
  enabled: true
  check_interval: 3600
  github_owner: makr-code
  github_repo: ThemisDB

auto_update:
  enabled: false
  critical_only: true
  scheduled: true
  schedule_time: "02:00"
  schedule_days: [Sunday]

hot_reload:
  enabled: true
  download_directory: /tmp/themis_updates
  verify_signatures: true
  keep_rollback_points: 3

notifications:
  enabled: true
  webhook_url: https://hooks.slack.com/...
```

---

## Additional Headers

| Header | Key Types | Description |
|---|---|---|
| `blue_green_deployment.h` | `BlueGreenDeployment`, `DeploymentConfig` | Zero-downtime blue/green deployment switching |
| `build_verifier.h` | `BuildVerifier` | Post-install build integrity verification |
| `canary_rollout.h` | `CanaryRollout`, `CanaryConfig` | Gradual traffic-shifting canary deployments |
| `cluster_update_manager.h` | `ClusterUpdateManager` | Coordinated rolling updates across cluster nodes |
| `coordinated_update_manager.h` | `CoordinatedUpdateManager` | Multi-component coordinated update sequencing |
| `delta_update_engine.h` | `DeltaUpdateEngine` | Binary-diff / patch-based delta updates |
| `dependency_resolver.h` | `DependencyResolver` | Update dependency graph resolution |
| `hardware_telemetry.h` | `HardwareTelemetry` | Hardware capability probing for update compatibility |
| `in_place_schema_migrator.h` | `InPlaceSchemaMigrator` | Live schema migration without downtime |
| `notification_webhook.h` | `NotificationWebhook` | Webhook dispatcher for update lifecycle events |
| `parallel_downloader.h` | `ParallelDownloader` | Concurrent multi-file download with resume support |
| `preflight_health_check.h` | `PreflightHealthCheck` | Pre-update system readiness checks |
| `schema_migration.h` | `SchemaMigration`, `MigrationStep` | Schema migration definition and execution |
| `schema_migration_tester.h` | `SchemaMigrationTester` | Dry-run schema migration test harness; validates migrations on an isolated staging environment before production apply |
| `tenant_update_scheduler.h` | `TenantUpdateScheduler` | Per-tenant update scheduling for multi-tenant deployments |
| `update_history_logger.h` | `UpdateHistoryLogger` | Persistent log of applied updates and outcomes |
| `update_state_machine.h` | `UpdateStateMachine`, `UpdateState` | FSM governing the update lifecycle |

---

## Integration Examples

### Complete Update System

```cpp
#include "updates/hot_reload_engine.h"
#include "updates/manifest_database.h"
#include "updates/updates_config.h"

// Load configuration
auto config = UpdatesConfig::loadFromYaml("/etc/themisdb/updates.yaml");

// Initialize storage
auto storage = std::make_shared<RocksDBWrapper>(storage_config);
auto verifier = std::make_shared<acceleration::PluginSecurityVerifier>();
auto update_checker = std::make_shared<utils::UpdateChecker>(config.checker);

// Create manifest database
auto manifest_db = std::make_shared<ManifestDatabase>(storage, verifier);

// Create hot-reload engine
HotReloadEngine::Config engine_config;
engine_config.download_directory = config.hot_reload.download_directory;
engine_config.backup_directory = config.hot_reload.backup_directory;
engine_config.verify_signatures = config.hot_reload.verify_signatures;
engine_config.create_backup = config.hot_reload.create_backup;

auto engine = std::make_unique<HotReloadEngine>(
    manifest_db,
    update_checker,
    engine_config
);

// Set progress callback
engine->setProgressCallback([](int pct, const std::string& msg) {
    std::cout << "[" << pct << "%] " << msg << "\n";
});

// Check for updates
if (update_checker->checkForUpdate()) {
    auto latest = update_checker->getLatestVersion();
    LOG_INFO("Update available: {}", latest);

    // Download
    auto download = engine->downloadRelease(latest);
    if (download.success) {
        // Verify
        auto verify = engine->verifyRelease(download.manifest);
        if (verify.verified) {
            // Apply
            auto reload = engine->applyHotReload(latest);
            if (reload.success) {
                LOG_INFO("Updated to {}", latest);
            } else {
                LOG_ERROR("Update failed: {}", reload.error_message);
            }
        }
    }
}
```

### Scheduled Updates with Notifications

```cpp
// Periodic update check
void checkScheduledUpdate(
    UpdatesConfig& config,
    HotReloadEngine& engine,
    ManifestDatabase& manifest_db
) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&time);

    // Check if scheduled time
    if (config.auto_update.scheduled) {
        int schedule_hour = std::stoi(config.auto_update.schedule_time.substr(0, 2));
        int schedule_min = std::stoi(config.auto_update.schedule_time.substr(3, 2));

        if (tm->tm_hour == schedule_hour && tm->tm_min == schedule_min) {
            // Check for updates
            auto latest = manifest_db.getLatestManifest();
            if (latest && latest->is_critical) {
                // Notify
                sendNotification(config.notifications.webhook_url,
                               "Applying critical update: " + latest->version);

                // Apply
                auto result = engine.applyHotReload(latest->version);

                if (result.success) {
                    sendNotification(config.notifications.webhook_url,
                                   "Update successful: " + latest->version);
                } else {
                    sendNotification(config.notifications.webhook_url,
                                   "Update failed: " + result.error_message);
                }
            }
        }
    }
}
```

## Dependencies

### Internal Dependencies
- `storage/rocksdb_wrapper.h` - RocksDB storage backend
- `acceleration/plugin_security.h` - Digital signature verification
- `utils/update_checker.h` - GitHub release checking
- `utils/logger.h` - Logging infrastructure

### External Dependencies
- `<nlohmann/json.hpp>` - JSON parsing and serialization
- `<openssl/evp.h>` - SHA-256 hashing and signature verification
- `<curl/curl.h>` - HTTP downloads (optional, controlled by THEMIS_ENABLE_CURL)

### Build Requirements
```cmake
# Link updates module headers
target_include_directories(my_app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link updates library
target_link_libraries(my_app themis-updates)
```

## Design Patterns

### Builder Pattern
Used in configuration structures:
```cpp
UpdatesConfig config;
config.checker.enabled = true;
config.hot_reload.verify_signatures = true;
```

### Observer Pattern
Used for progress callbacks:
```cpp
engine->setProgressCallback([](int pct, const std::string& msg) {
    // Observer gets notified of progress
});
```

### Strategy Pattern
Different verification strategies:
```cpp
// Signature verification strategy
verifier->setStrategy(VerificationStrategy::STRICT);
```

### Factory Pattern
Static factory methods for parsing:
```cpp
auto manifest = ReleaseManifest::fromJson(json_data);
```

## Thread Safety Guarantees

| Component | Thread Safety | Notes |
|-----------|---------------|-------|
| HotReloadEngine | **Not thread-safe** | Use external locking for concurrent access |
| ManifestDatabase | **Thread-safe** | RocksDB handles concurrency |
| ReleaseManifest | **Thread-safe** | Immutable after creation |
| UpdatesConfig | **Not thread-safe** | Const after initialization |

## Error Handling

All components use Result types and optionals:
```cpp
// Result pattern
DownloadResult result = engine->downloadRelease("1.5.0");
if (!result.success) {
    LOG_ERROR("Download failed: {}", result.error_message);
}

// Optional pattern
std::optional<ReleaseManifest> manifest = manifest_db->getManifest("1.5.0");
if (!manifest) {
    LOG_ERROR("Manifest not found");
}
```

## Best Practices

1. **Always Verify Signatures**
   ```cpp
   config.verify_signatures = true;  // Never disable in production
   ```

2. **Always Create Backups**
   ```cpp
   config.create_backup = true;  // Enable rollback capability
   ```

3. **Use Dry-Run for Testing**
   ```cpp
   auto result = engine->applyHotReload("1.5.0", true);  // verify_only=true
   ```

4. **Clean Old Rollback Points**
   ```cpp
   engine->cleanRollbackPoints(3);  // Keep last 3 rollbacks
   ```

5. **Handle Failures Gracefully**
   ```cpp
   if (!reload.success) {
       engine->rollback(reload.rollback_id);
       notifyAdmins("Update failed, rolled back");
   }
   ```

## See Also

- [Updates Module Source Documentation](../../src/updates/README.md)
- [Future Enhancements](../../src/updates/FUTURE_ENHANCEMENTS.md)
- [Storage Module Headers](../storage/README.md)
- [Security Module Headers](../security/README.md)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
