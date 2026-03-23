<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# User Encrypted Storage Plugin Roadmap

## Current Status

v0.0.1 — `GocryptfsBackend`, `KeyRotationScheduler`, and `MultiLevelEncryptedStorage`
implement the core encrypted storage pipeline. `gocryptfs_backend.cpp` is rated
🟡 Release-Candidate (76/100); hardening is required before production deployment.
`KeyRotationScheduler` is Production-Ready (100/100).

---

## Completed ✅

- [x] `GocryptfsBackend` — `fork/execvp`-based FUSE container lifecycle
- [x] Safe subprocess execution: `executeCommandSafe()` with `fork/execvp`
- [x] Secure temp key file: `mkstemp()` + `fchmod(0600)` + `unlink()`
- [x] `isMounted()` via `/proc/mounts` (Linux) and `mount` output (macOS)
- [x] `checkAvailability()` — validates gocryptfs binary and `/dev/fuse`
- [x] `KeyRotationScheduler` — per-`SecurityLevel` rotation with background thread
- [x] `RotationCallback` invocation with `triggerRotation()` for manual override
- [x] `MultiLevelEncryptedStorage` — HOT/WARM/COLD tier orchestration
- [x] Deprecated `executeCommand()` retains backward compatibility via delegate to `executeCommandSafe()`

---

## In Progress [~]

- [~] `gocryptfs_backend.cpp` hardening (76/100 → target 90+/100)

---

## Planned Features

### v0.1.0 — Hardening and Tests (Target: Q3 2026)

- [ ] Fix `const_cast` in `createPasswordFile()` — use output parameter or return value (Target: Q3 2026)
- [ ] Secure key delivery via stdin pipe instead of `/tmp` password file (Target: Q3 2026)
  - Inputs: key material bytes, gocryptfs command args
  - Pipe key hex to gocryptfs stdin (`-passfile /dev/stdin` or `--extpass`)
  - Eliminates key material touching the filesystem entirely
  - Errors: broken pipe, gocryptfs ignoring stdin → fallback error + abort
  - Tests: no temp file created when stdin delivery is used
- [ ] Unit tests: `isMounted()`, `createPasswordFile()`, `executeCommandSafe()` exit codes (Target: Q3 2026)
- [ ] Integration tests: create → mount → write file → unmount → re-mount → verify file (Target: Q3 2026)
- [ ] Remove deprecated `executeCommand()` after confirming no external callers (Target: Q3 2026)

### v0.2.0 — Key Management (Target: Q4 2026)

- [ ] KDF integration: derive container key from master key using Argon2id (Target: Q4 2026)
  - Inputs: master_key bytes, user_id, salt
  - Outputs: 256-bit derived container key
  - Constraints: Argon2id m=65536, t=3, p=4; ≤ 200 ms on reference hardware
  - Tests: same master_key + salt → same derived key (deterministic)
- [ ] Key escrow: encrypted key backup to ThemisDB secrets store (Target: Q4 2026)
- [ ] `KeyRotationScheduler`: persist `last_check_ms` to RocksDB to survive restarts (Target: Q4 2026)

### v0.3.0 — Multi-User and Quota (Target: Q1 2027)

- [ ] Per-user container isolation: one encrypted dir per user_id (Target: Q1 2027)
- [ ] Storage quota enforcement per container (via FUSE config or OS quotas) (Target: Q1 2027)
- [ ] Prometheus metrics: mount count, rotation events, container sizes (Target: Q1 2027)

---

## Implementation Phases

### Phase 1: Design / API Contract ✅
- [x] Define `GocryptfsBackend` container lifecycle API
- [x] Define `KeyRotationScheduler` `SecurityLevel` → schedule mapping
- [x] Define `MultiLevelEncryptedStorage` tier model

### Phase 2: Core Implementation ✅
- [x] `executeCommandSafe()` via `fork/execvp`
- [x] `createPasswordFile()` with `mkstemp` + `fchmod(0600)`
- [x] `isMounted()` via `/proc/mounts`
- [x] `KeyRotationScheduler::schedulerLoop()` background thread

### Phase 3: Error Handling & Edge Cases ✅
- [x] Already-mounted guard in `mountContainer()`
- [x] Not-mounted guard in `unmountContainer()`
- [x] `_exit(127)` in child on `execvp` failure
- [x] Exit code check in parent with error propagation

### Phase 4: Tests [ ]
- [ ] Unit tests (Target: Q3 2026)
- [ ] Integration tests (Target: Q3 2026)

### Phase 5: Performance / Hardening [~]
- [~] Fix `const_cast` in `createPasswordFile()` (Target: Q3 2026)
- [ ] Stdin key delivery (Target: Q3 2026)
- [ ] KDF integration (Target: Q4 2026)

### Phase 6: Documentation & Acceptance ✅
- [x] README, ARCHITECTURE, AUDIT, CHANGELOG, ROADMAP, SECURITY, FUTURE_ENHANCEMENTS

---

## Production Readiness Checklist

| Area | Status | Notes |
|------|--------|-------|
| `GocryptfsBackend` core API | ✅ | create/mount/unmount/isMounted implemented |
| Safe subprocess | ✅ | `fork/execvp`; no shell injection possible |
| Key file security | ✅ | `mkstemp` + `fchmod(0600)` + immediate `unlink` |
| `KeyRotationScheduler` | ✅ | Production-Ready (100/100) |
| `gocryptfs_backend.cpp` quality | ⚠️ | 76/100; `const_cast` and other issues to fix |
| Tests | ❌ | No confirmed tests |
| Stdin key delivery | ❌ | Planned Q3 2026 |
| KDF integration | ❌ | Planned Q4 2026 |

---

## Known Issues & Limitations

- `const_cast<std::string&>(path) = temp_template` in `createPasswordFile()` is a
  code smell; path should be an output parameter.
- Key material passes through `/tmp` before reaching gocryptfs; stdin delivery is safer.
- `KeyRotationScheduler` does not persist `last_check_ms`; restart loses interval state.
- `getBackendVersion()` uses `const_cast` to call `executeCommand()` on a const object.
