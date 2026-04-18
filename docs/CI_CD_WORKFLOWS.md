# Git Flow CI/CD Pipeline Documentation

## Overview

ThemisDB uses a comprehensive Git Flow CI/CD pipeline that enforces branch protection rules and ensures code quality at every stage of development. This document describes the automated workflows and how they integrate with the branching strategy.

## Workflow Structure

### 1. Feature/Bugfix CI (`feature-ci.yml`)

**Triggers:**
- Pull requests to `develop` from any branch
- Primarily designed for `feature/*` and `bugfix/*` branches

**Purpose:**
- Validates branch naming and target
- Runs full build and test suite
- Performs code quality checks
- Runs security scans

**Jobs:**
1. **validate-branch**: Ensures feature/bugfix branches target `develop`
2. **build-and-test**: Full compilation and unit tests
3. **code-quality**: cppcheck and static analysis
4. **security-check**: Gitleaks secret scanning
5. **status-check**: Final validation of all checks

**Required Checks:**
- ✅ Branch validation must pass
- ✅ Build and tests must pass
- ⚠️  Code quality (warnings allowed)
- ⚠️  Security check (warnings allowed)

---

### 2. Develop Branch CI (`develop-ci.yml`)

**Triggers:**
- Push to `develop` branch
- Pull requests targeting `develop`

**Purpose:**
- Full CI suite for integration branch
- Validates merged code quality
- Creates artifacts for integration testing
- Ensures develop remains stable

**Jobs:**
1. **validate**: Quick validation of VERSION file and branch strategy
2. **build-linux**: Full build and test on Linux
3. **code-quality**: Comprehensive code quality checks
4. **security-scan**: Security vulnerability scanning
5. **integration-tests**: Integration test suite (on push only)
6. **summary**: Combined status report

**Artifacts:**
- Build artifacts for integration testing (7-day retention)
- Code quality reports
- Test results

---

### 3. Release CI (`release-ci.yml`)

**Triggers:**
- Push to `release/*` branches
- Pull requests to `main` from any branch
- Manual workflow dispatch

**Purpose:**
- Validate release preparation
- Ensure version number is updated
- Run full test suite
- Package release artifacts

**Jobs:**
1. **validate-release**: 
   - Check `release/*` branch naming
   - Validate VERSION file format (semantic versioning)
   - Verify CHANGELOG.md exists and mentions version
   - Check for release notes file
2. **build-and-test**: Full release build with benchmarks enabled
3. **security-and-quality**: Comprehensive security and quality checks
4. **release-summary**: Status report with next steps

**Validation Rules:**
- ✅ VERSION file must exist and follow semver (X.Y.Z)
- ✅ Release branches must target `main`
- ⚠️  CHANGELOG.md should mention the version
- ⚠️  Release notes (RELEASE_NOTES_vX.Y.Z.md) recommended

**Artifacts:**
- Release packages (30-day retention)
- Security scan reports
- Build artifacts

---

### 4. Hotfix CI (`hotfix-ci.yml`)

**Triggers:**
- Pull requests to `main` from `hotfix/*` branches
- Push to `hotfix/*` branches

**Purpose:**
- Fast-track critical production fixes
- Run accelerated test suite
- Automatically create PR to sync back to `develop`

**Jobs:**
1. **validate-hotfix**:
   - Verify `hotfix/*` branch naming
   - Ensure targets `main` branch
   - Validate VERSION file update
2. **build-and-critical-tests**: Build with critical tests only (180s timeout)
3. **security-check**: Quick security scan
4. **sync-to-develop**: Auto-create PR to merge hotfix back to `develop` (on push)
5. **hotfix-summary**: Status report

**Automatic Sync:**
When a hotfix is pushed (after merge to main), the workflow automatically:
1. Creates a PR from `hotfix/*` to `develop`
2. Labels it as "hotfix" and "automated"
3. Includes version information in the PR description

**Artifacts:**
- Hotfix packages (30-day retention)

---

### 5. Main Branch CI (`main-ci.yml`)

**Triggers:**
- Push to `main` branch (from release or hotfix merges)
- Git tags matching `v*` pattern
- Manual workflow dispatch

**Purpose:**
- Verify production deployment
- Create GitHub releases
- Publish Docker images
- Deploy documentation

