> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/user_storage_encrypted/ARCHITECTURE.md -->

# User Storage Encrypted Module — Public Header Architecture

**Module Path:** `include/user_storage_encrypted/`  
**Implementation:** `../../src/user_storage_encrypted/`  
**Canonical architecture doc:** [`../../src/user_storage_encrypted/ARCHITECTURE.md`](../../src/user_storage_encrypted/ARCHITECTURE.md)

---

## 1. Overview

`include/user_storage_encrypted/` defines the **public per-user encrypted storage with pluggable backends (gocryptfs), key derivation, key rotation scheduling, multi-level storage, and security levels API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/user_storage_encrypted/ARCHITECTURE.md`](../../src/user_storage_encrypted/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Storage Interfaces

| Header | Public Type | Purpose |
|--------|------------|---------|
| `encryption_backend_interface.hpp` | `IEncryptionBackend` | Pluggable encryption backend interface |
| `gocryptfs_backend.hpp` | `GocryptfsBackend` | Gocryptfs-based encrypted storage backend |
| `multi_level_storage.hpp` | `MultiLevelStorage` | Tiered multi-level encrypted storage |
| `irotation_store.hpp` | `IRotationStore` | Key rotation metadata store interface |
| `user_models.hpp` | `UserModels` | User storage model definitions |
| `security_level.hpp` | `SecurityLevel` | Storage security level classification |
### 2.2 Key Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `key_derivation_service.hpp` | `KeyDerivationService` | HKDF-based per-user key derivation |
| `key_rotation_scheduler.hpp` | `KeyRotationScheduler` | Scheduled cryptographic key rotation |

---

## 3. Namespace Layout

All public types reside in the `themis::user_storage_encrypted` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/user_storage_encrypted/` expose the **stable public API**; internal types live in `src/user_storage_encrypted/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
