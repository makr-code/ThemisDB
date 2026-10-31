> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/scraper/FUTURE_ENHANCEMENTS.md -->

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

# Scraper Module — Public Header Future Enhancements

**Module Path:** `include/scraper/`  
**Canonical implementation enhancements:** [`../../src/scraper/FUTURE_ENHANCEMENTS.md`](../../src/scraper/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/scraper/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/scraper/FUTURE_ENHANCEMENTS.md`](../../src/scraper/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/scraper/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: web scraping, LLM-assisted content evaluation, JavaScript rendering, metadata writing, and government source cataloguing.

Planned extensions follow the same namespace (`themis::scraper`) and include-guard conventions.

---

## Planned Enhancements

### TODO (priority/date not established)

- [ ] Review deprecated entry points and add migration notes if applicable.
- [ ] Review `noexcept` specifications on query/read paths where justified.
- [ ] Connect diagnostic types to runtime errors where required. `scraper_diagnostics.h` currently defines standalone primitives; `ScraperPlugin` does not emit events.

### Additional TODOs (priority/date not established)

- [ ] Consider convenience factories only where a verified caller use case requires them.
- [ ] Revisit language-standard constraints before any C++20-only public API; the module target currently requests C++17.
- [ ] Define ABI markers only if the project adopts an ABI stability policy.

### Longer-Term TODOs (uncommitted)

- [ ] Assess a C++20 module wrapper if repository language support permits it.
- [ ] Assess a header-only usage path only if an embedding use case is established.
- [ ] Document a layer mapping only after an authoritative architecture source establishes one.

---

## Alignment with Strategic Plan

No canonical scraper-to-architecture layer assignment was verified in the reviewed scraper headers or module CMake files. Treat any proposed layer mapping as an assumption until an authoritative architecture source establishes it.
