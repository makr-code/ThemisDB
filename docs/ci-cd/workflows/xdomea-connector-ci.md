# XDOMEA Connector CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/xdomea-connector-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **XDOMEA Connector**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `include/document/xdomea_connector.h`, `tests/test_xdomea_connector.cpp`, `tests/CMakeLists.txt`, `.github/workflows/xdomea-connector-ci.yml`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `include/document/xdomea_connector.h`, `tests/test_xdomea_connector.cpp`, `tests/CMakeLists.txt`, `.github/workflows/xdomea-connector-ci.yml`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `xdomea-connector-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `xdomea-connector`
**Anzeigename:** XDOMEA Connector – ${{ matrix.os }} / ${{ matrix.compiler }}

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install C++ build environment** — `sudo apt-get update -qq`
- **Configure focused tests** — `cmake -S tests -B build_xdomea \`
- **Build focused test binary** — `cmake --build build_xdomea --target test_xdomea_connector_focused -- -j$(nproc)`
- **Run via ctest** — `ctest --output-on-failure \`
- **Run tests directly** — `./test_xdomea_connector_focused \`
- **Upload test artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## XDOMEA Connector CI — ${{ matrix.os }} / ${{ matrix.compiler }}" >> $GI`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/xdomea-connector-ci.yml)
- [Alle Workflows](README.md)


