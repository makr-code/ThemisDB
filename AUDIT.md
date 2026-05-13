# Root Audit-Navigation und Abgleich-Nachweis

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

## Review- und Dokumentationsaudit-Nachweis (Issue-Abschluss)

- [x] Fachreview gegen Checklisten durchgeführt
  Referenzen: [docs/DOCUMENTATION_REVIEW_GUIDELINES.md](docs/DOCUMENTATION_REVIEW_GUIDELINES.md), [docs/PR_DOCUMENTATION_CHECKLIST.md](docs/PR_DOCUMENTATION_CHECKLIST.md)
- [x] Dokumentationsaudit systematisch durchgeführt
  Referenzen: [docs/SYSTEMATISCHER_REVIEWPLAN.md](docs/SYSTEMATISCHER_REVIEWPLAN.md), [docs/de/development/SOURCE_CODE_AUDIT.md](docs/de/development/SOURCE_CODE_AUDIT.md)
- [x] Audit-/Review-Ergebnis in Root-Dokumenten dokumentiert
  Referenz: [docs/audit-framework/AUDIT_RUNBOOK.md](docs/audit-framework/AUDIT_RUNBOOK.md)
- [x] Betroffene Dateien/Scope festgehalten
  `ARCHITECTURE.md`, `AUDIT.md`, `SECURITY.md`, `CTEST.md`, `PERFORMANCE_EXPECTATIONS.md`, `PERFORMANCE_OPTIMIZATION_PLAN.md`, `PERFORMANCE_BOTTLENECKS.md`
