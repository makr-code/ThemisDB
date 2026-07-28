# CHIMERA Docker Image Scope

Dieses Dokument grenzt verbindlich ab, was im DockerHub-Image enthalten sein soll.

## Ziel des Images

Das DockerHub-Image `themisdb/chimera` ist ein Runtime-Image fuer:

- den Endurance-Runner (`run_endurance.py`, Standard-ENTRYPOINT),
- den CLI-Laufzeitpfad (`chimera_cli.py`) per Command-Override,
- den optionalen Dashboard/API-Server (`chimera_server.py`) per Command-Override.

## Enthalten (In-Scope)

Runtime-Python-Module:

- `run_endurance.py`
- `chimera_cli.py`
- `chimera_server.py`
- `benchmark_harness.py`
- `benchmark_standards.py`
- `reporter.py`
- `statistics.py`
- `colors.py`
- `citations.py`
- `scoring.py`
- `dashboard.py`
- `exporter.py`
- `__init__.py`

Konfigurationsbeispiel zur Laufzeit-Discovery:

- `adapters/chimera/adapter_config_example.toml`

## Nicht enthalten (Out-of-Scope)

Nicht fuer die Laufzeit noetige Inhalte bleiben ausserhalb des Images:

- Tests (`tests/`, `test_*.py`)
- Dokumentation (`docs/`, weitere Markdown-Referenztexte)
- Demo-Outputs (`demo_reports/`)
- Entwicklungsumgebungen/Caches (`.venv/`, `__pycache__/`, `.pytest_cache/`)
- Lokale Betriebsdaten (`endurance_results/`, `certs/`, `.env*`)

## Abgrenzung Repository vs Runtime

- Das Repository bleibt die vollstaendige Quelle fuer Entwicklung, Tests, Doku und Release-Skripte.
- Das DockerHub-Image ist bewusst auf Runtime-Artefakte reduziert.

## Release-Empfehlung

Vor Push nach DockerHub:

1. Import-Smoketest (`python -c "import run_endurance, chimera_cli, chimera_server"`)
2. Build-Test (`docker build -t chimera-benchmark .`)
3. Optional Dry-Run des Release-Skripts (`scripts/release_dockerhub.ps1 -DryRun`)
