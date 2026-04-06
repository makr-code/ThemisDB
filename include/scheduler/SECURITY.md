<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Scheduler Module

## Scope

Covers all public headers in `include/scheduler/`. Implementation hardening in `../../src/scheduler/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Task injection via cron expression | High — arbitrary execution | Cron expressions validated against allowlist syntax; no shell expansion |
| Sandbox escape via cgroups v2 | Critical — host privilege escalation | `sandbox_execution` uses seccomp-bpf to block dangerous syscalls; cgroups v2 limits CPU/memory |
| Auth context leakage across tasks | High — privilege confusion | `RequestContext` cleared via `clearRequestContext()` before thread pool returns thread |
| Distributed task replay | Medium — duplicate execution | `DistributedTaskCoordinator` uses idempotency keys; duplicate tasks rejected |
| Audit log tampering | High — forensic evasion | `TaskAuditManager` writes to append-only RocksDB column family; no delete operations |
| External scheduler adapter SSRF | Medium — internal network probe | `ExternalSchedulerAdapter` validates webhook URLs against allowlist |
| Task result store poisoning | Medium — wrong results | `TaskResultStore` keys include task ID + execution timestamp; no overwrite of completed tasks |
| Anomaly detector suppression | Low — undetected misbehavior | `TaskAnomalyDetector` thresholds are operator-configured and not user-controllable |

## Security Controls

1. **seccomp-bpf syscall filter** — `sandbox_execution` blocks all syscalls not in the task allowlist.
2. **cgroups v2 resource limits** — CPU shares and memory limits enforced per sandboxed task.
3. **TLS auth context RAII** — `RequestContext` is scoped; `clearRequestContext()` guaranteed by RAII wrapper.
4. **Idempotency keys** — `DistributedTaskCoordinator` rejects duplicate task submissions.
5. **Append-only audit log** — `TaskAuditManager` uses RocksDB append-only mode; no deletes.
6. **Cron expression validation** — Input validated against strict grammar before parsing.

## Known Limitations

- `sandbox_execution` on non-Linux platforms (macOS, Windows) uses process-level isolation only — weaker than cgroups v2 + seccomp-bpf.
- GPU task scheduling (planned Q4 2026) will require separate CUDA stream isolation security review.
