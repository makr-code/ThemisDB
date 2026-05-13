# API-Spezifikationen (`docs/apis`)

## Spezifikationsquelle & Ownership

- **OpenAPI Source of Truth:** [`/docs/openapi.yaml`](../openapi.yaml)
- **Ownership:** API-Dokumentationspflege erfolgt über `docs/apis/**` und `docs/api/**`
- **Generator-Konfiguration:** [`/openapitools.json`](../../openapitools.json)

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

- Jede API-Änderung in `docs/openapi.yaml` muss in der Referenzdoku unter [`docs/api`](../api/README.md) nachvollziehbar dokumentiert werden.
- API-spezifische Ergänzungen in `docs/apis/**` (z. B. Import API) dürfen nicht der OpenAPI-Spezifikation widersprechen.
- Änderungen an Generator-Pfaden oder Generator-Version müssen in `openapitools.json` und in der API-Referenz dokumentiert werden.

## Breaking-Change-Prozess

- Verbindlicher Prozess: [`docs/api/API_VERSIONING.md`](../api/API_VERSIONING.md) (Deprecation-/Versioning-Policy)
- Ergänzende Registry: [`docs/api/DEPRECATION_REGISTRY.md`](../api/DEPRECATION_REGISTRY.md)
