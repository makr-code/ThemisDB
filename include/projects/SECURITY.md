<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Projects Module

## Scope

Covers all public headers in `include/projects/`. Implementation hardening in `../../src/projects/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Unauthorized project access | High — data exfiltration | `DocumentManager` enforces project-level RBAC on every operation |
| Snapshot poisoning | High — data corruption | Snapshots are content-addressed (SHA-256); tampering is detected on restore |
| Project name injection | Low — enumeration | Project names are validated against allowlist character set (alphanumeric, dash, underscore) |
| Template injection | Medium — malicious bootstrapping | Templates are verified against a schema and executed in a restricted context |
| Privilege escalation via object sharing | Medium — cross-project leak | Cross-project operations require explicit permission delegation (planned; not yet active) |

## Security Controls

1. **RBAC on all operations** — `DocumentManager` checks role token on every create/read/update/delete.
2. **Content-addressed snapshots** — SHA-256 digest verified on snapshot restore.
3. **Input validation** — project and document names validated for length (≤ 256 chars) and allowed characters.
4. **Template schema validation** — project templates validated before instantiation.

## Known Limitations

- Cross-project object sharing (planned Q4 2026) will require a full RBAC delegation design review.
- Audit logging for project operations is planned for Q3 2026 and not yet active.
