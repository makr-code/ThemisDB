<!-- Status: current | validated: 2026-03-22 -->

# User Storage Encrypted — Future Enhancements

## stdin-pipe Key Delivery

### Scope
Replace the current `mkstemp` + `/tmp` password-file approach in `GocryptfsBackend` with secure in-process stdin-pipe delivery. This eliminates the window where key material exists as a world-readable (mode 0600 but still on-disk) temporary file.

### Design Constraints
- Must remain compatible with the `IEncryptionBackend` interface.
- Must not introduce blocking I/O that starves the calling thread for > 500 ms.
- Key material must never appear in process command-line arguments.

### Required Interfaces
- `IEncryptionBackend::mount(const MountOptions&)` — add `pipe_fd` field to `MountOptions`.
- `GocryptfsBackend` internals: replace `mkstemp` path with `pipe(2)` + `write` before `execvp`.

### Implementation Notes
- Use `posix_spawn` or `fork/execvp` with pre-opened pipe fds.
- Close write-end of pipe immediately after writing key material.
- Zeroize key buffer with `explicit_bzero` / `OPENSSL_cleanse` after write.

### Test Strategy
- Unit: mock `IEncryptionBackend`; verify no `/tmp` file created during mount.
- Integration: mount real gocryptfs volume via pipe; verify encrypted directory accessible.
- Security: run under `strace` and verify key does not appear in `openat` calls to `/tmp`.

### Performance Targets
- Mount latency overhead vs. current file approach: < 5 ms on Linux x86-64.

### Security / Reliability
- Eliminate TOCTOU window on key file.
- Key material in memory only; zeroized immediately after pipe write.
- Failure mode: if pipe write fails, mount must fail cleanly with no partial state.

---

## KDF Integration

### Scope
Integrate a Key Derivation Function (HKDF-SHA256 via `../../include/utils/hkdf_helper.h`) into `KeyRotationScheduler` to derive per-rotation subkeys from a root key, avoiding raw key reuse.

### Design Constraints
- Root key never leaves secure memory region.
- Derived keys scoped to single rotation interval.

### Required Interfaces
- `hkdf_helper.h`: `HkdfHelper::derive(root_key, context, length)`.
- `KeyRotationScheduler`: add `set_kdf_context(std::string_view)` method.

### Implementation Notes
- Context string: `"themisdb.user_storage.<user_id>.<rotation_epoch>"`.
- Salt: random 32 bytes, stored alongside encrypted volume metadata.

### Test Strategy
- Unit: verify derived keys differ across rotation epochs.
- Property-based: same inputs always produce same derived key (determinism).

### Performance Targets
- KDF call overhead < 1 ms per rotation event.

### Security / Reliability
- HKDF output is cryptographically independent across epochs.
- Root key compromise limited to past epochs if forward secrecy mode enabled.

---

## Unit and Integration Test Suite

### Scope
Initial test suite covering all six public-header interfaces.

### Test Strategy
- `encryption_backend_interface.hpp`: mock backend; verify interface contract.
- `gocryptfs_backend.hpp`: integration tests with real gocryptfs binary.
- `key_rotation_scheduler.hpp`: timer-based unit tests; mock clock.
- `multi_level_storage.hpp`: tier promotion/demotion; encryption boundary tests.
- `security_level.hpp`: enum value mapping; algorithm selection.
- `user_models.hpp`: serialization round-trips.

### Performance Targets
- All unit tests complete in < 30 s on CI.
