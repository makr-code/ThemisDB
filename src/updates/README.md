# ThemisDB Updates Module

## Module Purpose

The Updates module provides ThemisDB's comprehensive update and migration system, enabling zero-downtime schema evolution, version management, and hot-reload capabilities. It handles database schema migrations, release verification, automatic backups, and rollback capabilities to ensure safe and reliable database evolution.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `in_place_schema_migrator.cpp` | In-place schema migration without data copy; preview, apply, rollback |
| `coordinated_update_manager.cpp` | Multi-node update orchestration with replication-safe sequencing |
| `cluster_update_manager.cpp` | Cluster-wide rolling update with leader-last ordering and health checks |
| `update_history_logger.cpp` | Schema and data version tracking; who updated what and when |
| `dependency_resolver.cpp` | Migration dependency ordering; topological sort and conflict resolution |

## Scope

**In Scope:**
- Hot-reload engine for zero-downtime updates
- Release manifest management and verification
- Schema migration and evolution framework
- Version compatibility checking and upgrade paths
- Digital signature verification for updates
- Automatic backup before migrations
- Rollback capability with multiple restore points
- Incremental and full migration strategies
- Update scheduling and notification system
- Dependency tracking for complex migrations

**Out of Scope:**
- Network protocols for downloading updates (handled by utils module)
- Storage layer implementation (handled by storage module)
- User authentication for update permissions (handled by auth module)
- Build and release pipeline (CI/CD infrastructure)

## Key Components

### HotReloadEngine
**Location:** `hot_reload_engine.cpp`, `../include/updates/hot_reload_engine.h`

Central engine for applying hot-reload updates with atomic file replacement and rollback support.

**Features:**
- **Download Management**: Resume-capable downloads from GitHub releases
- **Atomic Updates**: All-or-nothing file replacement with fsync guarantees
- **Automatic Backup**: Creates rollback points before any changes
- **Signature Verification**: CMS/PKCS#7 signature validation with X.509 certificates
- **Progress Tracking**: Callback-based progress reporting for UI integration
- **Dry-Run Mode**: Test upgrades without applying changes
- **Platform Support**: Windows, Linux, and macOS with platform-specific handling

**Thread Safety:**
- Single-threaded design (no concurrent updates allowed)
- Thread-safe for read operations (listRollbackPoints)
- Uses filesystem locks to prevent concurrent updates

**Configuration:**
```cpp
HotReloadEngine::Config config;
config.download_directory = "/tmp/themis_updates";
config.backup_directory = "/var/lib/themisdb/rollback";
config.install_directory = "/opt/themisdb";
config.verify_signatures = true;        // Always verify signatures
config.create_backup = true;            // Always backup before update
config.dry_run = false;                 // Set true for testing

auto engine = std::make_unique<HotReloadEngine>(
    manifest_db,
    update_checker,
    config
);
```

**Update Workflow:**
```cpp
// 1. Download and verify release
auto download_result = engine->downloadRelease("1.5.0");
if (!download_result.success) {
    LOG_ERROR("Download failed: {}", download_result.error_message);
    return;
}

// 2. Verify release compatibility
auto verify_result = engine->verifyRelease(download_result.manifest);
if (!verify_result.verified) {
    LOG_ERROR("Verification failed: {}", verify_result.error_message);
    return;
}

// 3. Check upgrade compatibility
if (!engine->isCompatibleUpgrade("1.4.0", "1.5.0")) {
    LOG_ERROR("Incompatible upgrade path");
    return;
}

// 4. Apply update (creates backup automatically)
auto reload_result = engine->applyHotReload("1.5.0");
if (!reload_result.success) {
    LOG_ERROR("Update failed: {}", reload_result.error_message);
    // Automatic rollback on failure
    return;
}

LOG_INFO("Updated {} files", reload_result.files_updated.size());
LOG_INFO("Rollback ID: {}", reload_result.rollback_id);
```

