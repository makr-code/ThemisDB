# eID Authenticator CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/eid-authenticator-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **eID Authenticator**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `include/auth/eid_authenticator.h`, `tests/test_eid_authenticator.cpp`, `tests/CMakeLists.txt`, `.github/workflows/eid-authenticator-ci.yml`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `include/auth/eid_authenticator.h`, `tests/test_eid_authenticator.cpp`, `tests/CMakeLists.txt`, `.github/workflows/eid-authenticator-ci.yml`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `eid-authenticator-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `eid-authenticator`
**Anzeigename:** eID Authenticator – ${{ matrix.os }} / ${{ matrix.compiler }}

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install C++ build environment** — `sudo apt-get update -qq`
- **Configure focused tests** — `cmake -S tests -B build_eid \`
- **Build focused test binary** — `cmake --build build_eid --target test_eid_authenticator_focused -- -j$(nproc)`
- **Run via ctest** — `ctest --output-on-failure \`
- **Run tests directly** — `./test_eid_authenticator_focused \`
- **Upload test artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## eID Authenticator CI — ${{ matrix.os }} / ${{ matrix.compiler }}" >> $G`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/eid-authenticator-ci.yml)
- [Alle Workflows](README.md)


