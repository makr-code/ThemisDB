# ThemisDB CI/CD Architecture

**Version:** 3.0  
**Last Updated:** 2026-08-11  
**Status:** Active

## Overview

This document describes the consolidated CI/CD architecture for ThemisDB.
The August 2026 consolidation reduced 47 fragmented workflows to 17 well-named
workflows following a `{lane}-{purpose}.yml` naming convention.

## Architecture Summary

```
17 Workflows (flat .github/workflows/ layout)

Lane ci-         → core build, PR gates, release pipeline, benchmarks
Lane release-    → changelog management
Lane security-   → SAST/DAST, pentest cadence
Lane compliance- → supply chain, SBOM, SOC2
Lane quality-    → static analysis, Doxygen, perf regression
Lane governance- → maturity verification, merge gates, RC validation
Lane maintenance-→ docs hygiene, orphan checks, wiki sync
Lane automation- → community automations (greet, label, summarize)
Unchanged        → codeql, docker-image, edition-hyperscaler-ci,
                   copilot-ollama-router-ci, copilot-regression-guard
```

### Key Metrics

- **Workflow Reduction:** 47 → 17 workflows (64% reduction)
- **Lanes:** 8 (ci, release, security, compliance, quality, governance, maintenance, automation)
- **Unchanged:** 5 (codeql, docker-image, edition-hyperscaler-ci, copilot-ollama-router-ci, copilot-regression-guard)
- **Naming convention:** `{lane}-{purpose}.yml` — no numeric prefixes, readable without context

## Workflow Inventory

| File | Lane | Purpose | Replaces |
|------|------|---------|----------|
| `ci-build.yml` | ci | Core C++ build matrix (Ubuntu+Windows), sanitizer option | `cmake-multi-platform.yml` |
| `ci-release.yml` | ci | Multi-edition release: build, package, publish | `release-build.yml` |
| `ci-pr-gates.yml` | ci | 8 PR gate jobs (parallel); release-critical mandatory gate | 8× `09-pr-gates_*.yml` |
| `ci-benchmarks.yml` | ci | Voice, GPU matrix (CUDA/HIP/Vulkan), nightly module sweep | 3 benchmark workflows |
| `release-changelog.yml` | release | Incremental update + historical backfill (`mode` input) | 2 changelog workflows |
| `security-scanning.yml` | security | Kubesec + OWASP ZAP DAST | `kubesec.yml` + `security-dast-ci.yml` |
| `security-pentest-quarterly.yml` | security | Quarterly pentest cadence | `security_pentest-quarterly.yml` (renamed) |
| `compliance-supply-chain.yml` | compliance | License policy, SBOM, SOC2 evidence | 3 compliance workflows |
| `gate-pr-doxygen-governance.yml` | quality | GS3 Doxygen structure gate, Doxygen XML/warning validation, Tier-1 coverage escalation | targeted PR Doxygen enforcement |
| `governance-gates.yml` | governance | Maturity, phase gates, merge enforcer, RC validation, audit, waivers | 6 governance workflows |
| `maintenance-docs.yml` | maintenance | AI-context sync, code maturity, orphan check, hygiene, alignment | 5 maintenance workflows |
| `automation-community.yml` | automation | Greet contributors, auto-label PRs, summarize issues | `greetings.yml` + `label.yml` + `summary.yml` |
| `codeql.yml` | — | GitHub SARIF upload, own schedule cadence | *(unchanged)* |
| `docker-image.yml` | — | Container build & push | *(unchanged)* |
| `edition-hyperscaler-ci.yml` | — | Edition-specific hyperscaler targets | *(unchanged)* |
| `copilot-ollama-router-ci.yml` | — | Tooling CI for Ollama router extension | *(unchanged)* |
| `copilot-regression-guard.yml` | — | Regression guard with fixture-based tests | *(unchanged)* |

## Governance Constraints