**Rollback Example:**
```cpp
// List available rollback points
auto rollback_points = engine->listRollbackPoints();
for (const auto& [id, timestamp] : rollback_points) {
    std::cout << "Rollback point: " << id << " at " << timestamp << "\n";
}

// Rollback to previous version
if (engine->rollback(reload_result.rollback_id)) {
    LOG_INFO("Successfully rolled back");
} else {
    LOG_ERROR("Rollback failed");
}

// Clean old rollback points (keep last 3)
engine->cleanRollbackPoints(3);
```

**Progress Tracking:**
```cpp
engine->setProgressCallback([](int percentage, const std::string& message) {
    std::cout << "[" << percentage << "%] " << message << "\n";
});
```

### ManifestDatabase
**Location:** `manifest_database.cpp`, `../include/updates/manifest_database.h`

RocksDB-backed database for storing and retrieving release manifests with signature caching.

**Column Families:**
- **release_manifests**: `version → ReleaseManifest` (JSON)
- **file_registry**: `path:version → ReleaseFile` (JSON)
- **signature_cache**: `hash → verification result`
- **download_cache**: `version:file → local path`

**Features:**
- **Persistent Storage**: All manifests stored in RocksDB
- **Signature Caching**: Avoid re-verifying known good signatures
- **Download Caching**: Track downloaded files to avoid re-downloads
- **Version Sorting**: Semantic version comparison for latest version
- **Transactional Updates**: Atomic manifest storage with file registry

**Usage:**
```cpp
auto manifest_db = std::make_shared<ManifestDatabase>(
    rocksdb_wrapper,
    security_verifier
);

// Store new manifest
ReleaseManifest manifest;
manifest.version = "1.5.0";
manifest.tag_name = "v1.5.0";
manifest.release_notes = "Added vector search support";
manifest.is_critical = false;

// Add files
ReleaseFile file;
file.path = "bin/themis_server";
file.type = "executable";
file.sha256_hash = "abc123...";
file.size_bytes = 10485760;
file.platform = "linux";
file.architecture = "x64";
file.permissions = "0755";
manifest.files.push_back(file);

manifest_db->storeManifest(manifest);

// Retrieve manifest
auto stored = manifest_db->getManifest("1.5.0");
if (stored) {
    std::cout << "Version: " << stored->version << "\n";
    std::cout << "Files: " << stored->files.size() << "\n";
}

// Get latest version
auto latest = manifest_db->getLatestManifest();
if (latest) {
    std::cout << "Latest version: " << latest->version << "\n";
}

// List all versions
auto versions = manifest_db->listVersions();
for (const auto& version : versions) {
    std::cout << "Available: " << version << "\n";
}
```

**Signature Verification:**
```cpp
// Verify manifest signature
if (manifest_db->verifyManifest(manifest)) {
    LOG_INFO("Manifest signature valid");
} else {
    LOG_ERROR("Manifest signature invalid");
}

// Cache signature verification result
manifest_db->cacheSignatureVerification(
    manifest.manifest_hash,
    true,
    manifest.signing_certificate
);

// Check cached verification
auto cached = manifest_db->getCachedSignatureVerification(manifest.manifest_hash);
if (cached && *cached) {
    LOG_INFO("Signature previously verified, skipping");
}
```

**File Registry:**
```cpp
// Store individual file
ReleaseFile file;
file.path = "lib/libthemis.so";
file.sha256_hash = "def456...";
manifest_db->storeFile(file, "1.5.0");

// Retrieve file info
auto file_info = manifest_db->getFile("lib/libthemis.so", "1.5.0");
if (file_info) {
    std::cout << "File size: " << file_info->size_bytes << "\n";
    std::cout << "Hash: " << file_info->sha256_hash << "\n";
}

// Verify file integrity
if (manifest_db->verifyFile("lib/libthemis.so", "1.5.0")) {
    LOG_INFO("File integrity verified");
}
```

**Download Cache:**
```cpp
// Cache downloaded file location
manifest_db->cacheDownload("1.5.0", "themis_server", "/tmp/themis_updates/themis_server");

// Check if file already downloaded
auto cached_path = manifest_db->getCachedDownload("1.5.0", "themis_server");
if (cached_path) {
    LOG_INFO("File already downloaded at: {}", *cached_path);
    // Skip download
} else {
    // Download file
}
```

