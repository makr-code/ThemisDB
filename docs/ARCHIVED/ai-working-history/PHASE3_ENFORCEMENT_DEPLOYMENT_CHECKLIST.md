# Phase 3 Enforcement Deployment - Integration Checklist

**Date:** 2026-08-10  
**Status:** Core Implementation Complete - Awaiting Integration & Testing  
**Target Go-Live:** 2026-10-31 (after 2-week dry-run)

---

## Completed Deliverables

### Block 1: Core Merge Gate Enforcement Bot
- ✅ **Tier 0 Validators** (`.github/scripts/tier0_gate_validator.py` - 470 lines)
  - Governance registry freshness (MATURITY_EVIDENCE_MANIFEST.json ≤24 hours)
  - Module phase gates (no cycle detection in PHASE_DEPENDENCY_GRAPH.md)
  - AI/ML compliance (Model Cards for llm, rag, ethics_ai)
  - Security gates (sanitizer ≤30 days PASS, pentest ≤90 days PASS)
  - GA promotion sign-off (Section 9 of GA_PROMOTION_SIGN_OFF.md)
  - CLI: `python tier0_gate_validator.py --pr <pr_number> --branch develop`

- ✅ **Tier 1 Gate Escalators** (`.github/scripts/tier1_gate_escalator.py` - 380 lines)
  - Doxygen coverage validation (Phase 6 modules ≥95% coverage)
  - BSI C5 compliance gap tracking (no "unassigned" gaps in audit)
  - EU AI Act Model Card coverage (≥75% of AI modules with signed cards)
  - CLI: `python tier1_gate_escalator.py --pr <pr_number> --branch community`

### Block 2: Waiver Management System
- ✅ **Waiver Command Parser** (`.github/scripts/waiver_validator.py` - 410 lines)
  - Regex parser for `/approve-with-waiver <gate_id> "<justification>"`
  - Approver team membership validation (stub: currently returns True)
  - Waiver expiration calculation (14 days from issuance)
  - Append-only logging to `ai_working/ENFORCEMENT_WAIVERS.md`
  - CLI: `python waiver_validator.py --pr <pr> --comment-text "..."` → JSON output

- ✅ **Waiver Tracking** (`ai_working/ENFORCEMENT_WAIVERS.md`)
  - Append-only markdown table schema (PR, Gate, Approver, Justification, Issued, Expires, Status)
  - Workflow procedures documented
  - Escalation policy documented (waivers create tracked issues)

### Block 3: Audit Logging & Monitoring
- ✅ **Audit Log Schema** (`ai_working/MERGE_GATE_AUDIT_LOG.md`)
  - JSONL format specification (one JSON per line, immutable)
  - Schema fields: timestamp, pr_number, branches, merged_by, gate_results dict, waivers_applied array, duration_seconds
  - Example queries provided (jq filters for common analysis patterns)

- ✅ **Live Dashboard** (`docs/governance/MERGE_GATE_STATUS_LIVE.md`)
  - Template structure for current PRs under validation
  - Active waivers with expiration countdown
  - Recent escalations with assignments
  - Gate health status (PASS/FAIL counts per gate)
  - Updated by workflows on schedule (every 5 minutes recommended)

- ✅ **Auto-Escalation Issues** (Created by `12-governance_merge-gate-enforcer.yml`)
  - Label: `[severity] merge-gate-failure` for Tier 0 blocks
  - Label: `[security] gate-stale` for stale security evidence
  - Label: `[phase-regression]` for phase dependency violations
  - Auto-closed after remediation and gate re-validation

### Block 4: Validation Workflows
- ✅ **Merge Gate Enforcer Workflow** (`.github/workflows/12-governance_merge-gate-enforcer.yml`)
  - Triggers: `pull_request` (opened, synchronize) on develop, community, military, hyperscaler
  - Jobs: tier0-validation (required), tier1-validation (conditional), waiver-processor (on comment), block-merge (final check)
  - Outputs: Structured PR comment with gate matrix (Pass/Fail/Waivered)
  - Check run status: NEUTRAL (pass), FAILURE (Tier 0 fail), REQUESTED_CHANGES (Tier 1 fail)
  - Timeout: 15-minute limit on gate validation

- ✅ **Waiver Expiration Checker** (`.github/workflows/12-governance_waiver-expiration-check.yml`)
  - Trigger: Daily schedule at 09:00 UTC
  - Scans `ai_working/ENFORCEMENT_WAIVERS.md` for entries expiring within 3 days
  - Posts reminder comment on affected PRs
  - Updates dashboard with expiration countdown
  - Auto-closes expired tracking issues

- ✅ **Audit Summary Reporter** (`.github/workflows/12-governance_gate-audit-summary.yml`)
  - Trigger: Weekly Monday 08:00 UTC
  - Analyzes `MERGE_GATE_AUDIT_LOG.jsonl` for 7-day window
  - Generates summary: most common failures, waiver trends, approval SLAs
  - Updates live dashboard with weekly metrics
  - Posts summary to GitHub Discussion (if configured)

