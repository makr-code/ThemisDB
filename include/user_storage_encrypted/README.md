> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-05-13 -->

# User Storage Encrypted

Encrypted, multi-security-level user-storage subsystem for ThemisDB. This module
provides per-user transparent filesystem encryption using gocryptfs (FUSE), Argon2id
key derivation, automatic key rotation, and Prometheus metrics.

**Version:** 0.3.0 · **Status:** 🟢 Production-Ready · **Namespace:** `themis::plugins::user_storage`

---

## Headers

| Header | Primary Class / Type | Purpose |
|--------|---------------------|---------|
| `encryption_backend_interface.hpp` | `EncryptionBackendInterface`, `Result<T>` | Abstract backend interface and result type |
| `gocryptfs_backend.hpp` | `GocryptfsBackend` | gocryptfs FUSE backend (fork/execvp, stdin key delivery) |
| `irotation_store.hpp` | `IRotationStore`, `NullRotationStore`, `FileRotationStore` | Key rotation state persistence interfaces |
| `key_derivation_service.hpp` | `IKeyDerivationService`, `Argon2idKeyDerivationService` | Argon2id-based per-container key derivation |
| `key_rotation_scheduler.hpp` | `KeyRotationScheduler` | Automatic key rotation with configurable intervals |
| `multi_level_storage.hpp` | `MultiLevelEncryptedStorage`, `LevelConfig`, `StorageMetrics` | Multi-security-level orchestrator + Prometheus metrics |
| `security_level.hpp` | `SecurityLevel` enum | Security classification levels (OFFEN → STRENG_GEHEIM) |
| `user_models.hpp` | `User`, `Group`, `HealthStatus` | User/group data and health check types |

---

## API Reference

### `security_level.hpp` — `SecurityLevel`

```cpp
namespace themis::plugins::user_storage {
    enum class SecurityLevel { OFFEN = 0, VS_NFD = 1, GEHEIM = 2, STRENG_GEHEIM = 3 };
    std::string securityLevelToString(SecurityLevel level);
    SecurityLevel stringToSecurityLevel(const std::string& str); // throws std::invalid_argument
}
```

| Level | String | Encryption | Key Provider | Default Rotation |
|-------|--------|-----------|--------------|-----------------|
| `OFFEN` | `"offen"` | None | — | — |
| `VS_NFD` | `"vs-nfd"` | AES-256-GCM | Vault | 90 days |
| `GEHEIM` | `"geheim"` | AES-256-GCM | Vault | 60 days |
| `STRENG_GEHEIM` | `"streng-geheim"` | AES-256-GCM | HSM | 30 days |

---

### `encryption_backend_interface.hpp` — `EncryptionBackendInterface` / `Result<T>`

`Result<T>` is a lightweight error-propagation type returned by all fallible operations:

```cpp
Result<void> r = backend.initialize("{}");
if (r.isError()) { /* r.error() returns the message */ }
if (r.isSuccess()) { /* ok */ }
```

`EncryptionBackendInterface` is the polymorphic backend contract:

| Method | Description |
|--------|-------------|
| `initialize(config_json)` | Configure the backend from a JSON string |
| `createContainer(enc_dir, mnt, key)` | Create and initialise an encrypted container |
| `mountContainer(enc_dir, mnt, key)` | Mount an existing container |
| `unmountContainer(mnt)` | Unmount a container |
| `isMounted(mnt)` | Check mount status (reads `/proc/mounts` on Linux) |
| `checkAvailability()` | Verify the backend binary and kernel support |
| `getBackendName()` | E.g. `"gocryptfs"` |
| `getBackendVersion()` | Backend binary version string |

---

### `gocryptfs_backend.hpp` — `GocryptfsBackend`

Concrete implementation of `EncryptionBackendInterface` using gocryptfs FUSE.

```cpp
// Without KDF (key_material used directly):
GocryptfsBackend backend;

// With Argon2id KDF (per-container key derivation from master key):
Argon2idKeyDerivationService kdf;
GocryptfsBackend backend(&kdf);
```

Key delivery is always via stdin pipe (`-passfile /dev/stdin`); no key material
touches the filesystem. `explicit_bzero` clears the pipe buffer after delivery.

Public test/integration helpers:

| Method | Description |
|--------|-------------|
| `executeCommandWithStdin(args, stdin_data)` | Fork/exec with data written to child stdin |
| `deliverKeyViaStdin(args, key_hex)` | Like above, zeroes `key_hex` after write |

---

### `key_derivation_service.hpp` — `IKeyDerivationService` / `Argon2idKeyDerivationService`

```cpp
Argon2idKeyDerivationService kdf; // OWASP defaults: m=65536, t=3, p=4
auto salt   = kdf.generateSalt().value();     // 32-byte random salt
auto key    = kdf.deriveKey(master_key, salt).value(); // 32-byte derived key
auto salt2  = kdf.loadOrCreateSalt("/data/enc/.themis_kdf_salt").value();
```