### ReleaseManifest
**Location:** `release_manifest.cpp`, `../include/updates/release_manifest.h`

Data structure representing a complete release with all files, signatures, and metadata.

**Structure:**
```cpp
struct ReleaseManifest {
    // Release Identity
    std::string version;              // "1.5.0"
    std::string tag_name;             // "v1.5.0"
    std::string release_notes;        // Changelog
    std::chrono::system_clock::time_point release_date;
    bool is_critical = false;         // Security update?

    // Files
    std::vector<ReleaseFile> files;   // All files in release

    // Signatures
    std::string manifest_hash;        // SHA-256 of manifest
    std::string signature;            // CMS/PKCS#7 signature
    std::string signing_certificate;  // X.509 certificate
    std::string timestamp_token;      // RFC 3161 timestamp

    // Build Info
    std::string build_commit;         // Git commit SHA
    std::string build_date;           // ISO 8601 timestamp
    std::string compiler_version;     // "GCC 11.2.0"

    // Dependencies
    std::vector<std::string> dependencies;

    // Upgrade Path
    std::string min_upgrade_from;     // "1.0.0"

    // Schema Version
    int schema_version = 1;
};
```

**ReleaseFile Structure:**
```cpp
struct ReleaseFile {
    std::string path;                 // "bin/themis_server"
    std::string type;                 // "executable", "library", "config"
    std::string sha256_hash;
    uint64_t size_bytes;
    std::string file_signature;
    std::string platform;             // "windows", "linux", "macos"
    std::string architecture;         // "x64", "arm64"
    std::string permissions;          // "0755" (Unix)
    std::string download_url;         // GitHub asset URL
    json metadata;                    // Additional metadata
};
```

**JSON Serialization:**
```cpp
// Convert to JSON
json j = manifest.toJson();
std::string manifest_json = j.dump(2);  // Pretty-print with indent

// Parse from JSON
auto parsed = ReleaseManifest::fromJson(j);
if (parsed) {
    std::cout << "Parsed version: " << parsed->version << "\n";
} else {
    LOG_ERROR("Failed to parse manifest");
}

// Calculate manifest hash (for signature verification)
std::string hash = manifest.calculateHash();
```

**Manifest Validation:**
```cpp
bool validateManifest(const ReleaseManifest& manifest) {
    // Check required fields
    if (manifest.version.empty()) return false;
    if (manifest.files.empty()) return false;

    // Verify hash matches
    std::string computed_hash = manifest.calculateHash();
    if (computed_hash != manifest.manifest_hash) {
        LOG_ERROR("Manifest hash mismatch");
        return false;
    }

    // Verify all files have required fields
    for (const auto& file : manifest.files) {
        if (file.path.empty() || file.sha256_hash.empty()) {
            LOG_ERROR("File missing required fields: {}", file.path);
            return false;
        }
    }

    return true;
}
```

### UpdatesConfig
**Location:** `updates_config.cpp`, `../include/updates/updates_config.h`

Configuration system for update checker, auto-update, and hot-reload settings.

**Configuration Structure:**
```cpp
UpdatesConfig config;

// Update Checker Settings
config.checker.enabled = true;
config.checker.check_interval = std::chrono::seconds(3600);  // 1 hour
config.checker.github_owner = "makr-code";
config.checker.github_repo = "ThemisDB";
config.checker.github_api_token = "ghp_xxx...";  // Optional, higher rate limits

// Auto-Update Settings
config.auto_update.enabled = false;
config.auto_update.critical_only = true;        // Only auto-apply critical updates
config.auto_update.require_approval = true;     // Manual approval required
config.auto_update.scheduled = true;
config.auto_update.schedule_time = "02:00";     // 2 AM
config.auto_update.schedule_days = {"Sunday"};  // Weekly on Sunday

// Hot-Reload Settings
config.hot_reload.enabled = true;
config.hot_reload.download_directory = "/tmp/themis_updates";
config.hot_reload.backup_directory = "/var/lib/themisdb/rollback";
config.hot_reload.verify_signatures = true;     // Always verify
config.hot_reload.create_backup = true;         // Always backup
config.hot_reload.keep_rollback_points = 3;     // Keep last 3 rollbacks

// Notification Settings
config.notifications.enabled = true;
config.notifications.on_events = {
    "update_available",
    "critical_update",
    "update_applied",
    "update_failed",
    "rollback_performed"
};
config.notifications.webhook_url = "https://hooks.slack.com/...";
```

