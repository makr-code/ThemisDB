# Maturity Automation Runbook

**Document:** Operations Guide for Phase 2/3 Automation Infrastructure  
**Version:** 1.0  
**Owner:** Platform Release / @makr-code  
**Last Updated:** 2026-08-10  
**Target Audience:** Release Leads, Infrastructure Engineers, DevOps

---

## Quick Start

### Daily Checks (5 min)

1. **Check bot health**
   ```bash
   # Verify bot is authenticated and processing PRs
   curl https://api.github.com/app/installations \
     -H "Authorization: token $GITHUB_TOKEN"
   ```

2. **Review escalations**
   - Check GitHub issues labeled `[severity] merge-gate-failure`
   - Verify release leads have reviewed and responded
   - Look for patterns (recurring failures)

3. **Check waiver expirations**
   - View `ai_working/ENFORCEMENT_WAIVERS.md`
   - Flag any expiring within 3 days
   - Post reminder on PR if needed

### Weekly Review (30 min every Monday)

1. **Merge gate audit**
   ```bash
   # Review last 7 days of merges
   tail -50 ai_working/MERGE_GATE_AUDIT_LOG.json
   ```
   - Identify gate failures and patterns
   - Any Tier 0 gates unexpectedly failing? Investigate.
   - Any Tier 1 gates trending toward Tier 0? Flag for policy review.

2. **Validate gate thresholds**
   - Are freshness requirements (7, 30, 90 days) appropriate?
   - Are any gates consistently stale? Consider automation schedule adjustment.
   - Any false-positive rate in gate checks? Refine validation rules.

3. **Waiver analysis**
   - Which gates most frequently waived?
   - Should frequent Tier 1 waivers move to Tier 2 (logged only)?
   - Update RELEASE_PROMOTION_GATE_POLICY.md if thresholds need adjustment

4. **Post weekly summary to Slack `#themisdb-release`**
   ```
   📊 **Merge Gate Weekly Summary**
   • PRs merged: X
   • Gates passed: Y
   • Gates failed: Z
   • Waivers issued: W
   • Avg gate time: V min
   • Blockers this week: [list]
   ```

---

## Phase 2 Infrastructure Setup

### Workflow Execution

#### `10-governance_maturity-verification.yml`
- **Trigger:** Daily at 03:00 UTC + every PR to develop
- **Purpose:** Continuous validation of evidence registry and gate freshness
- **Duration:** ~15 min
- **Status Page:** `docs/governance/MATURITY_AUTOMATION_DASHBOARD.md` (updated hourly)

**If workflow fails:**
1. Check GitHub Actions logs
2. Common issues:
   - Evidence artifact file missing → verify path in MATURITY_EVIDENCE_MANIFEST.json
   - Manifest stale → run verify workflow manually
   - Network issue accessing benchmarks → retry with backoff
3. Create GitHub issue: `[automation] gate-verification-failure`

#### `.github/scripts/module_phase_verifier.py`
- **Purpose:** Scans ROADMAP.md to verify phase state vs. implementation
- **Manual invocation:**
  ```bash
  python .github/scripts/module_phase_verifier.py \
    --module <name> --phase <N> --verify-acceptance
  python .github/scripts/module_phase_verifier.py --all-modules --generate-report
  ```
- **Automated:** Runs weekly to detect regressions

---

## Phase 3 Enforcement Setup

### Merge Gate Bot Configuration

1. **GitHub App Creation**
   - Create app at https://github.com/settings/apps/new
   - App name: `ThemisDB-Merge-Gate-Enforcer`
   - Permissions per `MERGE_GATE_BOT_CONFIG.md`
   - Install to makr-code/ThemisDB repo
   - Store app ID + secret in GitHub Secrets

2. **Bot Activation**
   - Enable in `.github/workflows/` with `GITHUB_TOKEN` permissions
   - First test: Create test PR to `develop` with label `[automation] merge-gate-check`
   - Verify bot posts validation comment within 10 min

3. **Initial Validation**
   - Run on small PRs first (single-module changes)
   - Verify all checks complete without errors
   - Review comment format and clarity
   - Adjust thresholds based on feedback

---

## Gate Validation Rules

### Tier 0 Gates (Hard Block)

| Gate | Freshness | Check | Automation |
|------|-----------|-------|-----------|
| W7 Performance | ≤ 7 days | manifest exists + recent | Auto-fail if stale |
| W8 Security | ≤ 30 days (sanitizer), ≤ 90 days (pentest) | section contains "PASS" | Auto-fail if stale |
| GA Sign-Off | N/A (manual) | human signature | Manual review |
| Phase Deps | ≤ 7 days | DAG acyclic + prereqs met | Auto-fail if broken |

**Remediation on Tier 0 Failure:**
1. PR merge is blocked
2. Bot posts detailed comment with specific failure
3. Release lead must resolve before merge retry
4. No waiver possible

### Tier 1 Gates (Escalation Required)

| Gate | Freshness | Waiver Window | After Waiver |
|------|-----------|---------------|--------------|
| Doxygen < 95% | ≤ 7 days | 72 hours | Allow merge + log |
| BSI Gaps | ≤ 30 days | N/A (no waiver) | Must close gaps |
| EU AI Act < 75% | ≤ 7 days | 72 hours (military only) | Allow merge + log |

**Escalation Process:**
1. PR fails Tier 1 gate
2. Bot posts comment: "Tier 1 gate failed; release lead review required"
3. Release lead posts `/approve-with-waiver <gate_id> <justification>`
4. Bot logs waiver (expires 14 days)
5. Merge allowed

---

## Audit Trail & Logging

### `MERGE_GATE_AUDIT_LOG.json`

