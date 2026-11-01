---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/whisper/ROADMAP.md -->

# Whisper ASR Module — Public Header Roadmap

**Module Path:** `include/whisper/`  
**Canonical implementation roadmap:** [`../../src/whisper/ROADMAP.md`](../../src/whisper/ROADMAP.md)

---

## Overview

Tracks public `whisper` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/whisper/ROADMAP.md`](../../src/whisper/ROADMAP.md)

---

## Current Status

The headers declare the ASR plugin, audio readers, VAD, transcription, configuration, and plugin
registration API. Declared streaming and diarisation methods do not imply incremental or
multi-speaker whisper.cpp behavior; see the public [README](README.md).

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/whisper/` implementation docs established
- [x] Layer association (**LLM/Voice**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::whisper`) documented

---

## In Progress

- [ ] Decide whether any public entry point is deprecated and add a migration notice only if supported by an approved API change (Target: TODO; previous Q3 2026 target passed)

---

## Planned

- [ ] API contract index for high-frequency entry points with usage examples (Target: TODO; previous Q3 2026 target passed)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
