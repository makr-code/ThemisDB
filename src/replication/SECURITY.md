---
Author: ThemisDB Contributors
Created: 2026-05-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: security
Canonical_References:
  - ../../include/replication/replication_manager.h
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/replication_slot.h
  - ../../include/replication/replication_api_contract.h
  - ../../src/replication/replication_manager.cpp
  - ../../SECURITY.md
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication security boundaries

Report vulnerabilities through the project-level [SECURITY.md](../../SECURITY.md).
This page records security-relevant interfaces; it is not a security assessment or
attestation.

## Relevant surfaces

- `ReplicationConfig` declares `cert_path`, `key_path`, `ca_path`, and
  `require_mtls`. Their presence in a struct does not prove that every replication
  transport loads or enforces them.
- `AsyncWalShipper` delegates actual transport to an injected callback. The header's
  default handler is a no-op; transport authentication, encryption, peer identity,
  retries, and error policy are the handler/integration owner's responsibility.
- Manager APIs expose health, quorum, failover/promotion, and placement operations.
  Their exact validation and caller authorization requirements must be reviewed at
  their integration boundary.
- `replication_api_contract.h` documents an error taxonomy and stream/CRDT contract.
  It does not alone demonstrate that all runtime paths enforce every listed contract.

## Threats to assess

| Area | Source-backed observation | Not established by these sources |
|---|---|---|
| Configuration and credentials | Replication config fields exist for TLS paths and an mTLS flag. | Credential parsing, TLS enforcement, rotation, or safe secret handling in every transport. |
| State transitions | Manager exposes explicit role, promotion, failover, and quorum APIs. | Authentication/authorization of all callers or all deployment controls. |
| Queue and transport | Queue capacity and enqueue result are exposed; transport is injected. | End-to-end delivery guarantees, retry policy, authenticated peer validation. |
| Slots and streams | Slot progress and logical changes have public APIs. | Complete persistence, replay, and data-loss guarantees across all failure paths. |
| Contract claims | A contract header and focused tests exist. | Full conformance of every implementation and integration. |

## Follow-up

**TODO:** Security owners should verify transport implementations, authorization at
all call sites, secret handling, replay/integrity behavior, and fail-closed behavior
against deployed configurations. No security tests or runtime validation were run for
this documentation task.
