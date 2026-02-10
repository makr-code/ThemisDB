# GitHub Actions Workflows Consolidation Plan

**Version:** 1.0  
**Date:** 2026-02-10  
**Status:** Planning Phase

## Executive Summary

This document outlines a comprehensive consolidation strategy for the 53 GitHub Actions workflows currently in the ThemisDB repository. The current state exhibits significant duplication, inconsistent patterns, and maintenance overhead. This plan proposes a structured approach to reduce complexity while maintaining functionality and improving maintainability.

### Current State
- **Total Workflows:** 53 files (51 successfully parsed, 2 with YAML errors)
- **Total Jobs:** 175+ jobs across all workflows
- **Unique Actions:** 43 different actions used
- **Main Issues:**
  - Heavy duplication of setup steps (checkout, apt dependencies, vcpkg setup, CMake configuration)
  - Inconsistent naming conventions
  - Overlapping triggers and responsibilities
  - No reusable workflows (except 1: `reusable-test-report.yml`)
  - Minimal use of composite actions
  - Difficult to maintain and understand the CI/CD landscape

### Target State
- **Entry Workflows:** 8-12 main workflow files
- **Reusable Workflows:** 5-8 workflow_call templates
- **Composite Actions:** 6-10 custom actions
- **Benefits:**
  - 60-70% reduction in workflow files
  - Centralized maintenance of common patterns
  - Consistent standards across all workflows
  - Easier onboarding and troubleshooting
  - Reduced CI/CD execution time through better caching

---

## Table of Contents