### Block 5: Validation Scripts Enhancement
- ✅ **Wave 7 Gates** (`benchmark_gate_validator.py` - added methods)
  - GATE-W7-01: MATURITY_EVIDENCE_MANIFEST.json age ≤7 days
  - CLI: `python benchmark_gate_validator.py --wave7`
  - Output: JSON with gate status, threshold, current values

- ✅ **Wave 8 Gates** (`benchmark_gate_validator.py` - added methods)
  - GATE-W8-01: Sanitizer evidence age ≤30 days
  - GATE-W8-02: Pentest evidence age ≤90 days
  - CLI: `python benchmark_gate_validator.py --wave8`
  - Output: JSON with gate status, threshold, current values

### Block 6: Operations & Documentation
- ✅ **Phase 3 Enforcement Runbook** (`docs/governance/PHASE3_ENFORCEMENT_RUNBOOK.md` - 12,700 lines)
  - Quick Start guide (basic workflow for PRs)
  - Workflow execution flow diagram
  - Gate details and remediation procedures
  - Troubleshooting guide (false positives, performance, timeouts)
  - Maintenance checklist (daily/weekly/monthly/quarterly tasks)
  - Policy update procedures
  - Bot health check commands

- ✅ **Updated ROADMAP.md** (`ROADMAP.md`)
  - Added Batch E — Phase 3 Enforcement Deployment entry
  - Marked core implementation complete
  - Flagged remaining external dependencies

---

## Remaining Work (External + Integration)

### Block 1: GitHub App Registration ⚠️ EXTERNAL DEPENDENCY
**Timeline:** 1-2 days (requires GitHub admin access)

**Steps:**
1. Create GitHub App "ThemisDB-Merge-Gate-Enforcer" at: https://github.com/organizations/makr-code/settings/apps/new
2. **Permissions Required:**
   - `pull_requests:write` (leave reviews, update checks)
   - `issues:write` (create escalation issues)
   - `checks:write` (create check runs)
   - `workflows:read` (read workflow status)
   - `contents:read` (read manifest files)

3. **Webhooks Configuration:**
   - URL: `https://api.github.com/repos/makr-code/ThemisDB/dispatches` (or custom webhook endpoint)
   - Events: `pull_request`, `issue_comment`, `pull_request_review`
   - Active: ✅

4. **Store Credentials:**
   - Add to repository secrets:
     - `MERGE_GATE_APP_ID` = App ID
     - `MERGE_GATE_PRIVATE_KEY` = Private key (PEM format, base64 encoded)
     - `MERGE_GATE_WEBHOOK_SECRET` = Webhook secret

5. **Reference in Workflows:**
   - Use `actions/github-script@v7` with `core.getInput('token')` from App token

**Current Placeholder:**
- Scripts use environment variables; workflows reference `secrets.MERGE_GATE_APP_ID` (to be configured)

---

### Block 2: GitHub Team Setup
**Timeline:** 1-2 hours (manual team creation + member assignment)

**Steps:**
1. Create GitHub team `@themisdb/release-leads` at: https://github.com/orgs/makr-code/teams
2. Add initial members:
   - @makr-code (owner)
   - Other release leads (TBD: add by PR/issue discussion)
3. Document team membership requirements in `docs/governance/RELEASE_GOVERNANCE.md` (create if needed)
4. Set waiver_validator.py approver check to validate team membership via GitHub API:
   ```python
   # Pseudocode in waiver_validator.py
   api_url = f"https://api.github.com/orgs/makr-code/teams/release-leads/memberships/{approver}"
   response = requests.get(api_url, headers={"Authorization": f"token {GITHUB_TOKEN}"})
   is_team_member = response.status_code == 200
   ```

**Current Placeholder:**
- Script returns `True` for all approvers (non-blocking for testing)

---

### Block 3: Dry-Run Phase Configuration
**Timeline:** 2 weeks (2026-08-10 → 2026-08-24)

**Steps:**
1. Set `MERGE_GATE_DRY_RUN_MODE=true` in repository variables
   - Workflows create comments only, no merge block
   - PR check status remains NEUTRAL (informational)

2. Monitor for false positives:
   - Track gate failures that should have passed
   - Identify timeouts or performance issues
   - Collect feedback from team

3. Validation Criteria for Hard Activation:
   - Zero false positives (or documented exceptions)
   - Average gate validation time ≤15 minutes
   - Waiver approval turnaround ≤48 hours
   - Dashboard auto-update success rate ≥99%

4. Activate Hard Block:
   - Set `MERGE_GATE_DRY_RUN_MODE=false` in repository variables
   - PR check status → FAILURE on Tier 0 fail
   - Merge button disabled until gates pass or waiver approved

**Current Placeholder:**
- Scripts/workflows ready for dry-run; set variables to switch modes

---

