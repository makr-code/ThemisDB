> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-31 -->
# Security - Transaction Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|---|---|
| Inconsistent distributed transaction state under faults | Distributed coordinator tracks prepare/commit/abort transitions with recovery-oriented handling |
| Transaction contention and deadlock abuse | Locking and deadlock handling paths with timeout/abort behavior |
| Compensation drift in long orchestration chains | SAGA compensation flows are designed for deterministic reverse handling |
| Excessive runtime pressure on transaction paths | Timeout and operational guardrails in manager/coordinator paths |

## Security Controls

- Transaction lifecycle paths return explicit status/error outcomes on invalid transitions.
- Distributed coordinator paths include liveness and recovery hooks.
- SAGA and distributed SAGA paths provide compensation-oriented failure handling.
- Audit-oriented transaction logging surfaces are available via transaction auditor APIs.

## Known Limitations

- Distributed transactions can still expose operationally complex in-doubt windows under severe multi-node failure conditions; recovery logic must run as configured.
- Compensation correctness depends on step definitions staying idempotent and side-effect safe.

## Sourcecode Verification (Module: transaction/security)

- Verified files:
  - `src/transaction/transaction_manager.cpp`
  - `src/transaction/lock_manager.cpp`
  - `src/transaction/distributed_transaction_manager.cpp`
  - `src/transaction/saga_orchestrator.cpp`
  - `src/transaction/distributed_saga.cpp`
  - `src/transaction/transaction_auditor.cpp`
- Verified controls:
  - transaction transition and timeout/error paths
  - distributed liveness/recovery behavior surfaces
  - compensation and auditing-related security-relevant flows
