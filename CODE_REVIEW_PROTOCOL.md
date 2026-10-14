---
Author: "ThemisDB Review Team"
Created: "2026-10-14"
Last Updated: "2026-10-14"
Status: "active"
---

# Code Review Protocol for Wave 10 Phase 1

This protocol governs the shared review flow for both governance and Phase 3 Batch 1 implementation work in Wave 10 Phase 1. It is intended to align with the parallel execution plan and the review checklists used during the converged review week.

## 1. Review objectives

The reviewer must confirm that every PR in the Wave 10 Phase 1 milestone meets the relevant review criteria before merge:

- Block D governance and compliance tests satisfy the release-gate and documentation requirements.
- Block A Phase 3 Batch 1 tests exercise their specified module lifecycle, failure paths, and edge cases.
- Integration points remain stable across CMake registration, shared helpers, and release evidence.
- Review outcomes are documented in a way that is traceable to the issue and milestone.

## 2. Scope and review lanes

### Governance lane (Block D)

Review all PRs that change:

- `audit/**/*.md`
- `PR_GOVERNANCE_TESTS_CHECKLIST.md`
- `GOVERNANCE_TEST_TEMPLATE.md`
- release gates, audit workflow logic, or governance evidence references

Review questions:

- Does the change validate the correct release wave and edition boundary?
- Does the change reflect the current roadmap, documentation governance, and audit contract?
- Does the PR capture the right evidence and release-readiness rationale?

### Implementation lane (Block A)

Review all PRs that change:

- `tests/distributed_tensor/**`
- `tests/llama_cpp/**`
- `tests/stable_diffusion/**`
- `tests/execution/**`
- shared CMake scope or cross-module test registration

Review questions:

- Are the public APIs under test enumerated and verified?
- Does the test fail for the correct reason when the implementation is wrong?
- Are edge cases and error paths covered without broad or redundant assertions?
- Are performance assertions realistic and tied to the module contract?

## 3. Required review workflow

### Step 1: Initial triage

- Validate the PR metadata: milestone, labels, issue link, and module scope.
- Confirm the PR matches the issue or tracking artifact for the relevant stream.
- Assess whether the change stays inside the planned scope for Wave 10 Phase 1.

### Step 2: Rule-based checks

Review the PR against the applicable checklist:

- `PR_GOVERNANCE_TESTS_CHECKLIST.md` for Block D
- `PR_PHASE3_BATCH1_CHECKLIST.md` for Block A

The reviewer should confirm the following before approval:

- the correct milestone is assigned;
- labels match the stream and module under review;
- test intent is clear and evidence is linked;
- expected pass/fail conditions are explicit;
- failures are actionable and not vague or redundant.

### Step 3: Behavioral validation

- Run or inspect the relevant local or CI validation commands.
- Confirm the change passes the target test command or build target.
- Check for flaky or non-deterministic assertions, especially around timing and order.
- Validate at least one negative-path assertion for each major test area.

### Step 4: Cross-module sanity check

For shared or cross-module changes, confirm:

- naming is consistent with the module and release scope;
- CMake registration or target names are unique and stable;
- release evidence files remain aligned with the current milestone;
- no unrelated refactors or broad cleanup are bundled into the PR.

## 4. Review outcome classifications

Use the following review states:

- `approved`: required checks are satisfied and the change is ready to merge.
- `changes requested`: the logic, scope, or evidence is incomplete and requires revision.
- `hold`: a dependency, branch, or agreement issue blocks merge until clarified.
- `needs discussion`: the change is directionally correct but requires a maintainer decision.

## 5. Merge gate

A PR may be merged only when all of the following are true:

- The applicable checklist is complete.
- Relevant validation evidence is attached or recorded.
- Review feedback is either resolved or explicitly accepted with rationale.
- The change remains within the issue scope and milestone boundaries.

## 6. Review sign-offs

The reviewer should record the following in the PR discussion before merge:

- module or governance stream reviewed;
- validation commands or CI links reviewed;
- risk or follow-up items, if any;
- final approval status.

## 7. Shared reviewer emphasis for Week 5

During the Week 5 code review ramp-up, the review should prioritize the following order:

1. Governance tests and release evidence completeness
2. Phase 3 Batch 1 module test correctness and edge coverage
3. CMake registration and integration stability
4. Final merge readiness and release confidence

## 8. Escalation path

If a PR cannot be approved because of ambiguity, build instability, or a missing requirement, escalate using the same path defined in `ai_working/PARALLEL_D_A_EXECUTION_PLAN.md`:

- build or CI issue → integration lead or CI owner
- governance ambiguity → governance lead or maintainer
- module API uncertainty → module owner
- reviewer bottleneck → project manager or issue owner

This protocol is the shared baseline for Wave 10 Phase 1 review convergence and should remain aligned with the governance and implementation checklists in this repository.
