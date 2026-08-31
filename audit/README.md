# ThemisDB Audit Hub and Canonical Map

**Last Updated:** 2026-08-31  
**Repository Metadata:** `VERSION=2.4.0-alpha`  
**Canonical Rule:** `/audit/**` is the audit source of truth; `/docs/**` is downstream publication/legacy mirror unless explicitly marked otherwise.

> **Baseline rule (2026-08-31):** Root audit documents without direct date reference in filename (`AUDIT.md`, `README.md`, `WAVE_C_AUDIT_EVIDENCE.md`) are synchronized to the latest consolidated baseline report.

---

## Scope Lock

- **Authoritative audit stack:** `audit/`
- **Downstream docs mirror/publication:** `docs/`
- **Historical/archive:** `audit/docs/audit-reports/v1.4.1/`, `audit/docs/audit-framework/evidence/v1.4.1/`, `audit/docs/ARCHIVED/`
- **Non-canonical working evidence:** `ai_working/**` (draft/evidence only; never release/security source of truth)

---

## Current Audit Set (Authoritative)

| Document | Purpose |
|---|---|
| `THEMISDB_AUDIT_MATURITY_SECURITY_MONETARY_REPORT_2026-08-31.md` | Consolidated source-verified audit, maturity, security, and monetary update |
| `AUDIT.md` | Central security/compliance/release audit summary |
| `MATURITY_REPORT_2026-08.md` | Monthly maturity and gate posture |
| `IMPLEMENTATION_AUDIT_2026-08-26.md` | Current implementation sync report |
| `IMPLEMENTATION_AUDIT_2026-08-12.md` | Prior implementation sync report |
| `IMPLEMENTATION_AUDIT_CORRECTED_2026-08-08.md` | Historical deep-dive delta report |
| `IMPLEMENTATION_AUDIT_2026-08-08.md` | Historical raw delta report |
| `IMPLEMENTATION_AUDIT_2026-08-07.md` | Historical base snapshot |
| `BSI_C5_2026_THEMISDB_AUDIT.md` | BSI-C5-2026 delta audit and actions |

---

## Inventory Matrix (`/audit` ↔ `/docs`)

| Topic | Canonical (`/audit`) | Downstream (`/docs`) | Status |
|---|---|---|---|
| Root audit navigation | `../AUDIT.md` → `audit/AUDIT.md` | n/a | ✅ synced |
| EU AI Act compliance set | `docs/compliance/EU_AI_ACT_*.md` (inside `audit/`) | `../docs/compliance/` currently does not mirror these files | 🟡 divergence tracked |
| Audit framework runbook/templates | `docs/audit-framework/*` (inside `audit/`) | `../docs/audit-framework/*` | 🟡 mirrored with overlap; `/audit` authoritative |
| Legacy `Audit` topic docs | `docs/Audit/*` (inside `audit/`) | `../docs/Audit/*` (inventory/gap-analysis docs) | ✅ scope-separated (no same-file duplicates) |
| Versioned audit bundles | `docs/audit-reports/v1.4.1/*` (inside `audit/`) | `../docs/audit-reports/v1.4.1/*` | 🟡 historical mirror; archival-only framing required |

---

## Quick Navigation

### Compliance & Governance (canonical in `/audit`)
- `docs/compliance/EU_AI_ACT_COMPLIANCE.md`
- `docs/compliance/EU_AI_ACT_RISK_MAPPING.md`
- `docs/compliance/EU_AI_ACT_EVIDENCE_BUNDLE.md`
- `docs/audit-framework/AUDIT_GOVERNANCE_STRUCTURE.md`
- `../docs/de/compliance/compliance_full_checklist.md`

### Release & Security Evidence
- `THEMISDB_AUDIT_MATURITY_SECURITY_MONETARY_REPORT_2026-08-31.md`
- `AUDIT.md`
- `MATURITY_REPORT_2026-08.md`
- `IMPLEMENTATION_AUDIT_2026-08-26.md`
- `IMPLEMENTATION_AUDIT_CORRECTED_2026-08-08.md`
- `../docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
- `../security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
- `../docs/governance/GA_PROMOTION_SIGN_OFF.md`

### Historical / Archival Paths
- `docs/audit-reports/q2-2026-quality-wave-1/AUDIT.md`
- `docs/audit-reports/v1.4.1/` (archival package)
- `docs/audit-framework/evidence/v1.4.1/` (archival evidence bundle)
- `docs/ARCHIVED/`

---

## Current Status Snapshot

- Technical GA hardening remains **PASS** (Wave 7/8/9 + sanitizer + pentest evidence).
- The only confirmed GA blocker remains the human sign-off in `../docs/governance/GA_PROMOTION_SIGN_OFF.md` §9.
- For current implementation drift handling, use `IMPLEMENTATION_AUDIT_2026-08-26.md` first.

### Compliance Snapshot

| Framework | Status | Primary Evidence |
|---|---|---|
| ISO 27001:2022 | ✅ 95% | `../docs/de/compliance/compliance_full_checklist.md` |
| BSI C5 (2026 delta) | 🟡 92% | `BSI_C5_2026_THEMISDB_AUDIT.md` |
| GDPR / DSGVO | ✅ 98% | `../docs/de/compliance/compliance_dpia.md` |
| EU AI Act | 🟡 65% | `docs/compliance/EU_AI_ACT_COMPLIANCE.md` |
| NIS2 | ✅ 94% | `../docs/de/compliance/compliance_bcp_drp.md` |
| SOC 2 Type II | ✅ 90% | `../docs/de/compliance/compliance_full_checklist.md` |

---

## Provenance

- Root release-readiness status: `../ROADMAP.md`
- Root release trace: `../CHANGELOG.md`
- Final governance gate: `../docs/governance/GA_PROMOTION_SIGN_OFF.md`
- Module-level implementation truth: `../src/*/ROADMAP.md`, `../src/*/AUDIT.md`
