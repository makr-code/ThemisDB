# API-Spezifikationen (`docs/apis`)

## Spezifikationsquelle & Ownership

- **OpenAPI Source of Truth:** [`/docs/openapi.yaml`](/docs/openapi.yaml)
- **Ownership:** `themisdb-api-maintainers` (siehe `.github/CODEOWNERS`); `docs/apis/**` für spezifikationsnahe Ergänzungen, `docs/api/**` für Referenz-/Policy-Dokumentation.
- **Generator-Konfiguration:** [`/openapitools.json`](/openapitools.json)

## Generator-Workflow (Inputs / Outputs)

**Input**
- `docs/openapi.yaml`

**Konfiguration**
- `openapitools.json` (Generator-Version und Zielpfade)
- `scripts/generate-sdks.sh` (lokale SDK-Generierung)

**Outputs**
- `openapi/generated/python/`
- `openapi/generated/javascript/`
- `openapi/generated/go/`

## Konsistenz-Regeln

- Jede API-Änderung in `docs/openapi.yaml` muss in der Referenzdoku unter [`docs/api`](../api/README.md) nachvollziehbar dokumentiert werden (betroffene Endpunkte, Request/Response-Auswirkung, Versioning-/Deprecation-Auswirkung).
- API-spezifische Ergänzungen in `docs/apis/**` (z. B. Import API) dürfen nicht der OpenAPI-Spezifikation widersprechen.
- Änderungen an Generator-Pfaden oder Generator-Version müssen in `openapitools.json` und in der API-Referenz dokumentiert werden.
- Mindestvalidierung nach API-Doku-Änderungen mit den vorhandenen Repository-Skripten: `python3 scripts/docs-lint.py <changed-docs...>` und `python3 scripts/link-check.py --internal-only <changed-docs...>`.
- Für Forks/Transfers: siehe Maintenance-Hinweis in `openapitools.json` zur Anpassung des Go-`moduleName` (analog auch in `scripts/generate-sdks.sh`).

## Breaking-Change-Prozess

- Verbindlicher Prozess: [`docs/api/API_VERSIONING.md`](../api/API_VERSIONING.md) (Deprecation-/Versioning-Policy)
- Ergänzende Registry: [`docs/api/DEPRECATION_REGISTRY.md`](../api/DEPRECATION_REGISTRY.md)
