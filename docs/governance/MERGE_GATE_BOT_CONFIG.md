# Merge Gate Bot Configuration

**Document:** GitHub App & Automated Merge Gate Enforcement  
**Status:** Phase 3 Enforcement (2026-Q4)  
**Version:** 1.0  
**Owner:** Platform Release / @makr-code  
**Last Updated:** 2026-08-10

---

## Purpose

This document defines the configuration for the automated merge gate enforcement bot that validates all Tier 0/Tier 1 gates before allowing merges to release lanes (`develop`, `community`, `military`).

**Bot Behavior:** *Automatic gate validation, structured reporting, and human-decision points for waivers.*

---

## Bot Configuration

### GitHub App Permissions

```yaml
app_name: "ThemisDB-Merge-Gate-Enforcer"
description: "Automated validation of maturity gates before release-lane merges"

permissions:
  pull_requests: write      # Post comments, status checks
  contents: read            # Read manifest files
  issues: write             # Create/update gate failures as issues
  checks: write             # Create check runs with status
  workflows: read           # Inspect CI status
  metadata: read            # Repository metadata

events:
  pull_request:
    types:
      - opened
      - synchronize          # New commits to PR
      - labeled              # Manual "ready-for-merge" label
      - unlabeled
  pull_request_review:
      types:
        - submitted          # Review approval for Tier 1 waivers
  issue_comment:
      types:
        - created           # `/approve-with-waiver` commands
```

---

## Gate Validation Logic

### Trigger Conditions

Bot runs on:

1. **PR to `develop` branch**
   - Runs Tier 0 checks on every commit
   - Required for merge to release lanes

2. **PR to `community`, `military`, or `hyperscaler` branches**
   - Runs full Tier 0 + Tier 1 checks
   - Blocks merge if Tier 0 fails
   - Requires waiver for Tier 1 failures

3. **Manual trigger**: Label PR with `[automation] merge-gate-check`
   - Runs validation immediately
   - Useful for re-validating after manual fixes

---

## Validation Sequence

### Step 1: Governance Registry Check (5 min)

```
Task: Verify Evidence Registry is current
File: docs/governance/MATURITY_EVIDENCE_MANIFEST.json
Check: Last updated ≤ 24 hours ago
Action: 
  - If stale: POST comment → "Evidence Registry stale; re-run verification workflow"
  - If fresh: CONTINUE
```

### Step 2: Module Phase Gates (10 min)

```
Task: Verify all modules affected by PR have phase gates passing
File: docs/governance/PHASE_DEPENDENCY_GRAPH.md
Check: For each module modified in PR:
  1. Read current phase from src/<module>/ROADMAP.md
  2. Verify phase ≤ 6 (max phase)
  3. Verify prerequisites in dependency graph are met
  4. If phase advancement in PR (e.g., Phase 5 → 6):
     - Check acceptance criteria match
     - Check test ratio ≥ 80%
     - Wait for domain owner approval comment
Action:
  - If phase advancement incomplete: POST checklist comment
  - If prerequisites not met: POST blocker list with blocking modules
  - If all checks pass: CONTINUE
```

### Step 3: AI/ML Compliance Gate (5 min) [Only if PR touches `src/llm/`, `src/rag/`, `src/ethics_ai/`]

```
Task: Verify EU AI Act compliance for AI modules
File: docs/governance/MATURITY_EVIDENCE_MANIFEST.json
Check: 
  - For each AI module in PR:
    - Has Model Card file in docs/governance/ai-model-cards/<module>.md?
    - Model Card has required sections: purpose, data, bias, metrics, limitations?
    - Model Card has human signature/approval timestamp?
Action:
  - If missing Model Card: POST "EU AI Act: Model Card required before merge"
  - If Model Card incomplete: POST specific missing sections
  - If all checks pass: CONTINUE
```

### Step 4: Security Gate (10 min) [Only if PR modifies `src/security/`, `src/auth/`, or C++ code]

```
Task: Verify latest security verification is recent
Files:
  - docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md
  - security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md
Check:
  - ASan/UBSan/TSan: Last updated ≤ 30 days
  - Pentest: Last updated ≤ 90 days
  - Both show PASS status
Action:
  - If stale: POST "Security gates stale; run sanitizer/pentest before merge"
  - If old findings: POST "New sanitizer findings in modified code"
  - If all checks pass: CONTINUE
```

### Step 5: Tier 0 Status Check