`Argon2idKeyDerivationService` also implements `KeyDerivationService` (domain-separated API):

```cpp
auto key = kdf.derive(master_key, user_id, container_id, salt); // std::vector<uint8_t>
```

---

### `irotation_store.hpp` — `IRotationStore`

Pluggable persistence for `KeyRotationScheduler` state:

| Implementation | Description |
|----------------|-------------|
| `NullRotationStore` | No-op — no persistence (default) |
| `FileRotationStore` | JSON-file–backed, thread-safe |

```cpp
// File-backed store:
auto store = std::make_shared<FileRotationStore>("/var/lib/themisdb/rotation_state.json");
scheduler.initialize(3600, store);
```

The key format is `"user_storage:rotation_state:{level}"` → JSON `{"last_check_ms": N}`.

---

### `key_rotation_scheduler.hpp` — `KeyRotationScheduler`

```cpp
KeyRotationScheduler scheduler;
scheduler.initialize(/* check_interval_seconds= */ 3600);
scheduler.scheduleRotation(
    SecurityLevel::VS_NFD,
    /* interval_days= */ 90,
    /* auto_rotate= */ true,
    [](SecurityLevel lvl, bool ok, const std::string& err) { /* callback */ }
);
scheduler.isRotationDue(SecurityLevel::VS_NFD, last_rotation_ms); // → bool
scheduler.getNextRotationTime(SecurityLevel::GEHEIM);             // → int64_t ms
scheduler.triggerRotation(SecurityLevel::VS_NFD);                 // manual trigger
scheduler.cancelRotation(SecurityLevel::VS_NFD);
scheduler.shutdown(); // immediate thread wakeup via condition_variable
```

---

### `multi_level_storage.hpp` — `MultiLevelEncryptedStorage`

Implements `IThemisPlugin`. Manages one encrypted container per security level.

**User / Group API:**

| Method | Description |
|--------|-------------|
| `createUser(user, level)` | Create user record at given security level |
| `getUser(user_id, level)` | Read user record |
| `updateUser(user, level)` | Update user record |
| `deleteUser(user_id, level)` | Delete user record |
| `listUsers(level)` | List all users at a level |
| `createGroup(group, level)` | Create group record |
| `getGroup(group_id, level)` | Read group record |
| `updateGroup(group, level)` | Update group record |
| `deleteGroup(group_id, level)` | Delete group record |
| `listGroups(level)` | List all groups at a level |

**Container Management:**

| Method | Description |
|--------|-------------|
| `mountAll()` | Mount all configured security-level containers |
| `unmountAll()` | Unmount all containers |
| `mountLevel(level)` | Mount a single security level |
| `unmountLevel(level)` | Unmount a single security level |
| `rotateKey(level)` | Trigger key rotation for a level |

**Health + Metrics:**

| Method | Description |
|--------|-------------|
| `checkHealth()` | Overall health status |
| `checkLevelHealth(level)` | Health status per level |
| `getMetricsText()` | Prometheus text format (v0.0.4) |
| `recordKeyRotation(level)` | Increment rotation counter (also called by `rotateKey()`) |

**Prometheus metric families exported by `getMetricsText()`:**

| Metric | Type | Description |
|--------|------|-------------|
| `user_storage_mounts_active` | Gauge | Currently mounted containers |
| `user_storage_mount_operations_total` | Counter | Mount + unmount ops (`label: operation`) |
| `user_storage_key_rotations_total` | Counter | Key rotation callbacks fired (`label: level`) |
| `user_storage_container_size_bytes` | Gauge | Sum of encrypted container sizes on disk |

---

### `user_models.hpp` — Data Types

```cpp
struct User {
    std::string user_id, username, email, full_name;
    std::vector<std::string> roles;
    SecurityLevel classification;
    int64_t created_at_ms, updated_at_ms;
};

struct Group {
    std::string group_id, name, description;
    std::vector<std::string> member_ids;
    SecurityLevel classification;
    int64_t created_at_ms;
};

struct HealthStatus {
    bool healthy;
    std::string message;
    std::vector<std::string> errors;
    int64_t checked_at_ms;
};
```

---

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

Runtime dependency: `gocryptfs` >= v2.0 and FUSE support must be present on the host.
For Argon2id KDF: link against `libargon2` (`apt-get install libargon2-dev`).

---

## Usage

### Minimal — mount one container and store/retrieve a user

