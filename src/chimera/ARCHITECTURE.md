# Architecture - Chimera Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The chimera module currently centers on a single adapter implementation that exposes ThemisDB access through unified adapter contracts. It supports simulation-first behavior with optional engine-backed dispatch where available.

## Main Execution Planes

1. Adapter lifecycle plane
- connect/disconnect and capability reporting behavior
- operation preconditions and connection-state gates

2. Dispatch plane
- simulation-mode execution for adapter contract coverage
- conditional engine-backed dispatch for available integration paths

3. Error and observability plane
- structured result/error behavior for unsupported paths
- adapter-level consistency of capability and operation outcomes

## Core Contracts

| Contract | Behavior |
|---|---|
| adapter interfaces | provide unified multi-model adapter operation surfaces |
| lifecycle interfaces | enforce connection-state and operation preconditions |
| error contracts | provide structured failures for unsupported/unavailable paths |

## Failure Semantics

- operations without valid connection state fail with structured connection errors.
- unavailable engine-backed paths fail explicitly with not-implemented style errors.
- adapter state remains process-local and non-persistent.

## Sourcecode Verification (Module: chimera/architecture)

- Verified files:
  - src/chimera/themisdb_adapter.cpp
- Verified architecture claims:
  - single-adapter module composition in current source layout
  - explicit lifecycle and dispatch plane separation
  - bounded structured error behavior for unsupported paths