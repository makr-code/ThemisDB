# Module Agentic Task

## Purpose

Define the exact work contract for an agent that updates one module development-status issue under the active ThemisDB module status epic.

This task is intentionally issue-driven and evidence-driven. The agent does not implement product code by default. The primary goal is to produce and maintain a reliable GitHub module status issue whose claims are traceable to roadmap, future-planning, tests, build targets, and existing issue history.

## Current Tracking Context

- Active epic: `#5624` `[EPIC][STATUS][MODULES] ThemisDB Development Status 2026-07-18`
- One status sub-issue exists per module:
  - `api` -> `#5618`
  - `storage` -> `#5619`
  - `sharding` -> `#5620`
  - `llm` -> `#5621`
  - `server` -> `#5622`
  - `replication` -> `#5623`

## Task Objective

For one assigned module, the agent shall:

1. determine the current development status for the module issue from canonical repository sources;
2. summarize implementation maturity in the issue without inventing progress;
3. identify concrete open work for the issue from roadmap and future-enhancement planning;
4. collect verification evidence for the issue from relevant build and test targets where available;
5. capture risks, blockers, and the next milestone in the issue;
6. maintain the module sub-issue so that the parent epic can be used as a trustworthy operational dashboard.

## Minimum Required Module Tasks

These tasks are mandatory for each module run:

1. analyze the module issue scope against module-adjacent planning and architecture sources;
2. update developer documentation where drift is confirmed and relevant for the issue claim;
3. validate build/test truth for the issue or document the evidence gap explicitly;
4. classify roadmap/documentation drift and unresolved gaps that affect the issue;
5. update the GitHub module status issue with evidence-backed conclusions.

## Recommended Additional Module Tasks

Beyond the two core tasks above, the following are also useful and should normally be part of the module task:

1. Architecture alignment check
  - verify that the actual source structure and public responsibilities still match `ARCHITECTURE.md`
  - flag mismatches between declared module boundaries and real code ownership

2. Build and test mapping
  - identify the relevant CMake targets, focused tests, and validation commands for the module
  - record whether the module has sufficient executable verification coverage

3. Documentation drift classification
  - distinguish whether the drift is in Level 1 module docs, Level 2 summaries, Level 3 root docs, or Level 4 `docs/` output
  - map the issue to the correct source-of-truth domain

4. Gap triage
  - separate implemented-but-undocumented behavior from planned-but-unimplemented roadmap items
  - separate stale documentation from missing tests or missing code

5. Risk and blocker extraction
  - identify architectural debt, test gaps, validation blind spots, unresolved dependencies, or governance ambiguity
  - capture only blockers that materially affect delivery or status interpretation

6. Acceptance and readiness review
  - assess the module's `Implementation Phases` and `Production Readiness Checklist`
  - identify which acceptance items are satisfied, missing evidence, or clearly still open

7. Issue-history synthesis
  - inspect recent `area:<module>` issues to determine whether the module is in feature delivery, hardening, remediation, or governance mode
  - use issue history only to refine interpretation, not to override canonical module docs

8. Closed issue and PR completeness check
  - inspect selected closed issues and merged PRs for scope that may have been only partially implemented
  - treat closed issue/PR history as a gap-discovery source, not as primary status truth
  - check whether claimed acceptance, tests, docs, or follow-up work actually landed in canonical sources

9. Upstream/downstream docs sync recommendation
  - if module-doc changes imply root or public doc drift, note required downstream follow-up
  - do not silently leave Level 3 or Level 4 drift unmentioned

10. Actionable follow-up generation
  - produce a short list of concrete next steps that a maintainer or follow-up agent can execute
  - prefer issue-worthy tasks over vague recommendations

## Canonical Inputs

The agent must prioritize these inputs in this order:

1. `src/<module>/ROADMAP.md`
2. `src/<module>/FUTURE_ENHANCEMENTS.md`
3. `ARCHITECTURE.md`
4. relevant module tests under `tests/`, `src/`, or focused targets in CMake tasks
5. existing open/closed GitHub issues for the same module
6. relevant merged PRs for the same module when needed to validate closure completeness
7. `DOCUMENTATION_GOVERNANCE.md`
8. `MODULE_DOCUMENTATION_STANDARD.md`
9. `MODULE_DOCUMENTATION_TEMPLATES.md` when a file must be created or restructured
10. changelog or development summary documents only as secondary corroboration