```
If ANY Tier 0 gate failed:
  - POST structured failure report:
    ❌ TIER 0 GATES FAILED — Merge blocked
    
    | Gate | Status | Action Required |
    |------|--------|-----------------|
    | Module Phases | FAIL | <specific module/phase issue> |
    | AI Compliance | PASS | - |
    | Security | FAIL | Re-run sanitizer; 30 days stale |
    
    Cannot proceed without resolving all Tier 0 gates.
  
  - Create GitHub check: "Merge Gate Validation" → FAILURE
  - Block merge
  - Auto-assign release lead for oversight
```

### Step 6: Tier 1 Status Check [Only if branch = `community`/`military`]

```
If ANY Tier 1 gate failed:
  - POST escalation request:
    ⚠️ TIER 1 GATES REQUIRE ESCALATION
    
    | Gate | Status | Severity |
    |------|--------|----------|
    | Doxygen < 95% | FAIL | Phase 6 module: process |
    | BSI Gaps | FAIL | 2 open compliance gaps |
    | Model Cards < 75% | PASS | - |
    
    Release Lead Review Required (72 hours):
    - Review justification for each failure
    - Comment `/approve-with-waiver <gate_id> <justification>`
    - OR request fixes before merge
  
  - Create GitHub check: "Merge Gate Validation" → REQUESTED_CHANGES
  - Do NOT block merge (allow release lead decision)
  - Auto-assign release lead for review
```

### Step 7: Audit Logging

```
For every PR merge:
  Log to ai_working/MERGE_GATE_AUDIT_LOG.json:
  {
    "timestamp": "2026-08-15T10:30:00Z",
    "pr_number": 12345,
    "source_branch": "develop",
    "target_branch": "community",
    "merged_by": "github-user",
    "gate_results": {
      "governance_registry": "PASS",
      "module_phases": "PASS",
      "ai_compliance": "PASS",
      "security": "PASS",
      "tier_0_status": "PASS",
      "tier_1_status": "PASS"
    },
    "waivers_applied": [],
    "automation_version": "Phase3"
  }
```

---

## Waiver Workflow (`/approve-with-waiver` Command)

### Trigger

Release lead posts comment on PR:

```
/approve-with-waiver GATE-DOXYGEN-P6 "Doxygen gap in new process module; 10 tests added; will complete by next sprint"
```

### Bot Actions

1. **Validate Approver**
   - Check: Is commenter a Release Lead (GitHub team: `@themisdb/release-leads`)?
   - If not: POST "Only release leads can approve waivers"
   - If yes: CONTINUE

2. **Parse Command**
   - Extract: `gate_id` = "GATE-DOXYGEN-P6"
   - Extract: `justification` = "Doxygen gap..."
   - Validate gate_id exists in RELEASE_PROMOTION_GATE_POLICY.md
   - If invalid: POST "Unknown gate ID; check policy documentation"

3. **Log Waiver**
   - Append to `ai_working/ENFORCEMENT_WAIVERS.md`:
     ```markdown
     | PR | Gate | Approver | Justification | Issue Date | Expires | Status |
     | #12345 | GATE-DOXYGEN-P6 | @release-lead | "Doxygen gap in new process module; 10 tests added; will complete by next sprint" | 2026-08-15 | 2026-08-29 | ACTIVE |
     ```
   - Commit this change to PR branch (append-only)

4. **Unblock Merge**
   - Update GitHub check: "Merge Gate Validation" → APPROVED
   - POST comment: "✅ Waiver approved. Merge blocked until: 2026-08-29"
   - Allow merge to proceed

5. **Set Waiver Expiration**
   - Create GitHub issue (hidden from normal search):
     ```
     [waiver-expiration] PR #12345 GATE-DOXYGEN-P6 expires 2026-08-29
     Scheduled removal date: 2026-08-29
     ```
   - Auto-close after 14 days; if not renewed, issue posts warning
   - Prevent silent waiver extensions

---

## Automatic Escalation Rules

### When to Auto-Create Issue

Bot automatically creates GitHub issue if:

1. **Tier 0 gate fails on `community` merge attempt**
   ```
   [severity] merge-gate-failure — <gate_id> on PR #<N>
   
   Tier 0 gate <gate_id> failed on PR merge to <branch>.
   Merge blocked. Release lead must investigate.
   
   Affected files: <list>
   Error: <gate-specific error message>
   ```

