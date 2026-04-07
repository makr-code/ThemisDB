<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Governance Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Governance module public headers expose policy evaluation, compliance reporting, data masking, and OPA integration interfaces. Security concerns focus on policy bypass, privilege escalation via inheritance, and integrity of compliance audit trails.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Policy bypass via tenant hierarchy manipulation | `cross_tenant_policy_inheritance.h` enforces cycle detection; most-restrictive-wins merge prevents privilege escalation up the hierarchy |
| Unauthorized policy modification | `policy_manager.h` requires `admin:policy:write` scope; all mutations recorded in `policy_version_history.h` |
| Compliance audit trail tampering | `policy_version_history.h` records immutable timestamped entries; deletions require elevated scope |
| OPA policy injection | `opa_adapter.h` validates Rego policy syntax before registration; malformed policies rejected |
| Data masking bypass | `data_masker.h` applies masking at query result level; masking rules enforced before data leaves the module |
| Cross-tenant policy leakage | `evaluateEffectivePolicy()` resolves policies within tenant subtree only; sibling tenant policies are not visible |

## Security Controls

- Most-restrictive-wins merge prevents policy escalation across tenant hierarchies.
- All policy mutations are version-tracked with author and timestamp.
- OPA policies validated before registration to prevent injection.
- Data masking applied at result boundary.

## Known Limitations

- OPA adapter requires external OPA binary or embedded wasm; ensure OPA version is kept patched.
- Policy file hot-reload (`policy_file_watcher.h`) watches filesystem paths; restrict file system permissions on policy directories.
- Implementation-level security details: `../../src/governance/SECURITY.md`.