1. [Current State Analysis](#1-current-state-analysis)
2. [Identified Patterns and Duplications](#2-identified-patterns-and-duplications)
3. [Proposed Target Architecture](#3-proposed-target-architecture)
4. [Consolidation Clusters](#4-consolidation-clusters)
5. [Composite Actions Design](#5-composite-actions-design)
6. [Reusable Workflows Design](#6-reusable-workflows-design)
7. [Standard Conventions](#7-standard-conventions)
8. [Migration Plan](#8-migration-plan)
9. [Risk Assessment and Mitigation](#9-risk-assessment-and-mitigation)
10. [Success Metrics](#10-success-metrics)

---

## 1. Current State Analysis

### 1.1 Workflow Categories

Based on the inventory analysis, workflows are distributed across these categories:

| Category | Count | Examples |
|----------|-------|----------|
| **PR CI** | 6 | ci-develop.yml, feature-ci.yml, hotfix-ci.yml |
| **Branch CI** | 1 | main-ci.yml |
| **SDK Testing** | 9 | python-sdk-test.yml, java-sdk-test.yml, etc. |
| **Release** | 5 | release.yml, release-build.yml, docker-build.yml |
| **Security & Compliance** | 4 | audit-check.yml, security-scan.yml, license-compliance.yml |
| **Documentation** | 4 | docs.yml, docs-compendium.yml, wiki-sync.yml |
| **Testing** | 5 | chaos-tests.yml, durability-tests.yml, ci-sanitizers.yml |
| **Performance & Benchmarking** | 3 | performance-regression-check.yml, sharding-benchmark.yml |
| **Scheduled & Manual** | 6 | access-review.yml, attack-vector-analysis.yml |
| **Cross-Compilation** | 2 | ci-arm-cross.yml, cross-compile-review.yml |
| **LoRA/LLM** | 2 | llama-cpp-integration.yml, lora-framework-ci.yml |
| **Container & Deployment** | 1 | helm-chart-test.yml |
| **Fuzzing** | 1 | fuzzing.yml |
| **Sanitizers** | 1 | ci-sanitizers.yml |
| **Other** | 1 | chimera-neutrality-check.yml |

### 1.2 Most Used Actions

Analysis shows these actions are used most frequently:

1. `actions/checkout` - Used in ~51 workflows
2. `actions/upload-artifact` - Used in ~30 workflows
3. `actions/github-script` - Used in ~20 workflows
4. `actions/download-artifact` - Used in ~18 workflows
5. `actions/setup-python` - Used in ~15 workflows
6. `actions/cache` - Used in ~12 workflows
7. `actions/setup-node` - Used in ~8 workflows
8. `docker/build-push-action` - Used in ~6 workflows
9. `github/codeql-action/*` - Used in ~5 workflows
10. `actions/setup-java` - Used in ~4 workflows

### 1.3 Common Patterns Identified

#### Pattern A: CMake C++ Build with vcpkg
**Frequency:** ~10 workflows  
**Steps:**
1. Checkout with submodules
2. Install apt dependencies (cmake, ninja, pkg-config, etc.)
3. Setup/restore vcpkg cache
4. Bootstrap vcpkg
5. Install vcpkg dependencies
6. CMake configure with specific presets
7. CMake build
8. Run tests
9. Upload artifacts

**Workflows using this pattern:**
- ci.yml, ci-linux-full.yml, ci-windows-full.yml
- build-and-test.yml, ci-develop.yml, develop-ci.yml
- feature-ci.yml, hotfix-ci.yml, main-ci.yml
- release-build.yml

#### Pattern B: SDK Testing
**Frequency:** 9 workflows (one per language)  
**Steps:**
1. Checkout repository
2. Setup language runtime (Python, Java, Node, etc.)
3. Install dependencies
4. Run tests
5. Build package (optional)
6. Report results

**Workflows using this pattern:**
- python-sdk-test.yml, java-sdk-test.yml, go-sdk-test.yml
- csharp-sdk-test.yml, ruby-sdk-test.yml, rust-sdk-test.yml
- swift-sdk-test.yml, php-sdk-test.yml, javascript-sdk-test.yml

#### Pattern C: Security Scanning
**Frequency:** ~8 workflows  
**Steps:**
1. Checkout repository
2. Run security scanner (CodeQL, Trivy, OWASP ZAP, etc.)
3. Upload SARIF results
4. Create issues for findings
5. Generate reports

**Workflows using this pattern:**
- security-scan.yml, audit-check.yml, owasp-zap.yml
- attack-vector-analysis.yml, license-compliance.yml
- sbom.yml

#### Pattern D: Documentation Build & Deploy
**Frequency:** 4 workflows  
**Steps:**
1. Checkout repository
2. Setup Python and install MkDocs
3. Build documentation
4. Deploy to GitHub Pages
5. Upload artifacts

**Workflows using this pattern:**
- docs.yml, docs-compendium.yml
- wiki-sync.yml, documentation-validation.yml

#### Pattern E: Release Workflow
**Frequency:** 5 workflows  
**Steps:**
1. Trigger on tag push (v*.*.*)
2. Build artifacts for multiple platforms
3. Create GitHub release
4. Upload release assets
5. Build and push Docker images
6. Update deployment manifests

**Workflows using this pattern:**
- release.yml, release-build.yml, docker-build.yml
- release-ci.yml, retroactive-release.yml

### 1.4 Duplicate & Overlapping Workflows

Several workflows have significant overlap:

#### PR CI Workflows (6 workflows)
- `ci-develop.yml` - Triggers on develop PRs
- `develop-ci.yml` - Also triggers on develop PRs (DUPLICATE)
- `feature-ci.yml` - Triggers on feature/* PRs
- `hotfix-ci.yml` - Triggers on hotfix/* PRs
- `ci-linux-full.yml` - Full Linux build for PRs
- `ci-windows-full.yml` - Full Windows build for PRs

**Issue:** These can be consolidated into 1-2 workflows with matrix strategies or conditional logic based on branch naming.

#### SDK Test Workflows (9 workflows)
Each language has its own workflow file, but they follow the same pattern with only language-specific differences.

**Issue:** These can be consolidated into 1 workflow with matrix strategies for different languages, or 1 reusable workflow called 9 times.

#### Release Workflows (5 workflows)
Multiple workflows handle different aspects of releases:
- `release.yml` - Main release automation
- `release-build.yml` - Build release artifacts
- `release-ci.yml` - CI checks on release branches
- `docker-build.yml` - Docker image builds
- `retroactive-release.yml` - Rebuild old releases

**Issue:** These can be better organized with clear separation of concerns and reusable components.

---

## 2. Identified Patterns and Duplications

### 2.1 Common Setup Steps (Repeated 40+ times)

These steps are duplicated across many workflows:

```yaml
# Step 1: Checkout (appears in ~51 workflows)
- uses: actions/checkout@v4
  with:
    submodules: recursive
    fetch-depth: 0

# Step 2: System dependencies (appears in ~15 workflows)
- name: Install system dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build pkg-config libssl-dev

# Step 3: vcpkg setup (appears in ~12 workflows)
- name: Cache vcpkg
  uses: actions/cache@v4
  with:
    path: ~/.cache/vcpkg
    key: vcpkg-${{ runner.os }}-${{ hashFiles('vcpkg.json') }}

- name: Bootstrap vcpkg
  run: |
    git clone https://github.com/microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh

# Step 4: Python setup (appears in ~15 workflows)
- uses: actions/setup-python@v5
  with:
    python-version: '3.12'
    cache: 'pip'
```

### 2.2 Redundant Workflows

| Primary | Redundant | Reason |
|---------|-----------|--------|
| develop-ci.yml | ci-develop.yml | Both handle develop branch PRs |
| release-build.yml | release.yml | Overlapping release build logic |
| ci-linux-full.yml | ci.yml | Similar build configuration, different triggers |

### 2.3 Missing Reusability

Only **1 reusable workflow** exists (`reusable-test-report.yml`), and **0 composite actions** are defined in `.github/actions/`.

---

## 3. Proposed Target Architecture

### 3.1 Entry Workflows (8-12 files)

These are the main workflow files that users and CI/CD systems interact with:

#### Core CI/CD (4 workflows)
1. **`ci-pull-request.yml`** - Unified PR CI (replaces 6 workflows)
   - Triggers: All PRs regardless of branch
   - Matrix strategy for different build configurations
   - Calls reusable build workflow

2. **`ci-main-branch.yml`** - Main/Develop branch CI
   - Triggers: Push to main, develop
   - Full test suite
   - Performance benchmarks
   - Deployment to staging

3. **`ci-release.yml`** - Release pipeline
   - Triggers: Push to release/* branches, tags v*.*.*
   - Build release artifacts
   - Create GitHub release
   - Deploy to production

4. **`nightly.yml`** - Nightly builds and extended tests
   - Triggers: schedule (daily at 2 AM)
   - Full matrix builds (all OS, all configs)
   - Performance regression tests
   - Integration tests

#### SDK & Language Testing (1-2 workflows)
5. **`sdk-tests.yml`** - Unified SDK testing
   - Matrix strategy for all languages
   - Calls reusable SDK test workflow
   - Replaces 9 separate SDK workflows

#### Security & Compliance (2 workflows)
6. **`security.yml`** - Security scanning
   - CodeQL, OWASP ZAP, Trivy, etc.
   - Triggers: PR, push to main/develop, schedule (weekly)
   - Replaces: security-scan.yml, owasp-zap.yml

7. **`compliance.yml`** - Compliance checks
   - License compliance, SBOM generation, audit checks
   - Triggers: PR, release, schedule (monthly)
   - Replaces: license-compliance.yml, sbom.yml, audit-check.yml, audit-gate-issue.yml

#### Documentation & Deployment (2 workflows)
8. **`docs.yml`** - Documentation build & deploy
   - Unified docs pipeline
   - Replaces: docs.yml, docs-compendium.yml, wiki-sync.yml

9. **`deploy.yml`** - Deployment automation
   - Container builds, Helm charts, infrastructure
   - Replaces: docker-build.yml, helm-chart-test.yml, wordpress-theme-deploy.yml

#### Specialized Testing (2-3 workflows)
10. **`tests-extended.yml`** - Extended test suites
    - Chaos tests, durability tests, DR testing
    - Replaces: chaos-tests.yml, durability-tests.yml, dr-testing.yml

11. **`tests-specialized.yml`** - Specialized tests
    - Fuzzing, sanitizers, ARM cross-compilation
    - Replaces: fuzzing.yml, ci-sanitizers.yml, ci-arm-cross.yml

12. **`ops-automation.yml`** - Operational automation
    - Access reviews, incident drills, performance baselines
    - Replaces: access-review.yml, incident-drill.yml, update-performance-baselines.yml

### 3.2 Reusable Workflows (5-8 files)

Located in `.github/workflows/` with prefix `reusable-`:

1. **`reusable-cpp-build.yml`**
   - Parameters: OS, build-type, use-system-libs, vcpkg-manifest
   - Handles: Full C++ build with CMake and vcpkg
   - Used by: ci-pull-request.yml, ci-main-branch.yml, ci-release.yml

2. **`reusable-sdk-test.yml`**
   - Parameters: language, version-matrix, test-command
   - Handles: Language-specific SDK testing
   - Used by: sdk-tests.yml

3. **`reusable-security-scan.yml`**
   - Parameters: scanner-type (codeql/trivy/zap), upload-sarif
   - Handles: Security scanning with various tools
   - Used by: security.yml, compliance.yml

4. **`reusable-docs-build.yml`**
   - Parameters: docs-path, deploy-target
   - Handles: MkDocs build and deployment
   - Used by: docs.yml

5. **`reusable-container-build.yml`**
   - Parameters: dockerfile, platforms, registry, tags
   - Handles: Multi-platform Docker builds
   - Used by: deploy.yml, ci-release.yml

6. **`reusable-test-report.yml`** (already exists)
   - Keep and enhance as needed

7. **`reusable-benchmark.yml`**
   - Parameters: benchmark-suite, baseline-ref
   - Handles: Performance benchmarking
   - Used by: ci-main-branch.yml, tests-extended.yml

8. **`reusable-cross-compile.yml`**
   - Parameters: target-arch, toolchain
   - Handles: Cross-compilation setup and build
   - Used by: tests-specialized.yml

### 3.3 Composite Actions (6-10 custom actions)

Located in `.github/actions/*/action.yml`:

1. **`.github/actions/setup-cpp-env/`**
   - Inputs: use-system-libs, vcpkg-manifest-path
   - Steps: Install apt deps, setup vcpkg with caching
   - Used by: All C++ build workflows

2. **`.github/actions/setup-vcpkg/`**
   - Inputs: manifest-path, cache-key
   - Steps: Clone vcpkg, bootstrap, restore cache
   - Used by: setup-cpp-env

3. **`.github/actions/cmake-build/`**
   - Inputs: preset, build-type, install-prefix
   - Steps: CMake configure, build, test, install
   - Used by: reusable-cpp-build.yml

4. **`.github/actions/setup-language/`**
   - Inputs: language, version
   - Steps: Setup Python/Node/Java/etc. with caching
   - Used by: reusable-sdk-test.yml

5. **`.github/actions/report-results/`**
   - Inputs: test-results-path, coverage-path
   - Steps: Parse results, create PR comments, upload artifacts
   - Used by: All test workflows

6. **`.github/actions/security-report/`**
   - Inputs: sarif-path, create-issues
   - Steps: Upload SARIF, create GitHub issues, post to PR
   - Used by: reusable-security-scan.yml

7. **`.github/actions/artifact-publish/`**
   - Inputs: artifacts-path, release-tag
   - Steps: Create release, upload artifacts, update checksums
   - Used by: ci-release.yml

8. **`.github/actions/notification/`**
   - Inputs: status, message, channels
   - Steps: Post to Slack/Discord/email as configured
   - Used by: All workflows (on failure)

### 3.4 Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    ENTRY WORKFLOWS                          │
│  (User-facing, triggered by GitHub events)                  │
├─────────────────────────────────────────────────────────────┤
│ ci-pull-request.yml                                         │
│ ci-main-branch.yml                                          │
│ ci-release.yml                                              │
│ nightly.yml                                                 │
│ sdk-tests.yml                                               │
│ security.yml                                                │
│ compliance.yml                                              │
│ docs.yml                                                    │
│ deploy.yml                                                  │
│ tests-extended.yml                                          │
│ tests-specialized.yml                                       │
│ ops-automation.yml                                          │
└──────────────┬──────────────────────────────────────────────┘
               │ calls (workflow_call)
               ▼
┌─────────────────────────────────────────────────────────────┐
│                  REUSABLE WORKFLOWS                         │
│  (Parameterized, called by entry workflows)                 │
├─────────────────────────────────────────────────────────────┤
│ reusable-cpp-build.yml                                      │
│ reusable-sdk-test.yml                                       │
│ reusable-security-scan.yml                                  │
│ reusable-docs-build.yml                                     │
│ reusable-container-build.yml                                │
│ reusable-test-report.yml                                    │
│ reusable-benchmark.yml                                      │
│ reusable-cross-compile.yml                                  │
└──────────────┬──────────────────────────────────────────────┘
               │ uses (composite actions)
               ▼
┌─────────────────────────────────────────────────────────────┐
│                  COMPOSITE ACTIONS                          │
│  (Reusable steps, no workflow context)                      │
├─────────────────────────────────────────────────────────────┤
│ .github/actions/setup-cpp-env/                              │
│ .github/actions/setup-vcpkg/                                │
│ .github/actions/cmake-build/                                │
│ .github/actions/setup-language/                             │
│ .github/actions/report-results/                             │
│ .github/actions/security-report/                            │
│ .github/actions/artifact-publish/                           │
│ .github/actions/notification/                               │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Consolidation Clusters

### 4.1 Cluster 1: PR CI Consolidation

**Current Workflows (6):**
- ci-develop.yml
- develop-ci.yml
- feature-ci.yml
- hotfix-ci.yml
- ci-linux-full.yml
- ci-windows-full.yml

**Consolidation Strategy:**

Create **`ci-pull-request.yml`** that:
- Triggers on all PRs regardless of branch
- Uses matrix strategy for different configurations
- Calls `reusable-cpp-build.yml` with different parameters
- Applies branch-specific logic using conditionals

**Benefits:**
- Single source of truth for PR CI
- Easier to update CI logic
- Consistent behavior across all branches
- Reduced duplication

### 4.2 Cluster 2: SDK Testing Consolidation

**Current Workflows (9):**
- python-sdk-test.yml
- java-sdk-test.yml
- go-sdk-test.yml
- csharp-sdk-test.yml
- ruby-sdk-test.yml
- rust-sdk-test.yml
- swift-sdk-test.yml
- php-sdk-test.yml
- javascript-sdk-test.yml

**Consolidation Strategy:**

Create **`sdk-tests.yml`** with matrix strategy:

```yaml
strategy:
  matrix:
    sdk:
      - { lang: python, versions: ['3.10', '3.11', '3.12'], path: clients/python }
      - { lang: java, versions: ['11', '17', '21'], path: clients/java }
      - { lang: go, versions: ['1.21', '1.22'], path: clients/go }
      # ... etc
```

Calls **`reusable-sdk-test.yml`** which handles:
- Language-specific setup
- Dependency installation
- Test execution
- Build & packaging
- Result reporting

**Benefits:**
- 90% reduction in workflow files (9 → 1)
- Centralized SDK testing logic
- Easy to add new SDKs
- Consistent testing approach

### 4.3 Cluster 3: Security & Compliance Consolidation

**Current Workflows (6):**
- security-scan.yml
- audit-check.yml
- audit-gate-issue.yml
- owasp-zap.yml
- license-compliance.yml
- sbom.yml

**Consolidation Strategy:**

Create 2 workflows:

1. **`security.yml`** - Active security scanning
   - CodeQL analysis
   - OWASP ZAP scanning
   - Container vulnerability scanning
   - Calls `reusable-security-scan.yml`

2. **`compliance.yml`** - Compliance & audit
   - License compliance checks
   - SBOM generation
   - Dependency audits
   - Audit gate decisions

**Benefits:**
- Clear separation of concerns
- Reusable security scanning logic
- Centralized compliance checks

### 4.4 Cluster 4: Documentation Consolidation

**Current Workflows (4):**
- docs.yml
- docs-compendium.yml
- wiki-sync.yml
- documentation-validation.yml

**Consolidation Strategy:**

Create **`docs.yml`** with multiple jobs:
- `build-main-docs` - Main documentation
- `build-compendium` - Compendium documentation
- `sync-wiki` - Wiki synchronization
- `validate-docs` - Link checking and validation

Calls **`reusable-docs-build.yml`** with different parameters for each docs type.

**Benefits:**
- Single workflow for all documentation
- Shared caching and setup
- Consistent deployment process

### 4.5 Cluster 5: Release Consolidation

**Current Workflows (5):**
- release.yml
- release-build.yml
- release-ci.yml
- docker-build.yml
- retroactive-release.yml

**Consolidation Strategy:**

Keep 2 workflows:

1. **`ci-release.yml`** - Main release pipeline
   - Triggered by tags v*.*.*
   - Builds all artifacts
   - Creates GitHub release
   - Deploys Docker images
   - Updates manifests

2. **`deploy.yml`** - Deployment automation
   - Can be triggered manually or by release workflow
   - Handles Docker builds
   - Helm chart deployments

Calls:
- `reusable-cpp-build.yml` for artifacts
- `reusable-container-build.yml` for Docker

**Benefits:**
- Clear release process
- Reusable deployment logic
- Support for retroactive builds via workflow_dispatch

---

## 5. Composite Actions Design

### 5.1 setup-cpp-env

**Purpose:** Set up C++ build environment with system dependencies and vcpkg

**Location:** `.github/actions/setup-cpp-env/action.yml`

**Interface:**
```yaml
inputs:
  use-system-libs:
    description: 'Use system libraries from apt instead of vcpkg'
    required: false
    default: 'false'
  vcpkg-manifest:
    description: 'Path to vcpkg.json manifest'
    required: false
    default: 'vcpkg.json'
  cache-key-suffix:
    description: 'Additional suffix for cache key'
    required: false
    default: ''

outputs:
  vcpkg-root:
    description: 'Path to vcpkg installation'
  cache-hit:
    description: 'Whether cache was hit'
```

**Implementation:**
```yaml
runs:
  using: composite
  steps:
    - name: Install system dependencies
      shell: bash
      run: |
        sudo apt-get update
        sudo apt-get install -y --no-install-recommends \
          cmake ninja-build pkg-config \
          libssl-dev libcurl4-openssl-dev
    
    - name: Setup vcpkg
      uses: ./.github/actions/setup-vcpkg
      with:
        manifest-path: ${{ inputs.vcpkg-manifest }}
        cache-key-suffix: ${{ inputs.cache-key-suffix }}
```

### 5.2 setup-vcpkg

**Purpose:** Install and configure vcpkg with caching

**Location:** `.github/actions/setup-vcpkg/action.yml`

**Interface:**
```yaml
inputs:
  manifest-path:
    description: 'Path to vcpkg.json'
    required: false
    default: 'vcpkg.json'
  cache-key-suffix:
    description: 'Cache key suffix'
    required: false
    default: ''

outputs:
  vcpkg-root:
    description: 'VCPKG_ROOT path'
  cache-hit:
    description: 'Whether cache was restored'
```

### 5.3 cmake-build

**Purpose:** Configure, build, and test with CMake

**Location:** `.github/actions/cmake-build/action.yml`

**Interface:**
```yaml
inputs:
  preset:
    description: 'CMake preset to use'
    required: false
    default: 'default'
  build-type:
    description: 'Build type (Debug/Release)'
    required: false
    default: 'Release'
  run-tests:
    description: 'Run tests after build'
    required: false
    default: 'true'
  install-prefix:
    description: 'Install prefix for artifacts'
    required: false
    default: ''

outputs:
  build-dir:
    description: 'Build directory path'
  install-dir:
    description: 'Install directory path'
```

### 5.4 setup-language

**Purpose:** Setup language runtime with caching (Python/Node/Java/etc.)

**Location:** `.github/actions/setup-language/action.yml`

**Interface:**
```yaml
inputs:
  language:
    description: 'Language to setup (python/node/java/go/ruby/rust/swift/php)'
    required: true
  version:
    description: 'Version to install'
    required: true
  cache-dependency-path:
    description: 'Path to dependency file for cache key'
    required: false
    default: ''

outputs:
  cache-hit:
    description: 'Whether cache was hit'
```

### 5.5 report-results

**Purpose:** Report test results with PR comments and artifacts

**Location:** `.github/actions/report-results/action.yml`

**Interface:**
```yaml
inputs:
  test-results-path:
    description: 'Path to test results (JUnit XML)'
    required: false
  coverage-path:
    description: 'Path to coverage report'
    required: false
  create-pr-comment:
    description: 'Create PR comment with results'
    required: false
    default: 'true'
  artifact-name:
    description: 'Artifact name for test results'
    required: false
    default: 'test-results'
```

### 5.6 security-report

**Purpose:** Upload security findings and create issues

**Location:** `.github/actions/security-report/action.yml`

**Interface:**
```yaml
inputs:
  sarif-path:
    description: 'Path to SARIF file'
    required: false
  create-issues:
    description: 'Create GitHub issues for findings'
    required: false
    default: 'false'
  severity-threshold:
    description: 'Minimum severity to report (low/medium/high/critical)'
    required: false
    default: 'medium'
```

---

## 6. Reusable Workflows Design

### 6.1 reusable-cpp-build.yml

**Purpose:** Build and test ThemisDB C++ codebase

**Interface:**
```yaml
name: Reusable C++ Build

on:
  workflow_call:
    inputs:
      os:
        description: 'Operating system'
        required: false
        type: string
        default: 'ubuntu-latest'
      build-type:
        description: 'Build type'
        required: false
        type: string
        default: 'Release'
      use-system-libs:
        description: 'Use system libraries'
        required: false
        type: boolean
        default: false
      cmake-preset:
        description: 'CMake preset'
        required: false
        type: string
        default: 'default'
      run-tests:
        description: 'Run tests'
        required: false
        type: boolean
        default: true
      upload-artifacts:
        description: 'Upload build artifacts'
        required: false
        type: boolean
        default: false
    outputs:
      build-status:
        description: 'Build status'
        value: ${{ jobs.build.outputs.status }}

jobs:
  build:
    runs-on: ${{ inputs.os }}
    outputs:
      status: ${{ steps.build.outcome }}
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Setup C++ environment
        uses: ./.github/actions/setup-cpp-env
        with:
          use-system-libs: ${{ inputs.use-system-libs }}
      
      - name: Build with CMake
        id: build
        uses: ./.github/actions/cmake-build
        with:
          preset: ${{ inputs.cmake-preset }}
          build-type: ${{ inputs.build-type }}
          run-tests: ${{ inputs.run-tests }}
      
      - name: Upload artifacts
        if: inputs.upload-artifacts
        uses: actions/upload-artifact@v4
        with:
          name: themisdb-${{ inputs.os }}-${{ inputs.build-type }}
          path: build/install/
```

### 6.2 reusable-sdk-test.yml

**Purpose:** Test SDK for a specific language

**Interface:**
```yaml
name: Reusable SDK Test

on:
  workflow_call:
    inputs:
      language:
        description: 'SDK language'
        required: true
        type: string
      version:
        description: 'Language version'
        required: true
        type: string
      sdk-path:
        description: 'Path to SDK directory'
        required: true
        type: string
      dry-run:
        description: 'Dry run mode (no publishing)'
        required: false
        type: boolean
        default: true

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup language
        uses: ./.github/actions/setup-language
        with:
          language: ${{ inputs.language }}
          version: ${{ inputs.version }}
      
      - name: Install dependencies
        working-directory: ${{ inputs.sdk-path }}
        run: |
          # Language-specific dependency installation
          # Handled by action or conditional steps
      
      - name: Run tests
        working-directory: ${{ inputs.sdk-path }}
        run: |
          # Language-specific test command
      
      - name: Build package
        if: inputs.dry-run == false
        working-directory: ${{ inputs.sdk-path }}
        run: |
          # Language-specific build command
      
      - name: Report results
        uses: ./.github/actions/report-results
        with:
          test-results-path: ${{ inputs.sdk-path }}/test-results
```

### 6.3 reusable-security-scan.yml

**Purpose:** Run security scanning with various tools

**Interface:**
```yaml
name: Reusable Security Scan

on:
  workflow_call:
    inputs:
      scanner-type:
        description: 'Scanner type (codeql/trivy/zap/gitleaks)'
        required: true
        type: string
      target-path:
        description: 'Path to scan'
        required: false
        type: string
        default: '.'
      upload-sarif:
        description: 'Upload SARIF to GitHub'
        required: false
        type: boolean
        default: true
      create-issues:
        description: 'Create issues for findings'
        required: false
        type: boolean
        default: false

jobs:
  scan:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      security-events: write
      issues: write
    steps:
      - uses: actions/checkout@v4
      
      - name: Run CodeQL
        if: inputs.scanner-type == 'codeql'
        uses: github/codeql-action/analyze@v3
      
      - name: Run Trivy
        if: inputs.scanner-type == 'trivy'
        uses: aquasecurity/trivy-action@master
        with:
          scan-type: 'fs'
          scan-ref: ${{ inputs.target-path }}
      
      - name: Run OWASP ZAP
        if: inputs.scanner-type == 'zap'
        uses: zaproxy/action-baseline@v0.12.0
      
      - name: Security report
        uses: ./.github/actions/security-report
        with:
          sarif-path: results.sarif
          create-issues: ${{ inputs.create-issues }}
```

---

## 7. Standard Conventions

### 7.1 Naming Conventions

#### Workflow Files
- Entry workflows: `{purpose}.yml` or `{purpose}-{target}.yml`
  - Examples: `ci-pull-request.yml`, `ci-main-branch.yml`, `docs.yml`
- Reusable workflows: `reusable-{purpose}.yml`
  - Examples: `reusable-cpp-build.yml`, `reusable-sdk-test.yml`

#### Composite Actions
- Directory: `.github/actions/{action-name}/`
- File: `.github/actions/{action-name}/action.yml`
- Examples: `setup-cpp-env`, `cmake-build`, `report-results`

#### Jobs
- Use descriptive kebab-case IDs: `build-and-test`, `security-scan`, `deploy-production`
- Use descriptive names for display: `Build & Test`, `Security Scan`, `Deploy to Production`

#### Artifacts
- Format: `{project}-{platform}-{config}-{version}`
- Examples: `themisdb-linux-release-v1.2.3`, `sdk-python-wheel-3.12`

### 7.2 Standard Permissions

#### Default (most restrictive)
```yaml
permissions:
  contents: read
```

#### PR Workflows (with comments)
```yaml
permissions:
  contents: read
  pull-requests: write
  checks: write
```

#### Security Workflows
```yaml
permissions:
  contents: read
  security-events: write
  issues: write
```

#### Release Workflows
```yaml
permissions:
  contents: write
  packages: write
  id-token: write
```

### 7.3 Standard Concurrency

#### PR Workflows
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

#### Main Branch Workflows
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: false
```

#### Release Workflows
```yaml
concurrency:
  group: release-${{ github.ref }}
  cancel-in-progress: false
```

### 7.4 Standard Environment Variables

All workflows should use these standard env vars:

```yaml
env:
  BUILD_TYPE: Release
  CMAKE_BUILD_PARALLEL_LEVEL: 4
  CTEST_PARALLEL_LEVEL: 4
  VCPKG_MAX_CONCURRENCY: 4
  USE_SYSTEM_LIBS: ${{ inputs.use-system-libs || 'OFF' }}
```

### 7.5 Standard Timeouts

```yaml
# Quick CI checks (PR)
timeout-minutes: 30

# Full builds
timeout-minutes: 60

# Extended tests (nightly)
timeout-minutes: 120

# Security scans
timeout-minutes: 45
```

### 7.6 Standard Caching

#### vcpkg Cache
```yaml
- uses: actions/cache@v4
  with:
    path: |
      ~/.cache/vcpkg
      vcpkg_installed
    key: vcpkg-${{ runner.os }}-${{ hashFiles('vcpkg.json', 'vcpkg-configuration.json') }}
    restore-keys: |
      vcpkg-${{ runner.os }}-
```

#### CMake Build Cache
```yaml
- uses: actions/cache@v4
  with:
    path: build/.cache
    key: cmake-${{ runner.os }}-${{ hashFiles('CMakeLists.txt', 'cmake/**') }}
```

#### Language-Specific Caches
Use built-in caching in setup actions:
```yaml
- uses: actions/setup-python@v5
  with:
    python-version: '3.12'
    cache: 'pip'
    cache-dependency-path: '**/requirements*.txt'
```

---

## 8. Migration Plan

### 8.1 Phase 1: Foundation (Week 1-2)

**Goal:** Create reusable building blocks without disrupting existing workflows

**Tasks:**
1. Create composite actions:
   - [ ] `.github/actions/setup-cpp-env/`
   - [ ] `.github/actions/setup-vcpkg/`
   - [ ] `.github/actions/cmake-build/`
   - [ ] `.github/actions/setup-language/`
   - [ ] `.github/actions/report-results/`

2. Create initial reusable workflows:
   - [ ] `reusable-cpp-build.yml`
   - [ ] `reusable-sdk-test.yml`
   - [ ] `reusable-security-scan.yml`

3. Test reusable components:
   - [ ] Create test workflow to validate composite actions
   - [ ] Verify reusable workflows work correctly
   - [ ] Document usage and examples

**Success Criteria:**
- All composite actions work in isolation
- Reusable workflows can be called successfully
- No impact on existing workflows

**Rollback:** Delete new files if issues arise

### 8.2 Phase 2: Parallel New Workflows (Week 3-4)

**Goal:** Create new consolidated workflows alongside existing ones

**Tasks:**
1. Create new entry workflows:
   - [ ] `ci-pull-request.yml` (parallel to existing PR workflows)
   - [ ] `sdk-tests.yml` (parallel to existing SDK workflows)
   - [ ] `security.yml` (parallel to existing security workflows)

2. Configure new workflows to run in parallel:
   - Use different trigger paths or manual dispatch initially
   - Add clear indicators in workflow names (e.g., "[NEW]")
   - Monitor both old and new workflows

3. Compare results:
   - Verify new workflows produce same/better results
   - Check execution time and resource usage
   - Gather feedback from team

**Success Criteria:**
- New workflows run successfully
- Results match or exceed old workflows
- Team approves new approach

**Rollback:** Keep old workflows running, remove new ones if needed

### 8.3 Phase 3: Gradual Migration (Week 5-6)

**Goal:** Switch to new workflows for specific categories

**Tasks:**
1. SDK Testing Migration:
   - [ ] Enable `sdk-tests.yml` for all PR triggers
   - [ ] Disable 9 old SDK test workflows (rename to `*.yml.disabled`)
   - [ ] Monitor for 1 week

2. PR CI Migration:
   - [ ] Enable `ci-pull-request.yml` for all PRs
   - [ ] Disable old PR workflows (ci-develop, feature-ci, etc.)
   - [ ] Monitor for 1 week

3. Security Migration:
   - [ ] Enable new `security.yml`
   - [ ] Disable old security workflows
   - [ ] Monitor for 1 week

**Success Criteria:**
- New workflows handle all cases correctly
- No regressions in CI/CD functionality
- CI/CD execution time improved or maintained

**Rollback:** Re-enable old workflows, disable new ones

### 8.4 Phase 4: Complete Migration (Week 7-8)

**Goal:** Migrate all remaining workflows

**Tasks:**
1. Migrate remaining categories:
   - [ ] Documentation (`docs.yml`)
   - [ ] Release (`ci-release.yml`, `deploy.yml`)
   - [ ] Testing (`tests-extended.yml`, `tests-specialized.yml`)
   - [ ] Operations (`ops-automation.yml`)

2. Update branch protection rules:
   - Update required checks to use new workflow names
   - Remove old workflow names from required checks

3. Update documentation:
   - Update CONTRIBUTING.md with new CI/CD structure
   - Add CI/CD documentation to docs/ci-cd/
   - Update README badges if applicable

**Success Criteria:**
- All workflows migrated
- Branch protection updated
- Documentation current

**Rollback:** Revert to old workflows if critical issues

### 8.5 Phase 5: Cleanup (Week 9)

**Goal:** Remove old workflows and finalize migration

**Tasks:**
1. Archive old workflows:
   - [ ] Move old workflow files to `.github/workflows/_archived/`
   - [ ] Add README in archived folder explaining history
   - [ ] Keep in git history for reference

2. Final validation:
   - [ ] Run full CI/CD pipeline end-to-end
   - [ ] Verify all triggers work correctly
   - [ ] Test edge cases (manual dispatch, schedule, etc.)

3. Document lessons learned:
   - [ ] Update consolidation plan with actual results
   - [ ] Document any deviations from plan
   - [ ] Share knowledge with team

**Success Criteria:**
- Old workflows archived
- New structure fully operational
- Team trained on new structure

**Rollback:** N/A (keep old workflows in archive)

### 8.6 Migration Timeline

```
Week 1-2: Foundation
├─ Create composite actions
├─ Create reusable workflows
└─ Test components

Week 3-4: Parallel Workflows
├─ Create new entry workflows
├─ Run in parallel with old
└─ Compare and validate

Week 5-6: Gradual Migration
├─ Migrate SDK testing
├─ Migrate PR CI
└─ Migrate Security

Week 7-8: Complete Migration
├─ Migrate remaining workflows
├─ Update branch protection
└─ Update documentation

Week 9: Cleanup
├─ Archive old workflows
├─ Final validation
└─ Document learnings
```

---

## 9. Risk Assessment and Mitigation

### 9.1 Identified Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Breaking CI/CD during migration** | Medium | High | Parallel deployment, gradual rollout, quick rollback plan |
| **Incompatibility with existing tooling** | Low | Medium | Thorough testing, maintain backward compatibility |
| **Team resistance to change** | Medium | Medium | Clear communication, training, documentation |
| **Increased complexity in short term** | High | Low | Accept temporary duplication, clear migration path |
| **Missing edge cases** | Medium | Medium | Comprehensive testing, staged rollout, monitoring |
| **Performance regression** | Low | Medium | Benchmark before/after, optimize caching |
| **Loss of historical data** | Low | Low | Keep old workflows in archive, maintain git history |

### 9.2 Mitigation Strategies

#### Strategy 1: Parallel Deployment
- Run new workflows alongside old ones initially
- Compare results side-by-side
- Only switch when confident in new implementation

#### Strategy 2: Feature Flags
- Use workflow_dispatch with inputs for testing
- Gradually enable for different branches
- Control rollout pace

#### Strategy 3: Quick Rollback
- Keep old workflows in repository (disabled)
- Document rollback procedure
- Test rollback before migration

#### Strategy 4: Comprehensive Testing
- Test all trigger types (PR, push, tag, schedule, dispatch)
- Test matrix combinations
- Test failure scenarios

#### Strategy 5: Team Communication
- Weekly updates during migration
- Clear documentation of changes
- Training sessions on new structure

### 9.3 Rollback Procedure

If critical issues occur during migration:

1. **Immediate Rollback (< 5 minutes):**
   ```bash
   # Disable new workflow
   mv .github/workflows/ci-pull-request.yml .github/workflows/ci-pull-request.yml.disabled
   
   # Re-enable old workflows
   mv .github/workflows/feature-ci.yml.disabled .github/workflows/feature-ci.yml
   mv .github/workflows/develop-ci.yml.disabled .github/workflows/develop-ci.yml
   # ... repeat for other workflows
   
   # Commit and push
   git commit -am "Rollback: Re-enable old CI workflows"
   git push
   ```

2. **Update Branch Protection:**
   - Revert required checks to old workflow names
   - Temporarily disable checks if necessary

3. **Communicate:**
   - Notify team of rollback
   - Create incident post-mortem
   - Plan remediation

### 9.4 Success Metrics

Track these metrics to validate migration:

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| **Total Workflow Files** | 53 | 15-20 | File count |
| **Average PR CI Time** | ~15-20 min | <15 min | Workflow analytics |
| **Lines of Workflow YAML** | ~8000+ | <4000 | Line count |
| **Maintenance Overhead** | High | Low | Subjective (team survey) |
| **Failed Workflow Rate** | ~5% | <3% | Workflow analytics |
| **Cache Hit Rate** | ~40% | >70% | Cache metrics |

---

## 10. Success Metrics

### 10.1 Quantitative Metrics

#### Reduction Metrics
- **Workflow Count:** 53 → 15-20 (62-70% reduction)
- **Total YAML Lines:** ~8000 → <4000 (50%+ reduction)
- **Duplicate Steps:** ~200+ → <50 (75%+ reduction)

#### Performance Metrics
- **PR CI Time:** <15 minutes average
- **Cache Hit Rate:** >70%
- **Workflow Success Rate:** >97%
- **Time to Add New SDK:** <30 minutes (vs hours currently)

#### Maintenance Metrics
- **Time to Update Common Pattern:** <10 minutes (single file change)
- **Onboarding Time:** <2 hours to understand CI/CD (vs 8+ hours)
- **CI/CD Incidents:** <1 per month

### 10.2 Qualitative Metrics

#### Team Satisfaction
- Survey team before and after migration
- Measure understanding of CI/CD structure
- Assess confidence in making CI/CD changes

#### Documentation Quality
- All workflows documented
- Clear examples for common changes
- Up-to-date architecture diagrams

#### Maintainability
- Easier to update common patterns
- Reduced duplication
- Clear separation of concerns

---

## 11. Appendix

### 11.1 Workflow Consolidation Matrix

Complete mapping of old workflows to new structure:

| Old Workflow | New Workflow | Reusable Component | Status |
|--------------|--------------|-------------------|--------|
| ci-develop.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| develop-ci.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| feature-ci.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| hotfix-ci.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| ci-linux-full.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| ci-windows-full.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| ci.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| main-ci.yml | ci-main-branch.yml | reusable-cpp-build.yml | Consolidate |
| python-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| java-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| go-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| csharp-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| ruby-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| rust-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| swift-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| php-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| javascript-sdk-test.yml | sdk-tests.yml | reusable-sdk-test.yml | Consolidate |
| security-scan.yml | security.yml | reusable-security-scan.yml | Consolidate |
| owasp-zap.yml | security.yml | reusable-security-scan.yml | Consolidate |
| audit-check.yml | compliance.yml | - | Consolidate |
| audit-gate-issue.yml | compliance.yml | - | Consolidate |
| license-compliance.yml | compliance.yml | - | Consolidate |
| sbom.yml | compliance.yml | - | Consolidate |
| docs.yml | docs.yml | reusable-docs-build.yml | Enhance |
| docs-compendium.yml | docs.yml | reusable-docs-build.yml | Consolidate |
| wiki-sync.yml | docs.yml | reusable-docs-build.yml | Consolidate |
| documentation-validation.yml | docs.yml | - | Consolidate |
| release.yml | ci-release.yml | reusable-cpp-build.yml | Consolidate |
| release-build.yml | ci-release.yml | reusable-cpp-build.yml | Consolidate |
| release-ci.yml | ci-release.yml | reusable-cpp-build.yml | Consolidate |
| docker-build.yml | deploy.yml | reusable-container-build.yml | Consolidate |
| retroactive-release.yml | ci-release.yml | - | Keep separate |
| helm-chart-test.yml | deploy.yml | - | Consolidate |
| wordpress-theme-deploy.yml | deploy.yml | - | Consolidate |
| chaos-tests.yml | tests-extended.yml | - | Consolidate |
| durability-tests.yml | tests-extended.yml | - | Consolidate |
| dr-testing.yml | tests-extended.yml | - | Consolidate (fix YAML) |
| fuzzing.yml | tests-specialized.yml | - | Keep separate |
| ci-sanitizers.yml | tests-specialized.yml | reusable-cpp-build.yml | Consolidate |
| ci-arm-cross.yml | tests-specialized.yml | reusable-cross-compile.yml | Keep separate |
| cross-compile-review.yml | tests-specialized.yml | - | Consolidate |
| performance-regression-check.yml | nightly.yml | reusable-benchmark.yml | Consolidate |
| sharding-benchmark.yml | nightly.yml | reusable-benchmark.yml | Consolidate |
| update-performance-baselines.yml | nightly.yml | - | Consolidate |
| access-review.yml | ops-automation.yml | - | Consolidate |
| incident-drill.yml | ops-automation.yml | - | Consolidate (fix YAML) |
| attack-vector-analysis.yml | security.yml | - | Keep separate |
| llama-cpp-integration.yml | tests-specialized.yml | - | Keep separate |
| lora-framework-ci.yml | ci-pull-request.yml | - | Consolidate |
| build-and-test.yml | ci-pull-request.yml | reusable-cpp-build.yml | Consolidate |
| ci-local-test.yml | - | - | Keep for local testing |
| chimera-neutrality-check.yml | tests-specialized.yml | - | Keep separate |
| reusable-test-report.yml | - | - | Keep and enhance |

### 11.2 Composite Actions Dependencies

```
setup-cpp-env/
├─ Depends on: setup-vcpkg/
└─ Used by: reusable-cpp-build.yml, ci-pull-request.yml

setup-vcpkg/
├─ Depends on: (none - uses standard actions)
└─ Used by: setup-cpp-env/

cmake-build/
├─ Depends on: setup-cpp-env/
└─ Used by: reusable-cpp-build.yml

setup-language/
├─ Depends on: (none - uses standard actions)
└─ Used by: reusable-sdk-test.yml

report-results/
├─ Depends on: (none - uses standard actions)
└─ Used by: All test workflows

security-report/
├─ Depends on: (none - uses standard actions)
└─ Used by: reusable-security-scan.yml, security.yml
```

### 11.3 References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Reusing Workflows](https://docs.github.com/en/actions/using-workflows/reusing-workflows)
- [Creating Composite Actions](https://docs.github.com/en/actions/creating-actions/creating-a-composite-action)
- [Workflow Syntax](https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions)

### 11.4 Change Log

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-02-10 | CI/CD Team | Initial consolidation plan |

---

## Next Steps

1. **Review and Approval:**
   - Review this plan with the team
   - Gather feedback and adjust as needed
   - Get approval to proceed

2. **Start Phase 1:**
   - Create composite actions
   - Test in isolation
   - Document usage

3. **Track Progress:**
   - Use this document as living reference
   - Update with actual progress
   - Document deviations and learnings

4. **Communication:**
   - Regular updates to team
   - Clear documentation
   - Training and support

---

**End of Consolidation Plan**
