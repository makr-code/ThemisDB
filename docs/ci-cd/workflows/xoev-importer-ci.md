# XÖV Importer CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/xoev-importer-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **XÖV Importer**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `include/importers/xoev_importer.h`, `tests/test_xoev_importer.cpp`, `tests/CMakeLists.txt`, `.github/workflows/xoev-importer-ci.yml`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `include/importers/xoev_importer.h`, `tests/test_xoev_importer.cpp`, `tests/CMakeLists.txt`, `.github/workflows/xoev-importer-ci.yml`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `xoev-importer-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `xoev-importer`
**Anzeigename:** XÖV Importer – ${{ matrix.os }} / ${{ matrix.compiler }}

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install C++ build environment** — `sudo apt-get update -qq`
- **Configure focused tests** — `cmake -S tests -B build_xoev \`
- **Build focused test binary** — `cmake --build build_xoev --target test_xoev_importer_focused -- -j$(nproc)`
- **Run via ctest** — `ctest --output-on-failure \`
- **Run tests directly** — `./test_xoev_importer_focused \`
- **Upload test artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## XÖV Importer CI — ${{ matrix.os }} / ${{ matrix.compiler }}" >> $GITHUB`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/xoev-importer-ci.yml)
- [Alle Workflows](README.md)


