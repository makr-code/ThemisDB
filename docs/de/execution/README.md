# Execution Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../src/execution/README.md · ../../../include/execution/README.md -->

## Overview

The `execution` module coordinates runtime execution strategy, scheduling, and worker orchestration for query and data-processing tasks across ThemisDB components.

## Primary sources

- `src/execution/` — execution scheduler and runtime coordination
- `include/execution/` — public execution interfaces and policy contracts
- `src/execution/ARCHITECTURE.md` — execution flow and worker model
- `src/execution/ROADMAP.md` — implementation and production milestones

## Key responsibilities

- query and pipeline scheduling
- thread pool orchestration and execution planning
- runtime policy enforcement and resource coordination
- execution observability for failure recovery and tuning

## Related documentation

- [src/execution/README.md](../../../src/execution/README.md)
- [src/execution/ARCHITECTURE.md](../../../src/execution/ARCHITECTURE.md)
- [include/execution/README.md](../../../include/execution/README.md)

## Validation

This landing page ensures the German-facing docs reflect the implementation and scheduling interfaces defined in the execution module.
