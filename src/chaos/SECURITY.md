# Security - Chaos Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the chaos module focuses on controlled failure simulation boundaries, validated fault descriptors, bounded scheduler behavior, and predictable callback execution surfaces.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| invalid or malformed fault specifications | explicit validation on inject/schedule paths |
| unbounded in-process simulation side effects | process-local scope and bounded scheduler controls |
| callback misuse causing control-path instability | explicit callback registration and predictable invocation paths |
| accidental persistence assumptions for chaos state | documented process-local non-persistent behavior |

## Implemented Security Controls

- fault injection validates core descriptor inputs before activation.
- scheduler behavior remains bounded by configured control parameters.
- process-local scope limits blast radius of simulated failures.
- callback signaling is explicit and inspectable through module APIs.

## Security Follow-ups

- continue hardening concurrency and callback edge behavior under stress.
- maintain deterministic failure semantics for scheduler stop/restart transitions.
- keep diagnostics actionable for chaos-test operational incidents.

## Sourcecode Verification (Module: chaos/security)

- Verified files:
  - src/chaos/chaos_framework.cpp
- Verified controls:
  - descriptor validation and bounded scheduler behavior
  - process-local simulation boundaries
  - explicit callback/event state transition surfaces