The agent must not use secondary docs to override module roadmap truth.

## Required Deliverable In The Module Issue

The issue body must contain and keep current these sections:

1. `Module Identity`
2. `Current Status`
3. `Implementation Phases Snapshot`
4. `Evidence`
5. `Open Work`
6. `Risks / Blockers`
7. `Next Milestone`
8. `Closure Criteria`

This issue body is the primary deliverable of the task. Documentation updates and evidence collection exist to support the correctness of the issue.

## Status Classification Rules

Use these rules consistently:

- `[ ] open`
  - module has significant planned work remaining and no dominant active execution wave
- `[~] in progress`
  - module has active implementation or hardening work in current roadmap windows
- `[x] done`
  - only use if roadmap and future-enhancements indicate no meaningful open delivery items for the tracked scope
- `[?] blocked`
  - use only when progress depends on an external unresolved dependency, missing decision, or unavailable prerequisite

Do not mark a module as done only because substantial code already exists.

## Required Agent Workflow

### Step 1: Resolve module identity

- confirm module name and issue number
- confirm canonical roadmap and future-enhancements paths
- confirm expected area label

### Step 2: Extract roadmap state

- read `Current Status`
- read `In Progress`
- read `Planned Features`
- read `Implementation Phases`
- read `Production Readiness Checklist`
- read `Known Issues and Limitations`

### Step 3: Check architecture alignment

- read relevant sections in `ARCHITECTURE.md`
- confirm declared module purpose, boundaries, and key responsibilities
- note any meaningful mismatch between architecture claims and current code reality

### Step 4: Extract future-planning state

- read `Scope`
- read `Design Constraints`
- read `Required Interfaces`
- read `Implementation Notes`
- read `Test Strategy`
- read `Performance Targets`
- read `Security / Reliability`

### Step 5: Corroborate with issue history

- inspect recent closed issues with `area:<module>` when present
- identify whether the module is currently in a hardening wave, feature wave, migration wave, or documentation/governance wave
- note only patterns that change interpretation of the roadmap status

### Step 6: Check closed issue and PR completeness

- inspect selected closed issues and merged PRs where roadmap or documentation claims appear stronger than current source evidence
- verify whether the claimed scope is fully reflected in code, tests, and module documentation
- capture partial implementations, superseded work, or undocumented residual gaps
- do not treat closure state alone as evidence of completion

### Step 7: Gather validation evidence

- identify relevant build preset and focused test targets
- if a narrow validation command exists, prefer that over broad full-repo validation
- record the latest command, date, and result summary
- if no practical validation is available, write an explicit evidence gap note

### Step 8: Update developer documentation

- update Level 1 developer documentation when source-confirmed drift exists
- preserve source-of-truth ordering from `DOCUMENTATION_GOVERNANCE.md`
- if downstream docs also drift, record the required follow-up explicitly

### Step 9: Update the module issue

- rewrite placeholders with module-specific content
- keep summaries compact and factual
- preserve issue structure so the epic remains mechanically scannable

## Documentation Update Rules

When developer documentation is changed, the agent must:

- treat module-adjacent documentation as Level 1 primary documentation
- follow the per-file role and structure rules in `MODULE_DOCUMENTATION_STANDARD.md`
- use `MODULE_DOCUMENTATION_TEMPLATES.md` when a file is missing or needs full structural normalization
- avoid making root or public docs the primary source for module status claims
- state the canonical upstream source for any status assertion
- note downstream documentation follow-up when module-doc updates imply drift in root or public docs
- avoid introducing contradictory status semantics across roadmap, future-planning, and aggregate docs

## Suggested Task Shape Per Module

Use this task shape unless the user narrows scope further:

1. Issue scope confirmation and source-of-truth collection
2. Architecture and documentation drift check relevant to the issue
3. Build/test evidence collection for the issue
4. Drift and gap classification affecting issue correctness
5. Module issue update with next actions

## Output Quality Requirements

The issue update must:

- be source-traceable to roadmap/future documents
- be architecture-aware where `ARCHITECTURE.md` makes concrete module claims
- distinguish current fact from planned work
- distinguish fully landed work from merely closed issue/PR history
- avoid vague statements like `improve`, `enhance`, or `optimize` without a concrete task reference
- include at least two concrete open-work items when the module is not done
- include at least one concrete next milestone
- include evidence or an explicit evidence gap
- mention downstream doc follow-up when Level 3 or Level 4 drift is discovered

