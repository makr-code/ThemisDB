# Architecture - Governance Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The governance module composes policy evaluation/lifecycle, compliance controls, data-governance protections, and governance operations into a bounded runtime contract for ThemisDB.

## Main Execution Planes

1. Policy evaluation and lifecycle plane
- runtime policy decision and permission checks
- policy load/validate/version/review/watch coordination

2. Compliance control plane
- GDPR/CCPA/HIPAA/ISO/PCI/SOC control and rule evaluators
- compliance reporting and evidence aggregation surfaces

3. Data-governance protection plane
- masking, lineage, model-governance, and tenant-inheritance behavior
- OPA integration and fallback-aware enforcement paths

4. Governance operations plane
- scheduling/coordinator workflows and audit-oriented runtime telemetry
- policy conflict and rollback-capable change management behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| policy contract | deterministic policy decision and lifecycle semantics |
| compliance contract | explicit rule/control evaluation and reporting behavior |
| data-governance contract | bounded masking/lineage/model-governance protections |
| operations contract | explicit versioning/review/watch/coordination behavior |

## Failure Semantics

- invalid or inconsistent policies fail with explicit validation outcomes.
- unsupported/degraded integration paths (e.g. OPA) degrade via explicit fallback behavior.
- denied governance outcomes fail closed before protected operations proceed.

## Sourcecode Verification (Module: governance/architecture)

- Verified files:
  - src/governance/policy_engine.cpp
  - src/governance/policy_manager.cpp
  - src/governance/policy_manager_versioned.cpp
  - src/governance/policy_validation.cpp
  - src/governance/compliance_reporting.cpp
  - src/governance/data_masker.cpp
  - src/governance/data_lineage.cpp
  - src/governance/model_governance.cpp
  - src/governance/opa_adapter.cpp
- Verified architecture claims:
  - explicit policy/compliance/data-governance/operations planes
  - bounded deterministic failure behavior for invalid/degraded paths
  - module-local ownership of governance orchestration surfaces