# OpenAPI-/Generator-Dokumentationsaudit (2026-05)

## Scope

- `docs/openapi.yaml`
- `openapitools.json`
- `docs/apis/**`
- `docs/api/**`

## Fachreview & Audit-Nachweis

- [x] Fachreview gegen passende Checklisten durchgeführt
- [x] Dokumentationsaudit durchgeführt
- [x] Ergebnis dokumentiert
- [x] Relevante Dateien/Bereiche explizit festgehalten

## Verwendete Referenzen

- [`docs/DOCUMENTATION_REVIEW_GUIDELINES.md`](../DOCUMENTATION_REVIEW_GUIDELINES.md)
- [`docs/SYSTEMATISCHER_REVIEWPLAN.md`](../SYSTEMATISCHER_REVIEWPLAN.md)
- [`docs/PR_DOCUMENTATION_CHECKLIST.md`](../PR_DOCUMENTATION_CHECKLIST.md)
- [`docs/de/development/SOURCE_CODE_AUDIT.md`](../de/development/SOURCE_CODE_AUDIT.md)
- [`docs/audit-framework/AUDIT_RUNBOOK.md`](../audit-framework/AUDIT_RUNBOOK.md)

## Geprüfte und angepasste Bereiche

- OpenAPI-Metadaten um Source-of-Truth/Ownership/Breaking-Change-Referenz ergänzt (`docs/openapi.yaml`)
- Generator-Inputs/-Outputs und Generatorprofile dokumentiert (`openapitools.json`)
- API-Doku in `docs/apis/README.md` und `docs/api/README.md` auf gemeinsame Governance und Workflow konsolidiert
- Referenzpfad auf die zentrale OpenAPI-Spezifikation abgeglichen (`docs/api/API_REFERENCE.md`)

## Ergebnis

- Spezifikationsquelle und Ownership sind explizit definiert.
- Generator-Workflow (Input/Output) ist dokumentiert.
- Referenzdoku und OpenAPI-Verweise sind konsolidiert.
- Breaking-Change-Prozess ist über die Versioning-/Deprecation-Dokumente eindeutig verlinkt.
