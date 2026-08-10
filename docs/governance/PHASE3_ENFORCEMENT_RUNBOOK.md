# Phase 3 Enforcement Deployment - Integration & Operations Guide

**Document:** Phase 3 Merge Gate Enforcer - Operations Runbook  
**Status:** Active (2026-Q4)  
**Owner:** Platform Release / @makr-code  
**Last Updated:** 2026-08-10  

---

## Quick Start

### For PR Authors (5 minutes)

1. **Submit PR to `develop` or `community`**
   - Merge gate enforcer automatically runs Tier 0 validation
   - Check PR comment for gate status (green ✅ or red ❌)

2. **If Tier 0 fails:**
   - Read remediation steps in PR comment
   - Fix underlying issue
   - Push new commit (gates re-run automatically)

3. **If PR targets `community`/`military` and Tier 1 fails:**
   - Release lead can approve with waiver
   - See Release Lead section below

### For Release Leads (10 minutes)

1. **Review PR gates before merging**
   - Tier 0: Must PASS (required for all merges)
   - Tier 1: Must PASS or have waiver (community/military only)

2. **Approve waiver (if needed):**
   ```bash
   /approve-with-waiver T1-DOXYGEN-COVERAGE "Process module Phase 6; 15 tests added; will complete by next sprint"
   ```
   - Waiver expires in 14 days
   - Auto-expiration reminder posted 3 days before
   - Can be renewed before expiration

3. **Merge PR:**
   - Merge button enabled once all Tier 0 gates pass
   - Tier 1 gates must pass OR have valid waiver
   - Audit log automatically captures merge event

### For Operations (Daily/Weekly)

1. **Daily (09:00 UTC - automated):**
   - Check expiring waivers (less than 3 days)
   - Post reminder comments on affected PRs
   - Update live dashboard

2. **Weekly (Monday 08:00 UTC - automated):**
   - Generate audit summary report
   - Identify gate failure trends
   - Post summary to release team
   - Update live dashboard

3. **Monthly Review (manual):**
   - Analyze waiver trends: any gate with 3+ waivers/month should move to Tier 0
   - Check gate freshness thresholds (7d, 30d, 90d)
   - Review false positives and adjust rules

---

## Workflow Execution Flow

```
┌─────────────────────────────────────────────────────┐
│ PR opened/synchronized to develop/community/military │
└──────────────────────────┬──────────────────────────┘
                           │
                           v
        ┌──────────────────────────────────┐
        │ 12-governance_merge-gate-enforcer │
        └──────────────────┬───────────────┘
                           │
            ┌──────────────┴───────────────┐
            v                              v
    ┌──────────────┐            ┌─────────────────┐
    │ Tier 0 Check │            │ Tier 1 Check*   │
    │ (required)   │            │ (if community)  │
    └──────┬───────┘            └────────┬────────┘
           │                             │
    ┌──────v──────┐               ┌──────v──────┐
    │ ✅ PASS     │               │ ✅ PASS     │
    │ or          │               │ or          │
    │ ❌ FAIL     │               │ ⚠️  WAIVER  │
    │             │               │ REQUIRED    │
    └─────┬───────┘               └──────┬──────┘
          │                              │
    ┌─────v──────────────────────────────v─────┐
    │ Post PR comment with results & remediation│
    │ Update check run with PASS/FAILURE status │
    └────────────────────┬────────────────────┘
                         │
         ┌───────────────┴────────────────┐
         │                                │
    ┌────v─────────────┐    ┌────────────v────────┐
    │ Block merge if   │    │ Allow waiver via    │
    │ Tier 0 fails     │    │ /approve-with-waiver│
    │                  │    │ if Tier 1 fails     │
    └─────────────────┘    └──────────┬──────────┘
                                      │
                          ┌───────────v──────────────┐
                          │ Release lead posts       │
                          │ /approve-with-waiver ... │
                          └───────────┬──────────────┘
                                      │
                    ┌─────────────────┴─────────────────┐
                    v                                   v
        ┌──────────────────┐            ┌──────────────────────┐
        │ ✅ Waiver valid  │            │ ❌ Waiver rejected   │
        │ Merge allowed    │            │ (invalid format, etc)│
        │ Audit logged     │            │ Re-check comment     │
        └──────────────────┘            └──────────────────────┘
```

---

## Gate Details

### Tier 0 Gates (Hard Block)

