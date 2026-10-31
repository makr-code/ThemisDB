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
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/failover/FUTURE_ENHANCEMENTS.md -->

# Failover Module — Public Header Future Enhancements

**Module Path:** `include/failover/`  
**Canonical implementation enhancements:** [`../../src/failover/FUTURE_ENHANCEMENTS.md`](../../src/failover/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/failover/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/failover/FUTURE_ENHANCEMENTS.md`](../../src/failover/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes must follow repository policy in `VERSIONING.md`.
- `[x]` New entry points should be additive; preserve source compatibility for APIs designated stable by repository policy.

---

## Required Interfaces (Header Contract)

Current headers cover automatic failover and disaster-recovery coordination,
shared failover contracts, a local quorum log, and topology snapshots.

Planned extensions follow the same namespace (`themis::failover`) and include-guard conventions.

---

## Planned Enhancements

### Candidate work (requires owner confirmation)

- Confirm supported ABI/language/platform promises before adding compatibility annotations or matrices.
- Consider targeted API documentation improvements when public behavior changes.
- Add convenience or module-interface APIs only following an approved design; these are proposals, not committed roadmap work.

Runtime limitations and implementation work are tracked in [`../../src/failover/FUTURE_ENHANCEMENTS.md`](../../src/failover/FUTURE_ENHANCEMENTS.md) and [`../../src/failover/ROADMAP.md`](../../src/failover/ROADMAP.md).