### Block 4: Testing & Validation
**Timeline:** 3-4 days (comprehensive end-to-end testing)

**Test Cases:**
1. **Tier 0 Gate Validation:**
   - [  ] Create test PR that PASSES all Tier 0 gates
   - [  ] Create test PR with stale governance registry (>24 hours) → FAIL
   - [  ] Create test PR with missing Model Card → FAIL
   - [  ] Create test PR with failed security gate → FAIL
   - [  ] Verify gate results appear in PR comment

2. **Tier 1 Gate Escalation:**
   - [  ] Create test PR to `community` branch with low Doxygen coverage → Tier 1 FAIL
   - [  ] Test BSI C5 compliance gap detection → Tier 1 FAIL
   - [  ] Test AI Model Card coverage < 75% → Tier 1 FAIL

3. **Waiver Workflow:**
   - [  ] Post `/approve-with-waiver GATE-T1-01 "Critical release hotfix"` comment
   - [  ] Verify waiver is validated and logged to ENFORCEMENT_WAIVERS.md
   - [  ] Verify PR comment confirms waiver approval
   - [  ] Verify expiration issue is created (hidden)
   - [  ] Test waiver expiration reminder (3 days before)
   - [  ] Test waiver renewal workflow

4. **Audit Logging:**
   - [  ] Verify MERGE_GATE_AUDIT_LOG.jsonl entries are created
   - [  ] Parse entries with jq; verify schema correctness
   - [  ] Test 7-day summary query
   - [  ] Verify live dashboard is updated

5. **Performance & Reliability:**
   - [  ] Measure average gate validation time (target: ≤15 minutes)
   - [  ] Test with multiple simultaneous PRs (simulate parallel validation)
   - [  ] Test with network failures; verify graceful degradation
   - [  ] Test workflow timeout handling

---

### Block 5: Team Training & Communication
**Timeline:** 3-5 days (once testing complete)

**Steps:**
1. Create training documentation:
   - Quick-start guide for PR submitters (what gates are checked, what to fix)
   - Guide for release leads (how to approve waivers, when to escalate)
   - Troubleshooting for gate failures (common issues + fixes)

2. Schedule training sessions:
   - Release leads: waiver approval workflow & policy (1 hour)
   - All contributors: merge gate overview & common failures (30 minutes)
   - On-call/ops: runbook walkthrough & monitoring (1 hour)

3. Post to GitHub Discussions:
   - Announcement of Phase 3 Enforcement activation
   - Link to runbook & quick-start guides
   - Support contact (GitHub issue or Discussions thread)

4. Update QUICKSTART.md:
   - Add section on merge gates
   - Link to PHASE3_ENFORCEMENT_RUNBOOK.md

---

## Critical Path Timeline

```
2026-08-10: ✅ Core implementation delivered (this checkpoint)
2026-08-11: GitHub App registration (external)
2026-08-12: GitHub team setup + waiver validator API integration
2026-08-13: Dry-run mode activation; team review
2026-08-14–2026-08-24: 2-week dry-run phase (informational mode)
2026-08-25: Validation & hard block activation decision
2026-08-26–2026-08-29: Testing & integration fixes (if needed)
2026-08-30: Team training & communication launch
2026-09-01: Hard enforcement activation (merge block enabled)
```

**Go-Live:** 2026-09-01 (hard block enforcement)  
**Fallback:** If false positives detected, extend dry-run or roll back to informational mode

---

## Sign-Off Criteria

**Technical Readiness:**
- [  ] All scripts pass Python syntax validation
- [  ] All workflows pass GitHub Actions validation
- [  ] Gate validation time ≤15 minutes average
- [  ] Waiver expiration reminders working correctly
- [  ] Audit log entries correctly formatted

**Operational Readiness:**
- [  ] GitHub App credentials securely stored
- [  ] Team membership validation working
- [  ] Dry-run phase completed with zero false positives
- [  ] Runbook reviewed and approved by ops team
- [  ] Team training completed

**Governance Readiness:**
- [  ] Release policy aligned with gate configuration
- [  ] Waiver escalation policy approved
- [  ] Audit log retention policy defined (archive schedule)
- [  ] Regular audit review schedule established

---

## Success Metrics (Post-Launch)

- ✅ 100% of PRs to release lanes validated against Tier 0 gates
- ✅ Zero unauthorized merges (all merges either PASS Tier 0 or have valid Tier 1 waiver)
- ✅ Average gate validation time ≤15 minutes
- ✅ False positive rate ≤2%
- ✅ Release lead waiver approval turnaround ≤48 hours
- ✅ No critical security issues in merged code due to stale security gates

---

## Revision History

| Date | Status | Notes |
|------|--------|-------|
| 2026-08-10 | Core Implementation Complete | Blocks 1-4 delivered; Blocks 5-6 in progress; external dependencies flagged |
| TBD | Integration Testing | Dry-run phase results & feedback |
| TBD | Go-Live | Hard enforcement activation |
