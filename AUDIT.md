# Root Audit-Navigation und Abgleichsnachweis

Die zentrale Audit-Dokumentation liegt unter `audit/`.

## Kanonische Audit-Quellen

- Hauptdokument: [audit/AUDIT.md](audit/AUDIT.md)
- BSI C5 2026 Update: [audit/BSI_C5_2026_THEMISDB_AUDIT.md](audit/BSI_C5_2026_THEMISDB_AUDIT.md)
- Audit-Runbook: [docs/audit-framework/AUDIT_RUNBOOK.md](docs/audit-framework/AUDIT_RUNBOOK.md)

## Root-Dokument-Abgleich (Architektur / Security / Performance / Test)

Der Root-Abgleich folgt einer gemeinsamen Kontrollbasis:

1. **Sicherheitsmodell ↔ Architekturannahmen:** Hardening/Transport/AuthZ/Audit-Trail sind zwischen `ARCHITECTURE.md` und `SECURITY.md` harmonisiert.
2. **Audit-Aussagen ↔ technische Kontrollen:** Referenziert werden aktive Kontrollen aus `audit/AUDIT.md` und `SECURITY.md` (z. B. Gitleaks, clang-tidy, cppcheck, Trivy, RBAC, Audit-Logging).
3. **Security-Verifikation ↔ Testpfade:** Nachweisbare Testpfade sind in `CTEST.md` dokumentiert und mit den Security-/Audit-Dokumenten verlinkt.
4. **Performance ↔ Security/Audit:** Performance-Gates in `PERFORMANCE_EXPECTATIONS.md`, `PERFORMANCE_OPTIMIZATION_PLAN.md` und `PERFORMANCE_BOTTLENECKS.md` sind auf dieselben Sicherheits- und Audit-Randbedingungen ausgerichtet.

## Gap-Scanner Status (2026-06-11)

- Aktiver Scan-Artefaktpfad:
  - `ai_working/gap_scan_results.json`
  - `ai_working/gap_scan_report_ollama_gemma4.md`
- Aktive Remediation-Worklist:
  - `themis_core` only (third_party rein informativ)
  - kompakter 6-Zeilen-Template-Block pro Item fuer externe Abarbeitung
- Aktives GitHub-Tracking-Issue:
  - `#5475` — `[P0-HIGH] INCLUDE Module - Current Gap Worklist Tracking (2026-06-11)`
- Hinweis zur Tracker-Konsolidierung:
  - Historische modulare v3-P0-Tracker wurden in den Closed-State ueberfuehrt und durch den aktuellen Baseline-Tracker ersetzt.
  - Zweite Konsolidierungsrunde abgeschlossen: Cross-Module/v3-Tracker `#5172`, `#5232` bis `#5244` geschlossen (superseded by `#5475`).

### Aktuell offene Remediation-Issues (Snapshot)

- `#5475` — `[P0-HIGH] INCLUDE Module - Current Gap Worklist Tracking (2026-06-11)`
- `#5363` — Legacy migration tracking (offen)
- `#5364` — Legacy migration tracking (offen)
- `#5365` — Legacy migration tracking (offen)
- `#5366` — Legacy migration tracking (offen)

## Root-Sync-Referenzen (2026-06-11)

- README-Statusabgleich:
  - `README.md` Abschnitt `Scanner Baseline Update (2026-06-11)` ist auf denselben Tracking-Stand synchronisiert.
- Releasehistorie / Governance-Abgleich:
  - `CHANGELOG.md` enthaelt den Eintrag `Documentation / Governance — Scanner baseline and tracker consolidation (2026-06-11)`.
- Arbeitsmodus fuer Remediation:
  - Fokus bleibt auf abhakbarer Worklist (keine Top-N-Statistik) mit `themis_core` als actionable Scope.

## Review- und Dokumentationsaudit-Nachweis (Issue-Abschluss)

- [x] Fachreview gegen Checklisten durchgeführt
  Referenzen: [docs/DOCUMENTATION_REVIEW_GUIDELINES.md](docs/DOCUMENTATION_REVIEW_GUIDELINES.md), [docs/PR_DOCUMENTATION_CHECKLIST.md](docs/PR_DOCUMENTATION_CHECKLIST.md)
- [x] Dokumentationsaudit systematisch durchgeführt
  Referenzen: [docs/SYSTEMATISCHER_REVIEWPLAN.md](docs/SYSTEMATISCHER_REVIEWPLAN.md), [docs/de/development/SOURCE_CODE_AUDIT.md](docs/de/development/SOURCE_CODE_AUDIT.md)
- [x] Audit-/Review-Ergebnis in Root-Dokumenten dokumentiert
  Referenz: [docs/audit-framework/AUDIT_RUNBOOK.md](docs/audit-framework/AUDIT_RUNBOOK.md)
- [x] Betroffene Dateien/Scope festgehalten
  `ARCHITECTURE.md`, `AUDIT.md`, `SECURITY.md`, `CTEST.md`, `PERFORMANCE_EXPECTATIONS.md`, `PERFORMANCE_OPTIMIZATION_PLAN.md`, `PERFORMANCE_BOTTLENECKS.md`

---
Zuletzt geprueft (Root-Sync): 2026-06-11

## Epic Branch Flow — PR-Template-Referenz (Gap Scanner Wave 1)

Fuer alle Pull Requests, die im Rahmen des Gap-Scanner-Wave-1-Epics
(`epic/gap-wave1-5475`) erstellt werden, gilt das spezialisierte PR-Template:

- **Template-Pfad:** [`.github/PULL_REQUEST_TEMPLATE/epic-branch-flow.md`](.github/PULL_REQUEST_TEMPLATE/epic-branch-flow.md)

Das Template erzwingt die mandatory Merge-Reihenfolge
(Feature-Branch → EPIC-Branch → `develop`), das Build/Test-Evidence-Format,
den Risk/Rollback-Hinweis sowie die Gap-Scanner-Gates fuer Wave-1-PRs.

Fuer Standard-Feature-PRs ausserhalb dieses Epics gilt weiterhin das
Standard-Template: [`.github/pull_request_template.md`](.github/pull_request_template.md)

