# Security - Chimera Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the chimera module focuses on safe adapter lifecycle handling, bounded dispatch behavior, structured failure reporting, and minimizing ambiguity between simulation and engine-backed paths.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| operation attempts without valid adapter connection state | explicit connection-state checks and structured errors |
| unsupported engine dispatch ambiguity | explicit not-implemented error signaling on unavailable paths |
| capability/behavior drift in adapter contract reporting | centralized capability reporting surfaces in adapter runtime |
| accidental exposure via simulation-vs-engine mismatch | documented conditional dispatch boundaries and explicit failure behavior |

## Implemented Security Controls

- adapter operations enforce connection preconditions.
- unsupported dispatch paths fail explicitly and non-silently.
- capability reporting is surfaced through adapter contract methods.
- adapter runtime state is process-local and bounded.

## Security Follow-ups

- continue hardening behavior parity between simulation and engine-backed paths.
- maintain deterministic error taxonomy for dispatch and capability failures.
- keep diagnostics actionable for adapter integration incidents.

## Sourcecode Verification (Module: chimera/security)

- Verified files:
  - src/chimera/themisdb_adapter.cpp
- Verified controls:
  - explicit connection-state gating
  - structured unsupported-dispatch failures
  - bounded adapter runtime and capability surfaces