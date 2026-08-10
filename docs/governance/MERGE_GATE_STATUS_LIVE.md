# Merge Gate Status Dashboard (Live)

**Last Updated:** 2026-08-10T19:30:00Z  
**Status:** ✅ All Gates Operational  
**Automation Version:** Phase 3  

---

## Current Merge Requests Under Gate Validation

| PR # | Source | Target | Modules | Tier 0 | Tier 1 | Status | ETA |
|------|--------|--------|---------|--------|--------|--------|-----|
| (none) | - | - | - | - | - | - | - |

**Auto-refreshed:** Every 5 minutes  
**Last Check:** 2026-08-10 19:30 UTC

---

## Active Waivers

| PR | Gate | Approved By | Issued | Expires | Days Left |
|----|------|-----------|--------|---------|-----------|
| (none) | - | - | - | - | - |

**Next Expiration Check:** 2026-08-11 09:00 UTC

---

## Recent Escalations (Last 7 Days)

- (no escalations)

---

## Weekly Summary (Last 7 Days)

| Metric | Value |
|--------|-------|
| Total Validations | 0 |
| Pass Rate | N/A |
| Passes | 0 |
| Failures | 0 |
| Critical Failures | 0 |
| Waivers Issued | 0 |
| Avg Gate Time | N/A |

**Full Report:** See `ai_working/GATE_METRICS_WEEKLY.json`

---

## Gate Health Status

### Tier 0 Gates (Non-Waiverable)

| Gate ID | Status | Last Check | Freshness |
|---------|--------|-----------|-----------|
| T0-GOVERNANCE-REGISTRY | ✅ PASS | 2026-08-10 | Current |
| T0-MODULE-PHASES | ✅ PASS | 2026-08-10 | Current |
| T0-AI-COMPLIANCE | ✅ PASS | 2026-08-10 | Current |
| T0-SECURITY-SANITIZER | ✅ PASS | 2026-08-04 | 6 days |
| T0-SECURITY-PENTEST | ✅ PASS | 2026-05-20 | 82 days |
| T0-GA-SIGNOFF | ✅ PASS | 2026-08-10 | Current |

### Tier 1 Gates (Waiverable)

| Gate ID | Status | Last Check | Escalation Threshold |
|---------|--------|-----------|----------------------|
| T1-DOXYGEN-COVERAGE | ✅ PASS | 2026-08-10 | ≥3 waivers/4 weeks |
| T1-BSI-C5-COMPLIANCE | ✅ PASS | 2026-08-10 | No waiver allowed |
| T1-AI-MODEL-CARDS | ✅ PASS | 2026-08-10 | ≥3 waivers/4 weeks |

---

## Quick Reference

### For PR Authors

- PRs to `develop` require Tier 0 PASS only
- PRs to `community`/`military` require Tier 0 PASS + Tier 1 PASS (or waiver)
- Cannot merge until all Tier 0 gates pass
- Tier 1 failures can be waived by release leads (14-day expiration)

### For Release Leads

- Approve waivers: `/approve-with-waiver <GATE_ID> "justification"`
- List active: See "Active Waivers" section above
- Revoke waiver: Comment `/revoke-waiver <GATE_ID> "reason"`
- View audit: `ai_working/MERGE_GATE_AUDIT_LOG.md`

### For Operations

- Daily check (09:00 UTC): Check expiring waivers
- Weekly check (Monday 08:00 UTC): Generate audit summary
- Quarterly review: Policy adjustment and threshold tuning
- Gate status health: Check "Gate Health Status" section above

---

## Documentation

- **Policy:** `docs/governance/RELEASE_PROMOTION_GATE_POLICY.md`
- **Bot Config:** `docs/governance/MERGE_GATE_BOT_CONFIG.md`
- **Phase Closure:** `docs/governance/PHASE_CLOSURE_POLICY.md`
- **Waivers:** `ai_working/ENFORCEMENT_WAIVERS.md`
- **Audit Log:** `ai_working/MERGE_GATE_AUDIT_LOG.md`

---

**Next Update:** 2026-08-10 19:35 UTC  
**Automation:** `.github/workflows/12-governance_*.yml`
