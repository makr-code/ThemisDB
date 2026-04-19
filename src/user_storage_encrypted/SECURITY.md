> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — User Encrypted Storage Plugin

## Threat Model

### 1. Key Material Exposure via Filesystem
- **Risk:** Cryptographic key material written to a temporary file in `/tmp` is readable
  by other processes with filesystem access before it is unlinked.
- **Mitigation:** Temp files are created with `mkstemp()` and `fchmod(fd, 0600)` before
  the first write byte is committed. The file is `unlink()`ed immediately after gocryptfs
  reads it. The key is hex-encoded to avoid null-byte truncation issues.
- **Residual Risk:** On systems with swap enabled, the key hex string may persist in
  swap space. Stdin-based key delivery (planned v0.1.0) eliminates the temp file entirely.
- **Status:** ⚠️ Partially mitigated; stdin delivery planned Q3 2026

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
- **Mitigation:** Callbacks should be written to catch all exceptions. An exception
  guard in the loop is planned for v0.1.0.
- **Status:** ⚠️ No exception guard in `schedulerLoop()`; planned Q3 2026

### 5. Container Directory Traversal
- **Risk:** A crafted path with `../` components could escape the intended container
  directory when passed to `createContainer()`.
- **Mitigation:** The module does not sanitise path arguments beyond what `mkdir()` and
  `execvp()` enforce at the OS level. Callers are responsible for validating paths.
- **Status:** ⚠️ No explicit path normalisation; operator/caller responsibility

### 6. Stale Mounts After Crash
- **Risk:** If the ThemisDB process crashes with active FUSE mounts, the mount points
  remain mounted until the OS unmounts them.
- **Mitigation:** `isMounted()` is checked on every `mountContainer()` call and
  skips re-mounting if already mounted. A startup reconciliation scan is planned
  for v0.2.0 to unmount stale mounts from previous process instances.
- **Status:** ⚠️ Startup reconciliation planned Q4 2026

---

## Security Controls Summary

| Control | Implementation | Status |
|---------|---------------|--------|
| No shell injection | `fork/execvp` with explicit arg vector | ✅ |
| Key file permission | `fchmod(fd, 0600)` before write | ✅ |
| Key file lifetime | `unlink()` immediately after subprocess reads | ✅ |
| Mount point permissions | `mkdir(path, 0700)` | ✅ |
| Mount guard | `isMounted()` before every mount/unmount | ✅ |
| Stdin key delivery | Eliminates temp file entirely | ❌ (planned Q3 2026) |
| Scheduler exception guard | Catch in `schedulerLoop()` | ❌ (planned Q3 2026) |

---

## Known Limitations

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| USE-SEC-01 | Key material passes through `/tmp`; swap-based leakage possible | High | Open (planned Q3 2026) |
| USE-SEC-02 | No exception guard in `schedulerLoop()`; unhandled exception kills thread | Medium | Open |
| USE-SEC-03 | No path normalisation / traversal prevention on container paths | Medium | Open |
| USE-SEC-04 | No startup reconciliation of stale FUSE mounts | Low | Open (planned Q4 2026) |
