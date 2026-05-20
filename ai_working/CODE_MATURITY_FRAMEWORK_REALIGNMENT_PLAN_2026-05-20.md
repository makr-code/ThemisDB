# Code Maturity Framework Realignment Plan

Date: 2026-05-20
Scope: Gap Scanner -> ai_working artifacts -> Header generation -> CI workflow
Status: Draft for execution

## Goal

Create one coherent framework with clear source-of-truth artifacts and non-overlapping responsibilities.

## Current Problem Statement

The repository currently mixes multiple pipelines and output formats:

1. CI path:
- .github/workflows/08-maintenance_code-maturity.yml
- .github/scripts/code_maturity_header_writer.py
- outputs: docs/code_maturity_report.md, .github/version_tracking.json, .github/badges/*.json

2. Tooling path (local/manual):
- tools/gap_scanner_v2.py, tools/gap_audit_pipeline_v2.py, tools/file_header_updater.py
- outputs: ai_working/gap_scan_v2_*.json, ai_working/gap_scan_v2_summary.json, ai_working/module_gaps/*

3. Extended scanner path (v3):
- tools/gap_scanner_v3*.py
- outputs: ai_working/gap_scan_v3_*.json and issue templates

These paths overlap in intent but not in output contract.

## Execution Principles

- One canonical CI source of truth for quality status.
- Local scanner artifacts stay in ai_working and are explicitly non-canonical for release reporting.
- Header format contract must be unique and versioned.
- Documentation in ai_working is design/working-state; durable operator docs live under docs/.

## Deliverables

1. Canonical architecture doc:
- ai_working/CODE_MATURITY_FRAMEWORK_TODO_2026-05-20.md

2. Implementation backlog:
- Unified TODO table with priorities, owners, and done criteria.

3. Migration decisions:
- Decide whether docs/code_maturity_report.md remains canonical CI report or is replaced by a committed JSON->Markdown bridge.

4. Cleanup wave:
- Mark stale docs as legacy and add pointer to canonical docs.

## Acceptance Criteria

- No ambiguity about which tool writes which artifact.
- Exactly one canonical report artifact for CI gates.
- Header writers do not conflict in format or placement.
- Operators can run one documented command path for CI parity and one for deep audit.

## Implemented Step 1 (2026-05-20)

### A. Single header writer established

- Canonical header writer entrypoint: `.github/scripts/code_maturity_header_writer.py`
- `tools/gap_audit_pipeline_v2.py` delegates Stage 3 header updates to the canonical writer.
- Result: CI and local pipeline now use one shared header template path.

### B. Local remote metadata via gh

- `code_maturity_header_writer.py` delegates to implementation that resolves repository metadata with:
	1) `gh repo view --json nameWithOwner,defaultBranchRef,url` (preferred)
	2) git remote/branch fallback if `gh` is unavailable
- Metadata is rendered into the generated maturity report.

### C. Cyclic CI schedule at 03:00

- Workflow `.github/workflows/08-maintenance_code-maturity.yml` schedule set to daily 03:00 UTC (`0 3 * * *`).
- Check-only mode remains default for scheduled runs.
