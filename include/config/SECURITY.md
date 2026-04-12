<!-- Status: current | validated: 2026-04-08 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Config Module Public Headers

## Security Scope

This document covers security properties exposed through the public headers in `include/config/`.
For implementation-level security findings see `src/config/SECURITY.md`.

---

## `config_path_resolver.h`

### Path Traversal Prevention
`ConfigPathResolver::resolve()` and `tryResolve()` reject any path containing `..` segments or
embedded null bytes by throwing `InvalidPathException`.  Consumers **must not** bypass this by
pre-processing the input.

### Symlink Escape Hardening
When `validatePath()` resolves a path via `realpath()`, it checks that the canonical path
remains within the configured config root.  Symlinks pointing outside the root are rejected.

### Environment Variable Injection
`THEMIS_CONFIG_ENV` accepts only `dev`, `staging`, or `prod`.  Unknown values fall back to
`prod` with a `stderr` warning — they do not propagate to filesystem operations.

---

## `config_schema_validator.h`

### SSRF Prevention (`$ref` Resolution)
`ConfigSchemaValidator` resolves `$ref` only within the same schema document
(`$defs` / `definitions`).  External URI references are explicitly rejected to prevent
Server-Side Request Forgery (SSRF) attacks.  Issue #3742 (resolved in v2.0.0).

### Recursive `$ref` Cycle Detection
The validator tracks visited `$ref` paths and reports a schema error on cycle detection,
preventing infinite recursion.

---

## `config_encrypted_store.h`

### AES-256-GCM Encryption
Each `set()` call generates a fresh 96-bit random IV.  Authentication tags (128 bits) are
verified on every `get()`, so tampered ciphertexts are detected before values are returned.

### Key Material in Snapshots
`serialize()` includes the AES-256 key in the JSON snapshot.  Callers **must** wrap the
snapshot in a master-key envelope before writing to disk or transmitting over the network.

### Thread Safety
`get`, `tryGet`, `contains`, `keys`, `size`, `serialize` hold `std::shared_lock` (concurrent
readers allowed).  `set`, `remove`, `clear`, `rotateKey`, `deserialize` hold `std::unique_lock`.
`rotateKey` re-encryption holds `unique_lock` for full duration to guarantee atomicity.

---

## `config_audit_log.h`

### Audit Entry Integrity
`ConfigAuditLog` is an in-memory ring-buffer only.  It does not write to disk, so it does not
survive process restart.  For durable audit trails, consumers must persist `snapshot()` entries
to a tamper-evident log store.

---

## `config_errors.h`

Exception message formats are **stable** — they are part of the public ABI.  Changes to
message text require a major version bump to avoid breaking operators who parse error messages.

---

## References

- `src/config/SECURITY.md` — full security audit and findings
- `src/config/AUDIT.md` — implementation-level audit report
