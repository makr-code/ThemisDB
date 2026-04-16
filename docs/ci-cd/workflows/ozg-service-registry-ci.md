# OZG Service Registry CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/ozg-service-registry-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **OZG Service Registry**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `include/importers/ozg_service_registry.h`, `tests/test_ozg_service_registry.cpp`, `tests/CMakeLists.txt`, `.github/workflows/ozg-service-registry-ci.yml`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `include/importers/ozg_service_registry.h`, `tests/test_ozg_service_registry.cpp`, `tests/CMakeLists.txt`, `.github/workflows/ozg-service-registry-ci.yml`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `ozg-service-registry-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `ozg-service-registry`
**Anzeigename:** OZG Service Registry – ${{ matrix.os }} / ${{ matrix.compiler }}

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install C++ build environment** — `sudo apt-get update -qq`
- **Configure focused tests** — `cmake -S tests -B build_ozg \`
- **Build focused test binary** — `cmake --build build_ozg --target test_ozg_service_registry_focused -- -j$(nproc)`
- **Run via ctest** — `ctest --output-on-failure \`
- **Run tests directly** — `./test_ozg_service_registry_focused \`
- **Upload test artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## OZG Service Registry CI — ${{ matrix.os }} / ${{ matrix.compiler }}" >>`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/ozg-service-registry-ci.yml)
- [Alle Workflows](README.md)