| Gate | Validation | Freshness | Block On | Remediation |
|------|-----------|-----------|----------|-------------|
| `T0-GOVERNANCE-REGISTRY` | MATURITY_EVIDENCE_MANIFEST.json exists & current | ≤24h | Missing/Stale | Run `.github/workflows/10-governance_maturity-verification.yml` |
| `T0-MODULE-PHASES` | PHASE_DEPENDENCY_GRAPH.md has no cycles | ≤7d | Cycles detected | Resolve phase dependencies in module ROADMAP.md |
| `T0-AI-COMPLIANCE` | Model Cards exist for llm, rag, ethics_ai | N/A | Missing sections | Create/update Model Cards with all required sections |
| `T0-SECURITY-SANITIZER` | GA_SANITIZER_EVIDENCE_BUNDLE.md shows PASS | ≤30d | Stale/FAIL | Re-run ASan/UBSan/TSan and update evidence |
| `T0-SECURITY-PENTEST` | GA_PENTEST_EVIDENCE_BUNDLE.md shows PASS | ≤90d | Stale/FAIL | Re-run penetration test and update evidence |
| `T0-GA-SIGNOFF` | GA_PROMOTION_SIGN_OFF.md §9 has required sign-offs | N/A | Missing sign-off | Collect sign-offs from Security, Operations, Release leads |

### Tier 1 Gates (Waiverable)

| Gate | Validation | Threshold | Waiver Window | Escalation |
|------|-----------|-----------|---------------|------------|
| `T1-DOXYGEN-COVERAGE` | Doxygen coverage for Phase 6 modules | ≥95% | 14 days | 3+ waivers/4 weeks → Tier 0 |
| `T1-BSI-C5-COMPLIANCE` | BSI C5 compliance gaps all assigned | 0 unassigned | No waiver (community only fails) | N/A |
| `T1-AI-MODEL-CARDS` | Signed Model Cards for AI modules | ≥75% | 14 days | 3+ waivers/4 weeks → Tier 0 |

---

## Monitoring & Dashboards

### Live Dashboard

Location: `docs/governance/MERGE_GATE_STATUS_LIVE.md`

Auto-updated by:
- `12-governance_merge-gate-enforcer.yml` (on each PR)
- `12-governance_waiver-expiration-check.yml` (daily)
- `12-governance_gate-audit-summary.yml` (weekly)

### Audit Logs

**Waiver Log (append-only):**
- Location: `ai_working/ENFORCEMENT_WAIVERS.md`
- Format: Markdown table
- Schema: PR, Gate, Approver, Justification, Issue Date, Expires, Status

**Gate Validation Log (JSONL):**
- Location: `ai_working/MERGE_GATE_AUDIT_LOG.md` (schema)
- Format: One JSON object per line
- Query: See examples in file

**Weekly Metrics:**
- Location: `ai_working/GATE_METRICS_WEEKLY.json`
- Generated: Every Monday 08:00 UTC
- Contents: Pass rate, failure trends, waiver counts

---

## Troubleshooting

### "Gate Validation Failed" - How to Fix

1. **Read PR comment carefully**
   - Exact gate that failed
   - Why it failed (details)
   - How to fix (remediation)

2. **Common Fixes:**

   **T0-GOVERNANCE-REGISTRY stale:**
   ```bash
   # Trigger verification workflow
   gh workflow run 10-governance_maturity-verification.yml
   ```

   **T0-MODULE-PHASES cycle detected:**
   - Review `docs/governance/PHASE_DEPENDENCY_GRAPH.md`
   - Update phase dependencies in affected module ROADMAP.md files

   **T0-SECURITY-SANITIZER stale (> 30 days):**
   ```bash
   # Run sanitizer locally or on CI
   ./build-sanitizer.sh && ./run-tests-with-sanitizer.sh
   ```

   **T1-DOXYGEN-COVERAGE < 95%:**
   - Add Doxygen comments to public APIs in Phase 6 modules
   - Run: `doxygen Doxyfile.audit` to verify

3. **Still stuck?**
   - Create GitHub issue: `[governance] Gate validation failure - <gate_id>`
   - Include: Gate ID, PR #, error details, attempted fixes
   - Assign to domain lead for module

### "Waiver Command Rejected"

**Error: "Invalid GitHub username"**
- Check comment author is actually a member of `@themisdb/release-leads` team
- Contact team admin to add approver

**Error: "Unknown gate ID"**
- Gate ID must be from Tier 1 list: T1-DOXYGEN-COVERAGE, T1-BSI-C5-COMPLIANCE, T1-AI-MODEL-CARDS
- Check `RELEASE_PROMOTION_GATE_POLICY.md` for valid IDs

**Error: "Waiver command not found"**
- Check comment format: `/approve-with-waiver <GATE_ID> "justification"`
- Quotes around justification are required

### "Waiver Expired" - How to Renew

1. Release lead posts new waiver comment:
   ```bash
   /approve-with-waiver <GATE_ID> "still working on fix, ETA next sprint"
   ```
2. New 14-day expiration starts from today
3. Previous waiver entry marked EXPIRED

### No Active PR Merges - Debug Checklist

