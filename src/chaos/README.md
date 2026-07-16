# ThemisDB Chaos Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The chaos module provides in-process fault injection and deterministic chaos scheduling surfaces for resilience testing and controlled failure simulation in ThemisDB environments.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| chaos_framework.cpp | fault injection registry and scheduler runtime implementation |

## Scope

In scope:
- in-process fault lifecycle registration and recovery controls
- deterministic and configurable scheduler-driven fault activation
- callback and event signaling for injected/recovered fault states

Out of scope:
- external cluster-wide orchestration beyond process-local chaos surfaces
- direct OS/network sabotage beyond modeled simulation semantics
- non-chaos business workflows outside resilience simulation boundaries

## Runtime Behavior and Limits

- active faults are process-local and keyed by configured fault identity dimensions.
- scheduler execution depends on configured wake strategy and tick behavior.
- invalid fault specifications fail with structured validation behavior.

## Sourcecode Verification (Module: chaos/readme)

- Verified files:
  - src/chaos/chaos_framework.cpp
- Verified behavior surfaces:
  - fault registration/recovery and active-state inspection
  - scheduler-managed deferred fault injection paths
  - callback signaling and bounded in-process simulation behavior
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md