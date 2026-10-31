---
Author: "ThemisDB Contributors"
Created: "2024-01-15"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Operational Audit and Evidence: Current Interface Notes

This guide covers the module-local interface in
[`include/governance/operational_audit.h`](../../include/governance/operational_audit.h)
and implementation in `operational_audit.cpp`. It replaces prior examples that
assumed global singleton accessors, automatic cross-module logging, or
performance guarantees not established by the current implementation.

## Components and boundaries

- `OperationalAuditLogger` is constructed with a `max_events` limit (default
  100,000) and exposes event logging, query, export, statistics, and clear
  methods. Its declared event store is in-process bounded state. The API does
  not promise durable persistence or that all governance operations are logged
  automatically.
- `EventCorrelationEngine` is constructed with a shared logger and exposes
  correlation creation/query/export methods. Correlation is not automatic
  across modules; callers must supply and propagate correlation/causality data.
- `ComplianceEvidenceCollector` is also constructed with a shared logger. Its
  API exposes collection, recording, linking, and query methods. Evidence
  completeness depends on caller input and available events; do not present its
  output as independent regulatory proof.

There are no documented `getGlobalAuditLogger()`,
`getGlobalCorrelationEngine()`, or `getGlobalEvidenceCollector()` accessors in
the current public header. The old sample signatures and the historical event
code table in this file are not the current contract; use the enum and function
declarations in `operational_audit.h`.

## Safe use

1. Construct and own the logger, correlation engine, and collector according
   to their header contracts; keep shared dependencies alive for their use.
2. Explicitly call the logging or evidence APIs at the integration points that
   require records. Avoid placing sensitive payloads in event context.
3. Check limits and statistics; bounded in-process records can be lost when
   evicted or when the process ends. Configure a separate durable store if
   required by the deployment.
4. Treat fingerprints and audit-related output as module data, not proof that
   a legal retention, key-management, or chain-of-custody process is configured.
5. Confirm the selected build's test registration and result before relying on
   focused test coverage. `test_operational_audit_evidence.cpp` is currently
   excluded because `operational_audit.cpp` does not compile against the public
   header; source presence is not runnable coverage.

## Validation and performance

The operational-audit focused test source is
`tests/governance/test_operational_audit_evidence.cpp`, but it is excluded from
the module-focused CMake loop until the implementation/header mismatch is
resolved. No overhead percentage, query-latency target, or test pass is
asserted here. For audit integrity and
batch writing, see
[`AUDIT_INTEGRITY_DESIGN.md`](AUDIT_INTEGRITY_DESIGN.md) and
[`governance_audit_integrity.h`](../../include/governance/governance_audit_integrity.h)
and [`audit_batch_writer.h`](../../include/governance/audit_batch_writer.h).

## References

- [Operational audit public API](../../include/governance/operational_audit.h)
- [Governance test CMake](../../tests/governance/CMakeLists.txt)
- [Module architecture](ARCHITECTURE.md) · [Trust boundaries](TRUST.md)