**Jobs:**
1. **verify-merge**: Validate merge source and version
2. **verification-build**: Full build and test for production
3. **create-release**: Create GitHub release with artifacts (for tags only)
4. **publish-docker**: Build and push Docker image to GHCR
5. **deploy-docs**: Build and deploy documentation to GitHub Pages
6. **production-summary**: Deployment status report

**Production Artifacts:**
- Production binaries (90-day retention)
- GitHub release with tarball
- Docker image (Community): `themisdb/themisdb:latest`, `themisdb/themisdb:<version>`
- Documentation site (GitHub Pages)

---

## Branch Protection Rules

### Required Status Checks

#### For `main` branch:
- Main Branch CI / verify-merge
- Main Branch CI / verification-build
- (Or) Release CI / validate-release
- (Or) Hotfix CI / validate-hotfix

#### For `develop` branch:
- Develop CI / validate
- Develop CI / build-linux
- Feature/Bugfix CI / validate-branch (for PRs)
- Feature/Bugfix CI / build-and-test (for PRs)

### Manual Configuration

These workflows support branch protection, but GitHub branch protection rules must be configured manually:

1. **Navigate to**: Repository Settings → Branches → Add rule
2. **For `main` branch**:
   - Branch name pattern: `main`
   - ✅ Require pull request reviews (1 approval)
   - ✅ Require status checks: Select Main/Release/Hotfix CI checks
   - ✅ Require branches up to date
   - ✅ Include administrators
   - ❌ Allow force pushes: Disabled
   - ❌ Allow deletions: Disabled

3. **For `develop` branch**:
   - Branch name pattern: `develop`
   - ✅ Require pull request reviews (1 approval)
   - ✅ Require status checks: Select Develop/Feature CI checks
   - ✅ Require branches up to date
   - ❌ Allow force pushes: Disabled

See [BRANCH_PROTECTION_SETUP.md](ci-cd/branching-release-history/BRANCH_PROTECTION_SETUP.md) for detailed instructions.

---

## Workflow Triggers Summary

| Branch Pattern | PR Target | Workflow | Purpose |
|---------------|-----------|----------|---------|
| `feature/*` | `develop` | Feature/Bugfix CI, Develop CI | Validate feature |
| `bugfix/*` | `develop` | Feature/Bugfix CI, Develop CI | Validate bugfix |
| `release/*` | `main` | Release CI | Validate release |
| `hotfix/*` | `main` | Hotfix CI | Fast-track fix |
| Push to `develop` | N/A | Develop CI | Integration validation |
| Push to `main` | N/A | Main CI | Production deployment |
| Tag `v*` | N/A | Main CI | Create release |

---

## Version Management

### VERSION File

All workflows validate the `VERSION` file in the repository root:
- Must exist
- Must follow semantic versioning: `X.Y.Z` or `X.Y.Z-prerelease`
- Must be updated for releases and hotfixes

### Release Numbering

Follow semantic versioning:
- **Major (X)**: Breaking changes
- **Minor (Y)**: New features, backward compatible
- **Patch (Z)**: Bug fixes, backward compatible
- **Prerelease**: `-alpha`, `-beta`, `-rc.1`, etc.

Examples:
```
1.4.0        # Major release
1.4.1        # Patch release
2.0.0        # Major version with breaking changes
2.0.0-beta.1 # Pre-release
```

---

## Using the Workflows

### Feature Development

```bash
# Create feature branch
git checkout develop
git pull origin develop
git checkout -b feature/123-awesome-feature

# Make changes and commit
git add .
git commit -m "feat: Add awesome feature"

# Push and create PR
git push origin feature/123-awesome-feature
# Create PR to develop on GitHub
```

**What happens:**
1. Feature/Bugfix CI runs automatically
2. Validates branch targets `develop`
3. Runs build, tests, and checks
4. PR can be merged after approval and passing checks

---

### Release Process

```bash
# Create release branch from develop
git checkout develop
git pull origin develop
git checkout -b release/1.4.0

# Update version
echo "1.4.0" > VERSION

# Update changelog
# Edit CHANGELOG.md

# Create release notes
# Create RELEASE_NOTES_v1.4.0.md

# Commit changes
git add VERSION CHANGELOG.md RELEASE_NOTES_v1.4.0.md
git commit -m "chore: Prepare release v1.4.0"

# Push release branch
git push origin release/1.4.0

# Create PR to main
# Create PR on GitHub: release/1.4.0 → main
```