**Location:** `ai_working/MERGE_GATE_AUDIT_LOG.json` (append-only)

**Schema:**
```json
{
  "timestamp": "ISO 8601",
  "pr_number": int,
  "source_branch": "branch name",
  "target_branch": "branch name",
  "merged_by": "github handle",
  "gate_results": {
    "<gate_category>": "PASS | FAIL | SKIPPED"
  },
  "waivers_applied": [],
  "automation_version": "Phase2 | Phase3"
}
```

**How to Query:**
```bash
# Last 10 merges
tail -10 ai_working/MERGE_GATE_AUDIT_LOG.json | jq '.pr_number, .gate_results'

# Merges to 'community' in last 7 days
cat ai_working/MERGE_GATE_AUDIT_LOG.json | jq 'select(.target_branch=="community" and (.timestamp|fromdateiso8601) > now - 604800)'
```

---

## Emergency Procedures

### Release Blocker: Critical Security Patch Needed Immediately

**Scenario:** Security vulnerability found; need to release hotfix to `community` immediately

**Steps:**
1. Cherry-pick fix commit from `develop` to hotfix branch
2. Request release lead waiver with:
   - CVE ID or internal security assessment link
   - Customer impact statement
   - Justification: "Critical security hotfix; bypassing normal gate cycle"
3. Run abbreviated security gate:
   - Skip W7 performance (not security-critical)
   - Must pass W8 security (sanitizer/pentest)
4. Post to Slack #themisdb-release for visibility
5. Document in `ENFORCEMENT_WAIVERS.md` as "emergency-security-hotfix"

### Automation Failure: All PRs Blocked

**Scenario:** Bot crashes or gate validation is broken; all PRs blocked

**Recovery:**
1. Check bot health: `curl https://api.github.com/app/installations`
2. Verify manifest file exists: `docs/governance/MATURITY_EVIDENCE_MANIFEST.json`
3. Check GitHub Actions logs: `.github/workflows/10-governance_maturity-verification.yml`
4. If bot error: restart bot or reinstall GitHub App
5. If manifest error: manually verify and regenerate
6. Post status to Slack #themisdb-release
7. Temporarily disable bot on `community` branch while debugging (enable manual mode)

### Temporary Bypass (Emergency Only)

**If automation cannot be restored within 2 hours:**
1. Release lead posts issue: `[emergency] Merge gate automation down — manual mode enabled`
2. Manually review all gate checks:
   - W7 performance: verify manually in benchmarks/
   - W8 security: verify manually in docs/security/
   - GA sign-off: verify manually in docs/governance/
3. Document bypass with: "Manual verification performed on [timestamp] by [person]"
4. Post-incident: debug automation failure and implement fix

---

## Troubleshooting

### Bot Posts Confusing Comments

**Issue:** Comment contains cryptic error messages  
**Fix:**
1. Review MERGE_GATE_BOT_CONFIG.md for comment format
2. Check module_phase_verifier.py output format
3. Add context to error messages (file paths, suggestions)
4. Test on small PR first before deploying

### Gate Takes Too Long to Run

**Issue:** Merge gate validation takes > 30 min  
**Investigation:**
1. Profile which step is slow:
   - Governance registry check: 5 min?
   - Module phase gates: 10 min?
   - AI compliance check: 5 min?
   - Security gate: 10 min?
2. If step consistently slow:
   - Cache intermediate results
   - Parallelize independent checks
   - Reduce scope (check only affected modules, not all 67)

### False Positives: Gate Fails When It Should Pass

**Issue:** Tier 0 gate fails even though evidence is current  
**Investigation:**
1. Manually verify gate condition:
   ```bash
   # Example: check if W7 manifest is fresh
   ls -la benchmarks/wave7/release_gate_manifest_w7.json
   date -d "7 days ago" +%s
   stat -c "%Y" benchmarks/wave7/release_gate_manifest_w7.json
   # Compare timestamps
   ```
2. Review validation rule in MATURITY_EVIDENCE_MANIFEST.json
3. Adjust thresholds if necessary
4. Document false positive pattern in GitHub issue

---

## Dashboard & Visibility

### Daily Status Report

**File:** `docs/governance/MATURITY_AUTOMATION_DASHBOARD.md`  
**Updated:** Hourly via `.github/workflows/10-governance_maturity-verification.yml`  
**Contents:**
- Real-time gate status (all 18 gates)
- Age of last verification for each gate
- Active waivers with expiration dates
- Recent escalations
- Weekly summary metrics

**Access:** Embedded in repository homepage (README.md link)

---

## Maintenance Schedule

### Monthly

- Review gate thresholds: Are they still appropriate?
- Audit false-positive rate: < 5%?
- Check bot performance: average gate time < 30 min?

### Quarterly

- Full policy review: RELEASE_PROMOTION_GATE_POLICY.md
- Update automation scripts for new modules
- Pilot new gates on dev branch before production

### Annually

- Comprehensive compliance audit
- Cross-check gates against actual release needs
- Update documentation and runbooks

---

## Support & Escalation

### Who to Contact

- **Bot not working:** @devops-team (GitHub issue: `[infra] merge-gate-bot-down`)
- **Gate policy questions:** @release-lead
- **Automation script bugs:** @platform-engineering
- **Waiver requests:** @release-lead (72-hour turnaround)

### Links & References

- Policy: `RELEASE_PROMOTION_GATE_POLICY.md`
- Phase closure: `PHASE_CLOSURE_POLICY.md`
- Bot config: `MERGE_GATE_BOT_CONFIG.md`
- Evidence manifest: `MATURITY_EVIDENCE_MANIFEST.json`
- Phase dependency graph: `PHASE_DEPENDENCY_GRAPH.md`

