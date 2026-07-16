# ThemisDB Scripts

This directory contains utility scripts for deployment, operations, automation, and release management.

## Directory Structure

- **k8s/** - Kubernetes deployment manifests and Helm charts
- **systemd/** - systemd service files for Linux deployments

## Script Categories

### Milestone Sync from Roadmap

Synchronise GitHub milestones with the `(Target: …)` and `(Issue: #NNN)` annotations
found in every `src/**/ROADMAP.md` file.

- **`sync-milestones-from-roadmap.py`** – Parse roadmaps, create missing milestones
  (Q2 2026 / Q3 2026 / Q4 2026 / Q1 2027, …) and assign open issues to them.

**Usage:**

```bash
# Preview what would happen (no GitHub API writes)
python3 scripts/sync-milestones-from-roadmap.py --dry-run

# Only generate the audit report (docs/issue-milestone-audit.md)
python3 scripts/sync-milestones-from-roadmap.py --audit-only

# Apply changes (requires GITHUB_TOKEN with issues:write scope)
GITHUB_TOKEN=ghp_... python3 scripts/sync-milestones-from-roadmap.py

# Verbose output (one line per issue)
GITHUB_TOKEN=ghp_... python3 scripts/sync-milestones-from-roadmap.py --verbose
```

**Environment variables:**

| Variable | Description |
|----------|-------------|
| `GITHUB_TOKEN` | PAT or `GITHUB_TOKEN` (Actions) with `repo` scope |
| `GITHUB_REPOSITORY` | Fallback `owner/repo`; defaults to `makr-code/ThemisDB` |

**Audit report:** `docs/issue-milestone-audit.md` is generated automatically and lists
all 665 issue references found in roadmaps, including the 550 issues that lack an
explicit `(Target: …)` annotation and require manual milestone assignment.

**Tests:** `tests/test_sync_milestones.py`

```bash
python3 -m pytest tests/test_sync_milestones.py -v
```

### Issue Sync from Consolidated Roadmap

Create and reconcile GitHub issues from the consolidated [src/ROADMAP.md](../src/ROADMAP.md)
backlog. The script resolves each roadmap row to the linked
`src/<module>/FUTURE_ENHANCEMENTS.md` section, extracts acceptance criteria, and
can backfill issue references into the roadmap after successful creation. Generated
issues use governance-aligned `area:*`, `priority:*`, `type:*`, and `status:*`
labels and render the mandatory `Context`, `Goal`, `Acceptance Criteria`,
`Relationships`, and `References` sections in the issue body.

- **`sync-issues-from-roadmap.py`** – Preview, apply, and backfill roadmap issues.

**Usage:**

```bash
# Generate issue bodies and preview reports only
python3 scripts/sync-issues-from-roadmap.py --mode preview

# Limit to a specific module or priority for batching
python3 scripts/sync-issues-from-roadmap.py --mode preview --module auth --limit 5
python3 scripts/sync-issues-from-roadmap.py --mode preview --priority critical

# Create missing issues with gh CLI and write Issue references back to src/ROADMAP.md
python3 scripts/sync-issues-from-roadmap.py --mode apply --backfill

# Backfill later from a prior apply manifest
python3 scripts/sync-issues-from-roadmap.py --mode backfill \
  --manifest artifacts/roadmap-issues/roadmap-issues-apply.json

