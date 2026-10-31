---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
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

# Governance Audit Integrity: Implemented Surface and Limits

This page describes the module-local API in
[`include/governance/governance_audit_integrity.h`](../../include/governance/governance_audit_integrity.h)
and `src/governance/governance_audit_integrity.cpp`. It supersedes earlier
claims about an `AuditIntegrityVerifier`, `AuditKeyStore`, persistent audit
store, automatic key rotation, CLI commands, and guaranteed retention behavior:
those names/operations are not established by this interface.

## Declared surfaces

- `ImmutableAuditEntry` and `SignatureInfo` provide JSON conversion and entry
  integrity metadata. `ImmutableAuditEntry::verifyIntegrity()` checks that
  signature/hash fields are present and that a non-first sequence has a prior
  hash; it does not itself verify a cryptographic signature or chain.
- `AuditSigner` exposes HMAC-SHA256 and RSA-SHA256 modes, signing, and signature
  verification. The current RSA-named implementation is explicitly simplified:
  it computes a SHA-256 digest and does not perform RSA private/public-key
  signing or verification. Do not treat it as RSA security.
- `AuditTamperDetector`, `AuditRetentionManager`, and `AuditIntegrityManager`
  are declared in the same header. Their names alone do not prove durable
  storage, key custody/rotation, or an external retention schedule.
- `AuditBatchWriter` is a separate interface with batching, checkpoint,
  idempotency, and backpressure-related methods/configuration. See
  [`audit_batch_writer.h`](../../include/governance/audit_batch_writer.h);
  do not infer a production persistence backend without verifying its configured
  manager and deployment integration.

## Evidence and limits

Focused source coverage exists in
[`tests/governance/test_audit_integrity.cpp`](../../tests/governance/test_audit_integrity.cpp);
`tests/governance/CMakeLists.txt` adds `governance_audit_integrity.cpp` to that
focused test target. This statement confirms source/CMake evidence only; no test
was run for this documentation change.

The prior document's schema, HMAC key-store contract, command-line examples,
retention guarantees, key-rotation procedure, and measured latency claims are
not verified by the current public header and implementation and are withdrawn.
Confirm the actual storage, key-management, and retention integration before
making operational or compliance claims.

## References

- [Integrity API](../../include/governance/governance_audit_integrity.h)
- [Batch writer API](../../include/governance/audit_batch_writer.h)
- [Focused audit-integrity test](../../tests/governance/test_audit_integrity.cpp)
- [Governance test CMake](../../tests/governance/CMakeLists.txt)
