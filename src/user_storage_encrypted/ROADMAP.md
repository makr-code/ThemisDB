> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# User Encrypted Storage Plugin Roadmap

## Current Status

v0.1.0 — All three FUTURE_ENHANCEMENTS items (stdin key delivery, Argon2id KDF,
key rotation persistence) are implemented and tested.  `GocryptfsBackend` quality
is now ≥ 90/100; `KeyRotationScheduler` retains Production-Ready (100/100).

---

## Completed ✅

- [x] `GocryptfsBackend` — `fork/execvp`-based FUSE container lifecycle
- [x] Safe subprocess execution: `executeCommandSafe()` with `fork/execvp`
- [x] Stdin key delivery: `executeCommandWithStdin()` + `deliverKeyViaStdin()`;
      key passed via pipe to gocryptfs `-passfile /dev/stdin`; `explicit_bzero`
      clears pipe buffer; no key material written to `/tmp`
- [x] `KeyDerivationService` interface + `Argon2idKeyDerivationService` (m=65536,
      t=3, p=4, output=32 bytes); per-container salt in `.themis_kdf_salt`
- [x] `GocryptfsBackend(KeyDerivationService*)` constructor + `resolveKey()`
- [x] `isMounted()` via `/proc/mounts` (Linux) and `mount` output (macOS)
- [x] `checkAvailability()` — validates gocryptfs binary and `/dev/fuse`
- [x] `KeyRotationScheduler` — per-`SecurityLevel` rotation with background thread
- [x] `IRotationStore` + persistence: `last_check_ms` and `interval_days` written
      to store after each callback; state loaded on `scheduleRotation()`
- [x] `shutdown()` uses `condition_variable` for immediate thread wake
- [x] `MultiLevelEncryptedStorage` — HOT/WARM/COLD tier orchestration
- [x] 20 unit + integration tests (`test_user_storage_features.cpp`)
- [x] CI workflow (`user-storage-encrypted-ci.yml`)
- [x] Deprecated `executeCommand()` fully removed (v0.3.0); all call sites use `executeCommandSafe()`
- [x] `reconcileStaleMounts()` — scans `/proc/mounts` for orphaned FUSE mounts on startup;
      unmounts via `fusermount -u` / `umount` fallback; non-fatal; called from `initialize()`

---

## In Progress [~]

- [~] Integration tests: create → mount → write file → unmount → re-mount → verify file (Target: Q3 2026)

---

## Planned Features

### v0.1.0 — Hardening and Tests ✅ (2026-03-24)

- [x] Fix `const_cast` in `createPasswordFile()` — returns `Result<std::string>` (done)
- [x] Secure key delivery via stdin pipe instead of `/tmp` password file (done)
- [x] Argon2id KDF (`Argon2idKeyDerivationService`, m=65536/t=3/p=4) (done)
- [x] `IRotationStore` persistence for `KeyRotationScheduler` (done)
- [x] 20 unit tests (AC-SD, AC-KDF, AC-PRS, AC-GCF) (done)
- [ ] Integration tests: create → mount → write file → unmount → re-mount → verify file (Target: Q3 2026)
- [x] Remove deprecated `executeCommand()` after confirming no external callers (v0.3.0)

### v0.2.0 — Stale Mount Reconciliation ✅ (2026-03-25)

- [x] `reconcileStaleMounts()` — scans `/proc/mounts` for orphaned FUSE mounts,
      unmounts via `fusermount -u` / `umount` fallback, non-fatal (done)
- [x] Called from `initialize()` before `initializeLevel()` (done)
- [x] 5 `StaleMountReconciliationTest` tests (done)

### v0.3.0 — Monitoring and Multi-User (Target: Q3 2026)

- [x] Prometheus metrics — `StorageMetrics` struct + `MultiLevelEncryptedStorage::getMetricsText()` (v0.3.0)
  - Metric families: `user_storage_mounts_active` (gauge), `user_storage_mount_operations_total` (counter, label: operation), `user_storage_key_rotations_total` (counter), `user_storage_container_size_bytes` (gauge)
  - Counters updated in `mountLevel()`, `unmountLevel()`, `rotateKey()`; `recordKeyRotation()` public for external callers
  - All atomic; thread-safe; overhead ≤ 0.1 ms per operation
