# ThemisDB GitHub Actions Workflows

This directory contains CI/CD workflows for building, testing, and publishing ThemisDB components.

## 🌊 Git Flow CI/CD Pipeline

ThemisDB uses a comprehensive Git Flow branching strategy with dedicated workflows for each branch type. See [CI_CD_WORKFLOWS.md](../../CI_CD_WORKFLOWS.md) for complete documentation.

### Git Flow Workflows (Primary)

| Workflow | File | Triggers | Purpose | Duration |
|----------|------|----------|---------|----------|
| **Feature/Bugfix CI** | `feature-ci.yml` | PR to `develop` from `feature/*`, `bugfix/*` | Validate features | ~30-45 min |
| **Develop CI** | `develop-ci.yml` | Push/PR to `develop` | Integration testing | ~30-45 min |
| **CI - Development** | `ci-develop.yml` | Push/PR to `develop` | Fast feedback CI (all platforms) | ~15-30 min |
| **Build & Test** | `build-and-test.yml` | PR to `main` | Main branch protection (required) | ~45-60 min |
| **Release** | `release.yml` | Tag push `v*` | Automated release creation | ~60-90 min |
| **Release CI** | `release-ci.yml` | Push to `release/*`, PR to `main` | Release preparation | ~45-60 min |
| **Hotfix CI** | `hotfix-ci.yml` | PR to `main` from `hotfix/*` | Fast-track fixes | ~20-30 min |
| **Main CI** | `main-ci.yml` | Push to `main`, tags `v*` | Production deployment | ~45-60 min |

### Supporting Workflows

| Workflow | File | Purpose | Status |
|----------|------|---------|--------|
| **CI** | `ci.yml` | General CI (supplementary) | ✅ Active |
| **Security Scan** | `security-scan.yml` | Vulnerability scanning | ✅ Active |
| **Documentation** | `docs.yml` | Build and deploy docs | ✅ Active |
| **Doc Metadata** | `add-doc-metadata.yml` | Add metadata to markdown files | ✅ Active |
| **Python SDK** | `python-sdk-test.yml` | Python SDK tests | ✅ Active |
| **Java SDK** | `java-sdk-test.yml` | Java SDK tests | ✅ Active |
| **C# SDK** | `csharp-sdk-test.yml` | .NET SDK tests | ✅ Active |
| **Helm Chart** | `helm-chart-test.yml` | Helm chart validation | ✅ Active |
| **Fuzzing** | `fuzzing.yml` | Fuzz testing | ✅ Active |
| **SBOM** | `sbom.yml` | Software Bill of Materials | ✅ Active |


## Git Flow Quick Reference

**Developing a feature?**
```bash
git checkout develop
git checkout -b feature/my-feature
# ... develop ...
git push origin feature/my-feature
# Create PR to develop → Triggers ci-develop.yml (fast) or feature-ci.yml
```

**Preparing a release?**
```bash
git checkout develop
git checkout -b release/v1.4.0
echo "1.4.0" > VERSION
# Update CHANGELOG.md
git commit -am "chore: Prepare release v1.4.0"
git push origin release/v1.4.0
# Create PR to main → Triggers build-and-test.yml (required checks)

# After PR is merged
git checkout main
git pull
git tag -a v1.4.0 -m "Release v1.4.0"
git push origin v1.4.0
# → Triggers release.yml → Automated GitHub Release
```

**Hotfixing production?**
```bash
git checkout main
git checkout -b hotfix/1.3.5-critical
# ... fix ...
echo "1.3.5" > VERSION
git commit -am "fix: Critical hotfix"
git push origin hotfix/1.3.5-critical
# Create PR to main → Triggers hotfix-ci.yml
# After merge, auto-creates PR to develop
```

For complete Git Flow documentation, see:
- [BRANCHING_STRATEGY.md](../../BRANCHING_STRATEGY.md) - Complete strategy guide
- [CI_CD_WORKFLOWS.md](../../CI_CD_WORKFLOWS.md) - Workflow documentation
- [BRANCH_PROTECTION_SETUP.md](../../BRANCH_PROTECTION_SETUP.md) - Protection setup
- [CONTRIBUTING.md](../../CONTRIBUTING.md) - Tag-based release process

