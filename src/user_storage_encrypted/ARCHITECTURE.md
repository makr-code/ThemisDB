> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# User Encrypted Storage Plugin — Architecture Guide

**Version:** 0.0.1
**Last Updated:** 2026-04-06
**Module Path:** `src/user_storage_encrypted/`

---

## 1. Overview

The User Encrypted Storage plugin provides transparent, per-user filesystem encryption
using gocryptfs (a FUSE-based encrypted filesystem). The plugin is structured in three
layers: the gocryptfs subprocess backend, the key rotation scheduler, and the
multi-level storage orchestrator.

---

## 2. Design Principles

- **No shell execution** — all gocryptfs and system utility invocations use
  `fork/execvp` with an explicit `std::vector<std::string>` argument list.
  This eliminates shell injection vulnerabilities.
- **Secure key material handling** — keys are written to a `mkstemp()` file with
  `fchmod(fd, 0600)` before the first byte is written. The file is `unlink()`ed
  immediately after gocryptfs reads it.
- **Fail-safe unmount** — `isMounted()` is checked before every mount and unmount
  call to avoid double-mount and spurious unmount errors.
- **Security-level–driven rotation** — `KeyRotationScheduler` maps each
  `SecurityLevel` to a `RotationSchedule`; a background thread fires callbacks
  at the configured interval.
- **Tiered storage** — `MultiLevelEncryptedStorage` manages independent encrypted
  containers per storage tier (HOT/WARM/COLD) with independent keys.

---

## 3. Component Architecture

### 3.1 Component Diagram

```
┌──────────────────────────────────────────────────────────┐
│            MultiLevelEncryptedStorage                    │
│  HOT tier  ─── GocryptfsBackend + key_hot               │
│  WARM tier ─── GocryptfsBackend + key_warm              │
│  COLD tier ─── GocryptfsBackend + key_cold              │
└──────────────────────────────────────────────────────────┘
                           │
              ┌────────────▼────────────────────┐
              │       GocryptfsBackend           │
              │                                 │
              │  initialize(config_json)        │
              │  checkAvailability()            │
              │  createContainer(enc, mnt, key) │
              │  mountContainer(enc, mnt, key)  │
              │  unmountContainer(mount_point)  │
              │  isMounted(mount_point)         │
              │  getBackendVersion()            │
              └──────────────┬──────────────────┘
                             │ fork / execvp
              ┌──────────────▼──────────────────┐
              │  executeCommandSafe(args)        │
              │  fork() + execvp()               │
              │  stdout/stderr via pipe()        │
              │  waitpid() for exit status       │
              └──────────────┬──────────────────┘
                             │
              ┌──────────────▼──────────────────┐
              │   gocryptfs / fusermount / umount│
              │   (system binaries)              │
              └─────────────────────────────────┘

              ┌─────────────────────────────────┐
              │    KeyRotationScheduler          │
              │                                 │
              │  initialize(interval_sec)       │
              │  scheduleRotation(level, days,  │
              │    auto_rotate, callback)       │
              │  triggerRotation(level)         │
              │  getNextRotationTime(level)     │
              │  shutdown()                     │
              │                                 │
              │  Impl:                          │
              │    map<SecurityLevel, Schedule> │
              │    std::thread schedulerLoop()  │
              │    std::atomic<bool> running_   │
              └─────────────────────────────────┘
```

### 3.2 Component Table

| Component | File | Responsibility |
|-----------|------|----------------|
| `GocryptfsBackend` | `gocryptfs_backend.cpp/.hpp` | FUSE container create/mount/unmount via safe subprocess |
| `KeyRotationScheduler` | `key_rotation_scheduler.cpp/.hpp` | Timed key rotation per SecurityLevel |
| `MultiLevelEncryptedStorage` | `multi_level_storage.cpp` | HOT/WARM/COLD tier orchestration |

---

## 4. Key Material Flow (v0.1.0+)

```
Caller supplies key_material: std::vector<uint8_t>
  │
  ├─ Optional: resolveKey(encrypted_dir, key_material, create_salt)
  │     If KDF service configured:
  │       Read/write {encrypted_dir}/.themis_kdf_salt
  │       Argon2id(master_key ‖ salt) → derived_key
  │     Otherwise: use key_material directly
  │
  ├─ deliverKeyViaStdin(args, derived_key):
  │     executeCommandWithStdin(args, derived_key)
  │       fork() child: stdin ← pipe read end
  │       parent: write hex(derived_key) → pipe write end
  │       explicit_bzero(buffer) after write
  │     → child receives key on STDIN; gocryptfs uses -passfile /dev/stdin
  │
  └─ No key material written to filesystem
```

---

## 5. Subprocess Model

`executeCommandSafe()` uses `fork/execvp` — never `system()` or `popen()`:

```
Parent:
  pipe(pipe_fd)
  fork()
    ├─ Child:
    │    dup2(pipe_fd[1], STDOUT_FILENO)
    │    dup2(pipe_fd[1], STDERR_FILENO)
    │    execvp(c_args[0], c_args.data())
    │    _exit(127) on failure
    └─ Parent:
         read(pipe_fd[0]) → output string
         waitpid(pid, &status, 0)
         return Result<string> on success or error with exit code
```

---

## 6. Mount Detection

`isMounted()` on Linux reads `/proc/mounts` line-by-line and checks for the
`mount_point` substring. On macOS/BSD it runs `mount` via `executeCommand()`.

---

## 7. Key Rotation Scheduling

```
KeyRotationScheduler::schedulerLoop():
  while running_:
    sleep(check_interval_seconds)
    for each (level, schedule) in schedules:
      if auto_rotate:
        elapsed = now - last_check_ms
        if elapsed >= schedule.interval_days * 86400000:
          schedule.callback(level)
          schedule.last_check_ms = now
```

---

## 8. Security Controls

| Control | Implementation |
|---------|---------------|
| No shell injection | `execvp` with explicit arg vector; no `system()` or `popen()` |
| Stdin key delivery | Key piped to gocryptfs via stdin (`-passfile /dev/stdin`); no filesystem trace |
| Buffer zeroing | `explicit_bzero` clears pipe write buffer after key delivery |
| Argon2id KDF | Optional per-container key derivation from master key; salt in `{encrypted_dir}/.themis_kdf_salt` |
| Mount double-check | `isMounted()` checked before every mount/unmount |
| Stale mount cleanup | `reconcileStaleMounts()` called from `initialize()` via `/proc/mounts` scan |

---

## 9. Known Limitations

- `gocryptfs_backend.cpp` uses a deprecated `executeCommand()` private method which
  delegates to `executeCommandSafe()`; the wrapper is retained for backward compatibility.
- `getBackendVersion()` uses `const_cast` to call `executeCommand()` on a const object (cosmetic).
- <!-- TODO: verify --> Exception guard in `schedulerLoop()` is recommended to prevent silent
  thread termination on unhandled callback exceptions.
