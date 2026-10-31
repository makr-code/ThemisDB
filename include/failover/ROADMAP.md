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
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/failover/ROADMAP.md -->

# Failover Module — Public Header Roadmap

**Module Path:** `include/failover/`  
**Canonical implementation roadmap:** [`../../src/failover/ROADMAP.md`](../../src/failover/ROADMAP.md)

---

## Overview

Tracks public `failover` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/failover/ROADMAP.md`](../../src/failover/ROADMAP.md)

---

## Current Status

The five headers in `include/failover/` declare automatic failover, disaster-recovery coordination, shared contracts, a local quorum log, and topology snapshots. Runtime status is tracked in [`../../src/failover/ROADMAP.md`](../../src/failover/ROADMAP.md) and root [`../../ROADMAP.md`](../../ROADMAP.md).

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/failover/` implementation docs established
- [x] Header inventory aligned to the current five-file public surface
- [x] Namespace layout (`themis::failover`) documented

---

## Open API Documentation Work

- [ ] Confirm and document any intended API stability/deprecation policy with maintainers; no module-specific deprecation policy is inferred here.

---

## Planned

- [ ] Maintain the API entry-point summary against the public headers.
- [ ] Add a compatibility matrix only after supported language versions and platforms are established by canonical build configuration.
- [ ] Confirm whether standalone compile checks for every public header are registered in CI.

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation/stability policy confirmed by maintainers
