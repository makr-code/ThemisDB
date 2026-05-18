# Documentation Audit Report — research/ (2026-05-13)

## Scope

- `/research/README.md`
- `/research/RESEARCH_GUIDE.md`
- `/research/papers/README.md`
- `/research/experiments/README.md`
- `/research/architecture_decisions/README.md`
- `/research/implementation_influence/README.md`
- `/research/stand_der_technik/README.md`
- `/research/best_practices/README.md`
- `/research/schema/README.md`

## Review-/Audit-Checklisten (verbindliche Referenzen)

- [docs/DOCUMENTATION_REVIEW_GUIDELINES.md](../docs/DOCUMENTATION_REVIEW_GUIDELINES.md)
- [docs/SYSTEMATISCHER_REVIEWPLAN.md](../docs/SYSTEMATISCHER_REVIEWPLAN.md)
- [docs/PR_DOCUMENTATION_CHECKLIST.md](../docs/PR_DOCUMENTATION_CHECKLIST.md)
- [docs/de/development/SOURCE_CODE_AUDIT.md](../docs/de/development/SOURCE_CODE_AUDIT.md)
- [docs/audit-framework/AUDIT_RUNBOOK.md](../docs/audit-framework/AUDIT_RUNBOOK.md)

## Nachweis

- [x] Fachreview gegen passende Doku-/Code-Checklisten durchgeführt
- [x] Dokumentationsaudit durchgeführt (Struktur, Statuskennzeichnung, Link-Konsistenz)
- [x] Ergebnis dokumentiert (dieser Report, im Repository verlinkbar)
- [x] Relevante Dateien/Bereiche explizit festgehalten (siehe Scope)

## Ergebnis (Kurzfassung)

1. **Cluster-Struktur klargestellt**: kanonische Cluster `papers`, `experiments`, `architecture_decisions`, `implementation_influence` sind in `research/README.md` explizit als kanonisch markiert.
2. **Veraltete Entwürfe markiert**: mehrere Top-Level-Drafts mit Nachfolgern wurden als `SUPERSEDED_DRAFT` gekennzeichnet.
3. **Produktionsnahe Verlinkung ergänzt**: `implementation_influence/by_module.md`, `by_version.md` und modulnahe `src/*/README.md` Links wurden im Index ergänzt.
4. **Index/Guideline aktualisiert**: `research/README.md` und `research/RESEARCH_GUIDE.md` enthalten jetzt ein einheitliches Statusmodell für Draft-Lifecycle.

## Validierung

- `python3 scripts/docs-lint.py research/README.md research/RESEARCH_GUIDE.md research/DOCUMENTATION_AUDIT_REPORT_2026-05-13.md`
- `python3 scripts/link-check.py --internal-only research/README.md research/RESEARCH_GUIDE.md research/DOCUMENTATION_AUDIT_REPORT_2026-05-13.md`
