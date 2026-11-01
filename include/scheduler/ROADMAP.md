---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: api-contract
Canonical references: include/scheduler/*.h; include/scheduler/README.md; src/scheduler/ROADMAP.md
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler public-header roadmap

This file tracks documentation and contract-review work for
`include/scheduler/`. Runtime work belongs in the
[implementation roadmap](../../src/scheduler/ROADMAP.md). No release date or
ABI commitment is implied by this list.

## Current inventory

The public-header directory contains ten headers:
`task_scheduler.h`, `scheduler_api_contract.h`,
`distributed_task_coordinator.h`, `event_trigger.h`,
`external_scheduler_adapter.h`, `hybrid_retention_manager.h`,
`task_audit_event.h`, `task_audit_manager.h`, `task_result_store.h`, and
`task_anomaly_detector.h`.

The [header index](README.md) maps their declared types and current cautions.
Namespace placement varies between `themis` and `themis::scheduler`; check each
header rather than assuming one namespace for the whole directory.

## Open documentation work

- [ ] Keep API examples and signatures derived from current headers; avoid
  duplicated full class declarations in summaries.
- [ ] Review the contract/header comments against implementation where the
  implementation differs, especially coordination checks and trigger failure
  behavior.
- [ ] Obtain explicit API-owner guidance before adding ABI-stability or
  compatibility claims.
- [ ] Confirm a supported per-header compile-check workflow before documenting
  it as a CI guarantee.

## Relationship to implementation

`include/scheduler/scheduler_api_contract.h` declares the scheduler error
enumeration, but error handling is operation-specific. The
The scheduler now checks the shared execution gate attached by
`DistributedTaskCoordinator`; the gate is closed until healthy leadership and
is re-checked by immediate, scheduled, and DAG dispatch. See the
[implementation architecture](../../src/scheduler/ARCHITECTURE.md).
