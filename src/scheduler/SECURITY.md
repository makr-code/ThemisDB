> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Scheduler Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Privilege escalation via scheduled tasks | Tasks run under caller's security context |
| DoS via excessive job scheduling | Rate limiting on schedule creation |
| Cron injection | Strict cron expression validation and sanitization |
| Job state poisoning | Job state stored with integrity checksums |

## Security Controls

- Job creation requires authentication
- Task execution audited via utils/AuditLogger
- Maximum concurrent jobs enforced to prevent resource exhaustion

## Known Limitations

None critical.