**Loading from YAML:**
```cpp
// Load from YAML file
auto config = UpdatesConfig::loadFromYaml("/etc/themisdb/updates.yaml");

// Save to YAML
config.saveToYaml("/etc/themisdb/updates.yaml");

// Convert to JSON
json j = config.toJson();
```

**YAML Format:**
```yaml
checker:
  enabled: true
  check_interval: 3600
  github_owner: makr-code
  github_repo: ThemisDB
  github_api_token: ghp_xxx...

auto_update:
  enabled: false
  critical_only: true
  require_approval: true
  scheduled: true
  schedule_time: "02:00"
  schedule_days:
    - Sunday

hot_reload:
  enabled: true
  download_directory: /tmp/themis_updates
  backup_directory: /var/lib/themisdb/rollback
  verify_signatures: true
  create_backup: true
  keep_rollback_points: 3

notifications:
  enabled: true
  on_events:
    - update_available
    - critical_update
    - update_applied
  webhook_url: https://hooks.slack.com/...
```

## Architecture

### Update Flow Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      Update Checker                              │
│  (Periodic check for new releases from GitHub)                  │
└────────────────────────┬────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│                   ManifestDatabase                               │
│  (Store and retrieve release manifests with signatures)          │
└────────────────────────┬────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│                   HotReloadEngine                                │
│  (Download, verify, and apply updates with rollback)            │
└────────────────────────┬────────────────────────────────────────┘
                         ↓
┌──────────────┬──────────────────┬──────────────────┬───────────┐
│  Download    │  Verify          │  Backup          │  Apply    │
│  Files       │  Signatures      │  Current         │  Update   │
└──────────────┴──────────────────┴──────────────────┴───────────┘
```

### Component Interaction

```
                    ┌──────────────────┐
                    │  UpdateChecker   │
                    └────────┬─────────┘
                             ↓
                    ┌──────────────────┐
                    │ GitHub Release   │
                    │   API Client     │
                    └────────┬─────────┘
                             ↓
┌──────────────────────────────────────────────────────┐
│                ManifestDatabase                       │
│  ┌────────────────┐  ┌──────────────────────────┐   │
│  │  RocksDB       │  │  PluginSecurity          │   │
│  │  Storage       │  │  Verifier                │   │
│  └────────────────┘  └──────────────────────────┘   │
└────────────────────────┬─────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────┐
│              HotReloadEngine                          │
│  ┌────────────────────────────────────────────────┐  │
│  │  Download → Verify → Backup → Apply → Commit  │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────┐
│                 Filesystem                            │
│  (Atomic file replacement, rollback points)          │
└──────────────────────────────────────────────────────┘
```

### Update State Machine

```
┌─────────┐
│  Idle   │
└────┬────┘
     │ Check for update
     ↓
┌─────────────┐
│ Downloading │ ←──────┐
└────┬────────┘        │ Retry
     │                 │
     │ Download complete
     ↓                 │
┌─────────────┐        │
│  Verifying  │ ───────┘ Verification failed (retry)
└────┬────────┘
     │ Verified
     ↓
┌─────────────┐
│  Backing Up │
└────┬────────┘
     │ Backup complete
     ↓
┌─────────────┐
│  Applying   │ ──────┐
└────┬────────┘       │ Apply failed
     │                │
     │ Applied        ↓
     ↓           ┌──────────┐
