# Architecture - Governance Module

<!-- Status: current | validated: 2026-09-09 -->
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

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` | Logging, audit trail, and observability helpers |
| observability (utils) | `include/utils/tracing.h` | Telemetry emission for governance decisions and compliance events |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/compliance_reporter.h`, `policy_manager.h` | Server exposes compliance reporting and policy management endpoints |
| llm | `include/governance/` | LLM module consults governance for ethics-enforcement and model-governance policies |

## Integration Points

### Critical Integration: Server Policy Management
**Files:** `src/governance/policy_manager.cpp`, `policy_engine.cpp` ↔ `server/policy_manager.h`
**Contract:** Server delegates all policy load/eval/version operations to the governance module; governance is the sole authority for access and compliance decisions.
**Thread Safety:** Policy reads are concurrent-safe; policy mutations acquire an internal write lock.

### Critical Integration: LLM Ethics Enforcement
**Files:** `src/governance/model_governance.cpp` ↔ `llm/`
**Contract:** LLM module calls governance model-governance check before serving output; a deny decision fails the LLM operation closed.
**Thread Safety:** Governance enforcement calls are stateless read operations; concurrent calls are safe.