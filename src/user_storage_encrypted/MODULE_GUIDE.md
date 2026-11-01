---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Module Guide for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This is the entry point for engineers working on the module. The runtime
implementation is under `src/user_storage_encrypted/`, and public declarations
are under `include/user_storage_encrypted/`.

## Reading order

1. [README.md](README.md) — module boundary and source map.
2. [ARCHITECTURE.md](ARCHITECTURE.md) — startup/dataflow and limitations.
3. [API.md](API.md) — runtime API and contract discrepancy.
4. [EMBEDDING.md](EMBEDDING.md) — build and host configuration.
5. [IMPLEMENTATION.md](IMPLEMENTATION.md) — source-level behavior.
6. [OPERATIONS.md](OPERATIONS.md) and [RUNBOOK.md](RUNBOOK.md) — health and
   recovery evidence.
7. [SECURITY.md](SECURITY.md) and [TRUST.md](TRUST.md) — boundaries/risks.
8. [ROADMAP.md](ROADMAP.md) — implementation and evidence follow-up.

## Ownership map

- Backend: `gocryptfs_backend.cpp`,
  `include/user_storage_encrypted/gocryptfs_backend.hpp`
- KDF: `key_derivation_service.cpp`,
  `include/user_storage_encrypted/key_derivation_service.hpp`
- Standalone scheduler/store: `key_rotation_scheduler.cpp`,
  `include/user_storage_encrypted/key_rotation_scheduler.hpp`,
  `include/user_storage_encrypted/irotation_store.hpp`
- Multi-level plugin: `multi_level_storage.cpp`,
  `include/user_storage_encrypted/multi_level_storage.hpp`
- Build: `src/user_storage_encrypted/CMakeLists.txt`
- Example config: `plugins/user_storage_encrypted/config/storage_config.yaml.example`

## Review checks

- Compare public declarations to implementation, including the two different
  contract namespaces.
- Confirm nested `multi_level_storage.levels` configuration and provider
  requirements.
- Do not assume the multi-level plugin enables the KDF or wires rotation
  settings to the standalone scheduler.
- Inspect actual mount/unmount outcomes: `unmountAll()` ignores per-level
  errors.
- Confirm health/metrics limitations; the size gauge is not updated.
- Review tests/benchmarks for source registration, skips, and simulations;
  source presence alone is not execution evidence.

## Source references

- `include/user_storage_encrypted/`
- `src/user_storage_encrypted/`
- `tests/user_storage_encrypted/CMakeLists.txt`
- `benchmarks/user_storage_encrypted/CMakeLists.txt`