┌─────────────┐  │ Rolling  │
│  Complete   │  │  Back    │
└─────────────┘  └────┬─────┘
                      │
                      ↓
                 ┌─────────┐
                 │  Rolled │
                 │  Back   │
                 └─────────┘
```

### Thread Safety Model

**HotReloadEngine:**
- Not thread-safe for concurrent updates
- Uses filesystem locks to prevent parallel updates
- Read operations (list rollback points) are thread-safe

**ManifestDatabase:**
- Thread-safe for all operations (RocksDB handles concurrency)
- Multiple readers and writers supported
- Uses RocksDB transactions for atomic updates

## Integration Points

### With Storage Module
Stores manifests and caches in RocksDB:
```cpp
auto storage = std::make_shared<RocksDBWrapper>(storage_config);
auto manifest_db = std::make_shared<ManifestDatabase>(storage, verifier);
```

### With Security Module
Verifies digital signatures on manifests and files:
```cpp
auto verifier = std::make_shared<acceleration::PluginSecurityVerifier>();
verifier->setPublicKey(public_key);

if (verifier->verifySignature(data, signature)) {
    LOG_INFO("Signature valid");
}
```

### With Query Module
Coordinates schema migrations:
```cpp
// Apply schema migration after update
engine->applyHotReload("1.5.0", [&](const ReleaseManifest& manifest) {
    // Check for schema changes
    if (manifest.dependencies.contains("schema_migration")) {
        // Apply schema changes
        query_engine->executeMigration(manifest.version);
    }
});
```

### With Network Module
Updates network protocol implementations:
```cpp
// Update wire protocol handlers
if (manifest.files.contains("lib/libthemis_protocol.so")) {
    // Reload protocol library
    network_manager->reloadProtocolHandlers();
}
```

## API/Usage Examples

### Basic Update Flow

```cpp
#include "updates/hot_reload_engine.h"
#include "updates/manifest_database.h"
#include "updates/updates_config.h"

// Initialize storage
auto storage = std::make_shared<RocksDBWrapper>(storage_config);
auto verifier = std::make_shared<acceleration::PluginSecurityVerifier>();
auto update_checker = std::make_shared<utils::UpdateChecker>(checker_config);

// Create manifest database
auto manifest_db = std::make_shared<ManifestDatabase>(storage, verifier);

// Create hot-reload engine
HotReloadEngine::Config config;
config.verify_signatures = true;
config.create_backup = true;

auto engine = std::make_unique<HotReloadEngine>(
    manifest_db,
    update_checker,
    config
);

// Check for updates
if (update_checker->checkForUpdate()) {
    auto latest = update_checker->getLatestVersion();
    LOG_INFO("Update available: {}", latest);

    // Download and apply update
    auto download = engine->downloadRelease(latest);
    if (download.success) {
        auto reload = engine->applyHotReload(latest);
        if (reload.success) {
            LOG_INFO("Update applied successfully");
        }
    }
}
```

### Scheduled Updates

```cpp
// Load configuration with schedule
auto config = UpdatesConfig::loadFromYaml("/etc/themisdb/updates.yaml");

// Check if update should run now
auto now = std::chrono::system_clock::now();
auto now_time = std::chrono::system_clock::to_time_t(now);
auto tm = std::localtime(&now_time);

