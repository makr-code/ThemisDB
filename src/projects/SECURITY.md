> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

# Security — Projects Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Unauthorized project state transitions | `ProjectLifecycle` guards every transition; invalid transitions rejected with an error |
| Privilege escalation in collaboration sessions | `CollaborationManager` enforces `Permission` enum at every session operation; callers without `WRITE` or higher cannot submit changes |
| Audit log tampering | `IProjectAuditLog` is append-only; entries include actor identity and timestamp |
| Sensitive project data in export bundles | `IProjectBundleManager` export options (`BundleExportOptions`) allow callers to control what is included; encryption at rest handled by the storage module |
| Invalid input to diff/merge | `ProjectDiff` and `ProjectMerge` validate both versions exist before computing deltas |

## Security Controls

- Permission-gated collaboration: all session writes require at minimum `WRITE` permission
- State-machine guards: `ProjectLifecycle` rejects all unauthorized or invalid state transitions
- Append-only audit trail: `IProjectAuditLog` records actor identity, action, and timestamp for every project operation
- No secrets or credentials are stored by this module; key management is handled by `src/security/`

## Known Limitations

- Test coverage for permission enforcement edge cases in `CollaborationManager` is not yet confirmed; security-focused tests are planned for 2026-Q4.
- Merge conflict resolution in `ProjectMerge` returns unresolved conflicts to the caller; no automatic policy is applied.