## New Tag-Based Release Workflows

ThemisDB now includes streamlined workflows for tag-based releases:

### 🚀 ci-develop.yml - Development Pipeline

**Fast feedback CI for the develop branch with multi-platform support.**

- **Triggers**: Push to `develop`, PRs to `develop`
- **Platforms**: Ubuntu (required), Windows, macOS
- **Features**:
  - Quick validation (VERSION file, branch strategy)
  - Parallel builds on all platforms
  - Unit tests and static analysis
  - Fast iteration (~15-30 minutes on Ubuntu)
- **Status**: Ubuntu required, Windows/macOS optional

### 🛡️ build-and-test.yml - Main Branch Protection

**Comprehensive testing before merging to production.**

- **Triggers**: PRs to `main` (from release/* or hotfix/* only)
- **Platforms**: Ubuntu, Windows, macOS (all required)
- **Features**:
  - Pre-merge validation (branch strategy, VERSION, CHANGELOG)
  - Full test suite with extended timeouts
  - Complete static analysis
  - Security scanning
  - All platforms must pass
- **Purpose**: Required status check before merge to main

### 📦 release.yml - Automated Release

**Fully automated release creation from version tags.**

- **Triggers**: Tag push matching `v*` (e.g., v1.4.0, v1.5.0-beta)
- **Features**:
  - Validates tag matches VERSION file
  - Builds optimized release binaries for all platforms
  - Packages with CPack (.tar.gz, .deb, .zip)
  - Generates release notes from CHANGELOG.md
  - Creates GitHub Release automatically
  - Uploads all artifacts
  - Detects pre-releases (tags with `-` like v1.5.0-alpha)
- **Usage**: Just push a tag, everything else is automatic!

**Example Release Flow:**
```bash
# 1. Prepare release on develop
git checkout -b release/v1.5.0 develop
echo "1.5.0" > VERSION
# Update CHANGELOG.md
git commit -am "chore: Prepare v1.5.0"

# 2. Create PR to main
git push origin release/v1.5.0
# PR triggers build-and-test.yml
# All platforms must pass

# 3. After PR merged, tag the release
git checkout main && git pull
git tag -a v1.5.0 -m "Release v1.5.0"
git push origin v1.5.0

# 4. GitHub Actions automatically:
# - Builds for Ubuntu, Windows, macOS
# - Creates GitHub Release
# - Uploads all binaries
# - Publishes release notes
```



## Local Testing with `act`

[act](https://github.com/nektos/act) allows you to run GitHub Actions locally.

### Installation

```bash
# macOS
brew install act

# Linux
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Windows
choco install act-cli
```

### Running Workflows Locally

```bash
# Dry-run (shows what would be executed)
act push -n

# Run on push event
act push

# Run on pull_request event
act pull_request

# Run a specific workflow
act -W .github/workflows/ci.yml

# Run with secrets
act push --secret-file .github/act-secrets

# Use specific runner image
act push -P ubuntu-latest=catthehacker/ubuntu:act-latest
```

### Example Commands

```bash
# Test CI workflow
act push -W .github/workflows/ci.yml -n

# Test Docker build
act push -W .github/workflows/docker-build-test.yml -n

# Test Python SDK workflow
act push -W .github/workflows/python-sdk-test.yml -n

# Test Java SDK workflow
act push -W .github/workflows/java-sdk-test.yml -n

# Test C# SDK workflow
act push -W .github/workflows/csharp-sdk-test.yml -n

# Test Helm chart workflow
act push -W .github/workflows/helm-chart-test.yml -n
```

### Secrets for Local Testing

Create a file `.github/act-secrets` (copy from `.github/act-secrets.example`):

```bash
cp .github/act-secrets.example .github/act-secrets
# Edit and fill in your test values
```

## Enabling Production Releases

To switch from dry-run to actual publishing, make the following changes:

### Docker (docker-build-test.yml)

```yaml
# Change:
push: false  # TODO: Change to 'true' for production releases

# To:
push: true

# Uncomment login steps:
- name: Login to GitHub Container Registry
  uses: docker/login-action@v3
  with:
    registry: ghcr.io
    username: ${{ github.actor }}
    password: ${{ secrets.GITHUB_TOKEN }}
```

### Python SDK (python-sdk-test.yml)

```yaml
# Uncomment:
- name: Publish to PyPI
  env:
    TWINE_USERNAME: __token__
    TWINE_PASSWORD: ${{ secrets.PYPI_API_TOKEN }}
  run: twine upload dist/*
```

### Java SDK (java-sdk-test.yml)

```yaml
# Uncomment:
- name: Deploy to Maven Central
  env:
    MAVEN_USERNAME: ${{ secrets.MAVEN_USERNAME }}
    MAVEN_PASSWORD: ${{ secrets.MAVEN_PASSWORD }}
    GPG_PASSPHRASE: ${{ secrets.GPG_PASSPHRASE }}
  run: mvn deploy -DskipTests -B
```

### C# SDK (csharp-sdk-test.yml)

```yaml
# Uncomment:
- name: Push to NuGet
  run: dotnet nuget push nupkg/*.nupkg --api-key ${{ secrets.NUGET_API_KEY }} --source https://api.nuget.org/v3/index.json
```

### Helm Chart (helm-chart-test.yml)

```yaml
# Uncomment:
- name: Login to GHCR
  uses: docker/login-action@v3
  with:
    registry: ghcr.io
    username: ${{ github.actor }}
    password: ${{ secrets.GITHUB_TOKEN }}

- name: Package and Push Helm chart
  run: |
    helm package helm/themisdb
    helm push themisdb-*.tgz oci://ghcr.io/${{ github.repository }}/charts
```

## Required Secrets

For production releases and nightly builds, configure these repository secrets:

| Secret | Purpose | Used By |
|--------|---------|---------|
| `GITHUB_TOKEN` | Automatically provided by GitHub Actions | All workflows |
| `DOCKER_USERNAME` | Docker Hub username | Nightly builds, Docker builds |
| `DOCKER_TOKEN` | Docker Hub access token | Nightly builds, Docker builds |
| `DOCKERHUB_USERNAME` | Docker Hub username (legacy) | Legacy workflows |
| `DOCKERHUB_TOKEN` | Docker Hub access token (legacy) | Legacy workflows |
| `PYPI_API_TOKEN` | PyPI API token | Python SDK releases |
| `MAVEN_USERNAME` | Maven Central username | Java SDK releases |
| `MAVEN_PASSWORD` | Maven Central password | Java SDK releases |
| `GPG_PASSPHRASE` | GPG signing passphrase | Release signing |
| `NUGET_API_KEY` | NuGet.org API key | C# SDK releases |

## Workflow Triggers

All workflows trigger on:
- `push` to `main` or `develop` branches
- `pull_request` targeting `main` or `develop`
- `workflow_dispatch` (manual trigger)

SDK workflows additionally filter by path:
- Python: `clients/python/**`
- Java: `clients/java/**`
- C#: `clients/csharp/**`
- Helm: `helm/**`

### Nightly Build Schedule

The **Nightly Build** workflow runs automatically:
- **Schedule**: Every day at 2:00 AM UTC
- **Purpose**: Build and push latest development snapshots to DockerHub
- **Output**: Docker images tagged as `nightly`, `nightly-YYYYMMDD`, and `VERSION-nightly`
- **Documentation**: See [Nightly Builds Guide](../../docs/deployment/deployment_nightly_builds.md)

To trigger manually:
```bash
# Via GitHub UI: Actions > Nightly Build & DockerHub Push > Run workflow
# Or use GitHub CLI:
gh workflow run nightly-build.yml
```

## Troubleshooting

### act: Docker image too large

Use a smaller runner image:

```bash
act push -P ubuntu-latest=catthehacker/ubuntu:act-20.04
```

### act: Secrets not found

Ensure secrets file exists and has correct format:

```bash
# Create secrets file
cp .github/act-secrets.example .github/act-secrets
# Run with secrets
act push --secret-file .github/act-secrets
```

### Workflow syntax errors

Validate workflows with actionlint:

```bash
# Install
brew install actionlint  # macOS
# or
go install github.com/rhysd/actionlint/cmd/actionlint@latest

# Run
actionlint
```
