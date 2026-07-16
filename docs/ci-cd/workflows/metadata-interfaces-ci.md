# Metadata Interfaces CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/metadata-interfaces-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Metadata Interfaces**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (8 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (8 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `metadata-interfaces-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `metadata-interfaces`
**Anzeigename:** Metadata Interfaces – ${{ matrix.os }} / ${{ matrix.compiler }}

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install C++ build environment** — `sudo apt-get update -qq`
- **Configure focused tests** — `cmake -S tests -B build_metadata_interfaces \`
- **Build focused test binaries** — `cmake --build build_metadata_interfaces --target \`
- **Run via ctest** — `ctest --output-on-failure \`
- **Run MetadataSecurityProvider tests directly** — `./test_metadata_security_provider_focused \`
- **Run MetadataChangeListener tests directly** — `./test_metadata_change_listener_focused \`
- **Run MetadataExportPolicy tests directly** — `./test_metadata_export_policy_focused \`
- **Upload test artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## Metadata Interfaces CI — ${{ matrix.os }} / ${{ matrix.compiler }}" >> `

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/metadata-interfaces-ci.yml)
- [Alle Workflows](README.md)


