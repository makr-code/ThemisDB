---
Author: "ThemisDB Team"
Created: "2026-10-09"
Last Updated: "2026-10-09"
Status: "active"
---

# Module Developer Documentation and Source Structure Guidelines

Purpose: This file is the repository-level contract for how module documentation and source code must be structured so that Copilot, humans, and release gates can understand the module's intent, evidence, and implementation state.

## 1. Required module shape

Every active module must be understandable in the same shape, regardless of subsystem.

### Required module docs

For each module under `src/<module>/` and `include/<module>/`, maintain the following artifacts when relevant:

- `README.md`
- `FEATURES.md` (marketing-oriented capability profile; WordPress content basis)
- `ARCHITECTURE.md`
- `ROADMAP.md`
- `CHANGELOG.md`
- `FUTURE_ENHANCEMENTS.md`
- `AUDIT.md` for production or audit-sensitive modules
- `SECURITY.md` for production/security-relevant modules
- `PERFORMANCE_EXPECTATIONS.md` for performance-sensitive modules
- `PRODUCTION_REQUIREMENTS.md` for operationally critical modules
- `MODULE_GAPS.md` for active gap tracking

Minimum rules:

- The module file set must remain stable and human-readable.
- If a required doc is missing, the task must explicitly create or update it as part of the change.
- Do not create duplicate names with different casing within the same scope.
- Use repository-local naming conventions and keep all section headings consistent.

### Mandatory sections in primary docs

Each primary module file must remain structured and action-oriented:

- `README.md`: purpose, scope, quickstart, build/run, known limitations, related docs
- `FEATURES.md`: marketing summary, capability highlights, business value, target scenarios, and references to developer docs
- `ARCHITECTURE.md`: context, components, data flow, interfaces, failure paths, non-goals
- `ROADMAP.md`: current status, in progress/planned items, implementation phases 1-6, readiness checklist, known issues, breaking changes
- `FUTURE_ENHANCEMENTS.md`: scope, design constraints, required interfaces, implementation notes, test strategy, performance targets, security/reliability
- `CHANGELOG.md`: added/changed/fixed/removed items with versioning and references

## 2. Source code expectations

### Public API vs internal implementation

Module ownership must be clear by path:

- Public API: `include/<module>/...`
- Internal implementation: `src/<module>/...`
- Tests: `tests/<module>/...`
- Benchmarks: `benchmarks/<module>/...`
- Examples: `examples/<module>/...` when present
- Third-party or vendored code: paths under `vcpkg`, `third_party`, `vendor`, `external`, `submodules`, or similar external roots

Rules:

- Public API additions require matching documentation, contract clarity, and release-note impact.
- Internal implementation may be more detailed, but it must still be explainable in module docs.
- External vendor or dependency code must not be treated as repo-owned implementation unless explicitly declared.

### Source level expectations

For C++ and related code:

- Public classes, functions, and methods must have Doxygen-style documentation.
- Document purpose, preconditions, postconditions, error modes, and edge cases.
- Make ownership, lifetime, and thread-safety behavior explicit.
- Prefer stable, small interfaces over hidden global state.
- Avoid undocumented stubs, simulation-only paths, or silent fallback logic unless explicitly marked and approved.

## 3. Doxygen contract

All meaningful public API declarations must be represented in Doxygen output and in the module evidence stream.

### Required Doxygen coverage

For each public symbol:

- `@brief` or equivalent summary
- `@param` for input arguments, when applicable
- `@return` for return values, when applicable
- `@throws` or equivalent error semantics for exceptional behavior
- lifetime and ownership notes where relevant
- edge-case behavior and failure modes

### Source-vs-doc compliance rules

Copilot must treat the following as a compliance signal:

- Source symbol exists but Doxygen output does not include it -> `missing_symbol`
- Doxygen symbol present but no brief description -> `missing_brief`
- Function with parameters but no parameter docs -> `missing_param_docs`
- Function with return value but no return docs -> `missing_return_docs`
- Module changes not reflected in docs or module evidence -> `doc_drift`

### Evidence output

For each module, generate or maintain compact evidence artifacts such as:

- `ai_context/developer_llm_wiki/modules/<module>/module_summary.md`
- `ai_context/developer_llm_wiki/modules/<module>/module_evidence.json`
- `ai_context/developer_llm_wiki/modules/<module>/module_gap_report.md`
- `ai_context/developer_llm_wiki/modules/<module>/doxygen_evidence.json`

The evidence layer must be concise and decision-oriented. It is not a replacement for the module docs; it is the structured proof layer behind them.

## 4. Required issue/action packet structure

When a module gap or implementation task is reported, the issue body must be human-readable and Copilot-actionable.

Required structure:

```markdown
## Scope
- Module: <module>
- Files: <paths>
- Status: <status>

## Source Evidence
- Source graph: <path>
- Doxygen artifact: <path>
- Docs compliance: <path>

## What needs to be done
- <clear task 1>
- <clear task 2>

## Validation
- Repro command: <command>
- Gate check: <command>
- Success condition: <condition>

## Acceptance criteria
- [ ] <criterion>
- [ ] <criterion>
- [ ] <criterion>
```

