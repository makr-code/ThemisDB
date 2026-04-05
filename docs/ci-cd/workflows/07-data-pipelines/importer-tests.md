# Importer Module Tests

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/07-data-pipelines_importer-tests.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Importer Module Tests**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (18 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (18 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `importer-unit-tests`
**Anzeigename:** Importer unit tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (importer tests only)** — `cmake -B build -G Ninja \`
- **Build test binary** — `cmake --build build --target themis_tests -- -j$(nproc)`
- **Run importer unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📥 Importer Tests – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

### `postgres-live-integration`
**Anzeigename:** PostgreSQL live integration tests

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Create test schema and seed data in PostgreSQL** — `psql -h localhost -U themis -d themis_test <<'EOSQL'`
- **Dump live database with pg_dump** — `pg_dump -h localhost -U themis -d themis_test \`
- **Configure (importer tests only)** — `cmake -B build -G Ninja \`
- **Build test binary** — `cmake --build build --target themis_tests -- -j$(nproc)`
- **Run live PostgreSQL integration tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📥 Importer Tests – Live PostgreSQL Integration" >> "$GITHUB_STEP_SUMMAR`

### `mongodb-live-integration`
**Anzeigename:** MongoDB live integration tests

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Seed MongoDB with test data** — `# Wait for MongoDB to be ready`
- **Export collections using mongoexport (NDJSON)** — `mongoexport \`
- **Configure (importer tests only)** — `cmake -B build -G Ninja \`
- **Build test binary** — `cmake --build build --target themis_tests -- -j$(nproc)`
- **Run MongoDB importer tests against live export** — `cd build`
- **Verify BSON type conversions in export** — `# Verify that ObjectId -> string, ISODate -> ISO 8601, Decimal128 -> string`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📥 Importer Tests – Live MongoDB Integration" >> "$GITHUB_STEP_SUMMARY"`

### `mysql-live-integration`
**Anzeigename:** MySQL live integration tests

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Create test schema and seed data in MySQL** — `mysql -h 127.0.0.1 -u root themis_test <<'EOSQL'`
- **Dump live database with mysqldump** — `mysqldump -h 127.0.0.1 -u root \`
- **Configure (importer tests only)** — `cmake -B build -G Ninja \`
- **Build test binary** — `cmake --build build --target themis_tests -- -j$(nproc)`
- **Run MySQL importer tests** — `cd build`
- **Verify dump structure** — `python3 - <<'PYEOF'`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📥 Importer Tests – Live MySQL Integration" >> "$GITHUB_STEP_SUMMARY"`

### `docs-lint`
**Anzeigename:** Lint importer docs

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true' || needs.ci-scope-classifier.outputs.ha`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install markdownlint** — `npm install -g markdownlint-cli`
- **Lint importers_roadmap.md** — `markdownlint docs/importers_roadmap.md --config .markdownlint.json`
- **Lint importers_runbook.md** — `markdownlint docs/importers_runbook.md --config .markdownlint.json`
- **Verify fixtures exist** — `for f in \`
- **Write job summary** — `echo "## 📝 Importer Tests – Docs Lint" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/07-data-pipelines_importer-tests.yml)
- [Alle Workflows](../README.md)


