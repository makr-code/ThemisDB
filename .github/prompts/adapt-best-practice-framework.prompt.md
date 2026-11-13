---
name: "ThemisDB Best-Practice Framework Adaptation"
description: "Review and adapt a scoped ThemisDB architecture or GitHub Actions framework using shallow hierarchies, reusable workflows, separation of concerns, and appropriate OOP boundaries."
argument-hint: "Scope (module, workflow, or paths) and requested outcome"
agent: "agent"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

Review and, when requested, adapt the specified ThemisDB framework or scope to fit the repository's established best practices. Treat the current repository architecture and governance as the baseline; do not impose a generic framework or perform a broad rewrite.

## Required context

Before proposing or making changes, consult the relevant sources:

- `AI_WIKI_INTEGRATION_PLAYBOOK.md`
- `ai_context/developer_llm_wiki/INDEX.md`
- `ai_context/developer_llm_wiki/MODULES_AND_APIS.md`
- `ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md`
- `ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md`
- `ai_context/developer_llm_wiki/WIKI_STATUS.json`
- `ai_context/OOP_AND_SOC_PRINCIPLES.md`
- The applicable root source-of-truth, module architecture, tests, and existing workflow examples

Resolve conflicts using the repository's source-of-truth order. Treat compiled wiki material as context, not as authority over active root governance or module contracts. State stale, missing, or conflicting evidence explicitly.

## Design criteria

Evaluate the requested scope against these principles:

1. **Shallow hierarchies:** Keep module, workflow-call, and job-dependency structures easy to navigate. Prefer a small number of clear layers and direct ownership. Do not flatten distinct responsibilities into a monolith just to reduce nesting.
2. **Reusable workflows:** Extract repeated CI orchestration only when the shared contract is stable and reduces meaningful duplication. Make inputs, outputs, permissions, secrets, runner requirements, and failure behavior explicit. Keep callers thin and avoid deep chains of reusable workflows.
3. **Separation of concerns:** Give each module, workflow, job, and helper one coherent responsibility. Keep build, test, security, release, and governance behavior separate where their lifecycle or permission needs differ. Define and preserve explicit contracts at boundaries.
4. **OOP where appropriate:** For runtime code, favor cohesive types, encapsulated state, stable interfaces, dependency injection, and RAII according to the existing C++ guidance. For YAML workflows, apply the same cohesion and boundary principles without inventing classes, wrappers, or abstractions that the workflow model does not need.
5. **Small, evidence-led changes:** Reuse existing repository patterns. Identify the code or workflow that directly controls the behavior, state a falsifiable hypothesis, and choose a focused validation that could disprove it before editing.

## Workflow interfaces and safe execution

When adapting GitHub Actions workflows, treat the workflow set as one framework while preserving clear ownership and avoiding a repository-wide rewrite unless that scope is explicitly requested:

- Define and document dry-run behavior for every workflow. Workflows that publish, deploy, distribute, push commits, update issues/PRs/wiki, or otherwise mutate external state must expose a consistent `dry_run` control on manual and reusable-workflow entry points where supported, with a safe default (`true` for potentially destructive or externally visible operations). A read-only workflow may be dry-run-equivalent by design, but must state that it performs no external mutation and must not skip its validation/build/test work. Do not broaden triggers solely to add a dry-run input.
- A dry run must still perform safe planning, validation, and build steps, but must prevent every consequential side effect. Summarize the actions that would have occurred. Do not treat `continue-on-error`, skipped validation, or a green status without the real checks as a dry run.
- Read-only validation workflows do not need an artificial dry-run switch. If they already expose one, make its behavior explicit and consistent.
- Standardize common input names, types, descriptions, and semantics across related workflows and reusable callers. Give each input a concise explanation of purpose, accepted values, effect, and source; prefer `choice` for enumerations and `boolean` for toggles. Prepopulate safe, useful defaults whenever a valid configuration-derived value is available.
- Do not ask operators to re-enter information already maintained in repository configuration. Resolve versions, editions, paths, matrices, feature switches, and release metadata from the owning configuration/manifests first. Use explicit dispatch inputs only as intentional overrides.
- Apply and document one resolution order: explicit non-empty operator override, then the authoritative configuration file or repository variable, then a safe documented default. Validate the resolved value and report its source in the run summary. Do not silently substitute an unrelated value.
- Read structured configuration with an appropriate parser or existing repository helper, not fragile regex/text extraction. Never store or print secrets in configuration-derived summaries; continue to use GitHub secrets and least-privilege permissions for credentials.
- Every workflow must produce a concise GitHub Actions run summary on success and failure, using `$GITHUB_STEP_SUMMARY` or the repository's established summary mechanism. Include purpose/scope, resolved inputs and their sources, key validation/build/test outcomes, artifacts or outputs, and actionable links or names for failed jobs/steps. Keep the workflow's real conclusion intact; summary generation must not mask failures.
- Provide useful debug evidence for failures: preserve relevant command output, identify the failing job/step and relevant configuration/runtime context, and upload bounded diagnostic artifacts when logs alone are insufficient. Debug output must be safe by default: redact credentials and sensitive values, avoid dumping entire environments/config files, and do not enable unrestricted shell tracing around secrets. An optional debug input may increase diagnostic detail but must not change the workflow's functional result.
- For a system-wide adaptation, inventory workflow entry points and side effects first, then group workflows by responsibility and identify reusable contracts. Report workflows intentionally left unchanged and why.

## Guardrails

- Start by stating scope, current behavior, evidence, and one concrete architectural issue or improvement opportunity.
- Separate verified facts from assumptions and recommendations; cite repository paths and relevant workflow/job names.
- Do not add a reusable workflow, interface, adapter, or extra hierarchy unless it removes real duplication or clarifies an existing boundary.
- Preserve existing public APIs, branch governance, security permissions, release gates, and trigger policy unless the requested scope explicitly requires a change.
- Do not weaken required checks, broaden secrets or write permissions, or turn failures into success/skip paths to make a workflow appear green.
- Do not introduce unapproved compatibility shims, stubs, mocks, or simulation paths.
- Keep unrelated user changes untouched. Stage and report only files within the requested scope.

## Execution and validation

If the request is analysis-only, do not edit files. Return findings and a prioritized, minimal adaptation proposal.

If implementation is requested:

1. Make the smallest change that addresses the verified issue while retaining the existing contracts.
2. Immediately run the narrowest relevant test, workflow linter, configure, build, or validation command available.
3. If that check fails, use its evidence to repair the same scope and rerun it before widening work.
4. Report exact validation commands and outcomes. Clearly distinguish passed checks from checks not run or blocked by environment.

## Response format

1. **Scope and current state** — behavior and source evidence.
2. **Assessment** — what already follows the principles and what does not.
3. **Adaptation** — minimal proposed or implemented changes, with rationale and trade-offs.
4. **Validation** — exact commands and results, or the reason validation was unavailable.
5. **Open questions and residual risks** — unresolved source conflicts, assumptions, and follow-up work.