## Non-Goals

This task does not require the agent to:

- implement missing roadmap items by default
- refactor code unless the user explicitly extends scope
- close the issue automatically based on assumption
- create additional parent epics for the same module scope
- accept a closed issue or merged PR as sufficient proof of completion without source verification
- treat source code review as the end product; it is only supporting evidence for the issue

## Closed Issue / PR Review Rules

When reviewing closed issues or merged PRs, the agent must:

- use them to detect possible partial implementation, drift, or follow-up debt
- verify claimed outcomes against code, tests, and module-adjacent docs
- explicitly call out `closed but only partially landed` situations when evidence supports that conclusion
- prefer a small number of relevant issues/PRs over broad historical archaeology
- stop once enough evidence exists to classify the module status reliably

## When The Closed Issue / PR Check Is Mandatory

The agent must perform this check when one or more of the following apply:

1. roadmap or module docs claim completion, but source/tests do not clearly confirm it
2. a module recently had a concentrated delivery or hardening wave
3. issue history suggests superseded, partially merged, or split scope
4. production-readiness or phase-check items appear overly optimistic relative to current evidence
5. there is visible drift between code reality and documentation claims

## When The Closed Issue / PR Check Is Optional

The agent may skip or keep this check minimal when all of the following are true:

1. roadmap status is clearly open or in progress
2. current code and tests already align with the documented status
3. no recent closure history appears relevant to status interpretation
4. there is no sign of overstated completion in docs or issue bodies

## Priority Order For Module Analysis

Apply this order unless the user narrows scope differently:

1. canonical module docs: roadmap and future-enhancements
2. architecture alignment
3. code and test reality
4. build/test evidence
5. closed issue and PR completeness check when triggered
6. developer-doc synchronization
7. module issue update and next actions

## Prompt Checklist

Use this checklist inside the agent prompt or task handoff:

- identify canonical module roadmap and future-enhancements files
- compare source reality against roadmap, future-planning, and architecture claims
- check focused tests/build targets or record explicit evidence gaps
- review relevant closed issues/PRs only if completion claims need corroboration or challenge
- update Level 1 developer documentation when drift is confirmed
- update the module status issue with status, evidence, open work, risks, and next milestone

## Escalation Conditions

The agent should stop and ask for direction if:

- multiple roadmap files conflict and no canonical module path is clear
- the issue scope should be split into more than one module issue
- the available tests/build targets are too ambiguous to report reliable evidence
- the module appears to need a separate epic rather than a status sub-issue

## Completion Criteria

The task is complete when all of the following are true:

- the module issue body is fully populated with non-placeholder content
- status classification is justified by roadmap evidence
- architecture-relevant mismatches are either documented or explicitly absent
- open work reflects actual roadmap or future-enhancement items
- closed issue/PR signals that materially affect status interpretation have been checked when needed
- evidence is documented or explicitly marked as missing
- Level 1 developer documentation has been updated when confirmed drift exists
- risks/blockers and next milestone are concrete
- the result is consistent with the parent epic format

## Suggested Operator Prompt

Use this prompt when assigning the task to an agent:

`Update the ThemisDB module status issue for <module>. The issue is the primary deliverable. Use src/<module>/ROADMAP.md and src/<module>/FUTURE_ENHANCEMENTS.md as canonical sources, corroborate with recent area:<module> issues, collect narrow build/test evidence where available, and fully populate the existing module status issue body with factual current status, open work, risks, and next milestone. Do not implement product code unless required to obtain verification evidence.`

Short checklist version:

`Treat the module issue as the primary output. Check roadmap/future docs, architecture alignment, current code/tests, focused build/test evidence, and only then relevant closed issues/PRs if completion claims need verification. Update Level 1 developer docs on confirmed drift and sync the module status issue with evidence-backed conclusions.`

## Short Answer To "Which Tasks Are Still Sensible?"

Yes. In addition to source analysis and developer-doc updates, the module task should normally also include:

1. architecture alignment against `ARCHITECTURE.md`
2. build/test target discovery and evidence capture
3. production-readiness and phase-check assessment
4. documentation drift classification by documentation level
5. gap triage: code vs docs vs tests vs roadmap
6. closed issue and merged PR completeness check
7. risk/blocker extraction
8. module issue synchronization in GitHub
9. downstream doc follow-up identification when root/public docs drift