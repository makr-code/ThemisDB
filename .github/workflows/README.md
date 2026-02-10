# ThemisDB GitHub Actions Workflows

**Last Updated:** 2026-02-10  
**Architecture Version:** 2.0 (Consolidated)

This directory contains the CI/CD workflows for ThemisDB. The workflow architecture was consolidated in February 2026 to reduce complexity and improve maintainability.

## 📊 Workflow Architecture Overview

The current architecture uses a three-tier structure:

```
Entry Workflows (12)           ← User-facing, event-triggered
  ├── Reusable Workflows (7)   ← Internal, parameterized
  │    └── Composite Actions (8) ← Low-level, shared steps
  └── Composite Actions (8)    ← Direct usage
```

**Metrics:**
- **Total Workflows:** 20 (down from 53, 62% reduction)
- **Entry Workflows:** 12 (triggered by GitHub events)
- **Reusable Workflows:** 7 (called by other workflows)
- **Composite Actions:** 8 (in `.github/actions/`)
- **Archived Workflows:** 51 (in `_archived/`)

## 🚀 Entry Workflows (User-Facing)

### Pull Request CI

**File:** `ci-pull-request.yml`  
**Purpose:** Fast validation for all pull requests  
**Triggers:** PRs to main, develop, release/*, feature/*, hotfix/*  
**Duration:** ~15-30 minutes

**Jobs:**
- Quick syntax validation (YAML, CMake)
- Fast C++ build with system libraries
- Full C++ build with vcpkg
- Security scanning (CodeQL, Trivy, Gitleaks)
- Critical SDK tests (Python, JavaScript)
- PR summary and test reports

### Main/Develop Branch CI

**File:** `ci-main-branch.yml`  
**Purpose:** Comprehensive builds for main and develop branches  
**Triggers:** Push to main or develop  
**Duration:** ~30-45 minutes

**Jobs:**
- Multi-platform C++ builds (Linux, macOS)
- Full security scanning
- Container build and push to registry
- Documentation deployment (on main)
- Build notifications

### Release Pipeline

**File:** `ci-release.yml`  
**Purpose:** Complete release pipeline  
**Triggers:** Tags (v*.*.*), release/* branches, manual dispatch  
**Duration:** ~60-90 minutes

**Jobs:**
- Multi-platform builds (Linux, macOS, Windows, ARM64)
- Security scanning and compliance checks
- Release container builds (multi-arch)
- Artifact publishing with checksums
- GitHub Release creation
- Release notifications

### Nightly Tests

**File:** `nightly.yml`  
**Purpose:** Extended test suite  
**Triggers:** Daily at 2 AM UTC, manual  
**Duration:** ~90-120 minutes

**Jobs:**
- Extended C++ builds (all platforms, all configurations)
- Chaos engineering tests
- Durability and DR tests
- Performance benchmarks
- Overnight regression tests
- Nightly summary and notifications

### SDK Tests

**File:** `sdk-tests.yml`  
**Purpose:** Unified SDK testing for all languages  
**Triggers:** Push/PR affecting SDK code, manual  
**Duration:** ~20-40 minutes

**Jobs:**
- Python SDK (pytest, type checking, coverage)
- JavaScript/TypeScript SDK (Jest, ESLint)
- Java SDK (JUnit, Maven)
- C# SDK (xUnit, NuGet)
- Go SDK (go test, go vet)
- Ruby SDK (RSpec, RuboCop)
- Rust SDK (cargo test, clippy)
- Swift SDK (XCTest)
- PHP SDK (PHPUnit, Psalm)

**Note:** Individual SDK workflows are in `_archived/` for reference.

### Security Scanning

**File:** `security.yml`  
**Purpose:** Comprehensive security analysis  
**Triggers:** Push/PR to main/develop, schedule (weekly), manual  
**Duration:** ~30-60 minutes

**Jobs:**
- CodeQL analysis (C++, Python, JavaScript)
- Container scanning (Trivy)
- Secret scanning (Gitleaks)
- Static analysis (cppcheck, clang-tidy)
- Dependency scanning
- SARIF report uploads

**Note:** Replaces `security-scan.yml` and `owasp-zap.yml` (archived).

### Compliance Checks

**File:** `compliance.yml`  
**Purpose:** License, audit, and SBOM generation  
**Triggers:** Push/PR to main/develop, schedule (weekly), manual  
**Duration:** ~15-30 minutes

**Jobs:**
- License compliance (vcpkg, npm, pip)
- Dependency audit (npm audit, pip-audit)
- SBOM generation (CycloneDX format)
- Vulnerability reports
- Compliance summary

**Note:** Replaces `audit-check.yml`, `sbom.yml`, and `license-compliance.yml` (archived).

### Documentation

**File:** `docs.yml`  
**Purpose:** Build and deploy documentation  
**Triggers:** Push/PR affecting docs, manual  
**Duration:** ~10-20 minutes

**Jobs:**
- MkDocs site build and deployment (GitHub Pages)
- Compendium build (if configured)
- Link validation
- Wiki synchronization (on main)
- Documentation summary

### Deployment

**File:** `deploy.yml`  
**Purpose:** Infrastructure and container deployment  
**Triggers:** Push to main, release tags, manual  
**Duration:** ~30-45 minutes

**Jobs:**
- Multi-platform container builds
- Container registry push (ghcr.io, Docker Hub)
- Helm chart packaging and push
- Infrastructure validation
- Deployment notifications

**Note:** Replaces `docker-build.yml` and `helm-chart-test.yml` (archived).

### Extended Tests

**File:** `tests-extended.yml`  
**Purpose:** Chaos, durability, and DR testing  
**Triggers:** Schedule (weekly), manual, release branches  
**Duration:** ~60-90 minutes

**Jobs:**
- Chaos engineering tests (fault injection, network partitions)
- Durability tests (crash recovery, data consistency)
- Disaster recovery tests (backup/restore, failover)
- Extended load tests
- Test report generation

**Note:** Replaces `chaos-tests.yml`, `durability-tests.yml`, and `dr-testing.yml` (archived).

### Specialized Tests

**File:** `tests-specialized.yml`  
**Purpose:** Fuzzing, sanitizers, and cross-compilation  
**Triggers:** Schedule (weekly), manual, security-relevant PRs  
**Duration:** ~60-120 minutes

**Jobs:**
- Fuzzing (AFL++, libFuzzer)
- Sanitizer builds (ASan, UBSan, TSan, MSan)
- Cross-compilation (ARM64, RISC-V, Windows ARM)
- Static analysis (Infer, Coverity)
- Specialized test reports

**Note:** Replaces `fuzzing.yml`, `ci-sanitizers.yml`, and `ci-arm-cross.yml` (archived).

### Operations Automation

**File:** `ops-automation.yml`  
**Purpose:** Operational tasks and reviews  
**Triggers:** Schedule (varies by task), manual, incidents  
**Duration:** ~10-30 minutes

**Jobs:**
- Access reviews (quarterly)
- Incident drill simulations (monthly)
- Baseline updates (performance, security)
- Operational health checks
- Automation reports

**Note:** Replaces `access-review.yml` and `incident-drill.yml` (archived).

## 🔄 Reusable Workflows (Internal)

These workflows are called by entry workflows using `workflow_call`:

| File | Purpose | Used By |
|------|---------|---------|
| `reusable-cpp-build.yml` | C++ build/test with configurable options | ci-pull-request, ci-main-branch, ci-release, nightly |
| `reusable-sdk-test.yml` | Multi-language SDK testing | sdk-tests |
| `reusable-security-scan.yml` | Security scanning (CodeQL, Trivy, etc.) | security, ci-pull-request, ci-main-branch |
| `reusable-docs-build.yml` | MkDocs building and deployment | docs |
| `reusable-container-build.yml` | Multi-platform Docker builds | deploy, ci-main-branch, ci-release |
| `reusable-benchmark.yml` | Performance benchmarking | nightly, tests-extended |
| `reusable-cross-compile.yml` | Cross-compilation (ARM/RISC-V) | tests-specialized, ci-release |
| `reusable-test-report.yml` | Test result reporting | All test workflows |

## 📦 Composite Actions

Located in `.github/actions/`, these provide reusable step sequences:

| Action | Purpose | Description |
|--------|---------|-------------|
| `setup-cpp-env` | C++ environment setup | Installs system dependencies, compilers |
| `setup-vcpkg` | vcpkg bootstrap | Caches and installs vcpkg dependencies |
| `cmake-build` | CMake workflow | Configure, build, test with CMake |
| `setup-language` | Multi-language runtime | Sets up Python, Node.js, Java, etc. |
| `report-results` | Test reporting | Uploads results, creates PR comments |
| `security-report` | Security reporting | SARIF upload, issue creation |
| `artifact-publish` | Release artifacts | Publishes with checksums and signatures |
| `notification` | Notifications | Sends workflow status notifications |

## 📁 Archived Workflows

51 legacy workflows have been moved to `_archived/` directory. These workflows were replaced by the consolidated architecture but are preserved for reference and potential rollback.

**See:** `_archived/README.md` for complete list and restoration procedure.

**Most notably archived:**
- `ci.yml` → Replaced by `ci-pull-request.yml`, `ci-main-branch.yml`
- `security-scan.yml` → Replaced by `security.yml`
- `audit-check.yml` → Replaced by `compliance.yml`
- `fuzzing.yml` → Replaced by `tests-specialized.yml`
- `ruby-sdk-test.yml`, `python-sdk-test.yml`, etc. → Replaced by `sdk-tests.yml`
- `develop-ci.yml`, `feature-ci.yml`, `hotfix-ci.yml` → Replaced by `ci-pull-request.yml`

## 🔧 Quick Start Guide

### Developing a Feature

```bash
git checkout develop
git checkout -b feature/my-feature
# ... develop and commit ...
git push origin feature/my-feature
# Create PR to develop → Triggers ci-pull-request.yml
```

### Preparing a Release

```bash
git checkout -b release/v1.6.0 develop
echo "1.6.0" > VERSION
# Update CHANGELOG.md
git commit -am "chore: Prepare v1.6.0"
git push origin release/v1.6.0
# Create PR to main → Triggers ci-pull-request.yml and ci-release.yml

# After PR merge:
git checkout main && git pull
git tag -a v1.6.0 -m "Release v1.6.0"
git push origin v1.6.0
# → Triggers ci-release.yml → Creates GitHub Release
```

### Running Manual Tests

```bash
# Security scan
gh workflow run security.yml

# Nightly tests
gh workflow run nightly.yml

# Specific SDK tests
gh workflow run sdk-tests.yml -f sdk=python

# Documentation build
gh workflow run docs.yml
```

## 🔐 Required Secrets

For production workflows, configure these repository secrets:

| Secret | Purpose | Used By |
|--------|---------|---------|
| `GITHUB_TOKEN` | GitHub API access | Automatically provided |
| `DOCKER_USERNAME` | Docker Hub username | deploy, ci-main-branch, nightly |
| `DOCKER_TOKEN` | Docker Hub token | deploy, ci-main-branch, nightly |
| `PYPI_API_TOKEN` | PyPI publishing | sdk-tests (Python) |
| `MAVEN_USERNAME` | Maven Central | sdk-tests (Java) |
| `MAVEN_PASSWORD` | Maven Central | sdk-tests (Java) |
| `NUGET_API_KEY` | NuGet.org | sdk-tests (C#) |
| `GPG_PASSPHRASE` | Release signing | ci-release |

## 📚 Documentation

For detailed information, see:

- **[docs/ci-cd/ci-architecture.md](../../docs/ci-cd/ci-architecture.md)** - Complete architecture documentation
- **[docs/ci-cd/consolidation-plan.md](../../docs/ci-cd/consolidation-plan.md)** - Consolidation strategy and migration plan
- **[docs/ci-cd/workflows-inventory.md](../../docs/ci-cd/workflows-inventory.md)** - Detailed inventory of all workflows
- **[CONTRIBUTING.md](../../CONTRIBUTING.md)** - Contribution guidelines and Git workflow
- **[_archived/README.md](_archived/README.md)** - Archived workflows and restoration procedure

## 🆘 Troubleshooting

### Workflow Not Triggering

1. Check trigger conditions (branches, paths, events)
2. Verify branch protection rules don't block the workflow
3. Check workflow file syntax with `actionlint`

### Build Failures

1. Check job logs in GitHub Actions UI
2. Review error messages in test reports
3. Run builds locally with `act` (see below)
4. Check recent changes to dependencies or configuration

### Local Testing with act

Install [act](https://github.com/nektos/act) to run workflows locally:

```bash
# Install act
brew install act  # macOS
# or download from: https://github.com/nektos/act/releases

# List workflows
act -l

# Run a workflow
act pull_request -W .github/workflows/ci-pull-request.yml

# Dry run (don't execute)
act pull_request -n

# With specific runner image
act -P ubuntu-latest=catthehacker/ubuntu:act-latest
```

### Validate Workflow Syntax

```bash
# Install actionlint
brew install actionlint
# or: go install github.com/rhysd/actionlint/cmd/actionlint@latest

# Validate all workflows
actionlint

# Validate specific workflow
actionlint .github/workflows/ci-pull-request.yml
```

## 📊 Workflow Status Badges

Add these badges to your documentation to show workflow status:

```markdown
[![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci-pull-request.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci-pull-request.yml)
[![Security](https://github.com/makr-code/ThemisDB/actions/workflows/security.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/security.yml)
[![Docs](https://github.com/makr-code/ThemisDB/actions/workflows/docs.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/docs.yml)
```

## 🎯 Migration Status

✅ **Complete** - CI/CD consolidation completed February 2026

- 53 workflows reduced to 20 (62% reduction)
- 51 workflows archived with full documentation
- New three-tier architecture implemented
- All documentation updated
- Rollback procedure documented

---

*For questions or issues with CI/CD workflows, see [SUPPORT.md](../../SUPPORT.md) or open an issue.*
