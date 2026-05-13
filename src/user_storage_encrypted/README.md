> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB User Encrypted Storage Plugin

**Version:** 0.2.0
**Status:** 🟢 Production-Ready (v0.2.0; stale mount reconciliation implemented)
**Last Updated:** 2026-03-22
**Module Path:** `src/user_storage_encrypted/`
**Namespace:** `themis::plugins::user_storage`

---

## Module Purpose

The User Encrypted Storage plugin provides per-user, transparent filesystem encryption
for ThemisDB user data directories. It combines three components:

1. **`GocryptfsBackend`** — manages gocryptfs FUSE mounts for encrypted containers,
   using `fork/exec` (not shell) for safe subprocess invocation.
2. **`KeyRotationScheduler`** — schedules automatic cryptographic key rotation per
   `SecurityLevel` on configurable intervals; runs a background thread.
3. **`MultiLevelEncryptedStorage`** — orchestrates multiple storage tiers
   (HOT / WARM / COLD) each with an independent `GocryptfsBackend` instance and
   its own key, enabling tiered data lifecycle policies.

All key material is delivered to gocryptfs via a stdin pipe (`-passfile /dev/stdin`) using
`fork/execvp` — key bytes never touch the filesystem. A `KeyDerivationService` interface
enables Argon2id-based per-container key derivation from a master key.

---

## Component Table

| File | Class / Role |
|------|-------------|
| `gocryptfs_backend.cpp` | `GocryptfsBackend` — FUSE mount lifecycle, safe `execvp` subprocess | 348 lines |
| `key_rotation_scheduler.cpp` | `KeyRotationScheduler` — per-security-level rotation scheduling | 181 lines |
| `key_derivation_service.cpp` | `Argon2idKeyDerivationService` — Argon2id KDF with libargon2 | — |
| `multi_level_storage.cpp` | `MultiLevelEncryptedStorage` — HOT/WARM/COLD tier orchestration | — |
| `gocryptfs_backend.hpp` | `GocryptfsBackend` header | — |
| `key_rotation_scheduler.hpp` | `KeyRotationScheduler` header | — |
| `CMakeLists.txt` | Build configuration | — |

---

## Security Levels

`KeyRotationScheduler` manages rotation schedules per `SecurityLevel` (defined in
`include/user_storage_encrypted/security_level.hpp`):

| Level | String | Default Interval | Encryption / Key Provider |
|-------|--------|-----------------|--------------------------|
| `OFFEN` | `"offen"` | — | None (unencrypted) |
| `VS_NFD` | `"vs-nfd"` | 90 days | AES-256-GCM + Vault |
| `GEHEIM` | `"geheim"` | 60 days | AES-256-GCM + Vault |
| `STRENG_GEHEIM` | `"streng-geheim"` | 30 days | AES-256-GCM + HSM |

---

## Quick-Start Example

```cpp
#include "user_storage_encrypted/gocryptfs_backend.hpp"
#include "user_storage_encrypted/key_rotation_scheduler.hpp"

using namespace themis::plugins::user_storage;

// 1. Create and mount an encrypted container
GocryptfsBackend backend;
backend.initialize("{}");
backend.checkAvailability();  // verify gocryptfs + FUSE present

std::vector<uint8_t> key(32, 0x42);  // 256-bit key
backend.createContainer("/data/encrypted", "/mnt/plaintext", key);
backend.mountContainer("/data/encrypted", "/mnt/plaintext", key);

// 2. Schedule key rotation
KeyRotationScheduler scheduler;
scheduler.initialize(/* check_interval_seconds= */ 3600);
scheduler.scheduleRotation(
    SecurityLevel::VS_NFD,
    /* interval_days= */ 90,
    /* auto_rotate= */ true,
    [](SecurityLevel level, bool ok, const std::string& err) {
        if (!ok) { /* log rotation error */ }
    }
);

// 3. Unmount
backend.unmountContainer("/mnt/plaintext");

// 4. Shutdown scheduler
scheduler.shutdown();
```

---

## Installation

This module is built as part of ThemisDB. Build prerequisites:

```bash
# Runtime and build dependencies
apt-get install gocryptfs fuse libargon2-dev

# Configure and build
cmake --preset linux-release && cmake --build --preset linux-release
```

CMake build option to enable this plugin: `-DTHEMIS_PLUGIN_USER_STORAGE_ENCRYPTED=ON`.

---

## Usage

The implementation files are compiled into the `themis_user_storage_encrypted` shared
library. Include the public headers from
`include/user_storage_encrypted/` — see
[`../../include/user_storage_encrypted/README.md`](../../include/user_storage_encrypted/README.md)
for the full public API reference.

---

## See Also

- `ARCHITECTURE.md` — component diagram, key material flow, FUSE subprocess model
- `SECURITY.md` — key material handling, threat model, known limitations
- `ROADMAP.md` — implementation phases and hardening backlog
- `FUTURE_ENHANCEMENTS.md` — detailed specifications for planned enhancements
- [`../../include/user_storage_encrypted/README.md`](../../include/user_storage_encrypted/README.md) — public API reference for all 8 headers

---

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| Build fails: `libargon2 not found` | `libargon2-dev` not installed | `apt-get install libargon2-dev` |
| `checkAvailability()` returns error | `gocryptfs` not in `PATH` or `/dev/fuse` missing | Install gocryptfs ≥ v2.0; ensure `fuse` kernel module loaded |
| Mount/unmount subprocess fails with exit 127 | gocryptfs binary not found by `execvp` | Verify gocryptfs is in `PATH` for the ThemisDB process |
| Key rotation callback not called | `scheduleRotation()` not invoked or `auto_rotate=false` | Call `scheduleRotation()` with `auto_rotate=true` after `initialize()` |
| Stale mount prevents re-mount on startup | Crash left FUSE mounts active | `reconcileStaleMounts()` runs automatically on `initialize()` |


