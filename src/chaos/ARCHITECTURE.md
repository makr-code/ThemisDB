# Architecture - Chaos Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The chaos module centers on two cooperating runtime components: a fault registry that controls active simulated failures, and a scheduler that triggers timed injections under deterministic wake policies.

## Main Execution Planes

1. Fault registry plane
- register, query, and recover process-local faults
- enforce validation for fault descriptors and activation semantics

2. Scheduling plane
- schedule immediate and delayed fault injections
- coordinate worker wake strategy and pending fault execution

3. Callback and observability plane
- publish inject/recover event notifications to registered callbacks
- expose pending/active counts for operational inspection

## Core Contracts

| Contract | Behavior |
|---|---|
| fault injector interfaces | deterministic in-process fault lifecycle control |
| scheduler interfaces | bounded timing-based fault activation orchestration |
| callback interfaces | explicit notification hooks for fault state transitions |

## Failure Semantics

- invalid fault specifications are rejected by validation gates.
- scheduler startup/configuration failures remain explicit and bounded.
- non-persistent process-local state is reset with runtime restart.

## Sourcecode Verification (Module: chaos/architecture)

- Verified files:
  - src/chaos/chaos_framework.cpp
- Verified architecture claims:
  - explicit registry and scheduler separation
  - bounded callback and pending-state control surfaces
  - deterministic in-process simulation-oriented runtime model
---

### Direct Downstream Consumers (modules that use this module)

> **Production consumer route: INTEGRATION-READY (admin/staging only)**
> Handler header `include/server/chaos_admin_api_handler.h` has been created as the
> production admin integration point, gated by `THEMIS_CHAOS_ADMIN`. This endpoint
> must **never** be enabled on production deployments without explicit operator approval.
> Wiring into `HttpServer` and CMake flag definition are the remaining steps.

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/server/chaos_admin_api_handler.h` → `ChaosAdminApiHandler` | Planned admin route: `POST /admin/chaos/inject`, `POST /admin/chaos/reset`, `GET /admin/chaos/status`, `GET /admin/chaos/history`. Gate: `THEMIS_CHAOS_ADMIN` CMake flag. Handler header implemented; `HttpServer` wiring pending. |
| _(tests)_ | `include/chaos/chaos_framework.h` | `tests/test_chaos_framework.cpp`, `tests/test_chaos_stress.cpp` — current only verified consumers. |
