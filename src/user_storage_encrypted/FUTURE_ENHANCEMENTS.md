> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

# Future Enhancements — User Encrypted Storage Plugin

---

## ✅ 1. Stdin-Based Key Delivery — Implemented (v0.1.0)

### Scope
Replace the `/tmp` password file mechanism with direct stdin pipe delivery to the
gocryptfs process, eliminating key material from the filesystem entirely.

### Design Constraints
- Use gocryptfs `-passfile /dev/stdin` or `--extpass` option.
- The pipe write must complete before gocryptfs reads; use a dedicated writer thread
  or non-blocking write with `select`/`poll`.
- Key material must not appear in `/proc/PID/cmdline` or environment variables.

### Implementation
- `GocryptfsBackend::executeCommandWithStdin()`: creates `stdout_pipe` + `stdin_pipe`,
  forks gocryptfs with `dup2(stdin_pipe[0], STDIN_FILENO)`, writes hex key to
  `stdin_pipe[1]` in the parent, then reads stdout.
- `GocryptfsBackend::deliverKeyViaStdin()`: writes hex string + `\n` to write-end fd,
  calls `explicit_bzero()` on the heap buffer before returning.
- `createPasswordFile()` removed from production code paths.

### Test Coverage
- `GocryptfsStdinTest.NoTmpFileCreatedDuringMount` — verifies zero `/tmp/gocryptfs_key_*` files
- `GocryptfsStdinTest.DeliverKeyViaStdinWritesHexKey` — verifies hex format

---

## ✅ 2. Argon2id Key Derivation Function — Implemented (v0.1.0)

### Scope
Derive per-container encryption keys from a master key using Argon2id, providing
key stretching and domain separation between containers.

### Design Constraints
- Parameters: m=65536 (64 MB), t=3 iterations, p=4 threads — OWASP recommendation.
- Salt: 16-byte random per container, stored alongside encrypted container metadata.
- Derived key: 32 bytes (256-bit for gocryptfs AES-256-GCM).
- Latency budget: ≤ 200 ms on reference hardware (4-core, 4 GB RAM).

### Implementation
- `KeyDerivationService` abstract interface in `key_derivation_service.hpp`.
- `Argon2idKeyDerivationService` uses `argon2id_hash_raw()` from libargon2.
  Combined password = `master_key ‖ user_id ‖ container_id`.
- `GocryptfsBackend(KeyDerivationService*)` constructor; `resolveKey()` helper
  reads/writes `.themis_kdf_salt` in `encrypted_dir`.

### Test Coverage (9 unit + 1 performance test)
- Determinism, domain separation (container_id / user_id / salt), output length
- Error rejection (empty master key, short salt)
- Salt generation (correct length, randomness)
- Performance: 40 ms measured on CI (budget: ≤ 200 ms reference / ≤ 3000 ms CI)

---

## ✅ 3. Key Rotation Persistence — Implemented (v0.1.0)

### Scope
Persist `last_check_ms` for each `SecurityLevel` so that key rotation
intervals survive process restarts.

### Design Constraints
- Key: `user_storage:rotation_state:{level}` → JSON `{last_check_ms, interval_days}`.
- Load on `KeyRotationScheduler::scheduleRotation()` if store is provided.
- Write after each successful callback invocation.

### Implementation
- `IRotationStore` interface (2 methods: `get` / `put`) in `key_rotation_scheduler.hpp`.
- `KeyRotationScheduler::initialize(int, std::shared_ptr<IRotationStore>)`.
- `persistRotationState()` / `loadRotationState()` private helpers.
- `shutdown()` uses `std::condition_variable` for immediate thread wake.

### Test Coverage (6 tests)
- Load persisted last_check_ms across simulated restart
- No-store initialization
- Persistence key format validation (JSON schema)
- State restoration across restart
- Corrupted state ignored gracefully
- All four SecurityLevels persisted independently

---

## 4. Startup Stale Mount Reconciliation

### Scope
On `MultiLevelEncryptedStorage::initialize()`, scan `/proc/mounts` and unmount any
stale gocryptfs mounts belonging to this ThemisDB instance.

### Design Constraints
- Only unmount mounts matching `{base_path}/user_*_encrypted` path pattern.
- Log each stale unmount at WARN level.
- If unmount fails (device busy), log ERROR and continue — do not abort startup.

### Test Strategy
- Integration: simulate crash with active mount; restart and verify reconciliation unmounts the stale mount.

---

## 5. Prometheus Metrics

### Scope
Expose encrypted storage operational metrics via the ThemisDB Prometheus endpoint.

