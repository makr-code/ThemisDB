# Module Documentation Standard

**Status:** Active  
**Effective Date:** 2026-07-18  
**Scope:** Level-1 module-adjacent markdown files under `src/<module>/`, `include/<module>/`, `tests/<module>/`, and `benchmarks/<module>/`

---

## 1. Goal

This document defines which module-local markdown files are intended for which information, how they must be structured, and how they are updated during normal work and agentic documentation runs.

Fillable starter templates for the standard module markdown files are provided in `MODULE_DOCUMENTATION_TEMPLATES.md`.

Design principle:

**Each category of module information has one primary markdown home.**

If information is duplicated across several module markdown files, the duplication must be summary-only and must not redefine source truth.

---

## 2. Canonical Core Module Markdown Set

The standard core set for `src/<module>/` is:

1. `README.md`
2. `ARCHITECTURE.md`
3. `ROADMAP.md`
4. `FUTURE_ENHANCEMENTS.md`
5. `SECURITY.md`
6. `PRODUCTION_REQUIREMENTS.md`
7. `PERFORMANCE_EXPECTATIONS.md`
8. `AUDIT.md`
9. `MODULE_GAPS.md`
10. `CHANGELOG.md`

If a module does not yet contain one of these files, the file should be created when the missing information is necessary for reliable Level-1 documentation.

---

## 3. File Purpose Matrix

| File | Primary purpose | Canonical content | Must not become |
|---|---|---|---|
| `README.md` | module overview and orientation | purpose, scope, interfaces, boundaries, verified implementation summary | roadmap or changelog dump |
| `ARCHITECTURE.md` | module structure and execution model | planes, components, contracts, boundaries, failure semantics | feature backlog |
| `ROADMAP.md` | current delivery plan and maturity state | current status, in progress, planned features, phases, readiness, limitations | historical release log |
| `FUTURE_ENHANCEMENTS.md` | forward-looking expansion and design backlog | scope, constraints, interfaces, test strategy, targets, risk backlog | current status summary |
| `SECURITY.md` | module-specific security posture | trust boundaries, authz/authn assumptions, fail-closed behavior, sensitive surfaces | generic global security policy |
| `PRODUCTION_REQUIREMENTS.md` | operational minimum bar | MUST/MUST NOT rules, runtime prerequisites, production checks, deployment assumptions | architecture narrative |
| `PERFORMANCE_EXPECTATIONS.md` | module-specific performance truth | hot paths, targets, benchmarks, release budgets, measurement notes | generic optimization wishlist |
| `AUDIT.md` | source-verified audit summary | verification scope, reviewed files, findings state, evidence links, unresolved notes | raw scanner dump |
| `MODULE_GAPS.md` | structured gap inventory | scanner-derived or curated gap lists, severity overview, verification notes | implementation plan |
| `CHANGELOG.md` | historical implementation record | completed changes, versioned release notes, unreleased shipped deltas | future planning document |

---

## 4. Required Structure Per Core File

### 4.1 `README.md`

Required order:

1. title
2. validation/provenance header
3. module purpose
4. relevant interfaces or key files
5. scope and out-of-scope boundaries
6. runtime behavior or operational limits
7. sourcecode verification summary

### 4.2 `ARCHITECTURE.md`

Required order:

1. title
2. validation/provenance header
3. overview
4. main execution planes or major subsystems
5. core contracts
6. failure semantics
7. sourcecode verification summary

### 4.3 `ROADMAP.md`

Required order:

1. title
2. status legend/header
3. `Current Status`
4. `In Progress`
5. `Planned Features`
6. `Implementation Phases`
7. `Production Readiness Checklist`
8. `Known Issues and Limitations`
9. `Breaking Changes` when relevant

### 4.4 `FUTURE_ENHANCEMENTS.md`

Required order:

1. title
2. validation/provenance header
3. `Scope`
4. `Design Constraints`
5. `Required Interfaces`
6. `Implementation Notes`
7. `Test Strategy`
8. `Performance Targets`
9. `Security / Reliability`
10. optional risk backlog or wave-specific sections

### 4.5 `SECURITY.md`

Required order:

1. title
2. validation/provenance header
3. security scope
4. trust boundaries
5. sensitive operations and failure semantics
6. required controls and assumptions
7. verification notes and open security gaps

### 4.6 `PRODUCTION_REQUIREMENTS.md`

Required order:

1. title
2. validation/provenance header
3. purpose and scope
4. canonical split / document boundaries
5. binding production requirements
6. security requirements
7. operating limits
8. minimal production checklist
9. review / source audit evidence

### 4.7 `PERFORMANCE_EXPECTATIONS.md`

Required order:

1. title
2. validation/provenance header
3. performance scope
4. critical hot paths
5. targets / thresholds / budgets
6. measurement methodology or benchmark mapping
7. assumptions and caveats
8. evidence or benchmark verification notes

### 4.8 `AUDIT.md`

Required order:

1. title
2. validation/provenance header
3. audit scope
4. reviewed files / reviewed surfaces
5. findings summary
6. resolved findings or drift notes
7. remaining follow-up items

### 4.9 `MODULE_GAPS.md`

Required order:

1. title
2. provenance note identifying scanner-derived vs curated content
3. summary totals
4. severity breakdown
5. type breakdown
6. top gaps or representative gaps
7. verification notes and interpretation caveats

