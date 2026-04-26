> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Scheduler Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Privilege escalation via scheduled tasks | `RequestContext` TLS API propagates authenticated `user_id`/`client_ip` to all audit events (`include/scheduler/task_scheduler.h:398–415`) |
| AQL injection via task payloads | AQL injection detection via `security/aql_injection_detector.h`; injection-checked before every task execution |
| Runaway task resource consumption | Per-task `timeout` and `max_retries` limits; `sandbox_execution` flag wraps task functions in `ModuleSandbox` with cgroups v2 memory/CPU limits and seccomp-bpf syscall filtering on Linux (`include/scheduler/task_scheduler.h:387`) |
| DoS via excessive job scheduling | Rate limiting on schedule creation <!-- TODO: verify --> |
| Cron expression injection | Strict cron expression validation in `CronExpression::parse()` |
| Job state poisoning | Job state stored with integrity checksums <!-- TODO: verify --> |
| Security event non-repudiation | `TaskSecurityEvent` / `TaskSecurityEventType` audit trail (`include/scheduler/task_audit_event.h`) |

## Security Controls

### Verified (from headers)

- **`sandbox_execution` flag** (`include/scheduler/task_scheduler.h:387`): when `true`, wraps user-provided task functions in `modules::ModuleSandbox` (cgroups v2 memory/CPU limits, seccomp-bpf syscall filtering on Linux; graceful fallback in constrained environments).
- **`RequestContext` TLS auth context** (`include/scheduler/task_scheduler.h:398`): HTTP handlers call `setRequestContext({user_id, client_ip})` before scheduler operations; scheduler thread falls back to `"system"`. All audit events carry actual `user_id`/`client_ip` instead of hardcoded `"system"`.
- **`TaskSecurityEvent` / `TaskSecurityEventType`** (`include/scheduler/task_audit_event.h`): structured security event schema for SIEM integration.
- **`utils::AuditLogger`** (`include/scheduler/task_scheduler.h:76,435`): optional audit logger injected at construction for persistent SIEM-compatible audit trail.
- **Maximum concurrent jobs enforced** via `Config::max_concurrent_tasks` to prevent resource exhaustion.

## Known Limitations

None critical.
