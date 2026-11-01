> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/scraper/ROADMAP.md -->

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-10-31<br>
**Status:** review

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

# Scraper Module — Public Header Roadmap

**Module Path:** `include/scraper/`  
**Canonical implementation roadmap:** [`../../src/scraper/ROADMAP.md`](../../src/scraper/ROADMAP.md)

---

## Overview

Tracks public `scraper` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/scraper/ROADMAP.md`](../../src/scraper/ROADMAP.md)

---

## Current Status

The public headers declare plugin/configuration, HTML/API retrieval, renderer, evaluator, metadata-writer, catalog, burst-control, and helper types. Header presence does not establish that every helper is integrated in the runtime; see [`../../src/scraper/ARCHITECTURE.md`](../../src/scraper/ARCHITECTURE.md).

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/scraper/` implementation docs established
- [x] Namespace layout (`themis::scraper`) documented

---

## In Progress

- [ ] Review per-header deprecation and breaking-change notices against `VERSIONING.md` (target date not established here)

---

## Planned

- [ ] Maintain examples and contract notes for commonly used entry points (target date not established here)
- [ ] Document verified compiler/platform coverage per header group (target date not established here)
- [ ] Verify CI compile coverage for all public headers (target date not established here)

Focused source/test registration is defined by `src/scraper/CMakeLists.txt` and `tests/scraper/CMakeLists.txt`; the latter creates one focused target and CTest entry per `test_scraper_*.cpp`. This roadmap does not claim that those targets have been built or run.

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