// Check if current time matches schedule
if (config.auto_update.scheduled) {
    int schedule_hour = std::stoi(config.auto_update.schedule_time.substr(0, 2));
    int schedule_minute = std::stoi(config.auto_update.schedule_time.substr(3, 2));

    if (tm->tm_hour == schedule_hour && tm->tm_min == schedule_minute) {
        // Check if today is scheduled day
        std::string day = getDayOfWeek(tm->tm_wday);
        if (std::find(config.auto_update.schedule_days.begin(),
                     config.auto_update.schedule_days.end(),
                     day) != config.auto_update.schedule_days.end()) {
            // Apply scheduled update
            engine->applyHotReload(latest_version);
        }
    }
}
```

### Critical Security Updates

```cpp
// Check for critical updates
auto latest_manifest = manifest_db->getLatestManifest();
if (latest_manifest && latest_manifest->is_critical) {
    LOG_WARN("Critical security update available: {}", latest_manifest->version);

    if (config.auto_update.critical_only && !config.auto_update.require_approval) {
        // Automatically apply critical updates
        LOG_INFO("Automatically applying critical security update");
        auto result = engine->applyHotReload(latest_manifest->version);

        if (result.success) {
            LOG_INFO("Critical security update applied");
            // Send notification
            notifyAdmin("Critical update applied: " + latest_manifest->version);
        } else {
            LOG_ERROR("Failed to apply critical update: {}", result.error_message);
            notifyAdmin("URGENT: Critical update failed!");
        }
    } else {
        // Require manual approval
        notifyAdmin("Critical security update requires approval: " +
                   latest_manifest->version);
    }
}
```

### Migration with Dependency Tracking

```cpp
struct Migration {
    std::string version;
    std::vector<std::string> dependencies;
    std::function<bool()> migrate;
};

std::vector<Migration> migrations = {
    {"1.4.0", {}, []() {
        // Migrate schema from 1.3.x to 1.4.0
        return executeSchemaMigration("1.4.0");
    }},
    {"1.5.0", {"1.4.0"}, []() {
        // Migrate schema from 1.4.0 to 1.5.0
        return executeSchemaMigration("1.5.0");
    }}
};

// Apply migrations in dependency order
std::string current_version = "1.3.5";
for (const auto& migration : migrations) {
    // Check if all dependencies satisfied
    bool can_apply = true;
    for (const auto& dep : migration.dependencies) {
        if (compareVersions(current_version, dep) < 0) {
            can_apply = false;
            break;
        }
    }

    if (can_apply && compareVersions(current_version, migration.version) < 0) {
        LOG_INFO("Applying migration to {}", migration.version);
        if (migration.migrate()) {
            current_version = migration.version;
        } else {
            LOG_ERROR("Migration to {} failed", migration.version);
            break;
        }
    }
}
```

## Dependencies

### Internal Dependencies
- **storage/rocksdb_wrapper**: RocksDB storage for manifests
- **acceleration/plugin_security**: Digital signature verification
- **utils/update_checker**: GitHub release checking
- **utils/logger**: Logging infrastructure
- **core/concerns**: Cross-cutting concerns

### External Dependencies
- **OpenSSL** (required): SHA-256 hashing, signature verification
- **nlohmann/json** (required): JSON parsing for manifests
- **libcurl** (optional): HTTP downloads
- **yaml-cpp** (optional): YAML configuration parsing

### Build Configuration
```cmake
# Link updates module
target_link_libraries(my_app themis-updates)

# Dependencies
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Optional features
option(THEMIS_ENABLE_CURL "Enable HTTP downloads" ON)
option(THEMIS_ENABLE_YAML "Enable YAML config" ON)

if(THEMIS_ENABLE_CURL)
    find_package(CURL REQUIRED)
    target_compile_definitions(themis-updates PRIVATE THEMIS_ENABLE_CURL)
endif()

if(THEMIS_ENABLE_YAML)
    find_package(yaml-cpp REQUIRED)
    target_compile_definitions(themis-updates PRIVATE THEMIS_ENABLE_YAML)