# Optional: run the same flow via workflow_dispatch in GitHub Actions
gh workflow run sync-roadmap-issues.yml -f mode=preview -f priority=critical -f limit=10
```

**Outputs:**

- `artifacts/roadmap-issues/roadmap-issues-preview.json`
- `artifacts/roadmap-issues/roadmap-issues-preview.md`
- `artifacts/roadmap-issues/bodies/*.md`
- `artifacts/roadmap-issues/roadmap-issues-apply.json`
- `artifacts/roadmap-issues/roadmap-issues-summary.json`
- `artifacts/roadmap-issues/roadmap-issues-summary.md`

The summary artifacts are written automatically after `preview`, `apply`, and
`backfill` runs and contain per-priority totals plus any remaining roadmap rows
without an issue reference.

**Tests:** `tests/test_sync_issues_from_roadmap.py`

```bash
python3 -m pytest tests/test_sync_issues_from_roadmap.py -v
```

---

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
- `prepare_release_mini_llm.py` - Prepare a small GGUF release bundle for llama.cpp (`default.gguf` + manifest)
- `run-llm-benchmarks.ps1` - Execute LLM inferencing benchmarks
- `setup-llm-benchmarks.ps1` - Complete workflow: download + build + benchmark

### Release Asset Helpers

Scripts for release packaging with bundled documentation and LLM assets:

- `generate_docs_database.py` - Generate the precompiled documentation JSON database
- `generate_docs_rocksdb.py` - Generate importer code for docs.db / RocksDB documentation bundle
- `prepare_release_mini_llm.py` - Download or stage a small GGUF model as `models/default.gguf`

**Quick Start:**
```bash
# Prepare release mini model bundle
python3 scripts/prepare_release_mini_llm.py --output-dir release/models

# Generate documentation database JSON
python3 scripts/generate_docs_database.py --output data/docs_database.json
```

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

### Local Quality Gate (Windows)

Run a local end-to-end quality flow (build, tests, static analysis, security scan, docs):

- **`quality-gate.ps1`** - PowerShell runner with per-step skip switches and report output
- **`quality_gate_gui.py`** - Tkinter desktop launcher for `quality-gate.ps1`

**Quick Start:**
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1
python .\scripts\quality_gate_gui.py
```

Detailed guide: [docs/development/LOCAL_QUALITY_GATE.md](../docs/development/LOCAL_QUALITY_GATE.md)

**CI/CD Integration:**
- Automated via `.github/workflows/documentation-validation.yml`
- Runs on PRs and pushes to main/develop branches
- Blocks merge if validation fails

**Documentation:**
- [Documentation Validation Guide (English)](../docs/DOCUMENTATION_VALIDATION.md)
- [Dokumentations-Validierung (Deutsch)](../docs/de/DOKUMENTATIONS_VALIDIERUNG.md)

### Legal Training Data Ingestion (Updated v2.0)

Scripts for automatically downloading and importing German legal training data from multiple HuggingFace sources with automatic fallback:

- **`ingest_legal_training_data.py`** - Download and convert German legal datasets from HuggingFace or local files
- **`generate_legal_rocksdb.py`** - Generate C++ RocksDB importer for legal training data

**⚠️ IMPORTANT UPDATE**: The original dataset `joelito/legal_mc_de` is no longer available. The system now supports multiple alternative datasets with automatic fallback.

**Dataset Information:**
- **Primary**: `joelNiklaus/MultiLegalPile` (German subset) - **RECOMMENDED**
  - URL: https://huggingface.co/datasets/joelNiklaus/MultiLegalPile
- **Alternative 1**: `elenanereiss/german-ler`
  - URL: https://huggingface.co/datasets/elenanereiss/german-ler
- **Alternative 2**: Local custom datasets (JSON format)
- **Language**: German (de)
- **Domain**: Legal documents
- **Default samples**: 10,000

**Features:**
- **Automatic fallback** across multiple datasets
- Dataset availability checking
- Local file support for custom datasets
- ThemisDB JSON format conversion
- RocksDB database generation
- CMake build integration

**Quick Start:**
```bash
# List available datasets
python3 scripts/ingest_legal_training_data.py --list-datasets

# Download using automatic fallback (tries MultiLegalPile first)
python3 scripts/ingest_legal_training_data.py \
    --output data/legal_training_data.json \
    --max-samples 10000

# Specify a specific dataset
python3 scripts/ingest_legal_training_data.py \
    --dataset joelNiklaus/MultiLegalPile \
    --max-samples 10000

# Use a local dataset
python3 scripts/ingest_legal_training_data.py \
    --local-file custom_legal_data.json

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
