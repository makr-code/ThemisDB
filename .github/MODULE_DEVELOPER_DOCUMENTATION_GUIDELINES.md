# Module Developer Documentation and Source Structure Guidelines

Purpose: This file is the repository-level contract for how module documentation and source code must be structured so that Copilot, humans, and release gates can understand the module's intent, evidence, and implementation state.

## 1. Required module shape

Every active module must be understandable in the same shape, regardless of subsystem.

### Required module docs

For each module under `src/<module>/` and `include/<module>/`, maintain the following artifacts when relevant:

- `README.md`
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