2. **Security gate stale (sanitizer/pentest > 30 days)**
   ```
   [security] gate-stale — sanitizer results > 30 days
   
   Latest ASan/UBSan/TSan results are stale.
   Schedule re-run before accepting new code.
   ```

3. **Phase regression detected during merge**
   ```
   [phase-regression] <module> Phase <N> — merge would cause regression
   
   Merge of PR #<N> detected phase regression in <module>:
   - Test count drop: <before> → <after>
   
   Merge blocked until regression resolved.
   ```

---

## Dashboard & Monitoring

### Real-Time Status

Bot maintains live Markdown file: `docs/governance/MERGE_GATE_STATUS_LIVE.md`

**Updated:** Every 5 minutes

**Contents:**
```markdown
# Merge Gate Status Dashboard (Live)

Last Updated: 2026-08-15T14:30:00Z

## Current Merge Requests Under Gate Validation

| PR # | Source | Target | Modules | Tier 0 | Tier 1 | Status |
|------|--------|--------|---------|--------|--------|--------|
| #12345 | develop | community | process, auth | ✅ | ⏳ Waiver | Pending |
| #12346 | develop | community | security | ✅ | ✅ | Ready |
| #12347 | develop | develop | query | ✅ | N/A | Ready |

## Active Waivers

| PR | Gate | Expires | Days Left |
|----|------|---------|-----------|
| #12345 | GATE-DOXYGEN-P6 | 2026-08-29 | 14 |

## Recent Escalations

- 2026-08-15 13:00: [security] Sanitizer results stale (38 days) - Assigned to @security-lead
- 2026-08-14 09:30: [phase-regression] Storage Phase 5 — test regression - Resolved by #12340

## Weekly Summary

**PRs Merged:** 5  
**Gates PASSED:** 22  
**Gates FAILED:** 1 (escalated)  
**Waivers Issued:** 1  
**Average Gate Time:** 12 minutes
```

---

## Runbook for Bot Operations

### Daily Checks

1. **Verify bot health**
   ```bash
   curl https://api.github.com/app/installations -H "Authorization: token $BOT_TOKEN"
   ```
   Ensure bot is authenticated and active

2. **Review yesterday's escalations**
   - Check GitHub issues created with `[severity] merge-gate-failure`
   - Verify release leads have reviewed

3. **Check waiver expirations**
   - Query `ENFORCEMENT_WAIVERS.md` for expiring waivers (within 3 days)
   - Post reminder comment on PR

### Weekly Reviews (Every Monday)

1. **Audit recent merges**
   - Review `MERGE_GATE_AUDIT_LOG.json` entries from past 7 days
   - Identify patterns: most common failures, waiver trends
   - Post summary to release team Slack

2. **Validate gate thresholds**
   - Are gate freshness requirements (7 days, 30 days, 90 days) appropriate?
   - Any gates consistently stale? Consider automation schedule adjustment
   - Any Tier 1 waivers trending toward Tier 0? Consider policy adjustment

### Quarterly Reviews (End of Q)

1. **Comprehensive policy review**
   - Review RELEASE_PROMOTION_GATE_POLICY.md against actual merge patterns
   - Identify gates that are too strict or too lenient
   - Propose policy adjustments for next quarter

2. **Waiver analysis**
   - Report: Which gates are most frequently waived?
   - Recommendation: Should frequent waivers move to Tier 2?
   - Decision: Update policy or strengthen gate automation

---

## Integration with GitHub Actions

### Workflows Triggered by Bot

When bot detects gate failures:

1. **`.github/workflows/gate_failure_notification.yml`**
   - Posts detailed Slack message to `#themisdb-release`
   - Includes remediation steps
   - Escalates to release lead (on-call)

2. **`.github/workflows/waiver_expiration_check.yml`**
   - Runs daily at 09:00 UTC
   - Checks `ENFORCEMENT_WAIVERS.md` for expiring entries
   - Posts reminder to PR 3 days before expiration

3. **`.github/workflows/gate_audit_summary.yml`**
   - Runs weekly (Monday 08:00 UTC)
   - Generates summary report
   - Updates live dashboard

---

## References

- `RELEASE_PROMOTION_GATE_POLICY.md` — Gate definitions and tier hierarchy
- `PHASE_CLOSURE_POLICY.md` — Phase advancement prerequisites
- `MATURITY_EVIDENCE_MANIFEST.json` — Gate artifact registry
- `.github/workflows/10-governance_maturity-verification.yml` — Gate verification
- `.github/workflows/12-governance_rc-validation.yml` — RC validation pipeline