- **Branch targets:** only `develop`, `minimal`, `community`, `enterprise`, `hyperscaler`, `military` — never `main` or `millitary`
- **Edition tag patterns** (RELEASE_STRATEGY.md §3): `vX.Y.Z`, `enterprise-vX.Y.Z`, `hyperscaler-vX.Y.Z`, `military-vX.Y.Z`, `minimal-vX.Y.Z`
- **Mandatory gate:** `ci-pr-gates.yml` `release-critical-tests` job is a required entry gate for PRs to `develop` and all edition branches (RELEASE_STRATEGY.md §2.3)
- **Community guardrail:** `compliance-supply-chain.yml` and `ci-release.yml` publish-community job must never reference private credentials, submodules, or artefacts (RELEASE_STRATEGY.md §2.4)

## Entry Workflows

Entry workflows are triggered by GitHub events (push, PR, schedule, workflow_dispatch) and orchestrate the CI/CD pipeline.

### 1. ci-pull-request.yml

**Purpose:** Fast validation for all pull requests

**Triggers:**
- Pull requests to main, develop, release/*, feature/*, hotfix/*
- Excludes: Documentation-only changes

**Jobs:**
- Quick validation checks (YAML, CMake syntax)
- Fast C++ build with system libraries
- Full C++ build with vcpkg
- Security scanning
- Critical SDK tests (Python, JavaScript)
- PR summary generation

**Concurrency:** Cancel in-progress runs on same ref

**Usage Example:**
```yaml
# Automatically triggered on PR creation
# Manual trigger: workflow_dispatch
```

### 2. ci-main-branch.yml

**Purpose:** Comprehensive builds for main and develop branches

**Triggers:**
- Push to main or develop branches
- Excludes: Documentation-only changes

**Jobs:**
- Multi-platform C++ builds (Linux, macOS)
- Full security scanning
- Container build and push
- Documentation deployment
- Build notifications

**Concurrency:** No cancellation (complete all builds)

### 3. ci-release.yml

**Purpose:** Release pipeline for versioned releases

**Triggers:**
- Tags: v*.*.*
- Branches: release/*
- Manual: workflow_dispatch with version input

**Jobs:**
- Multi-platform builds (Linux, macOS, Windows, ARM64)
- Security scanning
- Release container builds
- Artifact publishing with checksums
- GitHub Release creation
- Release notifications

**Permissions:** contents:write, packages:write

### 4. nightly.yml

**Purpose:** Extended test suite running nightly

**Triggers:**
- Schedule: Daily at 2 AM UTC
- Manual: workflow_dispatch

**Jobs:**
- Full C++ build matrix (OS × build type)
- Cross-compilation matrix (ARM64, ARMHF)
- Full SDK test matrix (9 languages × versions)
- Performance benchmarks
- Extended security scanning
- Documentation builds

**Timeout:** 120 minutes

### 5. sdk-tests.yml

**Purpose:** Unified SDK testing across all languages

**Triggers:**
- Pull requests/pushes to clients/** or sdks/**
- Manual: workflow_dispatch with language selection

**SDKs Tested:**
- Python (3.10, 3.11, 3.12)
- JavaScript/Node.js (18, 20, 21)
- Java (11, 17, 21)
- Go (1.20, 1.21, 1.22)
- Ruby (3.2)
- Rust (stable)
- C# (7.0)
- PHP (8.1, 8.2, 8.3)
- Swift (5.9)

**Matrix Strategy:** fail-fast: false

### 6. security.yml

**Purpose:** Comprehensive security scanning

**Triggers:**
- Pull requests to main/develop
- Push to main/develop
- Schedule: Weekly on Sundays at 2 AM UTC
- Manual: workflow_dispatch with scanner selection

**Scanners:**
- CodeQL (C++ analysis)
- Trivy (vulnerability scanning)
- Gitleaks (secret scanning)
- cppcheck (static analysis)
- License compliance checks
- Dependency auditing

**Permissions:** security-events:write

### 7. compliance.yml

**Purpose:** License, audit, and SBOM generation

**Triggers:**
- Pull requests to main/release/*
- Push to main
- Schedule: Monthly on 1st at 3 AM UTC
- Manual: workflow_dispatch

**Checks:**
- License policy validation
- SBOM generation (SPDX, CycloneDX)
- Security configuration audit
- Workflow permissions audit
- Access control review

### 8. docs.yml

**Purpose:** Documentation build and deployment

**Triggers:**
- Push/PR to docs/**, mkdocs.yml, **.md
- Manual: workflow_dispatch

**Components:**
- MkDocs site build
- Compendium documentation
- Link validation
- Wiki synchronization

**Deployment:** GitHub Pages (main branch only)

### 9. deploy.yml

**Purpose:** Production deployment orchestration

**Triggers:**
- Manual only: workflow_dispatch

**Parameters:**
- Environment: development, staging, production
- Component: all, container, helm, docs
- Version: tag or 'latest'

**Jobs:**
- Pre-deployment validation
- Container deployment
- Helm chart deployment
- Documentation deployment
- Post-deployment verification

**Environments:** GitHub Environments with protection rules

### 10. tests-extended.yml

**Purpose:** Long-running and specialized tests

**Triggers:**
- Schedule: Weekly on Saturdays at 3 AM UTC
- Manual: workflow_dispatch with test suite selection

**Test Suites:**
- Chaos engineering tests
- Durability tests (2-hour runtime)
- Disaster recovery scenarios

**Timeout:** 120 minutes

### 11. tests-specialized.yml

**Purpose:** Specialized testing (fuzzing, sanitizers, cross-compile)

**Triggers:**
- Schedule: Twice weekly (Tue, Fri at 4 AM UTC)
- Manual: workflow_dispatch with test type

**Test Types:**
- Fuzzing (libFuzzer)
- Sanitizers (ASan, TSan, UBSan)
- Cross-compilation (ARM64, ARMHF)

**Build Type:** RelWithDebInfo

### 12. ops-automation.yml

**Purpose:** Operational tasks and reviews

**Triggers:**
- Schedule: Monthly on 1st at 6 AM UTC
- Manual: workflow_dispatch with operation selection

**Operations:**
- Access control reviews
- Incident response drills
- Performance baseline updates

**Outputs:** GitHub Issues for manual reviews

## Reusable Workflows

Reusable workflows are called by entry workflows using `uses: ./.github/workflows/reusable-*.yml`

### 1. reusable-cpp-build.yml

**Purpose:** Configurable C++ build and test

**Inputs:**
```yaml
os: ubuntu-latest | macos-latest | windows-latest
build-type: Release | Debug | RelWithDebInfo
use-system-libs: ON | OFF
cmake-preset: (optional preset name)
run-tests: true | false
upload-artifacts: true | false
artifact-name: (custom name)
cache-key-suffix: (optional)
```

**Outputs:**
- build-status: success | failure

**Actions Used:**
- setup-cpp-env
- setup-vcpkg
- cmake-build
- report-results

**Example Usage:**
```yaml
jobs:
  build:
    uses: ./.github/workflows/reusable-cpp-build.yml
    with:
      os: ubuntu-latest
      build-type: Release
      use-system-libs: 'OFF'
      run-tests: true
```

### 2. reusable-sdk-test.yml

**Purpose:** Multi-language SDK testing

**Inputs:**
```yaml
language: python | java | javascript | go | ruby | rust | swift | csharp | php
version: (language version)
sdk-path: (default: clients/{language})
dry-run: true | false
```

**Supported Languages:**
- Python: pip + pytest
- JavaScript/TypeScript: npm + jest
- Java: Maven + JUnit
- Go: go test
- Ruby: RSpec/rake
- Rust: cargo test
- C#: dotnet test
- PHP: PHPUnit
- Swift: swift test

**Example Usage:**
```yaml
jobs:
  test-python:
    uses: ./.github/workflows/reusable-sdk-test.yml
    with:
      language: python
      version: '3.11'
```

### 3. reusable-security-scan.yml

**Purpose:** Unified security scanning

**Inputs:**
```yaml
scanner-type: all | codeql | trivy | gitleaks | cppcheck
target-path: (default: .)
upload-sarif: true | false
create-issues: true | false
severity-threshold: low | medium | high | critical
```

**Scanners:**
- **CodeQL:** Deep semantic analysis for C++
- **Trivy:** Vulnerability and misconfig scanning
- **Gitleaks:** Secret detection
- **cppcheck:** Static analysis for C++

**Example Usage:**
```yaml
jobs:
  security:
    uses: ./.github/workflows/reusable-security-scan.yml
    with:
      scanner-type: 'all'
      upload-sarif: true
    secrets: inherit
```

### 4. reusable-docs-build.yml

**Purpose:** Documentation building and deployment

**Inputs:**
```yaml
docs-path: (default: docs)
deploy-target: none | gh-pages | artifact
mkdocs-config: (default: mkdocs.yml)
python-version: (default: 3.11)
```

**Features:**
- MkDocs configuration validation
- Material theme support
- Plugin installation
- GitHub Pages deployment

### 5. reusable-container-build.yml

**Purpose:** Multi-platform container builds

**Inputs:**
```yaml
dockerfile: (default: Dockerfile)
platforms: linux/amd64,linux/arm64
registry: ghcr.io | docker.io
tags: (comma-separated)
push: true | false
build-args: (key=value pairs)
```

**Features:**
- QEMU setup for multi-arch
- Docker Buildx
- Registry authentication
- SBOM generation
- Layer caching

### 6. reusable-benchmark.yml

**Purpose:** Performance benchmarking

**Inputs:**
```yaml
benchmark-suite: performance | sharding | all
baseline-ref: (git ref for comparison)
upload-results: true | false
update-baseline: true | false
```

**Benchmarks:**
- Google Benchmark integration
- JSON output format
- Baseline comparison
- Results archiving

### 7. reusable-cross-compile.yml

**Purpose:** Cross-compilation for ARM architectures

**Inputs:**
```yaml
target-arch: arm64 | armhf | riscv64
toolchain: (optional custom toolchain)
build-type: Release | Debug
run-tests: true | false (QEMU-based)
upload-artifacts: true | false
```

**Features:**
- Cross-compilation toolchains
- QEMU for testing
- vcpkg triplet configuration
- Architecture-specific CMake settings

## Composite Actions

Composite actions encapsulate common step sequences for reuse across workflows.

### 1. setup-cpp-env

**Purpose:** Setup C++ build environment

**Inputs:**
- use-system-libs: ON | OFF
- vcpkg-manifest: vcpkg.json
- cache-key-suffix: (optional)

**Outputs:**
- vcpkg-root
- cache-hit

**What it does:**
- Install apt dependencies (build-essential, cmake, ninja, etc.)
- Optional: Install system libraries (RocksDB, spdlog, etc.)
- Setup vcpkg paths
- Configure caching

### 2. setup-vcpkg

**Purpose:** Bootstrap and install vcpkg dependencies

**Inputs:**
- manifest-path: vcpkg.json
- cache-key-suffix: (optional)
- vcpkg-root: (optional)

**Outputs:**
- vcpkg-root
- cache-hit

**Features:**
- Clone or reuse existing vcpkg
- Bootstrap vcpkg
- Enable binary caching (x-gha)
- Retry logic for network failures
- Install dependencies from manifest

### 3. cmake-build

**Purpose:** Configure, build, and test CMake projects

**Inputs:**
- preset: (CMake preset name)
- build-type: Release | Debug | RelWithDebInfo
- run-tests: true | false
- install-prefix: (optional)
- vcpkg-root: (optional)
- source-dir: .
- build-dir: build
- cmake-args: (additional args)
- parallel-level: 0 (auto) | N

**Outputs:**
- build-dir
- install-dir

**Features:**
- Parallel builds (auto-detect cores)
- CMake caching
- CTest integration
- JUnit XML output
- Optional installation

### 4. setup-language

**Purpose:** Setup language runtimes

**Inputs:**
- language: python | node | java | go | ruby | rust | dotnet | php | swift
- version: (language version)
- cache-dependency-path: (optional)

**Outputs:**
- cache-hit

**Supported:**
- Python: actions/setup-python@v5
- Node.js: actions/setup-node@v4
- Java: actions/setup-java@v4 (Temurin)
- Go: actions/setup-go@v5
- Ruby: ruby/setup-ruby@v1
- Rust: rustup
- .NET: actions/setup-dotnet@v4
- PHP: shivammathur/setup-php@v2
- Swift: swift-actions/setup-swift@v2

### 5. report-results

**Purpose:** Upload test results and create reports

**Inputs:**
- test-results-path: (JUnit XML path)
- coverage-path: (optional)
- create-pr-comment: true | false
- artifact-name: test-results
- report-name: Test Results
- reporter: java-junit | dart-json | dotnet-trx
- fail-on-error: true | false

**Features:**
- Artifact upload
- dorny/test-reporter integration
- GitHub Step Summary
- Coverage report handling

### 6. security-report

**Purpose:** Upload SARIF and create security issues

**Inputs:**
- sarif-path: (SARIF file path)
- create-issues: true | false
- severity-threshold: low | medium | high | critical
- category: (for CodeQL)
- artifact-name: security-report

**Outputs:**
- findings-count

**Features:**
- SARIF artifact upload
- github/codeql-action/upload-sarif integration
- Finding analysis
- Optional issue creation

### 7. artifact-publish

**Purpose:** Publish release artifacts with checksums

**Inputs:**
- artifacts-path: (path or glob)
- release-tag: (optional)
- artifact-name: release-artifacts
- generate-checksums: true | false
- retention-days: 90

**Outputs:**
- artifact-name
- checksum-file

**Features:**
- SHA256 checksum generation
- Artifact bundling
- GitHub Step Summary with checksums

### 8. notification

**Purpose:** Send workflow notifications

**Inputs:**
- status: success | failure | cancelled
- message: (custom message)
- channels: summary | slack | discord | email
- slack-webhook: (from secrets)
- discord-webhook: (from secrets)
- workflow-name: (optional)

**Outputs:**
- sent: true | false

**Channels:**
- **Summary:** GitHub Step Summary (always)
- **Slack:** Optional webhook integration
- **Discord:** Optional webhook integration
- **Email:** Placeholder for future

## Standard Conventions

### Naming

- **Entry workflows:** `{purpose}.yml` or `{purpose}-{target}.yml`
- **Reusable workflows:** `reusable-{purpose}.yml`
- **Composite actions:** `.github/actions/{action-name}/action.yml`
- **Artifacts:** `{project}-{platform}-{config}-{version}`
- **Job names:** Descriptive, title case

### Permissions

Default permissions follow the principle of least privilege:

```yaml
# Read-only default
permissions:
  contents: read

# PR workflows
permissions:
  contents: read
  pull-requests: write
  checks: write

# Security workflows
permissions:
  contents: read
  security-events: write
  issues: write

# Release workflows
permissions:
  contents: write
  packages: write
  id-token: write
```

### Concurrency

```yaml
# PR workflows: cancel in-progress
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

# Main/release: complete all builds
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: false
```

### Environment Variables

Standard environment variables across workflows:

```yaml
env:
  BUILD_TYPE: Release
  CMAKE_BUILD_PARALLEL_LEVEL: 4
  CTEST_PARALLEL_LEVEL: 4
  VCPKG_MAX_CONCURRENCY: 4
  USE_SYSTEM_LIBS: OFF
```

### Timeouts

- **Quick CI:** 30-45 minutes
- **Full builds:** 60 minutes
- **Nightly tests:** 120 minutes
- **Security scans:** 45 minutes
- **Documentation:** 20-30 minutes

### Caching Strategy

1. **vcpkg packages:** Cached by manifest hash
2. **CMake build:** Cached by CMakeLists.txt hash
3. **Language dependencies:** Native caching (pip, npm, etc.)
4. **Docker layers:** GitHub Actions cache

## Migration from Old Workflows

### Mapping Table

| Old Workflow(s) | New Workflow | Notes |
|----------------|--------------|-------|
| ci-develop.yml, feature-ci.yml, hotfix-ci.yml | ci-pull-request.yml | Unified PR CI |
| main-ci.yml, develop-ci.yml | ci-main-branch.yml | Branch-specific logic |
| release.yml, release-build.yml, release-ci.yml | ci-release.yml | Complete release pipeline |
| python-sdk-test.yml (×9) | sdk-tests.yml | Matrix strategy |
| security-scan.yml, owasp-zap.yml | security.yml | Unified security |
| license-compliance.yml, audit-check.yml | compliance.yml | Combined compliance |
| docs-compendium.yml, wiki-sync.yml | docs.yml | Unified docs |
| docker-build.yml, helm-chart-test.yml | deploy.yml | Deployment orchestration |
| chaos-tests.yml, durability-tests.yml | tests-extended.yml | Long-running tests |
| fuzzing.yml, ci-sanitizers.yml | tests-specialized.yml | Specialized testing |
| access-review.yml, incident-drill.yml | ops-automation.yml | Operations tasks |

### Branch Protection Updates

If you have branch protection rules referencing old workflows:

1. Go to Repository Settings → Branches
2. Edit branch protection rule for main/develop
3. Update required status checks:
   - Remove: old workflow names
   - Add: `Build (Ubuntu, Release)`, `Security Scan`, etc.
4. Save changes

### Gradual Migration Strategy

The migration has already moved old workflows to `_archived/`. To validate:

1. **Week 1-2:** New workflows run in parallel (completed)
2. **Week 3-4:** Monitor new workflow success rates
3. **Week 5-6:** Update branch protection rules
4. **Week 7-8:** Full cutover, archive old workflows (completed)

## Troubleshooting

### Common Issues

**Issue:** Workflow not triggered  
**Solution:** Check triggers, paths-ignore, and branch names

**Issue:** Reusable workflow not found  
**Solution:** Ensure path is correct: `./.github/workflows/reusable-*.yml`

**Issue:** Composite action fails  
**Solution:** Check inputs are provided, paths are correct

**Issue:** Permission denied  
**Solution:** Review workflow permissions, add required scopes

### Debug Mode

Enable debug logging:
1. Go to Repository Settings → Secrets
2. Add `ACTIONS_STEP_DEBUG` = `true`
3. Re-run workflow for detailed logs

### Getting Help

- Review this documentation
- Check `docs/ci-cd/consolidation-plan.md`
- Open an issue with `ci/cd` label
- Check archived workflows for reference

## Performance Optimization

### Cache Hit Rates

Target cache hit rates:
- vcpkg packages: >70%
- CMake build: >60%
- Language dependencies: >80%

### Build Times

Target build times:
- PR CI (fast): <10 minutes
- PR CI (full): <15 minutes
- Nightly full matrix: <90 minutes

### Cost Optimization

- Use `cancel-in-progress` for PR workflows
- Cache aggressively
- Use `paths-ignore` to skip unnecessary runs
- Limit concurrent runs with concurrency groups

## Best Practices

### For Developers

1. **Test locally first:** Use `act` or local builds
2. **Incremental commits:** Avoid large changesets
3. **Meaningful commit messages:** Help CI identify changes
4. **Watch CI feedback:** Address failures quickly

### For Maintainers

1. **Monitor workflow success rates:** Aim for >97%
2. **Review cache hit rates:** Optimize caching strategy
3. **Update dependencies:** Keep actions up-to-date
4. **Document changes:** Update this doc when modifying workflows

### For Contributors

1. **Read this guide:** Understand the CI architecture
2. **Follow conventions:** Match existing patterns
3. **Test workflow changes:** Use `workflow_dispatch` for testing
4. **Request reviews:** Get maintainer approval for workflow changes

## Future Enhancements

Planned improvements:

- [ ] Artifact attestation (SLSA)
- [ ] Advanced caching strategies
- [ ] Performance regression detection
- [ ] Automated rollback on failures
- [ ] Enhanced notification integrations
- [ ] Workflow visualization dashboard

## References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Composite Actions](https://docs.github.com/en/actions/creating-actions/creating-a-composite-action)
- [Reusable Workflows](https://docs.github.com/en/actions/using-workflows/reusing-workflows)
- [Workflow Syntax](https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions)

---

**Maintained by:** ThemisDB Team  
**Questions?** Open an issue with `ci/cd` label
