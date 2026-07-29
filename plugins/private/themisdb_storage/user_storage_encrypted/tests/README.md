# User Storage Encrypted – Tests

## Status

Tests are pending implementation. Placeholder CMakeLists.txt is present.

## Migration

| ThemisDB source | Destination |
|---|---|
| `tests/user_storage_encrypted/CMakeLists.txt` | `tests/CMakeLists.txt` (replace) |
| (no `test_*.cpp` exist yet) | Implement per ROADMAP.md |

## Planned tests

- `test_multi_level_storage.cpp` — CRUD per security level, access control
- `test_key_rotation_scheduler.cpp` — Rotation triggers, zero-downtime contract
- `test_key_derivation_service.cpp` — KDF correctness, Vault/HSM integration
- `test_gocryptfs_backend.cpp` — Mount/unmount, encryption verification
