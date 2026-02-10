# Archived Workflows

This directory contains workflow files that have been superseded by the new consolidated CI/CD architecture implemented in February 2026.

## Migration Overview

The ThemisDB repository has undergone a comprehensive CI/CD workflow consolidation to:
- Reduce 53 workflows down to 12 entry workflows
- Eliminate duplication through 7 reusable workflows
- Standardize patterns through 8 composite actions
- Improve maintainability and consistency

## New Workflow Architecture

### Entry Workflows (12)
- **ci-pull-request.yml** - Unified PR CI (replaces: ci-develop.yml, feature-ci.yml, hotfix-ci.yml, etc.)
- **ci-main-branch.yml** - Main/develop branch builds
- **ci-release.yml** - Release pipeline (replaces: release.yml, release-build.yml, release-ci.yml)
- **nightly.yml** - Extended nightly tests
- **sdk-tests.yml** - Unified SDK testing (replaces 9 individual SDK test workflows)
- **security.yml** - Security scanning (replaces: security-scan.yml, owasp-zap.yml)
- **compliance.yml** - License/audit/SBOM (replaces: license-compliance.yml, audit-check.yml, sbom.yml)
- **docs.yml** - Documentation (replaces: docs-compendium.yml, wiki-sync.yml, documentation-validation.yml)
- **deploy.yml** - Container/Helm/infrastructure deployment (replaces: docker-build.yml, helm-chart-test.yml)
- **tests-extended.yml** - Chaos/durability/DR tests (replaces: chaos-tests.yml, durability-tests.yml, dr-testing.yml)
- **tests-specialized.yml** - Fuzzing/sanitizers/cross-compile (replaces: fuzzing.yml, ci-sanitizers.yml, ci-arm-cross.yml)
- **ops-automation.yml** - Access reviews, incident drills (replaces: access-review.yml, incident-drill.yml)

### Reusable Workflows (7)
- **reusable-cpp-build.yml** - C++ build/test with configurable options
- **reusable-sdk-test.yml** - Multi-language SDK testing
- **reusable-security-scan.yml** - Security scanning (CodeQL, Trivy, Gitleaks, cppcheck)
- **reusable-docs-build.yml** - MkDocs building and deployment
- **reusable-container-build.yml** - Multi-platform Docker builds
- **reusable-benchmark.yml** - Performance benchmarking
- **reusable-cross-compile.yml** - Cross-compilation for ARM/RISC-V
- **reusable-test-report.yml** - Test reporting (existing, kept)

### Composite Actions (8)
- **setup-cpp-env** - C++ environment with apt dependencies
- **setup-vcpkg** - vcpkg bootstrap and installation
- **cmake-build** - CMake configure, build, test
- **setup-language** - Multi-language runtime setup
- **report-results** - Test results and PR comments
- **security-report** - SARIF upload and issue creation
- **artifact-publish** - Release artifacts with checksums
- **notification** - Workflow notifications

## Rollback Procedure

If issues arise with the new workflows, the archived workflows can be quickly restored:

1. **Disable new workflows:** Rename `.github/workflows/*.yml` to `*.yml.disabled`
2. **Restore old workflows:** Move files from `_archived/` back to `.github/workflows/`
3. **Re-enable branch protection:** Update required status checks in repository settings

## Benefits of New Architecture

- **60-70% reduction** in workflow files (53 → 12)
- **Centralized maintenance** through reusable workflows and composite actions
- **Consistent patterns** across all workflows
- **Improved caching** and faster CI times
- **Better security** with least-privilege permissions
- **Easier troubleshooting** with standardized structure

## Documentation

For detailed information on using the new workflow architecture, see:
- `docs/ci-cd/ci-architecture.md` - Architecture overview and usage guide
- `docs/ci-cd/consolidation-plan.md` - Complete consolidation plan
