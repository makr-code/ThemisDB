# ThemisDB Scripts

This directory contains utility scripts for deployment, operations, and automation.

## Directory Structure

- **k8s/** - Kubernetes deployment manifests and Helm charts
- **systemd/** - systemd service files for Linux deployments

## Script Categories

### Deployment Scripts
Scripts for deploying ThemisDB in various environments.

### Operations Scripts
Scripts for database operations, maintenance, and monitoring.

### Development Scripts
Build and development automation scripts are located in the project root:
- `build.sh` / `build.ps1` - Build scripts
- `setup.sh` / `setup.ps1` - Development environment setup
- `sync-wiki.ps1` - Wiki synchronization

### Documentation Scripts (New)
Scripts for maintaining documentation consistency:
- `add_doc_metadata.py` - Automatically add structured YAML metadata to markdown files

**Usage:**
```bash
# Dry-run mode (preview what would change)
python3 scripts/add_doc_metadata.py --dry-run

# Add metadata to all markdown files
python3 scripts/add_doc_metadata.py

# Add metadata to specific files
python3 scripts/add_doc_metadata.py --files README.md CONTRIBUTING.md
```

The script adds structured metadata including:
- Author (Themis DevTeam & Copilot)
- Document number (release tag or date)
- Creation and modification dates (from git history)
- First commit title
- Document title (first markdown heading)
- File path

See the [add-doc-metadata workflow](../.github/workflows/add-doc-metadata.yml) for automated execution.

### LLM & Benchmarking Scripts (New in v1.3.0+)
Scripts for managing LLM models and running inferencing benchmarks:
- `download-ollama-models.ps1` - Download models from Ollama and convert to GGUF
- `run-llm-benchmarks.ps1` - Execute LLM inferencing benchmarks
- `setup-llm-benchmarks.ps1` - Complete workflow: download + build + benchmark

**Quick Start:**
```powershell
# Download models and run benchmarks (all-in-one)
.\scripts\setup-llm-benchmarks.ps1

# Or step by step:
.\scripts\download-ollama-models.ps1 -ModelNames @("llama3.2:1b", "phi3:mini")
.\scripts\run-llm-benchmarks.ps1
```

See [LLM Benchmarking Guide](../docs/LLM_BENCHMARKING_GUIDE.md) for details.

## Usage

Each script includes documentation in the header comments. Run scripts with `-h` or `--help` for usage information where applicable.

## Documentation

For detailed deployment and operations documentation, see:
- [Deployment Guide](../docs/deployment.md)
- [Operations Runbook](../docs/operations_runbook.md)
- [TLS Setup](../docs/TLS_SETUP.md)

---

## Dokumenten-Metadaten

```yaml
Urheber: Themis DevTeam & Copilot
Dokumenten-Nr: Stand: 2026-02-17
Erstelldatum: 2026-02-17
Letzte Änderung: 2026-02-17
Commit-Titel: "Create documentation for build simplification proposals in ThemisDB repository."
Reviewer: 
Titel: "ThemisDB Scripts"
Dateipfad: scripts/README.md
```
