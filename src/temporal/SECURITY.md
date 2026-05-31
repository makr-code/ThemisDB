# Security - Temporal Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the temporal module focuses on deterministic temporal query semantics, explicit version-lifecycle transitions, bounded retention/snapshot behavior, and observable conflict/CDC error signaling.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| silent temporal version inconsistency | explicit system-time/valid-time lifecycle behavior |
| hidden snapshot or retention data-loss path | deterministic snapshot/retention status outcomes |
| opaque temporal conflict handling | explicit conflict-resolution outcomes |
| unobserved CDC/indexing degradation | diagnosable CDC/index behavior surfaces |

## Implemented Security Controls

- temporal query and version transitions expose explicit outcomes.
- snapshot and retention paths remain deterministic and diagnosable.
- conflict-resolution behavior is explicit and observable.
- CDC and indexing faults are surfaced through module diagnostics.

## Security Follow-ups

- expand fault-injection coverage for retention/snapshot edge failures.
- deepen stress coverage for concurrent bitemporal update contention.
- tighten diagnostics taxonomy across temporal lifecycle incidents.

## Sourcecode Verification (Module: temporal/security)

- Verified files:
  - src/temporal/temporal_query_engine.cpp
  - src/temporal/system_versioned_table.cpp
  - src/temporal/snapshot_manager.cpp
  - src/temporal/retention_manager.cpp
  - src/temporal/temporal_conflict_resolver.cpp
  - src/temporal/temporal_cdc.cpp
- Verified controls:
  - explicit temporal lifecycle and query error signaling
  - deterministic snapshot/retention behavior
  - observable conflict and CDC incident surfaces