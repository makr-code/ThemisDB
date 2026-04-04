<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB User Encrypted Storage Plugin

**Version:** 0.1.0  
**Status:** 🟢 Production-Ready (v0.1.0; stale mount reconciliation remains as hardening item)  
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

All key material is handled as `std::vector<uint8_t>` and written to temporary files
with mode `0600` via `mkstemp()` + `fchmod()` before being passed to gocryptfs. Temp
files are `unlink()`ed immediately after use.

---

## Component Table

| File | Class / Role |
|------|-------------|
| `gocryptfs_backend.cpp` | `GocryptfsBackend` — FUSE mount lifecycle, safe `execvp` subprocess | 348 lines |
| `key_rotation_scheduler.cpp` | `KeyRotationScheduler` — per-security-level rotation scheduling | 181 lines |
| `multi_level_storage.cpp` | `MultiLevelEncryptedStorage` — HOT/WARM/COLD tier orchestration | — lines |
| `gocryptfs_backend.hpp` | `GocryptfsBackend` header | — |
| `key_rotation_scheduler.hpp` | `KeyRotationScheduler` header | — |
| `CMakeLists.txt` | Build configuration | — |

---

## Security Levels

`KeyRotationScheduler` manages rotation schedules per `SecurityLevel`:

| Level | Default Interval | Description |
|-------|-----------------|-------------|
| `OFFEN` | 90 days | Public / unclassified data |
| `INTERN` | 60 days | Internal use only |
| `VERTRAULICH` | 30 days | Confidential |
| `GEHEIM` | 14 days | Secret |
| `STRENG_GEHEIM` | 7 days | Top secret |

---

## Quick-Start Example

```cpp
#include "gocryptfs_backend.hpp"
#include "key_rotation_scheduler.hpp"

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
    SecurityLevel::VERTRAULICH,
    /* interval_days= */ 30,
    /* auto_rotate= */ true,
    [](SecurityLevel level) { /* re-key callback */ }
);

// 3. Unmount
backend.unmountContainer("/mnt/plaintext");

// 4. Shutdown scheduler
scheduler.shutdown();
```

---

## See Also

- `ARCHITECTURE.md` — component diagram, key material flow, FUSE subprocess model
- `SECURITY.md` — key material handling, threat model, known limitations
- `ROADMAP.md` — implementation phases and hardening backlog
