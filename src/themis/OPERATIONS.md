---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Operations — Themis Core

## Scope

This document describes operational evidence and safe change controls for the
runtime foundation in `src/themis`. It does not declare deployment-specific
limits or replace service-level runbooks. The detailed signal names, alert
mapping, and command procedures are in
[`RUNBOOK_THEMIS_CORE.md`](../../docs/operability/RUNBOOK_THEMIS_CORE.md).

## Signals and diagnosis

The core exposes build configuration and reproducibility data through
[`build_info.h`](../../include/themis/build_info.h), while license and feature
state are available through the interfaces in
[`license_info.h`](../../include/themis/license_info.h) and
[`runtime_license_gate.h`](../../include/themis/runtime_license_gate.h).
Use these signals to establish what binary is running and why a feature is
allowed or denied. They are diagnostic inputs, not proof that a module is
trusted or that its dependencies are healthy.

The operator runbook maps these incident classes to logs and metrics:

- module load or verification failure;
- wire session stall or disconnect;
- trust violation;
- dependency gating timeout; and
- edition/license gate failure.

Confirm that the deployed version exports the corresponding signal. Do not
interpret a missing metric as a healthy result; compare it with server logs and
the version's configured observability.

## Change and release operations

Before deploying a changed core artifact:

1. retain the build revision and reproducibility manifest;
2. verify that the build configuration and edition match the intended
   deployment;
3. verify the module artifact and its signature using the release trust
   configuration;
4. validate dependencies and platform compatibility; and
5. run the current module contract tests and mapped release benchmarks.

For a rollback, restore a previously verified release and a compatible module
set together. Do not roll back only the verifier or license gate while
retaining artifacts that depend on the newer trust contract.

## Operational limits

The compile-time edition ceilings are defaults/contracts, not a substitute for
deployment-specific capacity planning. License-client timeouts and grace
periods are configurable and must be checked against the deployment
configuration. The exact wire limits and loader policies are version-specific;
consult the active interfaces and operator configuration. The module's
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) defines benchmark
comparison rules but does not publish per-deployment SLO values.

For incident triage and rollback decisions, use [`RUNBOOK.md`](./RUNBOOK.md).
