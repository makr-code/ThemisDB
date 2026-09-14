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

---

## 4) Betroffene zentrale Dokumente

- `audit/AUDIT.md` (zentrale Audit-Übersicht, aktualisiert)
- `../docs/audit-framework/COMPLIANCE_MAPPING.md` (Downstream-Referenz; Audit-SOT bleibt `audit/`)
- `docs/de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md`
- `docs/de/compliance/compliance_full_checklist.md`
