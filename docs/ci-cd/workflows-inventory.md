# GitHub Actions Workflows Inventory

**Generated:** 2026-02-21 07:02:11

**Total Workflows:** 12

## Table of Contents

1. [Summary Statistics](#summary-statistics)
2. [Workflows by Category](#workflows-by-category)
3. [Detailed Workflow Inventory](#detailed-workflow-inventory)
4. [Common Patterns Analysis](#common-patterns-analysis)
5. [Actions Usage Summary](#actions-usage-summary)

## Summary Statistics

- **Total Workflows:** 12
- **Total Jobs:** 23
- **Unique Actions Used:** 6

### Workflows by Category

- **Code Quality & Versioning:** 1 workflows
- **Other:** 2 workflows
- **PR CI:** 4 workflows
- **Release:** 1 workflows
- **Scheduled & Manual:** 2 workflows
- **Testing:** 2 workflows

## Workflows by Category

### Code Quality & Versioning

- **code-maturity-analysis.yml** - Code Maturity Analysis & Auto-Versioning

### Other

- **validate-ai-guardrails.yml** - Validate AI-Guardrails
- **validate-config-mapping.yml** - Validate Config Mapping

### PR CI

- **gpu-ci.yml** - GPU Module CI Gate
- **llm-cpu-fallback-ci.yml** - LLM CPU Fallback CI
- **llm-cuda-gpu-ci.yml** - LLM CUDA GPU CI
- **themis-core-ci.yml** - Themis Core Framework CI

### Release

- **create-release-archive.yml** - [Manual] Create Release Archive

### Scheduled & Manual

- **pii-redaction-check.yml** - PII Redaction Policy Check
- **validate-grafana-dashboards.yml** - Validate Grafana Dashboards

### Testing

- **chimera-tests.yml** - CHIMERA Suite Tests
- **importer-tests.yml** - Importer Module Tests

## Detailed Workflow Inventory

### chimera-tests.yml

**Name:** CHIMERA Suite Tests

**Path:** `.github/workflows/chimera-tests.yml`

**Triggers:**
- Events: push, pull_request, workflow_dispatch
- Paths: 4 path filters

**Jobs (1):**
- `test`: Run CHIMERA Tests (runs-on: ubuntu-latest)

**Technologies:** Python

**Actions Used (3):**
- `actions/checkout`
- `actions/setup-python`
- `codecov/codecov-action`

---

### code-maturity-analysis.yml

**Name:** Code Maturity Analysis & Auto-Versioning

**Path:** `.github/workflows/08-maintenance_code-maturity-analysis.yml`

**Triggers:**
- Events: push, schedule, workflow_dispatch
- Branches: main, develop, feature/**
- Paths: src/**, include/**, tests/**, benchmarks/**, projects/**, plugins/**, script/workflow self-triggers

**Jobs (1):**
- `analyze-and-version`: Code Maturity Analysis & Auto-Versioning (runs-on: ubuntu-latest)

**Technologies:** PHP, Python

**Permissions:**
- contents: write

**Actions Used (2):**
- `actions/checkout`
- `actions/setup-python`

---

### 04-release_create-release-archive.yml

**Name:** [Manual] Create Release Archive

**Path:** `.github/workflows/04-release_create-release-archive.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (1):**
- `create-release-archive`: create-release-archive (runs-on: ubuntu-latest)

**Technologies:** CMake

**Permissions:**
- contents: write

**Actions Used (2):**
- `actions/checkout`
- `softprops/action-gh-release`

---

### gpu-ci.yml

**Name:** GPU Module CI Gate

**Path:** `.github/workflows/gpu-ci.yml`

**Triggers:**
- Events: pull_request, push, workflow_dispatch
- Branches: main, develop
- Paths: 18 path filters

**Jobs (4):**
- `gpu-module-cpu-fallback`: GPU Module (CPU Fallback, ${{ matrix.edition }}) (runs-on: ubuntu-latest)
- `gpu-static-analysis`: GPU Static Analysis (clang-tidy) (runs-on: ubuntu-latest)
- `gpu-device-loss-simulation`: GPU Device Loss Simulation (runs-on: ubuntu-latest)
- `gpu-docs-gate`: GPU Documentation Gate (runs-on: ubuntu-latest)

**Technologies:** CMake, Go, Node.js

**Actions Used (1):**
- `actions/checkout`

---

### importer-tests.yml

**Name:** Importer Module Tests

**Path:** `.github/workflows/importer-tests.yml`

**Triggers:**
- Events: push, pull_request, workflow_dispatch
- Paths: 22 path filters

**Jobs (2):**
- `importer-unit-tests`: Importer unit tests (${{ matrix.os }} / ${{ matrix.compiler }}) (runs-on: ${{ matrix.os }})
- `docs-lint`: Lint importer docs (runs-on: ubuntu-latest)

**Technologies:** CMake, Node.js

**Actions Used (2):**
- `actions/checkout`
- `actions/upload-artifact`

---

### llm-cpu-fallback-ci.yml

**Name:** LLM CPU Fallback CI

**Path:** `.github/workflows/llm-cpu-fallback-ci.yml`

**Triggers:**
- Events: pull_request, push, workflow_dispatch
- Branches: main, develop
- Paths: 12 path filters

**Jobs (1):**
- `llm-cpu-fallback`: LLM Kernel Fusion (CPU Fallback) (runs-on: ubuntu-latest)

**Technologies:** CMake

**Actions Used (1):**
- `actions/checkout`

---

### llm-cuda-gpu-ci.yml

**Name:** LLM CUDA GPU CI

**Path:** `.github/workflows/llm-cuda-gpu-ci.yml`

**Triggers:**
- Events: pull_request, push, workflow_dispatch
- Branches: main, develop
- Paths: 10 path filters

**Jobs (2):**
- `cuda-compile-check`: CUDA Kernel Compile Check (nvcc, no GPU) (runs-on: ubuntu-22.04)
- `cuda-kernel-tests`: CUDA Kernel Tests (GPU Runner) (runs-on: self-hosted, gpu-cuda)

**Technologies:** CMake

**Actions Used (1):**
- `actions/checkout`

---

### pii-redaction-check.yml

**Name:** PII Redaction Policy Check

**Path:** `.github/workflows/pii-redaction-check.yml`

**Triggers:**
- Events: push, pull_request, workflow_dispatch
- Branches: main, develop, feature/**, bugfix/**, copilot/**
- Paths: 14 path filters

**Jobs (3):**
- `pii-leakage-lint`: PII Leakage Static Lint (runs-on: ubuntu-latest)
- `pii-pattern-config-validation`: PII Pattern Config Validation (runs-on: ubuntu-latest)
- `pii-docs-check`: PII Documentation Presence Check (runs-on: ubuntu-latest)

**Technologies:** Python

**Permissions:**
- contents: read

**Actions Used (2):**
- `actions/checkout`
- `actions/setup-python`

---

### themis-core-ci.yml

**Name:** Themis Core Framework CI

**Path:** `.github/workflows/themis-core-ci.yml`

**Triggers:**
- Events: push, pull_request, workflow_dispatch
- Paths: 46 path filters

**Jobs (3):**
- `core-tests`: Core tests (${{ matrix.os }} / ${{ matrix.compiler }}) (runs-on: ${{ matrix.os }})
- `coverage`: Coverage report (ubuntu-22.04 / gcc-12) (runs-on: ubuntu-22.04)
- `docs-lint`: Docs lint (runs-on: ubuntu-latest)

**Technologies:** CMake, vcpkg

**Actions Used (2):**
- `actions/checkout`
- `actions/upload-artifact`

---

### validate-ai-guardrails.yml

**Name:** Validate AI-Guardrails

**Path:** `.github/workflows/validate-ai-guardrails.yml`

**Triggers:**
- Events: push, pull_request
- Branches: main, develop, feature/**, bugfix/**
- Paths: 7 path filters

**Jobs (3):**
- `validate-copilot-instructions`: Validate Copilot Instructions (runs-on: ubuntu-latest)
- `lint-markdown`: Lint Markdown Files (runs-on: ubuntu-latest)
- `check-structure`: Check Module Structure (runs-on: ubuntu-latest)

**Technologies:** Python

**Permissions:**
- contents: read

**Actions Used (3):**
- `actions/checkout`
- `actions/setup-python`
- `nosborn/github-action-markdown-cli`

---

### validate-config-mapping.yml

**Name:** Validate Config Mapping

**Path:** `.github/workflows/validate-config-mapping.yml`

**Triggers:**
- Events: pull_request, push
- Branches: main, develop
- Paths: 8 path filters

**Jobs (1):**
- `validate-mapping`: Validate Config Path Mapping Table (runs-on: ubuntu-latest)

**Technologies:** Python

**Actions Used (2):**
- `actions/checkout`
- `actions/setup-python`

---

### validate-grafana-dashboards.yml

**Name:** Validate Grafana Dashboards

**Path:** `.github/workflows/validate-grafana-dashboards.yml`

**Triggers:**
- Events: pull_request, push, workflow_dispatch
- Branches: main, develop
- Paths: 6 path filters

**Jobs (1):**
- `validate-dashboards`: Validate Grafana Dashboard JSON (runs-on: ubuntu-latest)

**Technologies:** Python

**Actions Used (2):**
- `actions/checkout`
- `actions/setup-python`

---

## Common Patterns Analysis

### Most Used Actions

- `actions/checkout`: used in 12 workflows
- `actions/setup-python`: used in 6 workflows
- `actions/upload-artifact`: used in 2 workflows
- `codecov/codecov-action`: used in 1 workflows
- `softprops/action-gh-release`: used in 1 workflows
- `nosborn/github-action-markdown-cli`: used in 1 workflows

### Technology Distribution

- Python: 6 workflows
- CMake: 6 workflows
- Node.js: 2 workflows
- PHP: 1 workflows
- Go: 1 workflows
- vcpkg: 1 workflows

### Runner Distribution

- ubuntu-latest: 18 jobs
- ${{ matrix.os }}: 2 jobs
- ubuntu-22.04: 2 jobs
- self-hosted: 1 jobs
- gpu-cuda: 1 jobs

## Actions Usage Summary

**Total Unique Actions:** 6

- GitHub Official Actions: 3
- Third-Party Actions: 3
- Reusable Workflows: 0

### GitHub Official Actions

- `actions/checkout`
- `actions/setup-python`
- `actions/upload-artifact`

---

## Current Release Workflow Snapshot (2026-04-13)

Der obere Inventory-Report ist historisch und nicht vollständig. Dieser Abschnitt dokumentiert den aktuell gültigen Release-Stand.

### Release Workflows (aktuell)

- `.github/workflows/04-release_bootstrap-release-branches.yml` — [Manual] Bootstrap Release Branches
- `.github/workflows/04-release_build-binary-linux.yml` — Build Binary Release · Linux
- `.github/workflows/04-release_build-binary-windows.yml` — Build Binary Release · Windows
- `.github/workflows/04-release_canary-deployments-ci.yml` — Canary Deployments CI
- `.github/workflows/04-release_create-release-archive.yml` — [Manual] Create Release Archive
- `.github/workflows/04-release_docker-image.yml` — Docker Image CI
- `.github/workflows/04-release_dockerhub-publish-on-release.yml` — Publish Docker image to Docker Hub (on GitHub Release)
- `.github/workflows/04-release_publish-enterprise.yml` — Publish · Enterprise Edition
- `.github/workflows/04-release_publish-hyperscaler.yml` — Publish · Hyperscaler Edition

### Binary Packaging Standard (verbindlich)

- Dokumentation: `docs/ci-cd/workflows/04-release/binary-package-layout.md`
- Linux-Formate: `TGZ`, `DEB`, `RPM`
- Windows-Formate: `ZIP` (verpflichtend), `MSI` (optional via CPack/WIX)

