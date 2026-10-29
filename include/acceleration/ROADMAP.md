---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-10-29"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

> Build: `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/acceleration/ROADMAP.md -->

# Acceleration Module — Public Header Roadmap

**Module path:** `include/acceleration/`
**Canonical implementation roadmap:** [`../../src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md)

## Overview

This file tracks the public header contract for the acceleration module. Runtime implementation status, benchmark gates, and hardware-dependent validation remain in the canonical implementation roadmap because those are not guaranteed to hold on every development host.

## Current status

The public API currently includes the core compute-backend contract, capability selector, AI hardware dispatcher, fallback dispatchers, plugin validation interfaces, and multi-device resource management surfaces. The major runtime design points are aligned with the implementation code under `src/acceleration/`.

## Current status checklist

- [x] Public header contract is present under `include/acceleration/`
- [x] Device probing and backend selection surfaces are documented in the public API docs
- [x] AI hardware dispatcher and fallback semantics are represented in the public docs
- [x] Public docs link to the implementation roadmap and architecture guide
- [ ] Any future deprecation notices must be added directly to the impacted public headers and matched by implementation-roadmap updates

## In progress

- [ ] Add deprecation/migration notes for any legacy public APIs when they are identified in code review
- [ ] Confirm and document compatibility expectations for optional hardware backends in future release notes

## Planned

- [ ] Expand the public API contract index with canonical examples for `BackendRegistry`, `DeviceManager`, and `AiHardwareDispatcher`
- [ ] Capture compatibility matrix (platform / compiler / optional backend gate coverage) for public acceleration headers
- [ ] Add compile-time validation checks for public header coverage in the CI pipeline

## Production readiness checklist

- [x] Public header API is present and discoverable
- [x] Module docs link to source-code reality and implementation roadmap
- [x] CPU fallback and deterministic device selection are documented in the public narrative
- [ ] Deprecated public APIs are explicitly marked and tracked when identified

## TODO / open assumptions

- Hardware-specific throughput claims remain implementation- and benchmark-level facts rather than blanket public-header guarantees.
- Any future ABI-breaking public API change will need to be tracked in `VERSIONING.md` and the runtime roadmap before it is documented here as complete.
