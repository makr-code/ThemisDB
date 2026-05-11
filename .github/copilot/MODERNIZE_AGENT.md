# @Modernize Agent Configuration

This guide defines how to run modernization tasks safely and incrementally for ThemisDB.

## Purpose

Use `@Modernize` to migrate legacy C++ code paths toward modern C++20/23 patterns while preserving behavior.

## Execution Model

### 1) Assessment phase (`assessment.md`)

Produce a concise assessment with:

- Legacy constructs discovered (e.g. C++11/14 idioms, raw ownership, outdated patterns)
- Affected files/modules
- Risk classification (low/medium/high)
- Required validation commands

### 2) Plan phase (`plan.md`)

Create a concrete plan with:

- Small, reviewable increments
- Explicit acceptance criteria per increment
- Rollback strategy for each increment
- Documentation sync requirements

### 3) Autonomous execution phase

Execute the approved plan incrementally:

- Keep edits scoped to agreed files
- Preserve external behavior unless explicitly approved
- Update documentation in the same PR
- Re-run agreed validation commands after each increment

## Build/Validation Verification

Modernization output is only acceptable after validation:

- Run existing repository lint/build/test commands relevant to touched scope
- Report environment limitations if local validation cannot run fully
- Do not mark tasks complete without explicit validation evidence

## Trigger Scenarios

Use this agent when you encounter:

- Outdated MSVC/toolchain compatibility constraints requiring modernization
- C++11/14 constructs that should be migrated to C++20/23
- Legacy ownership or concurrency patterns that should align with repository guidance
- Repetitive modernization changes across multiple files

## Staging Workflow

For exploratory drafts, use `ai_working/` as temporary staging area before promoting reviewed results into production paths.
