# Enforcement Waivers Log

**Document:** Tier 1 Gate Waiver Tracking  
**Purpose:** Immutable record of all Tier 1 gate waivers for compliance audit  
**Format:** Markdown table (append-only)  

| PR | Gate | Approver | Justification | Issue Date | Expires | Status | Renewed |
|----|------|----------|---------------|------------|---------|--------|---------|
| (empty) | (empty) | (empty) | (empty) | (empty) | (empty) | (empty) | (empty) |

---

## Waiver Glossary

- **Gate:** Tier 1 gate ID (e.g., GATE-DOXYGEN-P6)
- **Approver:** GitHub handle of release lead who approved waiver
- **Justification:** Business reason for waiver
- **Expires:** Date waiver expires (14 days from issue date)
- **Status:** ACTIVE or EXPIRED
- **Renewed:** If ACTIVE but past original expiration, shows renewal date

---

## Weekly Waiver Review

Every Monday at 08:00 UTC, automated check:
- Counts active waivers
- Flags expiring waivers (< 3 days)
- Posts summary to release team

