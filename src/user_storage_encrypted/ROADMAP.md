<!-- Status: current | validated: 2026-03-22 -->
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
- [x] Deprecated `executeCommand()` retains backward compatibility via delegate to `executeCommandSafe()`

---

## In Progress [~]

- [~] Stale mount reconciliation on startup (FUTURE_ENHANCEMENTS §4)

---

## Planned Features

### v0.2.0 — Monitoring and Multi-User (Target: Q3 2026)

- [ ] Prometheus metrics: mount count, rotation events, container sizes (Target: Q3 2026)
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
- [x] 20 unit + integration tests (`test_user_storage_features.cpp`)
- [x] CI workflow covering gcc-12, gcc-13, clang-15

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
| Tests | ✅ | 20 tests: stdin, KDF, persistence |
| CI | ✅ | `user-storage-encrypted-ci.yml` |

---

## Known Issues & Limitations

- `getBackendVersion()` uses `const_cast` to call `executeCommand()` on a const object (cosmetic).
- Stale mount reconciliation on startup is planned but not yet implemented (FUTURE_ENHANCEMENTS §4).
- Prometheus metrics are planned but not yet implemented (FUTURE_ENHANCEMENTS §5).