**What happens:**
1. Release CI runs automatically
2. Validates version format and release preparation
3. Runs full test suite and creates artifacts
4. After approval, merge to `main`
5. Main CI deploys to production
6. Tag the release: `git tag -a v1.4.0 -m "Release v1.4.0"`
7. Create PR to merge `release/1.4.0` back to `develop`

---

### Hotfix Process

```bash
# Create hotfix branch from main
git checkout main
git pull origin main
git checkout -b hotfix/1.3.5-security-fix

# Make fix
git add .
git commit -m "fix: Patch security vulnerability"

# Update version
echo "1.3.5" > VERSION
git add VERSION
git commit -m "chore: Bump version to 1.3.5"

# Push hotfix
git push origin hotfix/1.3.5-security-fix

# Create PR to main
# Create PR on GitHub: hotfix/1.3.5-security-fix → main
```

**What happens:**
1. Hotfix CI runs with accelerated tests
2. Validates hotfix targets `main`
3. After merge to `main`, Main CI deploys
4. Hotfix CI automatically creates PR to sync to `develop`
5. Review and merge the sync PR

---

## Troubleshooting

### Workflow Failed: Branch Validation Error

**Problem**: `Feature/bugfix branches must target 'develop', not 'main'`

**Solution**: 
```bash
# Change PR target on GitHub from 'main' to 'develop'
# Or close PR and create new one with correct target
```

### Workflow Failed: VERSION File Validation

**Problem**: `Invalid version format: X.Y (expected: X.Y.Z)`

**Solution**:
```bash
# Update VERSION file with proper format
echo "1.4.0" > VERSION
git add VERSION
git commit -m "fix: Update version format"
git push
```

### Hotfix Sync PR Not Created

**Problem**: PR to develop not automatically created after hotfix merge

**Solution**:
```bash
# Manually create PR or trigger workflow
git checkout hotfix/1.3.5-security-fix
git push origin hotfix/1.3.5-security-fix  # Re-push to trigger
```

---

## Artifacts and Retention

| Artifact Type | Workflow | Retention | Purpose |
|--------------|----------|-----------|---------|
| Build artifacts | Develop CI | 7 days | Integration testing |
| Release packages | Release CI | 30 days | Release candidates |
| Hotfix packages | Hotfix CI | 30 days | Emergency fixes |
| Production artifacts | Main CI | 90 days | Production binaries |
| Test results | All CI workflows | 30-90 days | Test analysis and debugging |
| Coverage reports | Main/Develop CI | 30 days | Code coverage tracking |
| Code quality reports | All CI workflows | 7-30 days | Static analysis results |

**Note:** Test result artifacts include JUnit XML reports for all test suites. See [CI_TEST_REPORTING.md](CI_TEST_REPORTING.md) for details on test reporting infrastructure.

---

## Environment Variables

Common environment variables used across workflows:

```yaml
BUILD_TYPE: Release          # CMake build type
USE_SYSTEM_LIBS: ON         # Use system libraries where available
VCPKG_ROOT: vcpkg           # vcpkg installation directory
BUILD_DIR: build            # CMake build directory
```

---

## Security Considerations

All workflows include security checks:
1. **Gitleaks**: Scans for secrets in code
2. **cppcheck**: Static analysis for C++ vulnerabilities
3. **Trivy**: Dependency vulnerability scanning (in security-scan.yml)
4. **CodeQL**: Advanced security analysis (in security-scan.yml)

Security issues are reported but don't block merges (continue-on-error: true) to allow human review.

---

## Further Reading

- [BRANCHING_STRATEGY.md](ci-cd/branching-release-history/BRANCHING_STRATEGY.md) - Full Git Flow strategy (German)
- [BRANCHING_STRATEGY_EN.md](ci-cd/branching-release-history/BRANCHING_STRATEGY_EN.md) - Git Flow strategy (English)
- [BRANCH_PROTECTION_SETUP.md](ci-cd/branching-release-history/BRANCH_PROTECTION_SETUP.md) - Branch protection configuration
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Git Flow Original Article](https://nvie.com/posts/a-successful-git-branching-model/)

---

**Last Updated**: 2026-04-06  
**Maintained by**: ThemisDB Core Team
