# HttpServer ShardingManager CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/06-infrastructure/distributed/httpserver-shardingmanager-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **HttpServer ShardingManager**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (9 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (9 überwachte Pfade)

## Nebenläufigkeit

- **Gruppe:** `httpserver-shardingmanager-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `httpserver-shardingmanager-tests`
**Anzeigename:** HttpServer ShardingManager (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run HttpShardingAdminTest** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure/distributed/httpserver-shardingmanager-ci.yml)
- [Alle Workflows](../README.md)
