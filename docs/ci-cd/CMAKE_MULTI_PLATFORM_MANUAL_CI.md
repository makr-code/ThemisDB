# CMake Multi-Platform Manual CI

## Overview

`.github/workflows/cmake-multi-platform.yml` is the manual-only compile and test workflow for the `develop` branch flow.

Key properties:
- no automatic `push` trigger
- no automatic `pull_request` trigger
- execution only through `workflow_dispatch`
- automatic GitHub issue tracking for build, test, and sanitizer failures
- approval gating for follow-up compile runs after unresolved failures

## Manual execution

1. Open **Actions** in GitHub.
2. Select **CMake on multiple platforms**.
3. Choose the branch to validate.
4. Start the workflow with one of these sanitizer inputs:
   - `none`
   - `asan`
   - `ubsan`
   - `both`

The standard build matrix still validates:
- Linux + GCC
- Linux + Clang
- Windows + MSVC

Additional sanitizer jobs run only when requested through the manual input.

## Failure issue lifecycle

When a matrix build/test job or sanitizer job fails, the workflow follow-up job creates or updates a GitHub issue.

Deduplication is based on a stable signature composed from:
- workflow name
- branch/ref
- failure class (`ci/build`, `ci/test`, or `ci/sanitizer`)
- platform
- compiler

Operational behavior:
- if no open issue exists for the signature, a new issue is created
- if an open issue already exists for the signature, the workflow updates the issue body and appends a comment with the newest run metadata
- repeated failures therefore stay attached to one tracked issue per open signature

Each automated issue contains:
- workflow name
- run number
- run URL
- branch/ref
- commit SHA
- failing job and failing step(s)
- timestamp
- quick triage checklist

## Label taxonomy

The workflow ensures labels exist before use and then applies them automatically.

Core labels:
- `ci/failure` — automated CI failure tracker issue
- `ci/build` — build-stage failure in the regular compile matrix
- `ci/test` — test-stage failure in the regular compile matrix
- `ci/sanitizer` — failure in a sanitizer job
- `status/needs-approval` — blocks the next compile run behind maintainer approval
- `status/fixed` — applied when a later successful validation clears the tracked failure

Platform/compiler labels:
- `os/linux`
- `os/windows`
- `compiler/gcc`
- `compiler/clang`
- `compiler/msvc`

## Approval gate model

Open issues labeled with both `ci/failure` and `status/needs-approval` are treated as unresolved failure state.

At workflow start, the preflight job checks for open tracker issues that match:
- the same workflow
- the same branch/ref

If matching unresolved issues exist:
1. the workflow enters the `approval-gate` job
2. that job targets the GitHub environment `ci-approved`
3. build and sanitizer jobs wait until a human reviewer approves the environment deployment

This means a failed compile run does **not** prevent future manual execution forever; it requires one explicit maintainer approval before the next compile attempt proceeds.

## Success and state clearing

A successful run closes matching open failure issues for the signatures that were actually validated in that run.

Cleanup behavior:
- remove `status/needs-approval`
- add `status/fixed`
- add a resolution comment with the successful run URL
- close the issue

Sanitizer issues are only cleared when the corresponding sanitizer job was actually executed successfully.

## Required repository settings

The workflow file cannot enforce environment reviewers by itself. Repository administrators must configure the environment manually:

1. Go to **Settings** → **Environments**.
2. Create or open the environment `ci-approved`.
3. Add the required reviewers who are allowed to approve follow-up compile runs.
4. Save the protection rules.

Without required reviewers, the gate job still uses the environment name but will not provide a meaningful approval barrier.

## Maintainer operating procedure

### First failure
- inspect the generated CI issue
- use the labels to triage by failure class, OS, and compiler
- keep `status/needs-approval` in place while the failure is unresolved

### Next rerun after failure
- manually trigger **CMake on multiple platforms**
- approve the `ci-approved` environment when prompted
- review whether the rerun closes the matching issue(s)

### After success
- confirm the workflow closed the relevant tracker issue(s)
- confirm `status/fixed` replaced the approval-blocking state

## Notes

- The approval gate is branch-specific because tracker issues are matched on workflow + ref.
- The issue tracker is intentionally label-driven so maintainers can query states such as:
  - `is:issue is:open label:ci/failure label:status/needs-approval`
  - `is:issue label:ci/failure label:os/windows label:compiler/msvc`
  - `is:issue label:ci/sanitizer`
