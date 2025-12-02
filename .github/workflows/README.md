# ThemisDB GitHub Actions Workflows

This directory contains CI/CD workflows for building, testing, and publishing ThemisDB components.

## 🔶 Dry-Run Mode

**All workflows are currently in DRY-RUN mode.** No actual publishing to external registries occurs until the workflows are verified and explicitly enabled.

## Workflow Overview

| Workflow | File | Purpose | Status |
|----------|------|---------|--------|
| **CI** | `ci.yml` | C++ build, unit tests, code quality | 🔶 Dry-Run |
| **Docker Build** | `docker-build-test.yml` | Multi-arch Docker image build | 🔶 Dry-Run |
| **Python SDK** | `python-sdk-test.yml` | Python SDK tests and package build | 🔶 Dry-Run |
| **Java SDK** | `java-sdk-test.yml` | Java SDK tests and JAR build | 🔶 Dry-Run |
| **C# SDK** | `csharp-sdk-test.yml` | .NET SDK tests and NuGet package | 🔶 Dry-Run |
| **Helm Chart** | `helm-chart-test.yml` | Helm chart linting and validation | 🔶 Dry-Run |

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

For production releases, configure these repository secrets:

| Secret | Purpose |
|--------|---------|
| `GITHUB_TOKEN` | Automatically provided by GitHub Actions |
| `DOCKERHUB_USERNAME` | Docker Hub username |
| `DOCKERHUB_TOKEN` | Docker Hub access token |
| `PYPI_API_TOKEN` | PyPI API token |
| `MAVEN_USERNAME` | Maven Central username |
| `MAVEN_PASSWORD` | Maven Central password |
| `GPG_PASSPHRASE` | GPG signing passphrase |
| `NUGET_API_KEY` | NuGet.org API key |

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
