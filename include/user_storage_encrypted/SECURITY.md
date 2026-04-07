<!-- Status: current | validated: 2026-04-06 -->

# User Storage Encrypted — Security

## Scope
Security considerations for the public headers in `include/user_storage_encrypted/`. This covers the encryption backend abstraction, key lifecycle, tiered storage boundaries, and known limitations as of v0.0.1.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Key material written to /tmp | Key exfiltration by local user | Planned: stdin-pipe delivery; current: mkstemp 0600 |
| Key material in process argv | Key visible in `/proc/<pid>/cmdline` | Keys passed via file/pipe, never argv |
| Tier boundary bypass (HOT→COLD without re-encryption) | Data at rest under weaker encryption | `MultiLevelEncryptedStorage` enforces re-encryption on tier change |
| Key rotation failure leaving stale key active | Long-lived key increases compromise window | `KeyRotationScheduler` retries; rollback on failure |
| Backend subprocess hijack | Arbitrary code execution | Subprocess path validated; no shell interpolation |
| Insecure deserialization of `UserModels` | Memory corruption | Input validation in `user_models.hpp`; fuzzing planned |

## Security Controls

- **Backend abstraction:** `IEncryptionBackend` prevents direct filesystem access; all encryption operations mediated.
- **Security levels:** `SecurityLevel::MAXIMUM` selects strongest available algorithm; callers cannot downgrade silently.
- **Key file permissions:** `mkstemp` with mode 0600 limits key file access to process owner (interim measure).
- **No key in argv:** Key material is never passed as a command-line argument to the gocryptfs subprocess.
- **Tier-aware encryption:** Each HOT/WARM/COLD tier enforces its configured `SecurityLevel`; no silent downgrade on tier transition.

## Known Limitations

- **LIMITATION-01 (Medium):** Key material currently written to a temporary file (mkstemp 0600) before being passed to gocryptfs. This creates a short-lived on-disk exposure window. Remediation (stdin-pipe) is planned for Q2 2026.
- **LIMITATION-02 (Info):** No unit or integration tests exist yet; security properties are untested programmatically.
- **LIMITATION-03 (Info):** `UserModels` deserialization has not been fuzz-tested. Planned for Phase 5.
