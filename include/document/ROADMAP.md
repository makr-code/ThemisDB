> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/document/ROADMAP.md -->

# Document Module — Public Header Roadmap

**Module Path:** `include/document/`  
**Canonical implementation roadmap:** [`../../src/document/ROADMAP.md`](../../src/document/ROADMAP.md)

---

## Overview

Tracks public `document` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/document/ROADMAP.md`](../../src/document/ROADMAP.md)

---

## Current Status

The direct-scope public headers currently expose store/manager, lifecycle, schema validation, diff/merge, diagnostics, encrypted-entity helpers, round-trip, and XDOMEA APIs. See `ARCHITECTURE.md` for current names and implementation limits.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/document/` implementation docs established
- [x] Namespace layout documented from current headers (`themis::document`, with entity examples in `themis`)

---

## In Progress

- [~] Confirm public compatibility guarantees and deprecation/migration policy for each API; only `document_manager_deprecated.h` is explicitly a deprecated shim. TODO: record maintainer-approved policy and issue/milestone.

---

## Planned

- [ ] Decide whether a header compatibility matrix and standalone public-header compile checks are required. No approved issue or target date was confirmed in this review.

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Compatibility/deprecation policy confirmed by maintainers
