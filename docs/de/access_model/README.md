# Access Model Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../src/access_model/README.md · ../../../include/access_model/README.md -->

## Overview

The `access_model` module defines the unified placement and promotion model for data across cache and storage tiers. It provides the abstraction and orchestration needed to move hot entries into faster tiers while preserving deterministic eviction and observability behavior.

## Primary sources

- `src/access_model/` — implementation and orchestration logic
- `include/access_model/` — public C++ interfaces and policy contracts
- `src/access_model/ARCHITECTURE.md` — design and component flow
- `src/access_model/FUTURE_ENHANCEMENTS.md` — roadmap and planned work

## Key responsibilities

- `AccessTier` abstraction for cache and storage tiers
- promotion / demotion policy orchestration
- age-based and event-driven placement decisions
- metrics and tracing for access-tier transitions

## Related documentation

- [src/access_model/README.md](../../../src/access_model/README.md)
- [src/access_model/ARCHITECTURE.md](../../../src/access_model/ARCHITECTURE.md)
- [include/access_model/README.md](../../../include/access_model/README.md)

## Validation

This document aligns the German user-facing overview with the module’s implementation and public headers so the `src/include ↔ docs/de` scan remains consistent.
