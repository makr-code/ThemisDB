<!-- Status: planning snapshot | validated: 2026-10-31 -->
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

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3)

---

## Planned

- [x] API contract index for high-frequency entry points (see `../../src/updates/API.md`)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Header documentation checklist

- [x] Public header surfaces indexed
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations and compatibility status reviewed per header

> TODO: Reconcile the remaining historical target dates and checkbox states
> with a maintainer-approved header API plan. This checklist is not runtime or
> release-readiness evidence.
