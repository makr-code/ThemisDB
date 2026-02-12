# ThemisDB Scripts

This directory contains utility scripts for deployment, operations, automation, and release management.

## Directory Structure

- **k8s/** - Kubernetes deployment manifests and Helm charts
- **systemd/** - systemd service files for Linux deployments

## Script Categories

### Release Scripts

#### Retroactive Release Builder (NEW)
Build binaries for historical version tags retroactively.

- **`retroactive-release-builder.sh`** - Linux/macOS version
- **`retroactive-release-builder.ps1`** - Windows (PowerShell) version

**Purpose:** Extract source code at specific version tags and build/package binaries for all past releases.

**Documentation:** See [RETROACTIVE_RELEASE_BUILDING.md](../docs/RETROACTIVE_RELEASE_BUILDING.md)

**Quick Start:**
```bash
# List available tags
./scripts/retroactive-release-builder.sh --list-tags

# Build specific tag
./scripts/retroactive-release-builder.sh --tag v1.3.4 --platform linux

# Build all tags
./scripts/retroactive-release-builder.sh --all-tags
```

#### Standard Release Scripts
- **`orchestrate-release.ps1`** - Multi-edition release orchestrator
- **`build-community-release.ps1`** - Build Community Edition
- **`build-enterprise-release.ps1`** - Build Enterprise Edition
- **`build-hyperscaler-release.ps1`** - Build Hyperscaler Edition
- **`prepare-release.sh`** / **`prepare-release.ps1`** - Prepare releases
- **`create-github-release.ps1`** - Create GitHub releases
- **`build-release-packages.sh`** - Build Linux packages

### Complete Release (Git Flow)

Automate the entire Git Flow release process: `develop` → `release/vX.X.X` → `main` (+ tag) → retroactive build

- **`complete-release.sh`** - Linux/macOS automated release
- **`complete-release.ps1`** - Windows automated release

**Purpose:** Complete release workflow following Git Flow branching strategy

**Documentation:** See [RETROACTIVE_RELEASE_GITFLOW.md](../docs/RETROACTIVE_RELEASE_GITFLOW.md)

**Quick Start:**
```bash
# Complete release workflow
./scripts/complete-release.sh 1.5.0

# Dry run to preview
./scripts/complete-release.sh 1.5.0 --dry-run

# Skip retroactive build
./scripts/complete-release.sh 1.5.0 --skip-build
```

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

### Documentation Validation Scripts (New in v1.4.0+)

Scripts for automated documentation quality assurance and validation:

- **`docs-lint.py`** - Lint markdown documentation for structure and syntax issues
- **`link-check.py`** - Validate internal and external links
- **`toc-check.py`** - Validate table of contents in MkDocs configuration
- **`validate-docs.sh`** - Run all documentation validation checks

**Features:**
- Heading hierarchy validation
- Markdown syntax checking
- Link validation (internal and external)
- TOC consistency checking
- File naming convention validation
- Cross-reference validation

**Quick Start:**
```bash
# Run all validation checks
./scripts/validate-docs.sh

# Run individual checks
python3 scripts/docs-lint.py
python3 scripts/link-check.py
python3 scripts/toc-check.py

# Generate JSON reports for CI/CD
python3 scripts/docs-lint.py --format json --output lint-report.json
```

**CI/CD Integration:**
- Automated via `.github/workflows/documentation-validation.yml`
- Runs on PRs and pushes to main/develop branches
- Blocks merge if validation fails

**Documentation:**
- [Documentation Validation Guide (English)](../docs/DOCUMENTATION_VALIDATION.md)
- [Dokumentations-Validierung (Deutsch)](../docs/de/DOKUMENTATIONS_VALIDIERUNG.md)

### Legal Training Data Ingestion (New in v1.4.0+)

Scripts for automatically downloading and importing legal training data from HuggingFace:

- **`ingest_legal_training_data.py`** - Download and convert HuggingFace legal-bert-de dataset
- **`generate_legal_rocksdb.py`** - Generate C++ RocksDB importer for legal training data

**Dataset Information:**
- **Dataset**: `joelito/legal_mc_de`
- **URL**: https://huggingface.co/datasets/joelito/legal_mc_de
- **Language**: German (de)
- **Domain**: Legal documents
- **Default samples**: 10,000

**Features:**
- Automatic HuggingFace dataset download
- Dataset split validation
- ThemisDB JSON format conversion
- RocksDB database generation
- CMake build integration

**Quick Start:**
```bash
# Download and convert dataset to JSON
python3 scripts/ingest_legal_training_data.py \
    --output data/legal_training_data.json \
    --max-samples 10000

# Generate RocksDB importer
python3 scripts/generate_legal_rocksdb.py \
    --method cpp \
    --output data/legal_training.db
```

**CMake Integration:**
```bash
# Enable during build
cmake -B build -DTHEMIS_BUILD_LEGAL_TRAINING_DATA=ON

# Build the database
cmake --build build --target legal_training_data
```

**Documentation:**
- [Legal Training Data Build Guide](../docs/en/development/LEGAL_TRAINING_DATA_BUILD.md)

### Security & Compliance Scripts

Scripts for security auditing and compliance verification:

#### Comprehensive Code Audit

- **`comprehensive-code-audit.sh`** - Systematic security and compliance audit script

**Purpose:** Automate comprehensive security analysis covering SAST, dependency scanning, secret detection, container security, and dynamic analysis.

**Compliance Coverage:**
- BSI C5 (Cloud Computing Compliance Criteria Catalogue)
- ISO/IEC 27001 (Information Security Management)
- DSGVO/GDPR (EU Data Protection Regulation)
- NIS2 (Network and Information Security Directive)
- OWASP ASVS (Application Security Verification Standard)
- NIST Cybersecurity Framework

**Features:**
- Static Application Security Testing (SAST)
  - cppcheck - C++ static analysis
  - clang-tidy - Modern C++ linting and security checks
  - Semgrep - Pattern-based security scanning
- Dependency & Supply Chain Security
  - Trivy - Vulnerability scanning
  - vcpkg dependency inventory
- Secret Detection
  - Gitleaks - Credential and API key detection
- Container Security
  - Dockerfile security analysis
- Dynamic Analysis
  - Integration with Valgrind, ASAN, TSAN
  - Recommendations for runtime testing

**Quick Start:**
```bash
# Full comprehensive audit
./scripts/comprehensive-code-audit.sh

# Quick audit (skip time-consuming checks)
AUDIT_QUICK=1 ./scripts/comprehensive-code-audit.sh

# Audit specific categories
./scripts/comprehensive-code-audit.sh --skip-dependencies --skip-dynamic

# Continue even if issues found
./scripts/comprehensive-code-audit.sh --continue-on-error

# View all options
./scripts/comprehensive-code-audit.sh --help
```

**Output:**
- Audit results directory: `audit-results-<timestamp>/`
- Comprehensive report: `audit-results-<timestamp>/comprehensive-audit-report.md`
- Category-specific reports in subdirectories (sast/, dependencies/, secrets/, etc.)

**CI/CD Integration:**
- Referenced in security audit workflows
- Can be triggered manually via GitHub Actions

**Documentation:**
- [SECURITY.md](../SECURITY.md) - Security scanning section
- [Security Compliance Investigation Template](../.github/ISSUE_TEMPLATE/security-compliance-investigation.md)
- [Full Audit Checklist](../docs/de/compliance/compliance_full_checklist.md)
- [Audit TODO List](../docs/de/compliance/compliance_audit_todo.md)

**Prerequisites:**
```bash
# Ubuntu/Debian
sudo apt-get install cmake git build-essential cppcheck clang-tidy

# Install optional tools for full coverage
# Trivy - https://aquasecurity.github.io/trivy/
# Gitleaks - https://github.com/gitleaks/gitleaks
# Semgrep - https://semgrep.dev/
```

## Usage

Each script includes documentation in the header comments. Run scripts with `-h` or `--help` for usage information where applicable.

## Documentation

For detailed deployment and operations documentation, see:
- [Deployment Guide](../docs/deployment.md)
- [Operations Runbook](../docs/operations_runbook.md)
- [TLS Setup](../docs/TLS_SETUP.md)
