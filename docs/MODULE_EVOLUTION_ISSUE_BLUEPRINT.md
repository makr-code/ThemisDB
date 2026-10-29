---
Author: ThemisDB Contributors
Created: 2026-10-29
Last Updated: 2026-10-29
Status: active
AI_Used: true
AI_Models: GitHub Copilot
AI_Contributor: GitHub Copilot
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Module Documentation and Evolution Issue Blueprint

## Purpose

This document standardizes the two issue classes used across ThemisDB module work:

1. a documentation / current-state issue for each module, and
2. a research and evolution issue for the transition from v2.5.0 to v3.0.0.

The goal is to make Copilot and human maintainers operate from the same evidence-based structure, instead of vague prompts such as "document the module" or "plan the future evolution".

## Required issue design rules

Every issue must make the execution target explicit enough that a human or agent can complete it without interpretation.

### Mandatory structure

Each issue should contain the following sections:

- Scope
  - Module name and affected paths
  - Current status and evidence baseline
  - Explicit file/folder ownership boundaries
- Source evidence
  - canonical docs, code paths, tests, CI, and benchmarks that define the truth
- Objective
  - one concrete goal and why it matters
- Work package
  - ordered tasks with explicit deliverables
- Validation
  - commands, test targets, or documentation checks
- Acceptance criteria
  - objective, checkable outcomes
- Out of scope
  - anything intentionally excluded from the issue

### Rules for issue wording

The issue text must avoid vague verbs such as "improve", "optimize", or "refresh" unless they are tied to a measurable target.

Prefer language like:

- "reconcile the module README and source implementation against the actual API and failure behavior"
- "document the public contract, lifecycle assumptions, and edge cases for the module"
- "map the current implementation to the v2.5.0 to v3.0.0 evolution goals and identify remaining gaps"

Avoid language like:

- "make the module better"
- "do a general documentation pass"
- "plan the future of the system"

### Evidence policy: benchmark and release claims

Every issue that references performance, reliability, or release readiness must distinguish between a claim and verified evidence.

- A module issue is not release-ready simply because a benchmark name exists.
- Benchmark output must be tied to an exact run artifact, exit status, and the module's actual contract boundary.
- Any claim that a module is ready for the next release phase requires pass/fail evidence for the corresponding gate, not generic optimism.

Current benchmark reality: the full run captured under `benchmarks/benchmark_results/full_run_log.txt` shows that most benchmark jobs exited with code -1, while only a few isolated runs completed successfully. Therefore, the portfolio must treat benchmark evidence as a required validation gate, not as a background assumption.

## Documentation-state issue class

This issue class is intended for the current condition of each module in `src/` and related `include/` paths. It must ask Copilot to create or update the developer documentation as a factual, source-backed representation of the real state.

### Required content

Each documentation-state issue must state:

- module name and source paths
- whether the work is primary-doc, secondary-doc, or both
- the canonical source-of-truth files
- the concrete documentation files to create or update
- the validation path, including tests or checks where relevant

### Minimum task sequence

1. Compare the current source implementation against the module documentation.
2. Validate the roadmap and enhancement statements against real code behavior.
3. Update the module's developer-facing files to reflect actual architecture, constraints, and edge cases.
4. Add a short missing-implementations or gap summary when the module is incomplete.
5. Document validation and unresolved risks.

### Acceptance standard

The module documentation issue is complete only when:

- the README, ARCHITECTURE, ROADMAP, FUTURE_ENHANCEMENTS, and CHANGELOG state reflect the real module behavior,
- public contracts and failure modes are explicit,
- module documentation is aligned with the repository SOT order defined in `DOCUMENTATION_GOVERNANCE.md`, and
- the work is bounded to the target module and its direct evidence.

## Evolution issue class: v2.5.0 to v3.0.0

This issue class is for the research-based transition from v2.5.0 toward v3.0.0 and must be framed as a technical design and module rollout plan instead of a generic roadmap request.

### Required content

Each evolution issue must define:

- the target module and the relevant subsystem boundaries,
- the expected v2.5.0 baseline and the target v3.0.0 position,
- the technical dimensions to be advanced (performance, correctness, security, observability, interoperability, or architecture),
- the evidence required to support the claim,
- the validation strategy and the module-by-module rollout plan.

### Required planning structure

1. Baseline assessment of the current module state.
2. Scientific or engineering rationale for the v3.0.0 direction.
3. architecture and API impacts across module boundaries.
4. performance, reliability, and security expectations.
5. module rollout order and dependencies.
6. explicit gaps and known risks.

### Recommended project-wide concept

The v2.5.0 → v3.0.0 roadmap should be organized in phases:

- Phase 1: fact capture and module status normalization
- Phase 2: architecture contract and public API hardening
- Phase 3: research-backed performance and reliability improvements
- Phase 4: security and resilience validation
- Phase 5: release-grade evidence and module handoff

The concept is to drive module evolution from evidence and capability maturity, not from marketing language or generic optimism.

## Module rollout pattern

For the 73 modules under `src/`, the rollout should follow a consistent pattern:

1. Inventory the module and prove its actual status from source, tests, and docs.
2. Classify the module by maturity: production candidate, hardening, experimental, or placeholder.
3. Write the issue against the module's real source and public interface boundaries.
4. Require explicit validation and a measurable completion condition.
5. Link the module-level issue to the root roadmap and to the relevant v2.5.0 → v3.0.0 evolution objective.

### Recommended module issue summary

Each module issue should answer these five questions:

- What is the current state of this module and what evidence proves it?
- What is the required technical target for v3.0.0?
- What interfaces, tests, or docs must change?
- What risks or missing capabilities remain?
- What measurable validation defines completion?

## Operational guidance to Copilot

Copilot should be instructed to produce work in this order:

1. read the module's canonical source and current docs,
2. identify exact file and capability boundaries,
3. update issue text to include concrete scope, evidence, tasks, and validation,
4. produce a module-ready work package with measurable acceptance criteria,
5. link the module package to the overall v2.5.0 → v3.0.0 plan.

This avoids broad, non-actionable prompts and turns each issue into a bounded engineering task.

## Repository references

- `DOCUMENTATION_GOVERNANCE.md`
- `.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md`
- `.github/ISSUE_TEMPLATE/module_task.md`
- `.github/ISSUE_TEMPLATE/module_evolution_plan.md`
- `ROADMAP.md`
- `FUTURE_ENHANCEMENTS.md`
- `docs/MODULE_ISSUE_PORTFOLIO_BATCH_1.md`
- `docs/MODULE_ISSUE_PORTFOLIO_BATCH_2.md`
- `docs/MODULE_ISSUE_MASTER_CATALOG.md`

The issue portfolio files provide concrete rollout examples and a consolidated master index for the evidence-first issue standard across the module graph.
