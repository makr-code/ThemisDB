# OpenAPI SDK Generation

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_sdk-generation.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **OpenAPI SDK Generation**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `openapi/openapi.yaml`, `scripts/generate-sdks.sh`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `openapi/openapi.yaml`, `scripts/generate-sdks.sh`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Jobs

### `validate-spec`
**Anzeigename:** Validate OpenAPI Spec

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Cache openapi-generator-cli JAR** — `actions/cache@v4`
- **Download openapi-generator-cli** — `if [ ! -f "$JAR_PATH" ]; then`
- **Validate OpenAPI specification** — `java -jar "$JAR_PATH" validate -i openapi/openapi.yaml`

### `generate-python-sdk`
**Anzeigename:** Generate Python SDK

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `validate-spec`

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Cache openapi-generator-cli JAR** — `actions/cache@v4`
- **Download openapi-generator-cli** — `if [ ! -f "$JAR_PATH" ]; then`
- **Generate Python SDK** — `java -jar "$JAR_PATH" generate \`
- ***(ohne Name)*** — `actions/setup-python@v5`
- **Smoke-test Python SDK import** — `pip install -e openapi/generated/python/ --quiet`
- **Upload Python SDK artifact** — `actions/upload-artifact@v4`

### `generate-javascript-sdk`
**Anzeigename:** Generate JavaScript/TypeScript SDK

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `validate-spec`

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Cache openapi-generator-cli JAR** — `actions/cache@v4`
- **Download openapi-generator-cli** — `if [ ! -f "$JAR_PATH" ]; then`
- **Generate JavaScript/TypeScript SDK** — `java -jar "$JAR_PATH" generate \`
- ***(ohne Name)*** — `actions/setup-node@v4`
- **Build JavaScript SDK** — `npm install --ignore-scripts`
- **Upload JavaScript SDK artifact** — `actions/upload-artifact@v4`

### `generate-go-sdk`
**Anzeigename:** Generate Go SDK

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `validate-spec`

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Cache openapi-generator-cli JAR** — `actions/cache@v4`
- **Download openapi-generator-cli** — `if [ ! -f "$JAR_PATH" ]; then`
- **Generate Go SDK** — `java -jar "$JAR_PATH" generate \`
- ***(ohne Name)*** — `actions/setup-go@v5`
- **Build Go SDK** — `go mod tidy`
- **Upload Go SDK artifact** — `actions/upload-artifact@v4`

### `commit-generated-sdks`
**Anzeigename:** Commit Generated SDKs

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `generate-python-sdk`, `generate-javascript-sdk`, `generate-go-sdk`
**Bedingung:** `github.event_name == 'push' && (github.ref == 'refs/heads/main' || github.ref == 'refs/heads/develop`

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Download all generated SDKs** — `actions/download-artifact@v4`
- **Place SDKs in repository** — `mkdir -p openapi/generated/python openapi/generated/javascript openapi/generated`
- **Commit and push generated SDKs** — `stefanzweifel/git-auto-commit-action@v5`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_sdk-generation.yml)
- [Alle Workflows](../README.md)


