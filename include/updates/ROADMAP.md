---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

<!-- Status: planning snapshot | source checked: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/updates/ROADMAP.md -->

# Updates Module — Public Header Roadmap

**Module Path:** `include/updates/`  
**Canonical implementation roadmap:** [`../../src/updates/ROADMAP.md`](../../src/updates/ROADMAP.md)

---

## Overview

Tracks public `updates` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/updates/ROADMAP.md`](../../src/updates/ROADMAP.md)

---

## Current Status

The header directory contains public interfaces for hot reload, blue/green and
canary control, cluster/coordinated update management, schema migration,
manifest handling, telemetry, and preflight checks. See
[`README.md`](README.md) for the header index. This file tracks header
documentation work only; runtime delivery status is in the
[`implementation roadmap`](../../src/updates/ROADMAP.md).

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/updates/` implementation docs established
- [x] Namespace layout (`themis::updates`) documented

---

## In Progress

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: maintainer-approved schedule; prior target date elapsed)

---

## Planned

- [x] API contract index for high-frequency entry points (see `../../src/updates/API.md`)
- [ ] Compatibility matrix (C++ standard and platform coverage) per header group (Target: maintainer-approved schedule)
- [ ] CI-enforced compile checks for all public headers (Target: maintainer-approved schedule)

---

## Header documentation checklist

- [x] Public header surfaces indexed
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations and compatibility status reviewed per header

## Implementation Phases

These phases cover public-header contract documentation only. Runtime
implementation delivery remains tracked by the
[`implementation roadmap`](../../src/updates/ROADMAP.md).

### Phase 1 — Design / API contract
- [ ] Review each public header's ownership, thread-safety, and compatibility contract (Target: next API review).

### Phase 2 — Core implementation
- [ ] Confirm header declarations and implementation entry points remain aligned (Target: next module source review).

### Phase 3 — Error handling and edge cases
- [ ] Cross-check documented failure and invalid-input behavior against declarations and focused tests (Target: next module source review).

### Phase 4 — Tests
- [ ] Add/maintain compile coverage for public headers and record the configured target (Target: maintainer-approved schedule).

### Phase 5 — Performance / hardening
- [ ] Review API compatibility across supported C++ standards and platforms (Target: maintainer-approved schedule).

### Phase 6 — Documentation and acceptance
- [ ] Reconcile per-header deprecation and breaking-change notices with `VERSIONING.md` (Target: next API review).

## Production Readiness Checklist

- [ ] Public-header compatibility matrix reviewed by maintainers.
- [ ] Header compile coverage and supported configurations recorded.
- [ ] API changes cross-linked to implementation docs and focused tests.

## Known Issues & Limitations

- This roadmap does not establish runtime readiness; use the canonical
  implementation roadmap.
- A complete per-header compatibility matrix and compile-target inventory are
  not verified by this document.

## Breaking Changes

No public API change is proposed here. Any future breaking change must follow
the repository versioning policy and be documented in the affected headers.

> TODO: Reconcile the remaining historical target dates and checkbox states
> with a maintainer-approved header API plan. This checklist is not runtime or
> release-readiness evidence.
