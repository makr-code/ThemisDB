# Runbook: Prompt Engineering Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `prompt_engineering` module.
Covers the five most critical incident classes, their log patterns, triage
steps, and recommended remediation actions.

---

## Scenario 1 — Template Render Failure

**Log pattern:** `[PROMPT:TemplateFailed]`

**Symptoms**
- Template render requests return errors or empty outputs.
- Log lines contain `[PROMPT:TemplateFailed]` with template ID and error class.
- Prompt pipeline throughput drops; downstream generation tasks stall.

**Triage**
1. Identify the failing template ID from the log line.
2. Check whether the template registry has a valid entry: `themis_admin prompt template status <id>`.
3. Verify that all variable bindings referenced by the template are present in the context.
4. Review recent template deployments for malformed YAML or injection-guard violations.

**Remediation**
1. Roll back the template to the last known-good version: `themis_admin prompt template rollback <id>`.
2. Re-validate the template schema: `themis_admin prompt template validate <id>`.
3. If injection guards are triggering false positives, review `prompt_engineering.injection_guard.sensitivity` in the runtime config.
4. Restart the PromptManager if the template cache is corrupted: `themis_admin prompt manager restart`.

**Escalation**
If rollback does not resolve the issue within 10 minutes, escalate to the
prompt_engineering module owner with the full template ID, error class, and
the last 500 lines of the prompt engineering log.

---

## Scenario 2 — Version Conflict

**Log pattern:** `[PROMPT:VersionConflict]`

**Symptoms**
- Concurrent version mutation requests produce conflict errors.
- Log lines contain `[PROMPT:VersionConflict]` with the conflicting version IDs.
- Prompt versioning API returns `409 Conflict` or equivalent error code.

**Triage**
1. Identify the conflicting version IDs from the log.
2. Check the version history: `themis_admin prompt version history <template_id>`.
3. Determine which concurrent writers caused the conflict (check audit log).
4. Verify that the version locking mechanism is functioning correctly.

**Remediation**
1. Resolve the conflict by choosing the canonical version: `themis_admin prompt version resolve <template_id> --keep <version_id>`.
2. Re-enable writes after conflict resolution: `themis_admin prompt version unlock <template_id>`.
3. If conflicts are frequent, reduce write concurrency: set `prompt_engineering.version_control.max_concurrent_writers` to a lower value.
4. Audit recent deployment pipelines to prevent simultaneous version deployments.

**Escalation**
Persistent version conflicts that cannot be resolved by the operator indicate
a distributed coordination failure — escalate to the infrastructure team with
a dump of the version lock state.

---

## Scenario 3 — Optimization Stall

**Log pattern:** `[PROMPT:OptimizationStall]`

**Symptoms**
- The prompt optimization loop stops making progress.
- Log lines contain `[PROMPT:OptimizationStall]` with loop ID and last progress timestamp.
- Optimization metrics show zero improvement for more than the configured stall threshold.

**Triage**
1. Identify the stalled loop ID from the log.
2. Check optimization loop health: `themis_admin prompt optimizer status`.
3. Inspect the evaluation feedback queue for backlog or corruption.
4. Verify that the optimization budget (`max_steps`, `timeout_ms`) is correctly configured.

**Remediation**
1. Reset the stalled optimization loop: `themis_admin prompt optimizer reset <loop_id>`.
2. If the feedback queue is backed up, drain it: `themis_admin prompt feedback drain`.
3. Increase the optimization timeout if workload has grown: set `prompt_engineering.optimizer.timeout_ms`.
4. Restart the optimizer service if the loop cannot be individually reset: `themis_admin prompt optimizer restart`.

**Escalation**
If optimization loops stall repeatedly after restart, there may be a
regression in the optimizer — escalate to the ML platform team with loop ID,
feedback queue size, and the last 200 optimization step logs.

---

## Scenario 4 — Feedback Overflow

**Log pattern:** `[PROMPT:FeedbackOverflow]`

**Symptoms**
- The prompt feedback queue exceeds capacity.
- Log lines contain `[PROMPT:FeedbackOverflow]` with queue depth and drop count.
- Quality evaluation degrades silently as feedback events are dropped.

**Triage**
1. Check current feedback queue depth: `themis_admin prompt feedback status`.
2. Identify the source of the feedback surge (evaluate which pipelines are producing excess feedback).
3. Verify that feedback consumers are running and not stalled.
4. Check for any recent increases in prompt request volume.

**Remediation**
1. Increase the feedback queue capacity: set `prompt_engineering.feedback.queue_capacity`.
2. Scale up feedback consumer threads: set `prompt_engineering.feedback.consumer_threads`.
3. Temporarily throttle feedback producers if volume is unsustainable.
4. Drain the overflow queue: `themis_admin prompt feedback drain --overflow`.

**Escalation**
Sustained feedback overflow indicates a capacity sizing issue — escalate to
the capacity planning team with feedback volume metrics and queue depth
history.

---

## Scenario 5 — Version Rollback Required

**Log pattern:** `[PROMPT:VersionConflict]` + `[PROMPT:TemplateFailed]` (combined)

**Symptoms**
- A recently deployed template version causes widespread render failures.
- Both `[PROMPT:TemplateFailed]` and `[PROMPT:VersionConflict]` appear in the log.
- Error rate spikes across all template render paths.

**Triage**
1. Identify the recently deployed version from the deployment log.
2. Correlate the error spike timestamp with the deployment time.
3. Compare the failing template against the previous stable version.
4. Check for schema-breaking changes in the new version.

**Remediation**
1. Immediately roll back the template to the previous stable version:
   `themis_admin prompt template rollback <id> --to-version <stable_version>`.
2. Verify the rollback resolves the error rate spike within 2 minutes.
3. File a post-incident report documenting the breaking change.
4. Add a pre-deploy validation step for this template class.

**Escalation**
If rollback does not resolve within 5 minutes, escalate to the module owner
with full deployment context, error samples, and a diff of the template
versions.

---

## Reference

| Log Pattern                   | Severity | SLO Impact | Owner              |
|-------------------------------|----------|------------|--------------------|
| `[PROMPT:TemplateFailed]`     | High     | Yes        | prompt_engineering |
| `[PROMPT:VersionConflict]`    | Medium   | Partial    | prompt_engineering |
| `[PROMPT:OptimizationStall]`  | Medium   | No         | prompt_engineering |
| `[PROMPT:FeedbackOverflow]`   | Medium   | No         | prompt_engineering |

---

*Wave D operability deliverable — see `src/prompt_engineering/ROADMAP.md`.*
