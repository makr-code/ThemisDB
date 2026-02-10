# GitHub Actions Workflows Inventory

**Generated:** 2026-02-10 11:38:20

**Total Workflows:** 51

## Table of Contents

1. [Summary Statistics](#summary-statistics)
2. [Workflows by Category](#workflows-by-category)
3. [Detailed Workflow Inventory](#detailed-workflow-inventory)
4. [Common Patterns Analysis](#common-patterns-analysis)
5. [Actions Usage Summary](#actions-usage-summary)

## Summary Statistics

- **Total Workflows:** 51
- **Total Jobs:** 175
- **Unique Actions Used:** 43

### Workflows by Category

- **Branch CI:** 1 workflows
- **Container & Deployment:** 1 workflows
- **Cross-Compilation:** 2 workflows
- **Documentation:** 4 workflows
- **Fuzzing:** 1 workflows
- **LoRA/LLM:** 2 workflows
- **Other:** 1 workflows
- **PR CI:** 6 workflows
- **Performance & Benchmarking:** 3 workflows
- **Release:** 5 workflows
- **SDK Testing:** 9 workflows
- **Sanitizers:** 1 workflows
- **Scheduled & Manual:** 6 workflows
- **Security & Compliance:** 4 workflows
- **Testing:** 5 workflows

## Workflows by Category

### Branch CI

- **main-ci.yml** - Main Branch CI

### Container & Deployment

- **helm-chart-test.yml** - Helm Chart [Dry-Run]

### Cross-Compilation

- **ci-arm-cross.yml** - ARM Cross-Compilation
- **cross-compile-review.yml** - Cross-Compile Code Review

### Documentation

- **docs-compendium.yml** - Compendium Build & Deploy
- **docs.yml** - Documentation Build & Deploy
- **documentation-validation.yml** - Documentation Validation
- **wiki-sync.yml** - Sync Documentation to GitHub Wiki

### Fuzzing

- **fuzzing.yml** - AFL++ Fuzzing

### LoRA/LLM

- **llama-cpp-integration.yml** - llama.cpp Integration Tests
- **lora-framework-ci.yml** - LoRA Framework CI/CD

### Other

- **chimera-neutrality-check.yml** - CHIMERA Neutrality Check

### PR CI

- **ci-develop.yml** - CI - Development Pipeline
- **ci-linux-full.yml** - Linux Full Build & Error Tracking
- **ci-windows-full.yml** - Windows Full Build & Error Tracking
- **develop-ci.yml** - Develop CI
- **feature-ci.yml** - Feature/Bugfix CI
- **hotfix-ci.yml** - Hotfix CI

### Performance & Benchmarking

- **performance-regression-check.yml** - Performance Regression Check
- **sharding-benchmark.yml** - CHIMERA Suite - Sharding Benchmarks (Weekly)
- **update-performance-baselines.yml** - Update Performance Baselines

### Release

- **docker-build.yml** - Docker Build & Release
- **release-build.yml** - Release Build & Deploy
- **release-ci.yml** - Release CI
- **release.yml** - Release - Automated Build & Publish
- **retroactive-release.yml** - Retroactive Release Build

### SDK Testing

- **csharp-sdk-test.yml** - C# SDK [Dry-Run]
- **go-sdk-test.yml** - Go SDK [Dry-Run]
- **java-sdk-test.yml** - Java SDK [Dry-Run]
- **javascript-sdk-test.yml** - JavaScript/TypeScript SDK [Dry-Run]
- **php-sdk-test.yml** - PHP SDK [Dry-Run]
- **python-sdk-test.yml** - Python SDK [Dry-Run]
- **ruby-sdk-test.yml** - Ruby SDK [Dry-Run]
- **rust-sdk-test.yml** - Rust SDK [Dry-Run]
- **swift-sdk-test.yml** - Swift SDK [Dry-Run]

### Sanitizers

- **ci-sanitizers.yml** - Sanitizer Coverage

### Scheduled & Manual

- **access-review.yml** - Access Review Automation
- **attack-vector-analysis.yml** - Attack Vector Analysis (Systematic)
- **ci.yml** - CI
- **license-compliance.yml** - License Compliance
- **sbom.yml** - SBOM Generation
- **wordpress-theme-deploy.yml** - WordPress Theme Deploy

### Security & Compliance

- **audit-check.yml** - Audit Check
- **audit-gate-issue.yml** - Create Audit Gate Issue
- **owasp-zap.yml** - OWASP ZAP Security Scan
- **security-scan.yml** - Security Scanning (SAST)

### Testing

- **build-and-test.yml** - Build & Test - Main Branch Protection
- **chaos-tests.yml** - Chaos and Sanitizer Tests
- **ci-local-test.yml** - CI [System Libraries Only]
- **durability-tests.yml** - Durability & Crash Recovery Tests
- **reusable-test-report.yml** - Reusable Test Report Publisher

## Detailed Workflow Inventory

### access-review.yml

**Name:** Access Review Automation

**Path:** `.github/workflows/access-review.yml`

**Triggers:**
- Events: schedule, workflow_dispatch
- Schedule: 0 9 1 * *

**Jobs (1):**
- `access-review`: access-review (runs-on: ubuntu-latest)

**Actions Used (3):**
- `actions/checkout`
- `actions/github-script`
- `actions/upload-artifact`

---

### attack-vector-analysis.yml

**Name:** Attack Vector Analysis (Systematic)

**Path:** `.github/workflows/attack-vector-analysis.yml`

**Triggers:**
- Events: schedule, workflow_dispatch
- Schedule: 0 3 * * 2

**Jobs (8):**
- `preparation`: Analyse-Vorbereitung (runs-on: ubuntu-latest)
- `network-vectors`: Analyse Netzwerk-Angriffsvektoren (runs-on: ubuntu-latest)
- `auth-vectors`: Analyse Auth-Angriffsvektoren (runs-on: ubuntu-latest)
- `injection-vectors`: Analyse Injection-Angriffsvektoren (runs-on: ubuntu-latest)
- `crypto-vectors`: Analyse Krypto-Angriffsvektoren (runs-on: ubuntu-latest)
- `distributed-vectors`: Analyse Distributed-System-Angriffsvektoren (runs-on: ubuntu-latest)
- `generate-report`: Erstelle Analyse-Bericht (runs-on: ubuntu-latest)
- `validate-baseline`: Validiere gegen Security Baseline (runs-on: ubuntu-latest)

**Technologies:** CodeQL, Fuzzing, Go, Java, LoRA/LLM, OWASP ZAP, SBOM/Anchore

**Permissions:**
- contents: read
- security-events: write
- issues: write

**Actions Used (4):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/github-script`
- `actions/upload-artifact`

---

### audit-check.yml

**Name:** Audit Check

**Path:** `.github/workflows/audit-check.yml`

**Triggers:**
- Events: pull_request, push, release, workflow_dispatch
- Branches: main, master, develop, release/*, hotfix/*
- Tags: v*.*.*
- Types: opened, synchronize, reopened, published, prereleased

**Jobs (8):**
- `sast`: SAST - Static Analysis (runs-on: ubuntu-latest)
- `dependency-scan`: Dependency Vulnerability Scan (runs-on: ubuntu-latest)
- `test-coverage`: Test Coverage Analysis (runs-on: ubuntu-latest)
- `container-scan`: Container Security Scan (runs-on: ubuntu-latest)
- `dast`: DAST - Dynamic Analysis (runs-on: ubuntu-latest)
- `compliance-check`: Compliance Verification (runs-on: ubuntu-latest)
- `audit-report`: Generate Audit Report (runs-on: ubuntu-latest)
- `audit-gate`: Audit Gate Decision (runs-on: ubuntu-latest)

**Technologies:** CMake, CodeQL, Docker, OWASP ZAP, vcpkg

**Permissions:**
- contents: read
- security-events: write
- pull-requests: write
- issues: write
- actions: read

**Actions Used (10):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/github-script`
- `actions/upload-artifact`
- `aquasecurity/trivy-action`
- `docker/build-push-action`
- `docker/setup-buildx-action`
- `github/codeql-action/upload-sarif`
- `gitleaks/gitleaks-action`
- `zaproxy/action-baseline`

---

### audit-gate-issue.yml

**Name:** Create Audit Gate Issue

**Path:** `.github/workflows/audit-gate-issue.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Tags: v*.*.*, v*.*.*-rc.*, v*.*.*-beta.*

**Jobs (1):**
- `create-audit-issue`: Create Audit Tracking Issue (runs-on: ubuntu-latest)

**Technologies:** Go, OWASP ZAP

**Permissions:**
- contents: read
- issues: write

**Actions Used (2):**
- `actions/checkout`
- `actions/github-script`

---

### build-and-test.yml

**Name:** Build & Test - Main Branch Protection

**Path:** `.github/workflows/build-and-test.yml`

**Triggers:**
- Events: pull_request
- Branches: main

**Jobs (6):**
- `validate`: Pre-Build Validation (runs-on: ubuntu-latest)
- `build-test-ubuntu`: Full Build & Test - Ubuntu (runs-on: ubuntu-latest)
- `build-test-windows`: Full Build & Test - Windows (runs-on: windows-latest)
- `build-test-macos`: Full Build & Test - macOS (runs-on: macos-latest)
- `security-scan`: Security Scan (runs-on: ubuntu-latest)
- `build-test-status`: Build & Test Status Check (runs-on: ubuntu-latest)

**Technologies:** CMake, Java, vcpkg

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Concurrency:**
- group: build-test-main-${{ github.ref }}
- cancel-in-progress: True

**Actions Used (4):**
- `actions/checkout`
- `actions/upload-artifact`
- `dorny/test-reporter`
- `gitleaks/gitleaks-action`

---

### chaos-tests.yml

**Name:** Chaos and Sanitizer Tests

**Path:** `.github/workflows/chaos-tests.yml`

**Triggers:**
- Events: workflow_dispatch, schedule, pull_request
- Paths: 6 path filters
- Schedule: 0 2 * * *

**Jobs (4):**
- `thread-sanitizer`: ThreadSanitizer Tests (runs-on: ubuntu-latest)
- `chaos-tests`: Chaos Engineering Tests (runs-on: ubuntu-latest)
- `integration-tests`: Integration Tests (runs-on: ubuntu-latest)
- `summary`: Test Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, Sanitizers

**Permissions:**
- contents: read

**Actions Used (2):**
- `actions/checkout`
- `actions/upload-artifact`

---

### chimera-neutrality-check.yml

**Name:** CHIMERA Neutrality Check

**Path:** `.github/workflows/chimera-neutrality-check.yml`

**Triggers:**
- Events: pull_request, push
- Branches: main, develop
- Paths: 4 path filters

**Jobs (1):**
- `neutrality-check`: Check Vendor Neutrality (runs-on: ubuntu-latest)

**Technologies:** Python

**Actions Used (3):**
- `actions/checkout`
- `actions/github-script`
- `actions/setup-python`

---

### ci-arm-cross.yml

**Name:** ARM Cross-Compilation

**Path:** `.github/workflows/ci-arm-cross.yml`

**Triggers:**
- Events: push, pull_request, schedule, workflow_dispatch
- Branches: main, develop, release/**, main, develop
- Schedule: 0 4 * * 0

**Jobs (4):**
- `arm64-cross-compile`: ARM64 Cross-Compilation (runs-on: ubuntu-22.04)
- `armv7-cross-compile`: ARMv7 Cross-Compilation (runs-on: ubuntu-22.04)
- `arm-error-analysis`: ARM Error Analysis (runs-on: ubuntu-latest)
- `docker-arm-test`: Docker ARM Test (runs-on: ubuntu-latest)

**Technologies:** CMake, Docker, Go, Python

**Actions Used (6):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/setup-python`
- `actions/upload-artifact`
- `docker/setup-buildx-action`
- `docker/setup-qemu-action`

---

### ci-develop.yml

**Name:** CI - Development Pipeline

**Path:** `.github/workflows/ci-develop.yml`

**Triggers:**
- Events: push, pull_request
- Branches: develop, develop

**Jobs (5):**
- `validate`: Validate Changes (runs-on: ubuntu-latest)
- `build-ubuntu`: Build & Test - Ubuntu (runs-on: ubuntu-latest)
- `build-windows`: Build & Test - Windows (runs-on: windows-latest)
- `build-macos`: Build & Test - macOS (runs-on: macos-latest)
- `ci-summary`: CI Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, vcpkg

**Permissions:**
- contents: read
- pull-requests: write

**Concurrency:**
- group: ci-develop-${{ github.ref }}
- cancel-in-progress: True

**Actions Used (2):**
- `actions/checkout`
- `actions/upload-artifact`

---

### ci-linux-full.yml

**Name:** Linux Full Build & Error Tracking

**Path:** `.github/workflows/ci-linux-full.yml`

**Triggers:**
- Events: push, pull_request, schedule, workflow_dispatch
- Branches: main, develop, release/**, hotfix/**, main, develop
- Schedule: 0 3 * * *

**Jobs (4):**
- `linux-gcc-build`: Linux GCC ${{ matrix.gcc_version }} (runs-on: ubuntu-22.04)
- `linux-clang-build`: Linux Clang ${{ matrix.clang_version }} (runs-on: ubuntu-22.04)
- `linux-compatibility-check`: Linux Compatibility Check (runs-on: ubuntu-22.04)
- `error-summary`: Generate Linux Error Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, Python

**Actions Used (5):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/github-script`
- `actions/setup-python`
- `actions/upload-artifact`

---

### ci-local-test.yml

**Name:** CI [System Libraries Only]

**Path:** `.github/workflows/ci-local-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (1):**
- `build-system-libs-only`: Build with System Libraries Only (runs-on: ubuntu-24.04)

**Technologies:** CMake, vcpkg

**Permissions:**
- contents: read

**Concurrency:**
- group: ${{ github.workflow }}-${{ github.ref }}
- cancel-in-progress: True

**Actions Used (1):**
- `actions/checkout`

---

### ci-sanitizers.yml

**Name:** Sanitizer Coverage

**Path:** `.github/workflows/ci-sanitizers.yml`

**Triggers:**
- Events: push, pull_request, schedule, workflow_dispatch
- Branches: main, develop, main, develop
- Schedule: 0 5 * * 1

**Jobs (6):**
- `address-sanitizer`: AddressSanitizer (ASan) (runs-on: ubuntu-22.04)
- `undefined-behavior-sanitizer`: UndefinedBehaviorSanitizer (UBSan) (runs-on: ubuntu-22.04)
- `thread-sanitizer`: ThreadSanitizer (TSan) (runs-on: ubuntu-22.04)
- `memory-sanitizer`: MemorySanitizer (MSan) (runs-on: ubuntu-22.04)
- `sanitizer-summary`: Sanitizer Summary (runs-on: ubuntu-latest)
- `valgrind-check`: Valgrind Memory Check (runs-on: ubuntu-22.04)

**Technologies:** CMake, Sanitizers

**Actions Used (4):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/github-script`
- `actions/upload-artifact`

---

### ci-windows-full.yml

**Name:** Windows Full Build & Error Tracking

**Path:** `.github/workflows/ci-windows-full.yml`

**Triggers:**
- Events: push, pull_request, schedule, workflow_dispatch
- Branches: main, develop, release/**, hotfix/**, main, develop
- Schedule: 0 2 * * *

**Jobs (3):**
- `windows-msvc-build`: Windows MSVC ${{ matrix.config.name }} (runs-on: windows-2022)
- `windows-clang-cl`: Windows Clang-CL (runs-on: windows-2022)
- `error-summary`: Generate Error Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, Python, vcpkg

**Actions Used (7):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/github-script`
- `actions/setup-python`
- `actions/upload-artifact`
- `lukka/run-vcpkg`
- `microsoft/setup-msbuild`

---

### ci.yml

**Name:** CI

**Path:** `.github/workflows/ci.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (1):**
- `build-and-test-ubuntu`: Build & Test (Ubuntu) (runs-on: ubuntu-latest)

**Technologies:** CMake, Java, vcpkg

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Concurrency:**
- group: ${{ github.workflow }}-${{ github.ref }}
- cancel-in-progress: True

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `dorny/test-reporter`

---

### cross-compile-review.yml

**Name:** Cross-Compile Code Review

**Path:** `.github/workflows/cross-compile-review.yml`

**Triggers:**
- Events: pull_request, push
- Branches: main, develop
- Paths: 7 path filters

**Jobs (1):**
- `cross-compile-review`: Validate Cross-Compile Compatibility (runs-on: ubuntu-latest)

**Technologies:** CMake, Python

**Actions Used (5):**
- `actions/checkout`
- `actions/github-script`
- `actions/setup-python`
- `actions/upload-artifact`
- `tj-actions/changed-files`

---

### csharp-sdk-test.yml

**Name:** C# SDK [Dry-Run]

**Path:** `.github/workflows/csharp-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `build-and-test`: Build & Test (.NET 8.0) (runs-on: ubuntu-latest)
- `package`: Create NuGet Package (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** .NET

**Permissions:**
- contents: read

**Actions Used (3):**
- `actions/checkout`
- `actions/setup-dotnet`
- `actions/upload-artifact`

---

### develop-ci.yml

**Name:** Develop CI

**Path:** `.github/workflows/develop-ci.yml`

**Triggers:**
- Events: push, pull_request
- Branches: develop, develop

**Jobs (6):**
- `validate`: Quick Validation (runs-on: ubuntu-latest)
- `build-linux`: Build & Test (Linux) (runs-on: ubuntu-latest)
- `code-quality`: Code Quality & Linting (runs-on: ubuntu-latest)
- `security-scan`: Security Scan (runs-on: ubuntu-latest)
- `integration-tests`: Integration Tests (runs-on: ubuntu-latest)
- `summary`: CI Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, Java, vcpkg

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Concurrency:**
- group: develop-ci-${{ github.ref }}
- cancel-in-progress: True

**Actions Used (5):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/upload-artifact`
- `dorny/test-reporter`
- `gitleaks/gitleaks-action`

---

### docker-build.yml

**Name:** Docker Build & Release

**Path:** `.github/workflows/docker-build.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Tags: v*.*.*

**Jobs (4):**
- `set-version`: Determine Version (runs-on: ubuntu-latest)
- `build-binary`: Build Binary for Docker (runs-on: ubuntu-latest)
- `build-docker-simple`: Build Docker (Pre-built Binary) (runs-on: ubuntu-latest)
- `build-docker-multiarch`: Build Docker (Multi-Arch Full Build) (runs-on: ubuntu-latest)

**Technologies:** CMake, Docker, vcpkg

**Permissions:**
- contents: read
- packages: write

**Concurrency:**
- group: docker-build-${{ github.ref }}
- cancel-in-progress: False

**Actions Used (10):**
- `actions/cache`
- `actions/checkout`
- `actions/download-artifact`
- `actions/upload-artifact`
- `docker/build-push-action`
- `docker/login-action`
- `docker/metadata-action`
- `docker/setup-buildx-action`
- `docker/setup-qemu-action`
- `lukka/run-vcpkg`

---

### docs-compendium.yml

**Name:** Compendium Build & Deploy

**Path:** `.github/workflows/docs-compendium.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Tags: v*.*.*

**Jobs (2):**
- `build`: Build Compendium Documentation (runs-on: ubuntu-latest)
- `deploy`: Deploy Compendium to GitHub Pages (runs-on: ubuntu-latest)

**Technologies:** MkDocs, Python

**Permissions:**
- contents: write
- pages: write
- id-token: write

**Actions Used (7):**
- `actions/checkout`
- `actions/configure-pages`
- `actions/deploy-pages`
- `actions/download-artifact`
- `actions/setup-python`
- `actions/upload-artifact`
- `actions/upload-pages-artifact`

---

### docs.yml

**Name:** Documentation Build & Deploy

**Path:** `.github/workflows/docs.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Tags: v*.*.*

**Jobs (2):**
- `build`: Build Documentation (runs-on: ubuntu-latest)
- `deploy`: Deploy to GitHub Pages (runs-on: ubuntu-latest)

**Technologies:** MkDocs, Python

**Permissions:**
- contents: write
- pages: write
- id-token: write

**Actions Used (7):**
- `actions/checkout`
- `actions/configure-pages`
- `actions/deploy-pages`
- `actions/download-artifact`
- `actions/setup-python`
- `actions/upload-artifact`
- `actions/upload-pages-artifact`

---

### documentation-validation.yml

**Name:** Documentation Validation

**Path:** `.github/workflows/documentation-validation.yml`

**Triggers:**
- Events: pull_request, push, workflow_dispatch
- Branches: main, develop, release/**
- Paths: 16 path filters

**Jobs (5):**
- `docs-lint`: Documentation Linting (runs-on: ubuntu-latest)
- `link-check`: Link Validation (runs-on: ubuntu-latest)
- `external-link-check`: External Link Check (runs-on: ubuntu-latest)
- `toc-validation`: TOC Validation (runs-on: ubuntu-latest)
- `validation-summary`: Validation Summary (runs-on: ubuntu-latest)

**Technologies:** MkDocs, Python

**Permissions:**
- contents: read
- pull-requests: write

**Actions Used (5):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/setup-python`
- `actions/upload-artifact`
- `gaurav-nelson/github-action-markdown-link-check`

---

### durability-tests.yml

**Name:** Durability & Crash Recovery Tests

**Path:** `.github/workflows/durability-tests.yml`

**Triggers:**
- Events: push, pull_request
- Branches: main, develop, main, develop
- Paths: 5 path filters

**Jobs (6):**
- `windows-durability`: Windows Durability Tests (runs-on: windows-latest)
- `linux-durability`: Linux Durability Tests (runs-on: ubuntu-latest)
- `integration-tests`: Integration Tests (Full Suite) (runs-on: windows-latest)
- `quality-gates`: Quality Gates (runs-on: ubuntu-latest)
- `llm-durability-optional`: LLM Durability Tests (Optional) (runs-on: windows-latest)
- `docker-build`: Docker Build with Durability Tests (runs-on: ubuntu-latest)

**Technologies:** CMake, Docker, vcpkg

**Actions Used (5):**
- `actions/checkout`
- `actions/setup-vcpkg`
- `actions/upload-artifact`
- `docker/build-push-action`
- `docker/setup-buildx-action`

---

### feature-ci.yml

**Name:** Feature/Bugfix CI

**Path:** `.github/workflows/feature-ci.yml`

**Triggers:**
- Events: pull_request
- Branches: develop
- Types: opened, synchronize, reopened

**Jobs (5):**
- `validate-branch`: Validate Branch Strategy (runs-on: ubuntu-latest)
- `build-and-test`: Build & Test (runs-on: ubuntu-latest)
- `code-quality`: Code Quality (runs-on: ubuntu-latest)
- `security-check`: Security Check (runs-on: ubuntu-latest)
- `status-check`: All Checks Passed (runs-on: ubuntu-latest)

**Technologies:** CMake, Java, vcpkg

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Concurrency:**
- group: feature-ci-${{ github.event.pull_request.number }}
- cancel-in-progress: True

**Actions Used (4):**
- `actions/checkout`
- `actions/upload-artifact`
- `dorny/test-reporter`
- `gitleaks/gitleaks-action`

---

### fuzzing.yml

**Name:** AFL++ Fuzzing

**Path:** `.github/workflows/fuzzing.yml`

**Triggers:**
- Events: schedule, workflow_dispatch
- Schedule: 0 0 * * 0

**Jobs (6):**
- `build-fuzz-targets`: Build Fuzzing Targets (runs-on: ubuntu-latest)
- `fuzz-aql-parser`: Fuzz AQL Parser (runs-on: ubuntu-latest)
- `fuzz-json-parser`: Fuzz JSON Parser (runs-on: ubuntu-latest)
- `fuzz-crypto`: Fuzz Crypto Operations (runs-on: ubuntu-latest)
- `analyze-crashes`: Analyze Crashes (runs-on: ubuntu-latest)
- `coverage-report`: Generate Coverage Report (runs-on: ubuntu-latest)

**Technologies:** CMake, Fuzzing, Go, Rust, Sanitizers

**Permissions:**
- contents: read

**Actions Used (4):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/github-script`
- `actions/upload-artifact`

---

### go-sdk-test.yml

**Name:** Go SDK [Dry-Run]

**Path:** `.github/workflows/go-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `build-and-test`: Test Go ${{ matrix.go-version }} (runs-on: ubuntu-latest)
- `package`: Package Module (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** Go, Java

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Actions Used (4):**
- `actions/checkout`
- `actions/setup-go`
- `actions/upload-artifact`
- `dorny/test-reporter`

---

### helm-chart-test.yml

**Name:** Helm Chart [Dry-Run]

**Path:** `.github/workflows/helm-chart-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (1):**
- `lint-and-test`: Lint & Template Test (runs-on: ubuntu-latest)

**Technologies:** Helm

**Permissions:**
- contents: read

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `azure/setup-helm`

---

### hotfix-ci.yml

**Name:** Hotfix CI

**Path:** `.github/workflows/hotfix-ci.yml`

**Triggers:**
- Events: pull_request, push
- Branches: main, hotfix/*
- Types: opened, synchronize, reopened

**Jobs (5):**
- `validate-hotfix`: Validate Hotfix (runs-on: ubuntu-latest)
- `build-and-critical-tests`: Build & Critical Tests (runs-on: ubuntu-latest)
- `security-check`: Security Check (runs-on: ubuntu-latest)
- `sync-to-develop`: Sync to Develop (runs-on: ubuntu-latest)
- `hotfix-summary`: Hotfix Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, vcpkg

**Permissions:**
- contents: write
- pull-requests: write

**Concurrency:**
- group: hotfix-ci-${{ github.ref }}
- cancel-in-progress: False

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `gitleaks/gitleaks-action`

---

### java-sdk-test.yml

**Name:** Java SDK [Dry-Run]

**Path:** `.github/workflows/java-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `build-and-test`: Build & Test Java ${{ matrix.java-version }} (runs-on: ubuntu-latest)
- `package`: Package JAR (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** Java

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Actions Used (4):**
- `actions/checkout`
- `actions/setup-java`
- `actions/upload-artifact`
- `dorny/test-reporter`

---

### javascript-sdk-test.yml

**Name:** JavaScript/TypeScript SDK [Dry-Run]

**Path:** `.github/workflows/javascript-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `test`: Test Node.js ${{ matrix.node-version }} (runs-on: ubuntu-latest)
- `build`: Build Package (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** Java, Node.js

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Actions Used (4):**
- `actions/checkout`
- `actions/setup-node`
- `actions/upload-artifact`
- `dorny/test-reporter`

---

### license-compliance.yml

**Name:** License Compliance

**Path:** `.github/workflows/license-compliance.yml`

**Triggers:**
- Events: pull_request, push, schedule, workflow_dispatch
- Branches: main, master, develop, release/**, hotfix/**, main, master, develop
- Paths: 11 path filters
- Schedule: 0 3 1 * *

**Jobs (2):**
- `license-scan`: License Scanning (runs-on: ubuntu-latest)
- `compliance-status`: License Compliance Status (runs-on: ubuntu-latest)

**Technologies:** Go, Node.js, Python, Rust, vcpkg

**Permissions:**
- contents: read
- pull-requests: write
- checks: write
- issues: write

**Actions Used (3):**
- `actions/checkout`
- `actions/github-script`
- `actions/upload-artifact`

---

### llama-cpp-integration.yml

**Name:** llama.cpp Integration Tests

**Path:** `.github/workflows/llama-cpp-integration.yml`

**Triggers:**
- Events: pull_request, push, workflow_dispatch
- Branches: main, develop
- Paths: 8 path filters

**Jobs (5):**
- `verify-pinning`: Verify llama.cpp Version Pinning (runs-on: ubuntu-latest)
- `build-test-linux`: Build & Test (Ubuntu) (runs-on: ubuntu-22.04)
- `performance-check`: Performance Regression Check (runs-on: ubuntu-22.04)
- `feature-matrix`: Feature Compatibility Test (runs-on: ubuntu-22.04)
- `integration-summary`: Integration Test Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, Go, LoRA/LLM

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Concurrency:**
- group: llama-integration-${{ github.ref }}
- cancel-in-progress: True

**Actions Used (2):**
- `actions/checkout`
- `actions/upload-artifact`

---

### lora-framework-ci.yml

**Name:** LoRA Framework CI/CD

**Path:** `.github/workflows/lora-framework-ci.yml`

**Triggers:**
- Events: push, pull_request, workflow_dispatch
- Branches: develop, feature/*lora*, copilot/*lora*, develop, main
- Paths: 18 path filters

**Jobs (9):**
- `code-quality`: Code Quality Checks (runs-on: ubuntu-latest)
- `build-test-ubuntu`: Build & Test (Ubuntu 22.04) (runs-on: ubuntu-latest)
- `build-windows`: Build (Windows Server 2022) (runs-on: windows-latest)
- `build-macos`: Build (macOS 13) (runs-on: macos-13)
- `performance-benchmarks`: Performance Benchmarks (Detailed) (runs-on: ubuntu-latest)
- `code-coverage`: Code Coverage Analysis (runs-on: ubuntu-latest)
- `ci-status`: CI Status Summary (runs-on: ubuntu-latest)
- `docker-build`: Docker Build & Validation (runs-on: ubuntu-latest)
- `docker-test`: Docker Compose Integration Tests (runs-on: ubuntu-latest)

**Technologies:** CMake, Docker, Go, LoRA/LLM, vcpkg

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Concurrency:**
- group: ${{ github.workflow }}-${{ github.ref }}
- cancel-in-progress: True

**Actions Used (5):**
- `actions/checkout`
- `actions/upload-artifact`
- `docker/setup-buildx-action`
- `hendrikmuhs/ccache-action`
- `lukka/run-vcpkg`

---

### main-ci.yml

**Name:** Main Branch CI

**Path:** `.github/workflows/main-ci.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Tags: v*

**Jobs (6):**
- `verify-merge`: Verify Merge (runs-on: ubuntu-latest)
- `verification-build`: Verification Build & Test (runs-on: ubuntu-latest)
- `create-release`: Create GitHub Release (runs-on: ubuntu-latest)
- `publish-docker`: Publish Docker Image (runs-on: ubuntu-latest)
- `deploy-docs`: Deploy Documentation (runs-on: ubuntu-latest)
- `production-summary`: Production Deployment Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, Docker, Java, MkDocs, Python, vcpkg

**Permissions:**
- contents: write
- packages: write
- pull-requests: read
- checks: write

**Concurrency:**
- group: main-ci-${{ github.ref }}
- cancel-in-progress: False

**Actions Used (11):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/github-script`
- `actions/setup-python`
- `actions/upload-artifact`
- `docker/build-push-action`
- `docker/login-action`
- `docker/metadata-action`
- `docker/setup-buildx-action`
- `dorny/test-reporter`
- ... and 1 more

---

### owasp-zap.yml

**Name:** OWASP ZAP Security Scan

**Path:** `.github/workflows/owasp-zap.yml`

**Triggers:**
- Events: pull_request, push, workflow_dispatch, schedule
- Branches: main, develop, main, develop
- Paths: 5 path filters
- Schedule: 0 2 * * 1

**Jobs (3):**
- `zap_baseline_scan`: OWASP ZAP Baseline Scan (runs-on: ubuntu-latest)
- `zap_full_scan`: OWASP ZAP Full Scan (runs-on: ubuntu-latest)
- `zap_api_scan`: OWASP ZAP API Scan (runs-on: ubuntu-latest)

**Technologies:** Docker, OWASP ZAP

**Permissions:**
- contents: read
- security-events: write
- issues: write

**Actions Used (6):**
- `actions/checkout`
- `actions/github-script`
- `actions/upload-artifact`
- `zaproxy/action-api-scan`
- `zaproxy/action-baseline`
- `zaproxy/action-full-scan`

---

### performance-regression-check.yml

**Name:** Performance Regression Check

**Path:** `.github/workflows/performance-regression-check.yml`

**Triggers:**
- Events: pull_request, workflow_dispatch
- Branches: main, develop
- Paths: 4 path filters

**Jobs (1):**
- `detect-regressions`: Performance Regression Detection (runs-on: ubuntu-latest)

**Technologies:** CMake, Go, Python, vcpkg

**Permissions:**
- contents: read
- pull-requests: write
- statuses: write

**Concurrency:**
- group: perf-regression-${{ github.event.pull_request.number || github.ref }}
- cancel-in-progress: True

**Actions Used (5):**
- `actions/checkout`
- `actions/github-script`
- `actions/setup-python`
- `actions/upload-artifact`
- `slackapi/slack-github-action`

---

### php-sdk-test.yml

**Name:** PHP SDK [Dry-Run]

**Path:** `.github/workflows/php-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `test`: Test PHP ${{ matrix.php-version }} (runs-on: ubuntu-latest)
- `package`: Package Library (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** PHP

**Permissions:**
- contents: read

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `shivammathur/setup-php`

---

### python-sdk-test.yml

**Name:** Python SDK [Dry-Run]

**Path:** `.github/workflows/python-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `test`: Test Python ${{ matrix.python-version }} (runs-on: ubuntu-latest)
- `build`: Build Package (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** Java, Python

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Actions Used (4):**
- `actions/checkout`
- `actions/setup-python`
- `actions/upload-artifact`
- `dorny/test-reporter`

---

### release-build.yml

**Name:** Release Build & Deploy

**Path:** `.github/workflows/release-build.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Tags: v*

**Jobs (8):**
- `setup`: Setup Release (runs-on: ubuntu-latest)
- `generate-changelog`: Generate Release Changelog (runs-on: ubuntu-latest)
- `build-binary`: Build ThemisDB Binary (runs-on: ubuntu-latest)
- `build-docker`: Build & Push Docker Image (runs-on: ubuntu-latest)
- `generate-sbom`: Generate SBOM & Sign Artifacts (runs-on: ubuntu-latest)
- `build-docs`: Build Documentation (runs-on: ubuntu-latest)
- `deploy-docs`: Deploy Documentation (runs-on: ubuntu-latest)
- `notify`: Release Notification (runs-on: ubuntu-latest)

**Technologies:** .NET, CMake, CodeQL, Docker, Go, Java, LoRA/LLM, MkDocs, Python, SBOM/Anchore, vcpkg

**Permissions:**
- contents: write
- packages: write
- pages: write
- id-token: write

**Concurrency:**
- group: release-build-${{ github.ref }}
- cancel-in-progress: False

**Actions Used (14):**
- `actions/cache`
- `actions/checkout`
- `actions/configure-pages`
- `actions/deploy-pages`
- `actions/download-artifact`
- `actions/setup-python`
- `actions/upload-artifact`
- `actions/upload-pages-artifact`
- `docker/build-push-action`
- `docker/login-action`
- ... and 4 more

---

### release-ci.yml

**Name:** Release CI

**Path:** `.github/workflows/release-ci.yml`

**Triggers:**
- Events: push, pull_request, workflow_dispatch
- Branches: release/*, main
- Types: opened, synchronize, reopened

**Jobs (4):**
- `validate-release`: Validate Release (runs-on: ubuntu-latest)
- `build-and-test`: Build & Test (Release) (runs-on: ubuntu-latest)
- `security-and-quality`: Security & Quality Checks (runs-on: ubuntu-latest)
- `release-summary`: Release Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, vcpkg

**Permissions:**
- contents: read
- pull-requests: write

**Concurrency:**
- group: release-ci-${{ github.ref }}
- cancel-in-progress: False

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `gitleaks/gitleaks-action`

---

### release.yml

**Name:** Release - Automated Build & Publish

**Path:** `.github/workflows/release.yml`

**Triggers:**
- Events: push
- Tags: v*

**Jobs (5):**
- `prepare-release`: Prepare Release (runs-on: ubuntu-latest)
- `build-ubuntu`: Build Release - Ubuntu (runs-on: ubuntu-latest)
- `build-windows`: Build Release - Windows (runs-on: windows-latest)
- `build-macos`: Build Release - macOS (runs-on: macos-latest)
- `create-release`: Create GitHub Release (runs-on: ubuntu-latest)

**Technologies:** CMake, vcpkg

**Permissions:**
- contents: write
- packages: write

**Actions Used (4):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/upload-artifact`
- `softprops/action-gh-release`

---

### retroactive-release.yml

**Name:** Retroactive Release Build

**Path:** `.github/workflows/retroactive-release.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (4):**
- `build-linux`: Build Linux Packages (runs-on: ubuntu-latest)
- `build-windows`: Build Windows Packages (runs-on: windows-latest)
- `build-macos`: Build macOS Packages (runs-on: macos-latest)
- `summary`: Build Summary (runs-on: ubuntu-latest)

**Technologies:** CMake, vcpkg

**Permissions:**
- contents: write

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `softprops/action-gh-release`

---

### reusable-test-report.yml

**Name:** Reusable Test Report Publisher

**Path:** `.github/workflows/reusable-test-report.yml`

**Triggers:**
- Events: workflow_call

**Jobs (1):**
- `publish-test-results`: Publish Test Results (runs-on: ubuntu-latest)

**Technologies:** Java

**Permissions:**
- contents: read
- pull-requests: write
- checks: write
- issues: write

**Actions Used (4):**
- `actions/download-artifact`
- `actions/github-script`
- `actions/upload-artifact`
- `dorny/test-reporter`

---

### ruby-sdk-test.yml

**Name:** Ruby SDK [Dry-Run]

**Path:** `.github/workflows/ruby-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `test`: Test Ruby ${{ matrix.ruby-version }} (runs-on: ubuntu-latest)
- `package`: Build Gem (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** Ruby

**Permissions:**
- contents: read

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `ruby/setup-ruby`

---

### rust-sdk-test.yml

**Name:** Rust SDK [Dry-Run]

**Path:** `.github/workflows/rust-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (2):**
- `build-and-test`: Test Rust ${{ matrix.rust-version }} (runs-on: ubuntu-latest)
- `package`: Package Crate (Dry-Run) (runs-on: ubuntu-latest)

**Technologies:** Go, Java, Rust

**Permissions:**
- contents: read
- pull-requests: write
- checks: write

**Actions Used (4):**
- `actions-rust-lang/setup-rust-toolchain`
- `actions/checkout`
- `actions/upload-artifact`
- `dorny/test-reporter`

---

### sbom.yml

**Name:** SBOM Generation

**Path:** `.github/workflows/sbom.yml`

**Triggers:**
- Events: push, release, workflow_dispatch
- Tags: v*
- Types: published

**Jobs (2):**
- `generate-sbom`: generate-sbom (runs-on: ubuntu-latest)
- `vulnerability-scan`: vulnerability-scan (runs-on: ubuntu-latest)

**Technologies:** SBOM/Anchore

**Permissions:**
- contents: read
- packages: write
- id-token: write

**Actions Used (6):**
- `actions/checkout`
- `actions/download-artifact`
- `actions/upload-artifact`
- `anchore/sbom-action/download-syft`
- `anchore/scan-action/download-grype`
- `softprops/action-gh-release`

---

### security-scan.yml

**Name:** Security Scanning (SAST)

**Path:** `.github/workflows/security-scan.yml`

**Triggers:**
- Events: push, pull_request, schedule, workflow_dispatch
- Branches: main, master, develop, main, master, develop
- Schedule: 0 2 * * 0

**Jobs (6):**
- `secret-scanning`: Secret Scanning (runs-on: ubuntu-latest)
- `cpp-analysis`: C++ Static Analysis (runs-on: ubuntu-latest)
- `codeql-analysis`: CodeQL Analysis (runs-on: ubuntu-latest)
- `dependency-scan`: Dependency Vulnerability Scan (runs-on: ubuntu-latest)
- `license-check`: License Compliance (runs-on: ubuntu-latest)
- `security-summary`: Security Summary (runs-on: ubuntu-latest)

**Technologies:** CodeQL, Go, vcpkg

**Permissions:**
- contents: read
- security-events: write
- actions: read

**Actions Used (5):**
- `actions/checkout`
- `actions/upload-artifact`
- `github/codeql-action/analyze`
- `github/codeql-action/init`
- `gitleaks/gitleaks-action`

---

### sharding-benchmark.yml

**Name:** CHIMERA Suite - Sharding Benchmarks (Weekly)

**Path:** `.github/workflows/sharding-benchmark.yml`

**Triggers:**
- Events: schedule, workflow_dispatch
- Schedule: 0 3 * * 1

**Jobs (1):**
- `sharding-bench`: sharding-bench (runs-on: ubuntu-latest)

**Technologies:** CMake, Python

**Actions Used (4):**
- `actions/checkout`
- `actions/setup-python`
- `actions/upload-artifact`
- `slackapi/slack-github-action`

---

### swift-sdk-test.yml

**Name:** Swift SDK [Dry-Run]

**Path:** `.github/workflows/swift-sdk-test.yml`

**Triggers:**
- Events: workflow_dispatch

**Jobs (3):**
- `test-macos`: Test Swift ${{ matrix.swift-version }} on macOS (runs-on: macos-latest)
- `test-linux`: Test Swift on Linux (runs-on: ubuntu-latest)
- `package`: Build Package (Dry-Run) (runs-on: macos-latest)

**Technologies:** Swift

**Permissions:**
- contents: read

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `swift-actions/setup-swift`

---

### update-performance-baselines.yml

**Name:** Update Performance Baselines

**Path:** `.github/workflows/update-performance-baselines.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Branches: main, develop
- Tags: v*

**Jobs (1):**
- `update-baseline`: Update Performance Baseline (runs-on: ubuntu-latest)

**Technologies:** CMake, Python, vcpkg

**Permissions:**
- contents: write

**Actions Used (4):**
- `actions/checkout`
- `actions/github-script`
- `actions/setup-python`
- `actions/upload-artifact`

---

### wiki-sync.yml

**Name:** Sync Documentation to GitHub Wiki

**Path:** `.github/workflows/wiki-sync.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Tags: v*.*.*

**Jobs (1):**
- `sync-wiki`: Sync to GitHub Wiki (runs-on: ubuntu-latest)

**Permissions:**
- contents: write

**Actions Used (1):**
- `actions/checkout`

---

### wordpress-theme-deploy.yml

**Name:** WordPress Theme Deploy

**Path:** `.github/workflows/wordpress-theme-deploy.yml`

**Triggers:**
- Events: push, workflow_dispatch
- Branches: main, develop
- Paths: 1 path filters

**Jobs (1):**
- `build-and-deploy`: Build and Deploy WordPress Theme (runs-on: ubuntu-latest)

**Technologies:** Go

**Permissions:**
- contents: read

**Actions Used (3):**
- `actions/checkout`
- `actions/upload-artifact`
- `softprops/action-gh-release`

---

## Common Patterns Analysis

### Most Used Actions

- `actions/checkout`: used in 50 workflows
- `actions/upload-artifact`: used in 47 workflows
- `actions/download-artifact`: used in 17 workflows
- `actions/github-script`: used in 16 workflows
- `actions/setup-python`: used in 14 workflows
- `dorny/test-reporter`: used in 11 workflows
- `docker/setup-buildx-action`: used in 7 workflows
- `gitleaks/gitleaks-action`: used in 7 workflows
- `docker/build-push-action`: used in 5 workflows
- `lukka/run-vcpkg`: used in 4 workflows
- `softprops/action-gh-release`: used in 4 workflows
- `docker/setup-qemu-action`: used in 3 workflows
- `docker/login-action`: used in 3 workflows
- `docker/metadata-action`: used in 3 workflows
- `actions/configure-pages`: used in 3 workflows

### Technology Distribution

- CMake: 27 workflows
- vcpkg: 21 workflows
- Python: 15 workflows
- Go: 13 workflows
- Java: 13 workflows
- Docker: 8 workflows
- MkDocs: 5 workflows
- CodeQL: 4 workflows
- LoRA/LLM: 4 workflows
- OWASP ZAP: 4 workflows
- SBOM/Anchore: 3 workflows
- Sanitizers: 3 workflows
- Rust: 3 workflows
- Fuzzing: 2 workflows
- .NET: 2 workflows
- Node.js: 2 workflows
- Helm: 1 workflows
- PHP: 1 workflows
- Ruby: 1 workflows
- Swift: 1 workflows

### Runner Distribution

- ubuntu-latest: 144 jobs
- ubuntu-22.04: 13 jobs
- windows-latest: 8 jobs
- macos-latest: 6 jobs
- windows-2022: 2 jobs
- ubuntu-24.04: 1 jobs
- macos-13: 1 jobs

## Actions Usage Summary

**Total Unique Actions:** 43

- GitHub Official Actions: 14
- Third-Party Actions: 29
- Reusable Workflows: 0

### GitHub Official Actions

- `actions/cache`
- `actions/checkout`
- `actions/configure-pages`
- `actions/deploy-pages`
- `actions/download-artifact`
- `actions/github-script`
- `actions/setup-dotnet`
- `actions/setup-go`
- `actions/setup-java`
- `actions/setup-node`
- `actions/setup-python`
- `actions/setup-vcpkg`
- `actions/upload-artifact`
- `actions/upload-pages-artifact`

