# Tensor Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../src/tensor/README.md · ../../../include/tensor/README.md -->

## Overview

The `tensor` module provides the core tensor representation, compression, batching, and memory layout capabilities used across analytics, model, and training workloads in ThemisDB.

## Primary sources

- `src/tensor/` — tensor runtime logic and adapters
- `include/tensor/` — public tensor interfaces and shared contracts
- `src/tensor/ARCHITECTURE.md` — design and execution model
- `src/tensor/ROADMAP.md` — roadmap and milestone status

## Key responsibilities

- tensor layout and interoperability support
- compression and serialization strategy
- adapter and orchestrator integration for downstream workloads
- structural search and compression-aware operations

## Related documentation

- [src/tensor/README.md](../../../src/tensor/README.md)
- [src/tensor/ARCHITECTURE.md](../../../src/tensor/ARCHITECTURE.md)
- [include/tensor/README.md](../../../include/tensor/README.md)

## Validation

This landing page aligns the German-facing docs with the source implementation and public tensor API contract.
