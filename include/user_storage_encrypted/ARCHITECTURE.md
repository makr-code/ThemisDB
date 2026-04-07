<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/user_storage_encrypted/ -->

# User Storage Encrypted — Public Header Architecture

## Overview
`include/user_storage_encrypted/` exposes the public C++ headers for ThemisDB's encrypted user-storage subsystem. All encryption operations, key lifecycle management, tiered storage, and security-level policy are declared here. Implementation details live in `../../src/user_storage_encrypted/`.

## Design Principles
1. **Backend abstraction** — `IEncryptionBackend` decouples callers from concrete filesystem-encryption tools.
2. **Tiered storage** — HOT/WARM/COLD tiers are first-class citizens; encryption policy follows tier.
3. **Secure key delivery** — key material is never written to world-readable paths; planned: stdin-pipe delivery.
4. **Minimal surface** — public headers expose only what callers need; internals stay in `src/`.
5. **Composable security levels** — `SecurityLevel` enum drives algorithm selection without exposing raw parameters.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `encryption_backend_interface.hpp` | `IEncryptionBackend` | Abstract backend; all backends implement this |
| `gocryptfs_backend.hpp` | `GocryptfsBackend` | Gocryptfs-based backend; fork/execvp subprocess, mkstemp 0600 key files |
| `key_rotation_scheduler.hpp` | `KeyRotationScheduler` | Configurable-interval automatic key rotation |
| `multi_level_storage.hpp` | `MultiLevelEncryptedStorage` | HOT/WARM/COLD tiered storage with per-tier encryption |
| `security_level.hpp` | `SecurityLevel` (enum) | STANDARD / HIGH / MAXIMUM encryption strength |
| `user_models.hpp` | user metadata types | Storage-layer user metadata and record types |

## Component Relationships
```
MultiLevelEncryptedStorage
  ├── IEncryptionBackend  ←  GocryptfsBackend
  ├── KeyRotationScheduler
  └── SecurityLevel (policy)
```

Implementation in `../../src/user_storage_encrypted/`
