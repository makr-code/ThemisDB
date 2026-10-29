# Evaluation Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../src/evaluation/README.md · ../../../include/evaluation/README.md -->

## Overview

The `evaluation` module provides measurement and benchmarking primitives for model and pipeline quality, baseline comparison, and result validation across ThemisDB workloads.

## Primary sources

- `src/evaluation/` — evaluation runtime and baseline logic
- `include/evaluation/` — public evaluation interfaces
- `src/evaluation/ARCHITECTURE.md` — architecture and evaluation flow
- `src/evaluation/BASELINES.md` — baseline and reference definitions

## Key responsibilities

- compute objective and comparative metrics
- baseline definition and regression checking
- evaluation result aggregation and reporting
- quality gate support for model and system changes

## Related documentation

- [src/evaluation/README.md](../../../src/evaluation/README.md)
- [src/evaluation/ARCHITECTURE.md](../../../src/evaluation/ARCHITECTURE.md)
- [include/evaluation/README.md](../../../include/evaluation/README.md)

## Validation

This page keeps the German module index aligned with the evaluation module’s public interfaces and build-time validation artifacts.
