---
Author: ThemisDB Maintainers
Created: 2026-08-31
Last Updated: 2026-11-10
Status: active
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Audit Update — BSI C5 (Stand 2026)

**Datum:** 2026-04-21  
**Scope:** Update der zentralen ThemisDB-Auditlage mit C5-2026-Schwerpunkt  
**Basis:** bestehende C5-Mappings und Audit-Artefakte im Repository

---

## 1) Ergebnis

ThemisDB bleibt auf einer **tragfähigen BSI-C5-Basis**, benötigt für den 2026-orientierten Audit-Fokus jedoch zusätzliche Nachweise in fünf Bereichen:

1. Evidenzqualität und durchgängige Nachweisführung
2. Shared-Responsibility-Mapping pro Deployment-Modell
3. Supply-Chain-/Build-Integritätsnachweise pro Release
4. Verifizierbare Incident- und Recovery-Übungen
5. Vollständige kryptographische Lifecycle-Auditierung

**Gesamtstatus (C5-2026 Delta): 🟡 Teilweise erfüllt, Maßnahmen laufend**

---

## 2) Delta-Bewertung gegenüber bestehender C5-Basis

| Bereich | Ist-Stand ThemisDB | 2026-orientierter Nachweisbedarf | Status |
|---|---|---|---|
| Governance & Policies | C5-Mappings und Security-Dokumente vorhanden | Nachweis-Manifest je Release, eindeutige Evidence-IDs | 🟡 |
| Kryptographie & Key Lifecycle | AES-256-GCM, Key-Management-Strukturen vorhanden | Vollständige Audit-Trails für Rotation/Revocation/Break-Glass | 🟡 |
| Logging & Monitoring | Umfangreiche Audit- und Security-Logs vorhanden | Mapping von Log-Ereignissen auf C5-Kontrollfamilien | 🟡 |
| Incident & BCM | Incident-/Recovery-Dokumentation vorhanden | Regelmäßige Übungs- und Restore-Nachweise im Audit-Bundle | 🟡 |
| Supply Chain Integrity | Security-Scans und Compliance-Reports vorhanden | Signierte SBOM/Attestation-Evidenz je Release | 🟡 |

---

## 3) Verbindliche Maßnahmen für das ThemisDB-Audit

- [~] C5-Evidence-Manifest pro Release unter `audit/evidence/c5/<release>/manifest.json` etablieren (Automation in `.github/workflows/compliance-supply-chain.yml` + `tools/ci/generate_c5_evidence_manifest.py` ergänzt)
- [~] Shared-Responsibility-Matrix (Self-hosted/Managed/Hybrid) als Audit-Anlage ergänzen (`audit/evidence/c5/SHARED_RESPONSIBILITY_MATRIX.md` angelegt; quartalsweise Evidenzbefüllung offen)
- [~] Signierte SBOM- und Build-Provenance-Referenzen in den Audit-Report aufnehmen (C5-Manifest referenziert SBOM-/Workflow-Artefakte; tag-basierte Provenance-Nachweise weiter ausführen)
- [~] Incident-Drill- und Restore-Test-Evidenz in quartalsweise Audits integrieren (`audit/evidence/c5/INCIDENT_DRILL_EVIDENCE_INDEX.md` als kanonischer Index angelegt; Befüllung offen)
- [~] Audit-Events für Key-Lifecycle-Prozesse systematisch vervollständigen (`audit/evidence/c5/KEY_LIFECYCLE_AUDIT_EVENTS.md` als Event-Kanon angelegt; Test/Runtime-Mapping offen)

### Q4 2026 release-candidate evidence review (updated 2026-11-10)

- Candidate bundle: [`audit/evidence/c5/v2.5.0-rc-candidate/manifest.json`](evidence/c5/v2.5.0-rc-candidate/manifest.json); schema/run-reference report: [`manifest_validation.json`](evidence/c5/v2.5.0-rc-candidate/manifest_validation.json). Validation passed structural checks only; it is not evidence verification or certification.
- Source run: [Compliance: Supply Chain run 42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), commit `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`. The SOC 2 evidence-export job [failed](https://github.com/makr-code/ThemisDB/actions/runs/42321944686/job/128248301027) during `docker compose build themis`: vcpkg's `python3:x64-linux` build stopped after `autoreconf -vfi` exited 1. Correction to the previous comparison: `docker-compose.yml` selects the repository-root `Dockerfile`, not `docker/Dockerfile.unified`; the root Dockerfile at this failed-run commit omitted `autoconf-archive`, consistent with vcpkg's prerequisite warning. The vcpkg stderr file was not captured, so the omission is a likely source-level cause, not confirmed as the sole cause.
- Follow-up develop branch run [42761863772](https://github.com/makr-code/ThemisDB/actions/runs/42761863772), commit `c36177430075c5d882a0c344a3a359f29dd68d60`, succeeded at SBOM generation and C5 manifest packaging/validation/upload. Its SOC 2 job [failed](https://github.com/makr-code/ThemisDB/actions/runs/42761863772/job/129649196631) during root Dockerfile apt package installation: Perl package downloads from `security.ubuntu.com` timed out or returned 404 before vcpkg bootstrap/Python autoreconf. Therefore this run does not verify the autoreconf prerequisite. SBOM signing and tag-only SLSA provenance were skipped for the branch dispatch; this is not release-specific evidence.
- `BCM-03` remains **partial** because no current incident/restore drill run, artifact, and dated result were identified. The September 21, 2026 drill-index rows are historical source/test mappings, not fresh runtime evidence.
- No matching successful key-lifecycle CI trace or persisted release audit record is linked in the candidate package. The shared-responsibility matrix has no model-specific deployment evidence. Target-release SBOM/provenance signatures and authorized human compliance review are not recorded.
- Wave C remains open. These records do not claim independent certification; follow-up is tracked by [#7817](https://github.com/makr-code/ThemisDB/issues/7817).

---

## 4) Betroffene zentrale Dokumente

- `audit/AUDIT.md` (zentrale Audit-Übersicht, aktualisiert)
- `../docs/audit-framework/COMPLIANCE_MAPPING.md` (Downstream-Referenz; Audit-SOT bleibt `audit/`)
- `docs/de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md`
- `docs/de/compliance/compliance_full_checklist.md`
