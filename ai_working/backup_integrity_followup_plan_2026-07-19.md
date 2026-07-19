# Backup integrity follow-up plan (2026-07-19)

- Scope: `include/storage/backup_manager.h`, `src/storage/backup_manager.cpp`, `tests/integration/storage/backup_recovery_integration_test.cpp`
- Acceptance:
  - `Result<void>` helpers compile correctly in integrity paths.
  - Missing integrity manifest remains backward-compatible, but manifest parse/open failures fail verification.
  - `decompressBackup()` preserves the specific verification error code.
  - Public API docs match actual `Result<>` behavior and do not promise quarantine behavior that is not implemented.
  - Focused backup integration tests cover missing-manifest success and corrupted manifest/data failure paths.
- Validation:
  - Inspect relevant CI workflow failures for this branch/PR.
  - Run focused build/test for `backup_recovery_integration_test` or the smallest available storage-focused target.
