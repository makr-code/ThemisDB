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