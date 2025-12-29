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
