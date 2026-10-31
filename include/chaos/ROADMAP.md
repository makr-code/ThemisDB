---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos Engineering Module — Public Header Roadmap

**Module Path:** `include/chaos/`  
**Canonical implementation roadmap:** [`../../src/chaos/ROADMAP.md`](../../src/chaos/ROADMAP.md)

---

## Overview

Tracks public `chaos` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/chaos/ROADMAP.md`](../../src/chaos/ROADMAP.md)

---

## Current Status

The two inspected headers declare `FaultInjector`, `ChaosScheduler`, fault
descriptors, and the v1.x interoperability contract. This is a source inventory,
not a claim that every recommended contract limit is enforced at runtime.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/chaos/` implementation docs established
- [ ] **TODO/assumption:** confirm whether this module has an assigned architecture layer; the headers do not establish the former LLM/Graph association.
- [x] Namespace layout (`themis::chaos`) documented

---

## In Progress

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3)

---

## Planned

- [ ] API contract index for high-frequency entry points with usage examples (Target: 2026-Q3)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent

Dates in the open/planned items are unverified proposals, not delivery
commitments. Confirm the current planning authority before assigning a milestone.
