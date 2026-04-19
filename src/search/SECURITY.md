> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Search Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Search query injection | Query sanitization and parameterized search |
| Information disclosure via search | Result filtering based on caller permissions |
| DoS via expensive queries | Query complexity limits and timeouts |
| Index poisoning | Write-path authorization checks |

## Security Controls

- Search results filtered by document-level ACLs
- Query rate limiting enforced per principal
- Audit logging for all search operations

## Known Limitations

None critical.
