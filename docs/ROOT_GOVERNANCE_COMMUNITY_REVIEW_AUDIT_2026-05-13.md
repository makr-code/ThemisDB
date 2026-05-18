# Root Governance/Community Review & Documentation Audit (2026-05-13)

Issue: **[Docs][Root] Governance/Community-Dokumente harmonisieren**

## Referenzen (verbindlich)

- [docs/DOCUMENTATION_REVIEW_GUIDELINES.md](DOCUMENTATION_REVIEW_GUIDELINES.md)
- [docs/SYSTEMATISCHER_REVIEWPLAN.md](SYSTEMATISCHER_REVIEWPLAN.md)
- [docs/PR_DOCUMENTATION_CHECKLIST.md](PR_DOCUMENTATION_CHECKLIST.md)
- [docs/de/development/SOURCE_CODE_AUDIT.md](de/development/SOURCE_CODE_AUDIT.md)
- [docs/audit-framework/AUDIT_RUNBOOK.md](audit-framework/AUDIT_RUNBOOK.md)

## Geprüfte/betroffene Root-Dateien

- `GOVERNANCE.md`
- `CODE_OF_CONDUCT.md`
- `CONTRIBUTING.md`
- `MAINTAINERS.md`
- `SOP.md`
- `SECURITY.md`

## Nachweis: Fachreview gegen Doku-/Code-Checklisten

- [x] Rollen, Prozesse und Verantwortlichkeiten zwischen den Root-Dokumenten abgeglichen
- [x] Beitragspfad für externe und interne Contributor als einheitlicher Pfad dokumentiert
- [x] Referenzen zwischen Governance/Community/Security/SOP auf Konsistenz geprüft
- [x] Eskalations- und Kontaktpfade als kanonische Kanäle dokumentiert

## Nachweis: Dokumentationsaudit

- [x] Baseline-Dokumentationslint (`scripts/docs-lint.py`) auf betroffenen Dateien durchgeführt
- [x] Baseline-Linkprüfung (`scripts/link-check.py --internal-only`) auf betroffenen Dateien durchgeführt
- [x] Ergebnis gegen bestehende, nicht-issuespezifische Link-/Anchor-Altfälle abgegrenzt
- [x] Geänderte Root-Dokumente nach Harmonisierung erneut validiert

## Ergebniszusammenfassung

- Root-Governance- und Community-Dokumente verwenden nun konsistente Rollen- und Eskalationspfade.
- Der Beitragspfad ist für externe und interne Contributor in den Root-Dokumenten synchronisiert.
- Querverweise zwischen Governance, Conduct, Contributing, Maintainers, SOP und Security wurden ergänzt/vereinheitlicht.
- Kontakt- und Eskalationskanäle sind in den betroffenen Dokumenten eindeutig und wiederverwendbar dokumentiert.
