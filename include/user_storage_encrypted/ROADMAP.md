> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/user_storage_encrypted/ROADMAP.md -->

# User Storage Encrypted Module — Public Header Roadmap

**Module Path:** `include/user_storage_encrypted/`  
**Canonical implementation roadmap:** [`../../src/user_storage_encrypted/ROADMAP.md`](../../src/user_storage_encrypted/ROADMAP.md)

---

## Overview

Tracks public `user_storage_encrypted` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/user_storage_encrypted/ROADMAP.md`](../../src/user_storage_encrypted/ROADMAP.md)

---

## Current Status

All headers in `include/user_storage_encrypted/` are present and expose the per-user encrypted storage with pluggable backends (gocryptfs), key derivation, key rotation scheduling, multi-level storage, and security levels surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/user_storage_encrypted/` implementation docs established
- [x] Layer association (**Graph**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::user_storage_encrypted`) documented

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
