# Enforcement Waivers Log

**Document:** Phase 3 Tier 1 Gate Waiver Tracking  
**Status:** Active (2026-Q4)  
**Purpose:** Immutable record of all Tier 1 gate waivers for compliance audit  
**Format:** Markdown table (append-only)  
**Schema Version:** 1.0  

---

## Active Waivers

| PR | Gate | Approver | Justification | Issue Date | Expires | Status |
|-------|------|-----------|---------------|----------|---------|--------|
| (none) | - | - | - | - | - | - |

---

## Waiver Schema

- **PR**: Pull request number (e.g., `#12345`)
- **Gate**: Gate ID from RELEASE_PROMOTION_GATE_POLICY.md (e.g., `T1-DOXYGEN-COVERAGE`)
- **Approver**: GitHub username of release lead who approved (e.g., `@release-lead`)
- **Justification**: Reason for waiver (max 500 characters)
- **Issue Date**: Date waiver was issued (YYYY-MM-DD)
- **Expires**: Expiration date (YYYY-MM-DD, typically 14 days from issue)
- **Status**: `ACTIVE`, `EXPIRED`, or `REVOKED`

---

## Waiver Workflow

1. Release lead posts comment on PR: `/approve-with-waiver <GATE_ID> "justification"`
2. Waiver validator checks:
   - Approver is member of `@themisdb/release-leads` team
   - Gate ID is valid (Tier 1 only)
   - Justification is 10-500 characters
3. If valid, waiver appended to this log with 14-day expiration
4. Daily automation (09:00 UTC) identifies waivers expiring within 3 days
5. Weekly automation (Monday 08:00 UTC) generates audit summary
6. Gate status updates to `EXPIRED` after expiration date

---

## Escalation Policy

If any gate receives **3+ waivers within 4 weeks**, escalate to Tier 0:
1. Move gate from Tier 1 to Tier 0 in `RELEASE_PROMOTION_GATE_POLICY.md`
2. Create GitHub issue: `[governance] <GATE_ID> - escalated to Tier 0`
3. Assign to domain lead for resolution

---

## Review Schedule

- **Daily:** Check expiring waivers (09:00 UTC via `12-governance_waiver-expiration-check.yml`)
- **Weekly:** Audit summary and metrics (Monday 08:00 UTC via `12-governance_gate-audit-summary.yml`)
- **Quarterly:** Policy review and gate threshold adjustments

---

## Historical Waivers

(none yet)