```bash
# 1. Check gate validations are running
gh run list --workflow 12-governance_merge-gate-enforcer.yml --limit 5

# 2. Check live dashboard
cat docs/governance/MERGE_GATE_STATUS_LIVE.md

# 3. List recent waivers
cat ai_working/ENFORCEMENT_WAIVERS.md

# 4. Check audit log (last 10 lines)
tail -10 ai_working/MERGE_GATE_AUDIT_LOG.md

# 5. List recent PR validations
jq '.' ai_working/MERGE_GATE_AUDIT_LOG.jsonl | tail -5

# 6. Check for recent failures
jq 'select(.gate_results.tier0_status == "FAIL")' ai_working/MERGE_GATE_AUDIT_LOG.jsonl
```

---

## Policy Changes

### How to Update Gate Definitions

To add/modify a gate:

1. **Update policy document:**
   - Edit `docs/governance/RELEASE_PROMOTION_GATE_POLICY.md`
   - Update gate table with new requirements
   - Version the change: update "Last Updated" field

2. **Update validators:**
   - Modify `.github/scripts/tier0_gate_validator.py` or `tier1_gate_escalator.py`
   - Add new gate check in appropriate `_validate_*()` method
   - Test: `python script.py --all-gates`

3. **Update gate list in waiver validator:**
   - Edit `.github/scripts/waiver_validator.py`
   - Add new gate ID to `VALID_GATE_IDS` list

4. **Update documentation:**
   - Update this runbook with new gate details
   - Update live dashboard template

5. **PR & Review:**
   - Create PR with all changes together
   - Must be reviewed by release lead and security lead
   - Merge to develop; will be included in next release validation

### How to Escalate Tier 1 → Tier 0

If a gate receives 3+ waivers within 4 weeks:

1. **Create GitHub issue:**
   ```
   Title: [governance] <GATE_ID> - escalate to Tier 0
   Labels: governance, merge-gate
   Assign: Domain lead for gate
   ```

2. **Update policy:**
   - Move gate from Tier 1 to Tier 0 in RELEASE_PROMOTION_GATE_POLICY.md
   - Document reason for escalation

3. **Update scripts:**
   - Move validation from `tier1_gate_escalator.py` to `tier0_gate_validator.py`
   - Update `waiver_validator.py` VALID_GATE_IDS

4. **Notify team:**
   - Post announcement in release team chat/discussion
   - Explain why escalated and what needs to be fixed

---

## Performance & Optimization

### Gate Validation Time Budget

- **Target:** < 15 minutes total per PR validation
- **Tier 0 checks:** < 5 minutes
- **Tier 1 checks:** < 3 minutes
- **Waiver processing:** < 2 minutes

If exceeding budget:
- Profile slow validators: `time python tier0_gate_validator.py --all-gates`
- Consider parallelizing checks
- Cache results between runs (24h max)

### Scaling for High PR Volume

If gates run on every PR (e.g., GitHub CI):
- Use workflow concurrency limits: `concurrency: merge-gates-${{ github.base_ref }}`
- Cache governance registry: `actions/cache@v3`
- Batch similar gates: combine phase + dependency checks

---

## Maintenance Checklist

### Daily (automated)
- [ ] 09:00 UTC: Waiver expiration check (`.github/workflows/12-governance_waiver-expiration-check.yml`)

### Weekly (automated)
- [ ] Monday 08:00 UTC: Audit summary generation (`.github/workflows/12-governance_gate-audit-summary.yml`)

### Monthly (manual)
- [ ] Review gate freshness thresholds (7d, 30d, 90d)
- [ ] Analyze waiver trends: any gate trending toward escalation?
- [ ] Update live dashboard template if needed
- [ ] Review false positives: any gates blocking incorrectly?

### Quarterly (manual)
- [ ] Policy review: `docs/governance/RELEASE_PROMOTION_GATE_POLICY.md`
- [ ] Validator review: `.github/scripts/tier*_gate_*.py`
- [ ] Adjust gate thresholds based on trends
- [ ] Update runbook with lessons learned

---

## References

- **Gate Policy:** `docs/governance/RELEASE_PROMOTION_GATE_POLICY.md`
- **Bot Config:** `docs/governance/MERGE_GATE_BOT_CONFIG.md`
- **Phase Closure:** `docs/governance/PHASE_CLOSURE_POLICY.md`
- **Release Strategy:** `RELEASE_STRATEGY.md`
- **Branching Strategy:** `BRANCHING_STRATEGY.md`
- **Waivers:** `ai_working/ENFORCEMENT_WAIVERS.md`
- **Audit Log:** `ai_working/MERGE_GATE_AUDIT_LOG.md`
- **Live Dashboard:** `docs/governance/MERGE_GATE_STATUS_LIVE.md`

---

**For Support:** Create GitHub issue with `[governance]` label or contact @makr-code
