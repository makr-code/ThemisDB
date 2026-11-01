---
Author: ThemisDB Contributors
Created: 2026-10-31
Creation_Date_Note: Creation date of this rewritten page; the original file creation date is unknown.
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: api-contract
Canonical_References:
  - include/security/key_provider.h
  - include/security/encryption.h
  - include/security/access_control_manager.h
  - include/security/rbac.h
  - include/security/zero_trust_policy_enforcer.h
  - include/security/security_evidence_collector.h
  - src/security/ARCHITECTURE.md
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security public-header architecture

`include/security/` is a set of public declarations, not one uniform namespace or
a claim that all components form a mandatory runtime pipeline. The implementation
architecture and module boundaries are described in
[`src/security/ARCHITECTURE.md`](../../src/security/ARCHITECTURE.md).

Representative header contracts:

- Key providers and field encryption: `themis::KeyProvider`,
  `themis::VaultKeyProvider`, and `themis::FieldEncryption`.
- Access policy: `themis::security::RBAC` and
  `themis::security::AccessControlManager`.
- Request verification: `themis::security::ZeroTrustPolicyEnforcer`.
- Evidence export: `themis::security::SecurityEvidenceCollector`.

These namespace and type names are taken from the respective headers. Individual
callers own integration and configuration; this page does not assert startup
enforcement, backend availability, or product-wide security guarantees. See
[`README.md`](README.md) and [`src/security/API.md`](../../src/security/API.md)
for the current header index.