```cpp
#include "user_storage_encrypted/security_level.hpp"
#include "user_storage_encrypted/encryption_backend_interface.hpp"
#include "user_storage_encrypted/gocryptfs_backend.hpp"
#include "user_storage_encrypted/multi_level_storage.hpp"

using namespace themis::plugins::user_storage;

// 1. Check gocryptfs availability
GocryptfsBackend backend;
if (auto r = backend.checkAvailability(); r.isError()) {
    throw std::runtime_error("gocryptfs not available: " + r.error());
}

// 2. Bootstrap storage from JSON config
MultiLevelEncryptedStorage storage;
storage.initialize(R"({
  "base_path": "/var/lib/themisdb/user_storage",
  "levels": [
    {
      "level": "offen",
      "encrypted": false
    },
    {
      "level": "vs-nfd",
      "encrypted": true,
      "backend": "gocryptfs",
      "key_provider": "vault",
      "vault_addr": "https://vault.example.com",
      "rotation_interval_days": 90
    }
  ]
})");

// 3. Mount all configured levels
storage.mountAll();

// 4. Store a user
User u;
u.user_id = "user-123";
u.username = "alice";
u.classification = SecurityLevel::VS_NFD;
storage.createUser(u, SecurityLevel::VS_NFD);

// 5. Retrieve and check health
auto health = storage.checkHealth().value();

// 6. Expose Prometheus metrics
std::string metrics = storage.getMetricsText();

// 7. Shut down
storage.unmountAll();
storage.shutdown();
```

### Key Rotation with Persistence

```cpp
#include "user_storage_encrypted/key_rotation_scheduler.hpp"
#include "user_storage_encrypted/irotation_store.hpp"

using namespace themis::plugins::user_storage;

auto store = std::make_shared<FileRotationStore>("/var/lib/themisdb/rotation_state.json");
KeyRotationScheduler scheduler;
scheduler.initialize(3600, store);  // check every hour, persist state

scheduler.scheduleRotation(
    SecurityLevel::VS_NFD, 90, true,
    [](SecurityLevel lvl, bool ok, const std::string& err) {
        if (!ok) { /* log error */ }
    }
);

// At shutdown:
scheduler.shutdown();
```

### Argon2id KDF (per-container key derivation)

```cpp
#include "user_storage_encrypted/key_derivation_service.hpp"
#include "user_storage_encrypted/gocryptfs_backend.hpp"

using namespace themis::plugins::user_storage;

Argon2idKeyDerivationService kdf;  // OWASP defaults
GocryptfsBackend backend(&kdf);    // backend uses KDF for all containers

std::vector<uint8_t> master_key(32, 0x42);  // 256-bit master key
backend.createContainer("/data/enc", "/mnt/plain", master_key);
// Per-container Argon2id-derived key is used; salt stored in /data/enc/.themis_kdf_salt
```

---

## Runtime Behavior and Error Handling

- All fallible methods return `Result<T>`. Check `isError()` / `isSuccess()` before
  calling `.value()` (throws `std::runtime_error` on access to a failed result).
- `mountContainer()` is a no-op if the container is already mounted (guarded by
  `isMounted()` before the subprocess call).
- `unmountContainer()` is a no-op if the container is not currently mounted.
- `reconcileStaleMounts()` is called automatically during `initialize()` — it scans
  `/proc/mounts` for orphaned FUSE mounts and calls `fusermount -u` (with `umount`
  fallback). Failures are logged but never fatal.
- `KeyRotationScheduler::shutdown()` uses a `std::condition_variable` for immediate
  thread wakeup, avoiding up to `check_interval_seconds` of wait time.

---

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| `checkAvailability()` returns error | `gocryptfs` binary not in `PATH` or `/dev/fuse` missing | Install gocryptfs; ensure `fuse` kernel module is loaded |
| `createContainer()` / `mountContainer()` fails | Insufficient permissions on encrypted dir or mount point | Verify directory exists with 0700 permissions owned by the ThemisDB process user |
| `deriveKey()` is slow (>200 ms) | Argon2id memory cost too high for host RAM | Reduce `memory_kb` in `Argon2idParams` (default: 65536 KiB); budget is ≤200 ms on reference HW |
| Key rotation callback not fired | `scheduleRotation()` not called, or `auto_rotate=false` | Ensure `scheduleRotation()` is called after `initialize()` with `auto_rotate=true` |
| Stale mount blocks startup | Crash left FUSE mounts active | `reconcileStaleMounts()` handles this automatically on next `initialize()` |
| `stringToSecurityLevel()` throws | Unknown level string passed | Use one of: `"offen"`, `"vs-nfd"`, `"geheim"`, `"streng-geheim"` |

---

## See Also

- [`../../src/user_storage_encrypted/README.md`](../../src/user_storage_encrypted/README.md) — implementation overview and quick-start
- [`../../src/user_storage_encrypted/ARCHITECTURE.md`](../../src/user_storage_encrypted/ARCHITECTURE.md) — component diagram and key material flow
- [`../../src/user_storage_encrypted/SECURITY.md`](../../src/user_storage_encrypted/SECURITY.md) — threat model and security controls
- [`../../src/user_storage_encrypted/ROADMAP.md`](../../src/user_storage_encrypted/ROADMAP.md) — planned features and implementation phases
- [`../../src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md`](../../src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md) — detailed enhancement specifications
