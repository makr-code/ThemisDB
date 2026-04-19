> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — User Encrypted Storage Plugin

## Threat Model

### 1. Key Material Exposure via Filesystem
- **Risk:** Cryptographic key material written to a temporary file in `/tmp` is readable
  by other processes with filesystem access before it is unlinked.
- **Mitigation (v0.1.0+):** Key material is delivered to gocryptfs via a stdin pipe
  (`-passfile /dev/stdin`). `executeCommandWithStdin()` forks the child with the pipe
  read end wired to stdin; `deliverKeyViaStdin()` writes the hex-encoded key to the
  pipe write end and calls `explicit_bzero` to clear the buffer. No key material is
  written to the filesystem.
- **Status:** ✅ Resolved in v0.1.0 — stdin delivery eliminates temp file

### 2. Shell Injection via Path Arguments
- **Risk:** Encrypted directory paths or mount points containing shell metacharacters
  could be injected into shell-executed commands.
- **Mitigation:** All subprocess invocations use `executeCommandSafe()` which calls
  `fork()` + `execvp()` with an explicit `std::vector<std::string>` argument list.
  `system()` and `popen()` are not used anywhere in this module.
- **Status:** ✅ Shell injection is structurally impossible

### 3. Privilege Escalation via FUSE
- **Risk:** Mounting a FUSE filesystem under a path writable by another user could
  allow that user to access the decrypted data.
- **Mitigation:** Mount points are created with `mkdir(path, 0700)` limiting access
  to the process owner. Operators must ensure mount point parent directories have
  appropriate permissions.
- **Status:** ✅ 0700 on mount creation; operator responsibility for parent dirs

### 4. Key Rotation Callback Failure
- **Risk:** If the key rotation callback throws or crashes, the `schedulerLoop()`
  background thread may terminate, silently stopping all future rotations.
- **Mitigation:** Callbacks should be written to catch all exceptions. <!-- TODO: verify -->
  An exception guard in `schedulerLoop()` is recommended.
- **Status:** ⚠️ No confirmed exception guard in `schedulerLoop()`

### 5. Container Directory Traversal
- **Risk:** A crafted path with `../` components could escape the intended container
  directory when passed to `createContainer()`.
- **Mitigation:** The module does not sanitise path arguments beyond what `mkdir()` and
  `execvp()` enforce at the OS level. Callers are responsible for validating paths.
- **Status:** ⚠️ No explicit path normalisation; operator/caller responsibility

### 6. Stale Mounts After Crash
- **Risk:** If the ThemisDB process crashes with active FUSE mounts, the mount points
  remain mounted until the OS unmounts them.
- **Mitigation (v0.2.0+):** `MultiLevelEncryptedStorage::reconcileStaleMounts()` is called
  from `initialize()` before any `initializeLevel()` invocation. It scans `/proc/mounts`
  for orphaned FUSE mounts and calls `fusermount -u` (with `umount` fallback). Non-fatal:
  failures are logged at ERROR level and initialization continues.
- **Status:** ✅ Resolved in v0.2.0

---

## Security Controls Summary

| Control | Implementation | Status |
|---------|---------------|--------|
| No shell injection | `fork/execvp` with explicit arg vector | ✅ |
| Stdin key delivery | Key piped via stdin; `explicit_bzero` clears buffer | ✅ |
| Argon2id KDF | Per-container key derivation; salt in `.themis_kdf_salt` | ✅ |
| Mount point permissions | `mkdir(path, 0700)` | ✅ |
| Mount guard | `isMounted()` before every mount/unmount | ✅ |
| Stale mount cleanup | `reconcileStaleMounts()` on startup | ✅ |
| Scheduler exception guard | Catch in `schedulerLoop()` | <!-- TODO: verify --> ⚠️ |

---

## Known Limitations

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| USE-SEC-01 | <!-- TODO: verify --> No confirmed exception guard in `schedulerLoop()`; unhandled exception kills thread | Medium | Open |
| USE-SEC-02 | No path normalisation / traversal prevention on container paths | Medium | Open |