endif()
```

## Performance Characteristics

### Download Performance
- **Single file**: 10-50 MB/s (network dependent)
- **Resume support**: Saves bandwidth on interrupted downloads
- **Parallel downloads**: Not currently supported (sequential)
- **Caching**: Avoids re-downloading same files

### Verification Performance
- **SHA-256 hashing**: 500-1000 MB/s
- **Signature verification**: 100-500 ops/sec
- **Signature caching**: Near-instant for cached signatures

### Backup Performance
- **Copy speed**: 1-5 GB/s (filesystem dependent)
- **Hardlink creation**: Near-instant (filesystem dependent)
- **Compression**: Optional (not implemented)

### Apply Performance
- **File replacement**: 100-1000 files/sec
- **Atomic rename**: Microseconds per file
- **Fsync overhead**: 1-10ms per file

### Memory Usage
- **Manifest storage**: ~10 KB per version
- **Download buffer**: 1 MB per active download
- **Signature cache**: ~1 KB per cached signature
- **Total**: <100 MB for typical workloads

## Known Limitations

1. **Single-Node Updates Only**
   - No distributed update coordination
   - Each node must be updated independently
   - Future: Raft-based cluster-wide updates

2. **Sequential Downloads**
   - Files downloaded one at a time
   - No parallel download support
   - Workaround: Pre-download all files before applying

3. **No Delta Updates**
   - Full file replacement only
   - No binary diff/patch support
   - Future: Binary delta patches for large files

4. **Schema Migration Manual**
   - Schema migrations must be scripted separately
   - No automatic schema inference
   - Future: Automatic schema migration framework

5. **No Rollback Validation**
   - Rollback assumes previous version works
   - No validation before rollback
   - Workaround: Test rollback in staging environment

6. **Platform-Specific Files**
   - Separate manifests per platform
   - No cross-compilation support
   - Must download platform-specific release

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Features:**
- Hot-reload engine with rollback
- Manifest database with RocksDB
- Digital signature verification
- Automatic backup before updates
- Version compatibility checking
- Download caching and resume

⚠️ **Beta Features:**
- Scheduled updates
- Auto-update for critical releases
- Webhook notifications
- YAML configuration

🔬 **Experimental:**
- Parallel downloads
- Delta/binary patches
- Automatic schema migrations
- Distributed cluster updates

## Related Documentation

### Quick Links

- **Core Components:**
  - [Hot Reload Engine](../../docs/de/src/updates/hot_reload_engine.cpp.md) - Hot-reload implementation
  - [Manifest Database](../../docs/de/src/updates/manifest_database.cpp.md) - Manifest storage
  - [Release Manifest](../../docs/de/src/updates/release_manifest.cpp.md) - Release data structures
- **Architecture:**
  - [Update System Architecture](../../docs/updates/architecture.md) - System design
  - [Version Management](../../docs/updates/version_management.md) - Versioning strategy
  - [Migration Patterns](../../docs/updates/migration_patterns.md) - Schema migration best practices
- **Operations:**
  - [Update Procedures](../../docs/operations/update_procedures.md) - Operational guide
  - [Rollback Guide](../../docs/operations/rollback_guide.md) - Recovery procedures
  - [Release Process](../../docs/development/release_process.md) - Creating releases
- **Security:**
  - [Signature Verification](../../docs/security/signature_verification.md) - Security model
  - [Update Security](../../docs/security/update_security.md) - Threat analysis

## Contributing

When contributing to the updates module:

1. Always maintain rollback capability
2. Add tests for version compatibility checks
3. Document migration procedures
4. Test with various network conditions (slow, interrupted)
5. Verify signature validation for security

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned updates improvements
- [Storage Module](../storage/README.md) - Underlying storage layer
- [Security Module](../security/README.md) - Signature verification
- [Utils Module](../utils/README.md) - Update checker utilities

## Scientific References

1. Fowler, M., & Sadalage, P. J. (2010). **NoSQL Distilled: A Brief Guide to the Emerging World of Polyglot Persistence**. Addison-Wesley. ISBN: 978-0-321-82662-6

2. Ambler, S. W., & Sadalage, P. J. (2006). **Refactoring Databases: Evolutionary Database Design**. Addison-Wesley. ISBN: 978-0-321-29353-5

3. Neumann, T., & Weikum, G. (2010). **The RDF-3X Engine for Scalable Management of RDF Data**. *VLDB Journal*, 19(1), 91–113. https://doi.org/10.1007/s00778-009-0165-y

4. Bernstein, P. A., & Goodman, N. (1981). **Concurrency Control in Distributed Database Systems**. *ACM Computing Surveys*, 13(2), 185–221. https://doi.org/10.1145/356842.356846

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
