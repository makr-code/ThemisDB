# RedisCacheCoordinator Async Pub/Sub CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_resilience_redis-cache-coordinator-async-loop-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **RedisCacheCoordinator Async Pub/Sub**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (10 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (10 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `redis-cache-coordinator-tests`
**Anzeigename:** RedisCacheCoordinator (${{ matrix.name }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (RedisCacheCoordinator test targets)** — `cmake -B build -G Ninja \`
- **Build coordinator test binary** — `cmake --build build \`
- **Run RedisCacheCoordinator tests** — `cd build`
- **Build cache admin API handler test binary** — `cmake --build build \`
- **Run cache admin health endpoint tests** — `cd build`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_resilience_redis-cache-coordinator-async-loop-ci.yml)
- [Alle Workflows](../README.md)