### Planned Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `user_storage_mounts_active` | Gauge | Currently mounted encrypted containers |
| `user_storage_mount_operations_total` | Counter | Total mount/unmount operations (label: `operation`) |
| `user_storage_key_rotations_total` | Counter | Key rotation callbacks fired (label: `level`) |
| `user_storage_container_size_bytes` | Gauge | Encrypted container size on disk |

### Performance Targets
- Metrics collection overhead ≤ 0.1 ms per operation.


---

## 1. Stdin-Based Key Delivery

### Scope
Replace the `/tmp` password file mechanism with direct stdin pipe delivery to the
gocryptfs process, eliminating key material from the filesystem entirely.

### Design Constraints
- Use gocryptfs `-passfile /dev/stdin` or `--extpass` option.
- The pipe write must complete before gocryptfs reads; use a dedicated writer thread
  or non-blocking write with `select`/`poll`.
- Key material must not appear in `/proc/PID/cmdline` or environment variables.

### Required Interfaces
- Internal `deliverKeyViaStdin(pid_t child_pid, const std::vector<uint8_t>& key)`.
- `createPasswordFile()` replaced by `createStdinPipe()` returning the write end fd.

### Test Strategy
- Unit: confirm no file is created under `/tmp` during mount.
- Integration: mount succeeds with correct key; fails with wrong key.

### Security
- Zero filesystem traces of key material.
- Pipe buffer cleared after write via `explicit_bzero` or equivalent.

### Performance Targets
- Pipe key delivery overhead: ≤ 1 ms additional latency vs file method.

---

## 2. Argon2id Key Derivation Function

### Scope
Derive per-container encryption keys from a master key using Argon2id, providing
key stretching and domain separation between containers.

### Design Constraints
- Parameters: m=65536 (64 MB), t=3 iterations, p=4 threads — OWASP recommendation.
- Salt: 16-byte random per container, stored alongside encrypted container metadata.
- Derived key: 32 bytes (256-bit for gocryptfs AES-256-GCM).
- Latency budget: ≤ 200 ms on reference hardware (4-core, 4 GB RAM).

### Required Interfaces
- `KeyDerivationService::derive(master_key, user_id, container_id, salt) -> std::vector<uint8_t>`
- `GocryptfsBackend` constructor accepts optional `KeyDerivationService*`.

### Test Strategy
- Unit: same inputs → same output (deterministic KDF).
- Unit: different `container_id` → different derived key.
- Performance test: KDF completes in ≤ 200 ms on CI hardware.

### Security / Reliability
- Master key must never be stored; it is derived at startup from a hardware security module or operator-supplied passphrase.
- Argon2id chosen over bcrypt/scrypt for GPU resistance and side-channel resilience.

---

## 3. Key Rotation Persistence

### Scope
Persist `last_check_ms` for each `SecurityLevel` to RocksDB so that key rotation
intervals survive process restarts.

### Design Constraints
- Key: `user_storage:rotation_state:{level}` → JSON `{last_check_ms, interval_days}`.
- Load on `KeyRotationScheduler::initialize()` if `RocksDBWrapper` is provided.
- Write after each successful callback invocation.

### Required Interfaces
- `KeyRotationScheduler::initialize(int interval_sec, std::shared_ptr<RocksDBWrapper> db = nullptr)`

### Test Strategy
- Integration: schedule rotation, simulate restart, verify next rotation time is preserved.

### Performance Targets
- RocksDB write for rotation state: ≤ 1 ms.

---

## 4. Startup Stale Mount Reconciliation

### Scope
On `MultiLevelEncryptedStorage::initialize()`, scan `/proc/mounts` and unmount any
stale gocryptfs mounts belonging to this ThemisDB instance.

### Design Constraints
- Only unmount mounts matching `{base_path}/user_*_encrypted` path pattern.
- Log each stale unmount at WARN level.
- If unmount fails (device busy), log ERROR and continue — do not abort startup.

### Test Strategy
- Integration: simulate crash with active mount; restart and verify reconciliation unmounts the stale mount.

---

## 5. Prometheus Metrics

### Scope
Expose encrypted storage operational metrics via the ThemisDB Prometheus endpoint.

### Planned Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `user_storage_mounts_active` | Gauge | Currently mounted encrypted containers |
| `user_storage_mount_operations_total` | Counter | Total mount/unmount operations (label: `operation`) |
| `user_storage_key_rotations_total` | Counter | Key rotation callbacks fired (label: `level`) |
| `user_storage_container_size_bytes` | Gauge | Encrypted container size on disk |

### Performance Targets
- Metrics collection overhead ≤ 0.1 ms per operation.