### 4.10 `CHANGELOG.md`

Required order:

1. title
2. version or unreleased sections
3. completed user-visible or module-relevant changes
4. references to issues/PRs where appropriate

---

## 5. Update Rules Per File

### `README.md`
- Update when module purpose, scope boundaries, major interfaces, or verified implementation surfaces change.
- Do not use to track speculative work.

### `ARCHITECTURE.md`
- Update when execution planes, contracts, boundaries, key responsibilities, or failure semantics change.
- Do not use to track acceptance checklists or milestones.

### `ROADMAP.md`
- Update when module status, in-progress work, phases, readiness, or known limitations change.
- Completed items must move to `CHANGELOG.md` rather than remain as roadmap history clutter.

### `FUTURE_ENHANCEMENTS.md`
- Update when medium-term or long-term design intent changes.
- Do not use for currently shipped behavior except as explicit constraint context.

### `SECURITY.md`
- Update when trust boundaries, auth assumptions, failure modes, data sensitivity, or security controls change.

### `PRODUCTION_REQUIREMENTS.md`
- Update when deployment prerequisites, production safety assumptions, runtime controls, or operational minimums change.

### `PERFORMANCE_EXPECTATIONS.md`
- Update when benchmark targets, hot paths, measurement baselines, or release budgets change.

### `AUDIT.md`
- Update after source review, audit pass, or curated reconciliation of findings.
- Keep it human-curated even when scanner data exists elsewhere.

### `MODULE_GAPS.md`
- Update after gap scans or curated gap reclassification.
- If not rescanned in a run, state that clearly rather than implying freshness.

### `CHANGELOG.md`
- Update only for completed changes, shipped deltas, or accepted unreleased completed work.
- Do not store future tasks here.

---

## 6. Agentic Run Requirements

During an agentic module documentation run, all `src/<module>/*.md` files must be reviewed.

For each markdown file, exactly one of these outcomes is required:

1. content updated because source truth changed;
2. validation/provenance header refreshed because the file was re-verified and remains correct;
3. explicit note added that the file is historical/scanner-derived and was not re-generated in this pass.

An agentic run is incomplete if module markdown files were not at least reviewed against their intended role.

---

## 7. Provenance Header Rules

Maintained module markdown files should begin near the top with a compact provenance block such as:

`<!-- Status: current | validated: YYYY-MM-DD -->`

Optional additions when relevant:

- agentic issue reference
- upstream links
- scan or audit note

Do not fabricate validation dates. Only refresh them when the file was actually reviewed against current source truth.

---

## 8. When A New Module Markdown File Should Be Created

Create a new module-local markdown file only when one of these applies:

1. the information does not fit the canonical purpose of an existing file without causing semantic overload;
2. a feature area is large enough to require its own developer-facing contract or deep-dive;
3. an audit, quality-control, async/distributed, or advanced-features topic would otherwise distort the module core docs;
4. a recurring topic needs a stable Level-1 home rather than repeated issue comments or `ai_working` drafts.

Examples of acceptable specialized files:

- `ADVANCED_FEATURES_README.md`
- `ASYNC_AND_DISTRIBUTED_OPERATIONS.md`
- `QUALITY_CONTROL_README.md`
- `AI_ML_IMPACT_ASSESSMENT.md`
- `TODO_CRITICAL_GAPS.md`

---

## 9. Rules For Specialized Additional Markdown Files

Specialized module markdown files must:

1. have a narrowly defined scope in the title and opening section;
2. state which canonical core file they complement;
3. avoid redefining roadmap status, security posture, or release truth unless they are the designated primary file for that domain;
4. include provenance/validation notes if they make status-like claims;
5. be linked from `README.md` or the most relevant primary file when they are part of the maintained module surface.

If a specialized file becomes the only place where important module truth lives, the split is wrong and the primary file set must be updated.

---

## 10. Update Workflow

For any module-doc change, apply this order:

1. confirm source-of-truth domain;
2. update the correct primary module markdown file first;
3. update complementary module markdown files that summarize or depend on that truth;
4. update specialized files only if their scoped topic changed;
5. then update downstream Level-2, Level-3, and Level-4 docs if drift exists.

---

## 11. Decision Rules For Agents And Maintainers

When deciding where information belongs:

- implementation status -> `ROADMAP.md`
- completed shipped change -> `CHANGELOG.md`
- module overview and orientation -> `README.md`
- structure and execution model -> `ARCHITECTURE.md`
- long-term design intent -> `FUTURE_ENHANCEMENTS.md`
- security/trust/fail-closed behavior -> `SECURITY.md`
- operational minimums -> `PRODUCTION_REQUIREMENTS.md`
- benchmark and performance targets -> `PERFORMANCE_EXPECTATIONS.md`
- reviewed findings summary -> `AUDIT.md`
- scanner or curated gap inventory -> `MODULE_GAPS.md`

If no existing file is a clean fit, create a specialized markdown file and link it back to the appropriate primary file.

---

## 12. Relation To Other Governance Files

- `DOCUMENTATION_GOVERNANCE.md` defines precedence, levels, SOT rules, and orchestration.
- this file defines the per-module markdown contract within Level 1.
- `ai_working/MODULE_AGENTIC_TASK.md` defines how an agent executes against this standard.

If these documents disagree on source precedence, `DOCUMENTATION_GOVERNANCE.md` wins.