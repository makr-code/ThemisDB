> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Tools (`tools/`)

Werkzeuge für Betrieb, Doku-Automation, Analyse und Daten-Ingestion.

## Relevante CLI-Tools (verifizierte Pfade)

- `tools/themisctl.cpp` – CLI für Server-/API-Operationen
- `tools/ingest.py` – Datei-Ingestion
- `tools/ldap_export.py` – LDAP/AD-Export
- `tools/link_ownership.py` – Ownership-Kanten
- `tools/namespace_analyzer.py` – Namespace-Analyse
- `tools/primary_docs_indexer.py` – Primärdoku-Inventar
- `tools/module_docs_builder.py` – Generierung `docs/*/PRIMARY_SOURCES.md`
- `tools/ci/*.py` – CI-/Doku-Automationsskripte

## Relevante GUI-Tools

- `tools/themis_config_editor/` – bestehender Tkinter-Konfigurationseditor
- `tools/themis_config_wx/` – optionaler nativer wxWidgets-Konfigurationseditor (Tabbed Layout)

Build-Flag fuer den wxWidgets-Editor:

- `-DTHEMIS_BUILD_TOOLS=ON` (Default)

## Dry-Run / Selbsttest-Kommandos

```bash
python3 tools/primary_docs_indexer.py --help
python3 tools/module_docs_builder.py --dry-run
python3 tools/namespace_analyzer.py --help
python3 tools/ci/module_docs_issue_reporter.py --help
```

## Installation

Die meisten Tools laufen mit dem bestehenden Repo-Python (`python3`) ohne separate Paketinstallation; zusätzliche Abhängigkeiten sind tool-spezifisch dokumentiert.

## Usage

CLI-Parameter und Workflows sind über `--help` der jeweiligen Tools verfügbar.

## Navigation

- Tests für Tooling: [`tests/README.md`](tests/README.md)
- Root-Index: [`../INDEX.md`](../INDEX.md)