- [x] Remove deprecated `GocryptfsBackend::executeCommand()` (v0.3.0)
  - Replaced all 3 call sites (`checkAvailability`, `getBackendVersion`, `isMounted`) with `executeCommandSafe()`
  - Removed declaration from `gocryptfs_backend.hpp` and implementation from `gocryptfs_backend.cpp`
- [ ] Stale mount reconciliation on startup via `/proc/mounts` scan (Target: Q3 2026)
- [ ] Per-user container isolation: one encrypted dir per user_id (Target: Q1 2027)
- [ ] Storage quota enforcement per container (Target: Q1 2027)

---

## Implementation Phases

### Phase 1: Design / API Contract ✅
- [x] Define `GocryptfsBackend` container lifecycle API
- [x] Define `KeyRotationScheduler` `SecurityLevel` → schedule mapping
- [x] Define `MultiLevelEncryptedStorage` tier model

### Phase 2: Core Implementation ✅
- [x] `executeCommandSafe()` via `fork/execvp`
- [x] `executeCommandWithStdin()` + `deliverKeyViaStdin()` for stdin key delivery
- [x] `KeyDerivationService` + `Argon2idKeyDerivationService` (libargon2)
- [x] `resolveKey()` in `GocryptfsBackend` integrating KDF with per-container salt
- [x] `IRotationStore` interface + persistence in `KeyRotationScheduler`
- [x] `isMounted()` via `/proc/mounts`
- [x] `KeyRotationScheduler::schedulerLoop()` background thread with `condition_variable`

### Phase 3: Error Handling & Edge Cases ✅
- [x] Already-mounted guard in `mountContainer()`
- [x] Not-mounted guard in `unmountContainer()`
- [x] `_exit(127)` in child on `execvp` failure
- [x] Exit code check in parent with error propagation
- [x] `explicit_bzero` clears pipe buffer after key write
- [x] Corrupted persisted rotation state is silently ignored

### Phase 4: Tests ✅
- [x] 20 unit tests: stdin delivery, Argon2id KDF, IRotationStore persistence, GocryptfsBackend (done)
- [x] 5 stale mount reconciliation tests (done)
- [x] 12 v0.3.0 metric + deprecation tests (`test_user_storage_v03_focused`, USE-01..12) (done)
- [ ] Integration tests (Target: Q3 2026)

### Phase 5: Performance / Hardening ✅
- [x] Stdin key delivery eliminates `/tmp` key file window
- [x] Argon2id KDF: latency ≤ 200 ms on reference hardware (40 ms measured in CI)
- [x] `condition_variable` in scheduler enables immediate shutdown

### Phase 6: Documentation & Acceptance ✅
- [x] README, ARCHITECTURE, AUDIT, CHANGELOG, ROADMAP, SECURITY, FUTURE_ENHANCEMENTS

---

## Production Readiness Checklist

| Area | Status | Notes |
|------|--------|-------|
| `GocryptfsBackend` core API | ✅ | create/mount/unmount/isMounted implemented |
| Safe subprocess | ✅ | `fork/execvp`; no shell injection possible |
| Stdin key delivery | ✅ | Pipe + `explicit_bzero`; no filesystem trace |
| Argon2id KDF | ✅ | `Argon2idKeyDerivationService`; m=65536, t=3, p=4 |
| Key rotation persistence | ✅ | `IRotationStore`; JSON state per SecurityLevel |
| `KeyRotationScheduler` | ✅ | Production-Ready; `condition_variable` shutdown |
| Tests | ✅ | 20 v0.1.0 + 5 v0.2.0 stale-mount + 12 v0.3.0 metric tests |
| Prometheus metrics | ✅ | `getMetricsText()` — 4 families, `std::atomic`, thread-safe (v0.3.0) |
| `executeCommand()` removed | ✅ | All call sites migrated to `executeCommandSafe()` (v0.3.0) |
| CI | ✅ | `user-storage-encrypted-ci.yml` |

---

## Known Issues & Limitations

- `getBackendVersion()` uses `const_cast` to call `executeCommandSafe()` on a const object (cosmetic; `executeCommand()` fully removed in v0.3.0).
- Stale mount reconciliation on startup is planned but not yet implemented (FUTURE_ENHANCEMENTS §4).
- Per-user container isolation and storage quota enforcement are planned for v0.3.0 / Q1 2027.
