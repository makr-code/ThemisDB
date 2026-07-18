# Module Documentation Templates

**Status:** Active  
**Effective Date:** 2026-07-18  
**Scope:** Fillable templates for Level-1 module markdown files

This file provides practical templates for the module markdown contract defined in `MODULE_DOCUMENTATION_STANDARD.md`.

Use these templates when:

- a module is missing one of the standard core markdown files;
- an existing markdown file is being reworked to match the standard structure;
- an agentic documentation run needs a stable target structure.

---

## README.md Template

```md
# ThemisDB <Module> Module

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Agentic status sync: module issue #<number> -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · AUDIT.md -->

## Module Purpose

<Short factual statement of module purpose.>

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| <file or class> | <responsibility> |

## Scope

In scope:
- <scope item>

Out of scope:
- <boundary item>

## Runtime Behavior and Limits

- <runtime fact>
- <operational limit>

## Sourcecode Verification

- Verified files:
  - <path>
- Verified behavior surfaces:
  - <surface>
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md
```

---

## ARCHITECTURE.md Template

```md
# Architecture - <Module> Module

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Agentic status sync: module issue #<number> -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · AUDIT.md -->

## Overview

<Short technical architecture statement.>

## Main Execution Planes

1. <plane>
- <behavior>

2. <plane>
- <behavior>

## Core Contracts

| Contract | Behavior |
|---|---|
| <contract> | <behavior> |

## Failure Semantics

- <failure rule>
- <boundary rule>

## Sourcecode Verification

- Verified files:
  - <path>
- Verified architecture claims:
  - <claim>
```

---

## ROADMAP.md Template

```md
# <Module> Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

<Current module maturity and shipped reality.>

## In Progress

- [~] <active item> (Target: <Q/YYYY>)

## Planned Features

### Short-term (3-6 months)
- [ ] <planned item> (Target: <Q/YYYY>)

### Mid-term (6-12 months)
- [ ] <planned item> (Target: <Q/YYYY>)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] <task> (Target: <Q/YYYY>)

### Phase 2: Core Implementation
- [ ] <task> (Target: <Q/YYYY>)

### Phase 3: Error Handling and Edge Cases
- [ ] <task> (Target: <Q/YYYY>)

### Phase 4: Tests
- [ ] <task> (Target: <Q/YYYY>)

### Phase 5: Performance and Hardening
- [ ] <task> (Target: <Q/YYYY>)

### Phase 6: Documentation and Acceptance
- [ ] <task> (Target: <Q/YYYY>)

## Production Readiness Checklist

- [ ] <readiness criterion>
- [ ] <readiness criterion>

## Known Issues and Limitations

- <limitation>

## Breaking Changes

<State none explicitly if none are planned.>
```

---

## FUTURE_ENHANCEMENTS.md Template

```md
# <Module> Module - Future Enhancements

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

## Scope

- <forward-looking scope>

## Design Constraints

- <constraint>

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| <interface> | <consumer> | <notes> |

## Implementation Notes

- <implementation note>

## Test Strategy

- <test strategy>

## Performance Targets

- <target>

## Security / Reliability

- <security or reliability requirement>

## Risk Backlog

- <risk>
```

---

## SECURITY.md Template

```md
# Security - <Module> Module

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Links: README.md · ARCHITECTURE.md · PRODUCTION_REQUIREMENTS.md · AUDIT.md -->

## Security Scope

<Define what security-sensitive surfaces this module owns.>

## Trust Boundaries

- <boundary>

## Sensitive Operations and Failure Semantics

- <sensitive operation>
- <fail-closed rule>

## Required Controls and Assumptions

- <control>

## Verification Notes and Open Security Gaps

- Verified files:
  - <path>
- Open gaps:
  - <gap or none>
```

---

## PRODUCTION_REQUIREMENTS.md Template

```md
# ThemisDB <Module> Module - Production Requirements

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Links: README.md · SECURITY.md · ROADMAP.md · AUDIT.md -->

## Purpose and Scope

<Define the operational scope of this document.>

## Canonical Split / Document Boundaries

- `src/<module>/PRODUCTION_REQUIREMENTS.md`: <this document's role>
- `src/<module>/README.md`: <overview role>
- `src/<module>/ROADMAP.md`: <planning role>
- `src/<module>/FUTURE_ENHANCEMENTS.md`: <future role>

## Binding Production Requirements

- MUST: <requirement>
- MUST NOT: <forbidden behavior>

## Security Requirements

- <security requirement>

## Operating Limits

- <limit>

## Minimal Production Checklist

- [ ] <criterion>

## Review / Source Audit Evidence

- Reviewed files:
  - <path>
```

---

## PERFORMANCE_EXPECTATIONS.md Template

```md
# Performance Expectations - <Module> Module

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · AUDIT.md -->

## Performance Scope

<Define the hot paths and performance-relevant responsibilities.>

## Critical Hot Paths

- <hot path>

## Targets / Thresholds / Budgets

- <budget>

## Measurement Methodology or Benchmark Mapping

- <benchmark or method>

## Assumptions and Caveats

- <assumption>

## Evidence or Benchmark Verification Notes

- <evidence>
```

---

## AUDIT.md Template

```md
# Audit - <Module> Module

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Agentic status sync: module issue #<number> -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md · MODULE_GAPS.md -->

## Audit Scope

<Define reviewed source surfaces and documentation scope.>

## Reviewed Files / Reviewed Surfaces

- <path or surface>

## Findings Summary

- <finding summary>

## Resolved Findings or Drift Notes

- <resolved or reconciled item>

## Remaining Follow-Up Items

- <follow-up>
```

---

## MODULE_GAPS.md Template

```md
# <module> - MODULE_GAPS.md

This file documents scanner-derived or curated gap inventory for the **<module>** module.

> Provenance note: <scanner-derived / curated / mixed>. <State whether the file was re-scanned in this pass.>

## Summary

- Total gaps: <number>
- Status: <verified / historical / rescanned>
- Last updated: <date or scan reference>

### By Severity

- CRITICAL: <n>
- HIGH: <n>
- MEDIUM: <n>
- LOW: <n>

### By Type

- <type>: <n>

## Top Gaps

- [<type>] <file:line> (<severity>)

## Verification Notes and Interpretation Caveats

- <note>
```

---

## CHANGELOG.md Template

```md
# Changelog - <Module> Module

## [Unreleased]

### Added
- <completed unreleased change>

### Changed
- <completed unreleased change>

### Fixed
- <completed unreleased fix>

## [<version>] - <YYYY-MM-DD>

### Added
- <released change>
```

---

## Specialized File Template

Use this when the standard core set is insufficient and a new module markdown file is justified.

```md
# <Specialized Topic> - <Module> Module

<!-- Status: current | validated: YYYY-MM-DD -->
<!-- Complements: <core file name> -->
<!-- Links: README.md · <most relevant core file> -->

## Scope

<Narrowly scoped topic definition.>

## Why This Is A Separate File

<Explain why the topic would overload a core file.>

## Technical Content

<Topic-specific content>

## Verification / Evidence

- <evidence>

## Follow-Up

- <follow-up>
```

---

## Usage Rules

1. Do not copy templates mechanically without adapting them to module truth.
2. If a section has no justified content, state `none`, `not applicable`, or a factual equivalent rather than leaving ambiguous placeholders.
3. If a file is historical or scanner-derived, say that explicitly.
4. Update primary files first, then complementary and specialized files.
5. When a new specialized file is created, link it from `README.md` or the closest primary file.