Rules:

- Prefer concise, concrete tasks over broad statements like "improve" or "optimize".
- List exact files, modules, and commands when known.
- Keep the issue readable for a human reviewer while still being actionable for Copilot.

## 5. Copilot operating rule

When working on a module, Copilot must follow this order:

1. Read the human-readable module docs first.
2. Read the module evidence artifacts for current status and proof.
3. Read the actual source in the relevant path(s).
4. Validate with a small, relevant command before changing broader code.
5. Update docs and evidence in the same change when the behavior or contract changes.

This order keeps the workflow understandable and avoids reading large raw dumps before the actual task is clear.

The compact short-form version is available in [.github/copilot/module-quick-rules.md](copilot/module-quick-rules.md).

## 6. Resulting standard

The repository standard is:

- Human readable module docs remain the primary interface.
- Evidence files provide proof and traceability.
- Issues and implementation tasks stay concise and actionable.
- Source code and documentation remain aligned as a single module contract.

If a module cannot be explained by this model, it is a compliance gap until the documentation and evidence are brought into alignment.

## 7. Developer-quality documentation matrix for non-marketing docs

The repository also distinguishes between module capability pages and engineering-oriented developer documentation. Not every file is a marketing-facing feature page. Many files must satisfy a stricter engineering-quality bar centered on architecture, research, implementation, operations, and trust.

### 7.1 Shared quality rules for all developer docs

Every developer-facing document must be able to answer these questions:

- What is the purpose of this document and who is the intended reader?
- What is the source of truth behind the statement?
- What module, subsystem, or contract does it describe?
- Which failure modes, edge cases, or trade-offs apply?
- Why does this matter operationally or technically?
- What is the correct next action for a developer, maintainer, or operator?

There is no blanket marketing standard for developer docs. The standard is:

- source-grounded
- technically explicit
- operationally relevant
- reviewer- and maintainer-friendly
- verifiable against module code, architecture, or operating evidence

### 7.2 Doc type expectations

#### `FEATURES.md`

Purpose: capability and system-role summary.

Required focus:
- problem or scenario being solved
- module role in ThemisDB
- concrete capabilities
- business or operational value
- known limitations and trade-offs
- references to underlying module docs and source evidence

Quality rule:
- Keep it readable, but do not drift into generic marketing copy.
- A capability page is not a substitute for module architecture or implementation docs.

#### `ARCHITECTURE.md`

Purpose: system boundaries, contracts, runtime flow, and responsibility model.

Required focus:
- module boundaries and ownership
- interfaces and contracts
- data flow or event flow
- failure domains
- design constraints and non-goals

Quality rule:
- Clear ownership and explicit boundaries are more important than narrative polish.

#### `RESEARCH.md`, `MODELING.md`, `EMBEDDING.md`

Purpose: evidence and evaluation content for method-heavy work.

Required focus:
- problem definition and assumptions
- method or model description
- data basis and constraints
- metrics, evaluation, and benchmarks
- trade-offs and limits
- reproducibility or validation path

Quality rule:
- Claims must be measurable, evidence-based, and capable of review.
- Avoid vague phrases like "better", "optimized", or "improved" without a benchmark or metric.

#### `IMPLEMENTATION.md`, `API.md`, `MODULE_GUIDE.md`

Purpose: engineering execution guidance and operational contract details.

Required focus:
- purpose and scope
- interfaces and public contracts
- inputs, outputs, and invariants
- edge cases and exceptions
- validation or test expectations

Quality rule:
- The document must be actionable for a developer implementing or integrating the module.

#### `RUNBOOK.md`, `OPERATIONS.md`

Purpose: incident handling and operational recovery.

Required focus:
- failure modes and symptoms
- diagnosis path
- recovery steps and rollback conditions
- observability signals and relevant metrics
- safety boundaries and escalation notes

Quality rule:
- Documents must support real operations and incident triage, not abstract architecture discussion.

#### `SECURITY.md`, `TRUST.md`

Purpose: trust boundaries, risk posture, and control model.

Required focus:
- trust model and boundaries
- authentication and authorization posture
- key handling and secrets exposure risk
- mitigation and incident handling
- operational review boundary

Quality rule:
- Security documentation must name boundaries and risk explicitly rather than relying on implied trust.

### 7.3 Common quality gate for all docs

A developer-facing document is acceptable when it satisfies all of the following:

- It states the purpose and target reader.
- It identifies the relevant module or subsystem.
- It references the canonical source of truth or supporting evidence.
- It names failure modes, edge cases, or trade-offs.
- It explains operational or engineering relevance.
- It avoids unsupported or vague product language.
- It can be used by a maintainer or developer to make a concrete decision.

This is the repo-standard for engineering-quality documentation and should be treated as the default bar for any change that adds or updates internal developer docs.
