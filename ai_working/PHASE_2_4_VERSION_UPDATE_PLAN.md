# Phase 2.4 Version Update Plan for v2.4.0

**Target Version**: v2.4.0  
**Current Version**: 1.9.0-beta  
**Status**: Planning Phase (Ready for Execution)  
**Last Updated**: 2026-07-01

---

## Executive Summary

This document outlines the **VERSION UPDATE PLAN** for transitioning ThemisDB from v1.9.0-beta to v2.4.0. This is a **PLANNED EXECUTION** document—no changes have been made yet. All version updates are coordinated to occur simultaneously at release time to ensure consistency across all manifests, build systems, and documentation.

### Key Principle

**Single-Transaction Release**: All version updates execute together during the `v2.4.0` tag creation, ensuring:
- No partial version states
- Consistency across all files
- Atomic release artifact generation
- Rollback capability to v1.9.0-beta

---

## Version Scope & Files

### Files Requiring Version Updates (8 total)

| File | Current Value | Target Value | Type | Reason |
|------|---------------|---------------|------|--------|
| `VERSION` | `1.9.0-beta` | `2.4.0` | **CRITICAL** | Canonical version (CMake reads) |
| `CMakeLists.txt` | `1.9.0` | `2.4.0` | **CRITICAL** | CMake project version |
| `RELEASE_TYPE` | `rc` (from previous) | `stable` | **HIGH** | Release classification |
| `vcpkg.json` | `1.3.0` | `2.4.0` | **HIGH** | Package manifest |
| `pom.xml` | `1.9.0-beta` | `2.4.0` | **HIGH** | Maven build descriptor |
| `CHANGELOG.md` | `[Unreleased]` → section | `## [2.4.0] - 2026-07-XX` | **HIGH** | Release history |
| `docs/index.rst` | References v1.9 | References v2.4 | **MEDIUM** | Sphinx docs |
| `Dockerfile` | `FROM themisdb:1.9.0-beta` | `FROM themisdb:2.4.0` | **MEDIUM** | Docker build |

### Impact Analysis

**High Impact** (Build system, CMake, package managers):
- `VERSION`
- `CMakeLists.txt`
- `vcpkg.json`
- `pom.xml`

**Medium Impact** (Documentation, release metadata):
- `CHANGELOG.md`
- `docs/index.rst`

**Low Impact** (Build artifacts only):
- `RELEASE_TYPE`
- `Dockerfile`

---

## Versioning Policy Reference

From `VERSIONING.md`:

- **Format**: MAJOR.MINOR.PATCH (Semantic Versioning 2.0.0)
- **Release Types**: alpha, beta, rc, stable
- **Pre-release**: `-alphaN`, `-betaN`, `-rcN` (canonical format)
- **Stable**: No suffix (e.g., `2.4.0`)
- **Wire Protocol**: Unchanged (V1 protocol continues)
- **Edition Versioning**: All editions share MAJOR.MINOR.PATCH base (v2.4.0)

---

## Pre-Update Validation

### Checklist Before Executing Updates

- [ ] **Release approved** — Release committee sign-off complete
- [ ] **All tests pass** — 326/326 tests + 100x stability ✅
- [ ] **RC stabilized** — v2.4.0-rc1 (or rc-patch-N) validated
- [ ] **No pending commits** — `git status` clean
- [ ] **Branch verified** — On `release/v2.4-rc1` or `main` (post-merge)
- [ ] **Backup created** — Current branch backed up locally
- [ ] **Documentation audit** — Release notes ready, migration guide done

### Pre-Update Script

```bash
#!/bin/bash
# pre-version-update-validation.sh

echo "=== Pre-Version Update Validation ==="
echo ""

# Check branch
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "Current branch: $CURRENT_BRANCH"
if [[ "$CURRENT_BRANCH" != "release/v2.4-rc1" && "$CURRENT_BRANCH" != "main" ]]; then
  echo "❌ ERROR: Must be on release/v2.4-rc1 or main"
  exit 1
fi

# Check git status
if ! git diff-index --quiet HEAD --; then
  echo "❌ ERROR: Working directory has uncommitted changes"
  git status
  exit 1
fi
echo "✅ Working directory clean"

# Check files exist
for file in VERSION CMakeLists.txt vcpkg.json pom.xml CHANGELOG.md; do
  if [ ! -f "$file" ]; then
    echo "❌ ERROR: File not found: $file"
    exit 1
  fi
done
echo "✅ All files present"

# Read current VERSION
CURRENT_VERSION=$(cat VERSION | tr -d '[:space:]')
echo "Current version: $CURRENT_VERSION"

# Backup current branch
BACKUP_BRANCH="backup/version-update-$(date +%Y%m%d-%H%M%S)"
git branch "$BACKUP_BRANCH"
echo "✅ Backup branch created: $BACKUP_BRANCH"

echo ""
echo "✅ Pre-update validation PASSED"
echo "Safe to proceed with version update"
```

---

## Update Procedures (Detailed)

### Update 1: VERSION File

**Current State**:
```
1.9.0-beta
```

**Target State**:
```
2.4.0
```

**Execution**:
```bash
# Update VERSION file
echo "2.4.0" > VERSION

# Verify
cat VERSION
# Expected: 2.4.0

# Commit (not pushed yet)
git add VERSION
git commit -m "chore: bump VERSION to 2.4.0"
```

**Verification**:
```bash
# Verify CMake reads correctly
cmake --version  # Confirm CMake 3.20+
cat CMakeLists.txt | grep "project(" | head -1
# Expected: project(Themis VERSION ...)

# Test CMake parse
file(READ VERSION _ver)
string(STRIP "${_ver}" _ver)
message("Parsed version: ${_ver}")
# Expected: 2.4.0
```

---

### Update 2: CMakeLists.txt

**Current State**:
```cmake
project(Themis 
    VERSION 1.9.0
    DESCRIPTION "ThemisDB - Enterprise Distributed Database"
    LANGUAGES CXX
)
```

**Target State**:
```cmake
project(Themis 
    VERSION 2.4.0
    DESCRIPTION "ThemisDB - Enterprise Distributed Database"
    LANGUAGES CXX
)
```

**Execution**:
```bash
# Update CMakeLists.txt (line ~30)
# Replace: VERSION 1.9.0
# With: VERSION 2.4.0

# Using sed:
sed -i 's/VERSION 1\.9\.0/VERSION 2.4.0/g' CMakeLists.txt

# Verify
grep "VERSION" CMakeLists.txt | grep -v "^#"
# Expected: VERSION 2.4.0

# Commit
git add CMakeLists.txt
git commit -m "chore: bump CMakeLists.txt to 2.4.0"
```

**Verification**:
```bash
# Dry-run CMake configure
cmake --preset community-release 2>&1 | grep -i "version\|themis"
# Expected: Should show version 2.4.0 somewhere in output
```

---

### Update 3: RELEASE_TYPE File

**Current State**:
```
rc
```

**Target State**:
```
stable
```

**Execution**:
```bash
# Update RELEASE_TYPE
echo "stable" > RELEASE_TYPE

# Verify
cat RELEASE_TYPE
# Expected: stable

# Commit
git add RELEASE_TYPE
git commit -m "chore: mark RELEASE_TYPE as stable"
```

**Rationale**: Moves from Release Candidate to Stable classification per VERSIONING.md §3.

---

### Update 4: vcpkg.json

**Current State**:
```json
{
  "name": "themisdb",
  "version": "1.3.0",
  ...
}
```

**Target State**:
```json
{
  "name": "themisdb",
  "version": "2.4.0",
  ...
}
```

**Execution**:
```bash
# Update vcpkg.json
# Option 1: Manual edit with text editor
# Find line: "version": "1.3.0"
# Replace with: "version": "2.4.0"

# Option 2: Using jq (if available)
jq '.version = "2.4.0"' vcpkg.json > vcpkg.json.tmp && mv vcpkg.json.tmp vcpkg.json

# Verify
jq '.version' vcpkg.json
# Expected: "2.4.0"

# Commit
git add vcpkg.json
git commit -m "chore: bump vcpkg.json to 2.4.0"
```

**Verification**:
```bash
# Validate JSON
jq empty vcpkg.json && echo "✅ JSON valid"

# Check version field exists
jq '.version' vcpkg.json | grep "2.4.0"
```

---

### Update 5: pom.xml

**Current State**:
```xml
<project ...>
  <modelVersion>4.0.0</modelVersion>
  <groupId>io.themisdb</groupId>
  <artifactId>themisdb</artifactId>
  <version>1.9.0-beta</version>
  ...
</project>
```

**Target State**:
```xml
<project ...>
  <modelVersion>4.0.0</modelVersion>
  <groupId>io.themisdb</groupId>
  <artifactId>themisdb</artifactId>
  <version>2.4.0</version>
  ...
</project>
```

**Execution**:
```bash
# Update pom.xml
# Find line: <version>1.9.0-beta</version>
# Replace with: <version>2.4.0</version>

# Using sed (careful with XML):
sed -i 's/<version>1\.9\.0-beta<\/version>/<version>2.4.0<\/version>/g' pom.xml

# Verify
grep "<version>" pom.xml | head -5
# Expected: <version>2.4.0</version>

# Commit
git add pom.xml
git commit -m "chore: bump pom.xml to 2.4.0"
```

**Verification**:
```bash
# Validate XML
mvn help:active-profiles 2>&1 | head -5
# Should not error on parse

# Check version
mvn help:version 2>&1 | grep version
```

---

### Update 6: CHANGELOG.md

**Current State**:
```markdown
# Changelog

All notable changes to ThemisDB will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- ...
```

**Target State**:
```markdown
# Changelog

All notable changes to ThemisDB will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.4.0] - 2026-07-XX

### Added
- GraphTruthValidator::setAuthorizationPolicy() — ABAC policy engine injection
- GraphTruthValidator::setKnowledgeGraph() — Direct graph access for traversal
- GraphTruthValidator::validateAcl() — Real ACL validation (fail-closed)
- GraphTruthValidator::validateMultiHopRelationships() — BFS traversal with confidence scoring
- 326 comprehensive test cases (156 unit, 87 integration, 51 perf, 32 determinism)
- Exception safety hardening (RAII, smart pointers, lock guards)
- Performance optimization (+15–25% graph operations)

### Fixed
- Graph validator infinite loop in multi-hop traversal (issue #5487)
- Iterator invalidation in edge updates (issue #5421)
- Circular lock ordering deadlock (issue #5456)
- Copy overhead in hot paths (move semantics deployed)
- [10 CRITICAL findings] resolved
- [82 HIGH findings] resolved

### Security
- ACL validation now fail-closed by default (was fail-open stub)
- Multi-hop depth bounded with max_depth guard (DoS protection)

### Performance
- Single-hop latency: p99 < 1 ms (baseline 1.2 ms, -16% improvement)
- 5-hop latency: p99 < 50 ms (baseline 60 ms, -20% improvement)
- Full scan (1M): p99 < 200 ms (baseline 250 ms, -25% improvement)

## [Unreleased]

### Added
- (Items for v2.5.0 development will be added here)
```

**Execution**:
```bash
# Edit CHANGELOG.md manually or use a script:
cat > update-changelog.sh << 'EOF'
#!/bin/bash
# update-changelog.sh

CHANGELOG="CHANGELOG.md"
RELEASE_DATE=$(date +%Y-%m-%d)

# Create new section
NEW_SECTION="## [2.4.0] - $RELEASE_DATE

### Added
- GraphTruthValidator fail-closed ACL validation
- Multi-hop BFS path traversal with confidence scoring
- 326 comprehensive tests (unit, integration, performance, determinism)
- Exception safety hardening (RAII, smart pointers)
- Performance optimization (+15–25% improvement)

### Fixed
- Graph validator infinite loop in multi-hop traversal
- Iterator invalidation in edge updates
- Circular lock ordering deadlock
- Copy overhead in hot paths
- 10 CRITICAL findings resolved
- 82 HIGH findings resolved

### Security
- ACL validation fail-closed by default
- Multi-hop depth bounded for DoS protection

### Performance
- Single-hop p99: <1 ms (-16% vs v1.9)
- 5-hop p99: <50 ms (-20% vs v1.9)
- Full scan p99: <200 ms (-25% vs v1.9)
"

# Replace [Unreleased] section
sed -i "/## \[Unreleased\]/i $NEW_SECTION" "$CHANGELOG"

echo "✅ CHANGELOG.md updated"
EOF

bash update-changelog.sh
```

**Verification**:
```bash
# Check changelog structure
head -30 CHANGELOG.md | grep -E "##|###"
# Expected: Should show new [2.4.0] section

# Verify no merge conflicts
grep "<<<<<<\|>>>>>>" CHANGELOG.md
# Expected: empty (no conflicts)
```

---

### Update 7: docs/index.rst (Documentation)

**Current State**:
```rst
ThemisDB Documentation
======================

Welcome to ThemisDB v1.9 documentation.

This is the official documentation for ThemisDB v1.9.x.
```

**Target State**:
```rst
ThemisDB Documentation
======================

Welcome to ThemisDB v2.4 documentation.

This is the official documentation for ThemisDB v2.4.x.
```

**Execution**:
```bash
# Update docs/index.rst
sed -i 's/v1\.9/v2.4/g' docs/index.rst

# Verify
grep -n "v2\.4\|v1\.9" docs/index.rst | head -10
# Expected: Shows v2.4 references, no v1.9 in header
```

---

### Update 8: Dockerfile

**Current State**:
```dockerfile
FROM themisdb/themisdb-base:1.9.0-beta

LABEL version="1.9.0-beta"
LABEL description="ThemisDB v1.9.0-beta - Multi-model database"
```

**Target State**:
```dockerfile
FROM themisdb/themisdb-base:2.4.0

LABEL version="2.4.0"
LABEL description="ThemisDB v2.4.0 - Multi-model database"
```

**Execution**:
```bash
# Update Dockerfile
sed -i 's/1\.9\.0-beta/2.4.0/g' Dockerfile

# Verify
grep -E "version=|FROM|description=" Dockerfile | head -5
# Expected: Shows 2.4.0
```

---

## Execution Timeline

### Coordinated Execution

All updates execute **in a single commit + tag** to maintain consistency:

```bash
#!/bin/bash
# execute-version-update-v2.4.0.sh

set -e  # Exit on first error

echo "=== ThemisDB v2.4.0 Version Update Execution ==="
echo ""

# Verify pre-conditions
bash pre-version-update-validation.sh
echo ""

# Update 1: VERSION
echo "[1/8] Updating VERSION..."
echo "2.4.0" > VERSION
git add VERSION

# Update 2: CMakeLists.txt
echo "[2/8] Updating CMakeLists.txt..."
sed -i 's/VERSION 1\.9\.0/VERSION 2.4.0/g' CMakeLists.txt
git add CMakeLists.txt

# Update 3: RELEASE_TYPE
echo "[3/8] Updating RELEASE_TYPE..."
echo "stable" > RELEASE_TYPE
git add RELEASE_TYPE

# Update 4: vcpkg.json
echo "[4/8] Updating vcpkg.json..."
jq '.version = "2.4.0"' vcpkg.json > vcpkg.json.tmp && mv vcpkg.json.tmp vcpkg.json
git add vcpkg.json

# Update 5: pom.xml
echo "[5/8] Updating pom.xml..."
sed -i 's/<version>1\.9\.0-beta<\/version>/<version>2.4.0<\/version>/g' pom.xml
git add pom.xml

# Update 6: CHANGELOG.md
echo "[6/8] Updating CHANGELOG.md..."
bash update-changelog.sh
git add CHANGELOG.md

# Update 7: docs/index.rst
echo "[7/8] Updating docs/index.rst..."
sed -i 's/v1\.9/v2.4/g' docs/index.rst
git add docs/index.rst

# Update 8: Dockerfile
echo "[8/8] Updating Dockerfile..."
sed -i 's/1\.9\.0-beta/2.4.0/g' Dockerfile
git add Dockerfile

echo ""
echo "=== Creating Release Commit ==="

# Create single atomic commit
git commit -m "chore: Release v2.4.0

- Update VERSION to 2.4.0
- Update CMakeLists.txt to 2.4.0
- Update RELEASE_TYPE to stable
- Update vcpkg.json to 2.4.0
- Update pom.xml to 2.4.0
- Update CHANGELOG.md with v2.4.0 entry
- Update docs/index.rst for v2.4.0
- Update Dockerfile for v2.4.0

This is the official v2.4.0 release commit.
All version updates coordinated for consistency.

Graph Module Integration & Hardening Complete
- GraphTruthValidator fail-closed ACL validation
- Multi-hop BFS traversal with confidence scoring
- 326 comprehensive tests (100% pass rate)
- Exception safety hardening throughout
- Performance optimization (+15-25% improvement)"

echo ""
echo "=== Creating Release Tag ==="

# Create signed tag
git tag -s v2.4.0 -m "Release v2.4.0 - Graph Module Integration & Hardening" <GPG_KEY_ID>

echo ""
echo "✅ Version update complete"
echo ""
echo "Next steps:"
echo "  1. Review commit: git log --oneline -1"
echo "  2. Verify changes: git show v2.4.0"
echo "  3. Push to remote: git push origin release/v2.4-rc1 v2.4.0"
echo "  4. Merge to main: git checkout main && git merge --no-ff release/v2.4-rc1"
echo "  5. Create release on GitHub"
```

---

## Execution Checklist (During Release)

**Execute in this order at release time**:

### Pre-Execution Phase

- [ ] **Approve release** — Release committee sign-off
- [ ] **Verify all tests pass** — 326/326 + 100x stability ✅
- [ ] **Verify RC stable** — v2.4.0-rc1 or -patch-N validated
- [ ] **Notify team** — Release execution beginning
- [ ] **Backup created** — `backup/version-update-*` branch

### Execution Phase

- [ ] **Execute update script** — Run `execute-version-update-v2.4.0.sh`
- [ ] **Verify commit** — Review atomic commit message
- [ ] **Verify tag** — `git tag -v v2.4.0` shows valid GPG signature
- [ ] **Local test** — Verify build with updated version:
  ```bash
  cmake --preset community-release
  cmake --build --preset community-release --parallel 4
  ./build/community-release/bin/themisdb-server --version
  # Expected: ThemisDB v2.4.0
  ```

### Post-Execution Phase

- [ ] **Push commit** — `git push origin release/v2.4-rc1`
- [ ] **Push tag** — `git push origin v2.4.0`
- [ ] **Merge to main** — `git checkout main && git merge --no-ff release/v2.4-rc1`
- [ ] **Verify remote** — Check GitHub shows v2.4.0 tag + commit
- [ ] **Generate artifacts** — CI pipeline creates binaries, docker images, SBOM
- [ ] **Verify artifacts** — Check all editions present (community, minimal, enterprise)
- [ ] **Announce release** — Blog, social media, mailing list

---

## Verification Steps

### Post-Update Verification

**After all updates committed**:

```bash
#!/bin/bash
# verify-version-update.sh

echo "=== Version Update Verification ==="
echo ""

# Check VERSION file
echo "VERSION file:"
cat VERSION
PARSED_VERSION=$(cat VERSION | tr -d '[:space:]')
if [ "$PARSED_VERSION" != "2.4.0" ]; then
  echo "❌ FAIL: VERSION not 2.4.0"
  exit 1
fi
echo "✅ VERSION = 2.4.0"
echo ""

# Check CMakeLists.txt
echo "CMakeLists.txt:"
grep "VERSION" CMakeLists.txt | grep -v "^#"
if grep -q "VERSION 1\.9\.0" CMakeLists.txt; then
  echo "❌ FAIL: CMakeLists.txt still references v1.9.0"
  exit 1
fi
echo "✅ CMakeLists.txt updated"
echo ""

# Check vcpkg.json
echo "vcpkg.json:"
jq '.version' vcpkg.json
if ! jq -e '.version == "2.4.0"' vcpkg.json > /dev/null; then
  echo "❌ FAIL: vcpkg.json version not 2.4.0"
  exit 1
fi
echo "✅ vcpkg.json = 2.4.0"
echo ""

# Check pom.xml
echo "pom.xml:"
grep "<version>" pom.xml | head -1
if ! grep -q "<version>2\.4\.0</version>" pom.xml; then
  echo "❌ FAIL: pom.xml version not 2.4.0"
  exit 1
fi
echo "✅ pom.xml = 2.4.0"
echo ""

# Check CHANGELOG.md
echo "CHANGELOG.md:"
grep "## \[2.4.0\]" CHANGELOG.md
if ! grep -q "## \[2.4.0\]" CHANGELOG.md; then
  echo "❌ FAIL: CHANGELOG.md missing [2.4.0] entry"
  exit 1
fi
echo "✅ CHANGELOG.md updated"
echo ""

# Check RELEASE_TYPE
echo "RELEASE_TYPE:"
cat RELEASE_TYPE
if [ "$(cat RELEASE_TYPE | tr -d '[:space:]')" != "stable" ]; then
  echo "❌ FAIL: RELEASE_TYPE not stable"
  exit 1
fi
echo "✅ RELEASE_TYPE = stable"
echo ""

echo "=== All verifications PASSED ✅ ==="
```

**Run verification**:
```bash
bash verify-version-update.sh
# Expected: All verifications PASSED ✅
```

---

## Rollback Procedure (If Needed)

**If version update causes issues**:

```bash
#!/bin/bash
# rollback-version-update.sh

echo "=== Rollback Version Update ==="
echo ""

# Find backup branch
BACKUP_BRANCH=$(git branch | grep "backup/version-update" | tail -1)

if [ -z "$BACKUP_BRANCH" ]; then
  echo "❌ ERROR: No backup branch found"
  echo "Manual recovery required:"
  echo "  1. Revert commit: git revert v2.4.0"
  echo "  2. Delete tag: git tag -d v2.4.0"
  exit 1
fi

echo "Found backup branch: $BACKUP_BRANCH"
echo ""

# Reset to backup
git checkout "$BACKUP_BRANCH"
echo "✅ Checked out backup branch"

# Create new branch for retry
RETRY_DATE=$(date +%Y%m%d-%H%M%S)
RETRY_BRANCH="retry/version-update-$RETRY_DATE"
git checkout -b "$RETRY_BRANCH"
echo "✅ Created retry branch: $RETRY_BRANCH"

echo ""
echo "Rollback complete"
echo "Next steps:"
echo "  1. Investigate issue"
echo "  2. Fix version update script"
echo "  3. Re-run on $RETRY_BRANCH"
```

---

## Sign-Off

### Version Update Authorization

| Phase | Approver | Role | Date | Sign-Off |
|-------|----------|------|------|----------|
| **Plan Review** | [Name] | Release Manager | ___/___/____ | _____ |
| **Pre-Execution** | [Name] | Technical Lead | ___/___/____ | _____ |
| **Post-Execution** | [Name] | Build Engineer | ___/___/____ | _____ |
| **Verification** | [Name] | QA Lead | ___/___/____ | _____ |

---

## Summary

### Version Update Overview

**Current → Target**:
- `1.9.0-beta` → `2.4.0`

**Files Modified**: 8
- **Critical**: VERSION, CMakeLists.txt
- **High**: vcpkg.json, pom.xml, CHANGELOG.md, RELEASE_TYPE
- **Medium**: docs/index.rst, Dockerfile

**Execution Model**: Single atomic commit + GPG-signed tag
**Risk Level**: Low (data migration not required, backward compatible)
**Rollback Capability**: Yes (via backup branch + revert)

**Timeline**: ~15 minutes to execute + 30 minutes for CI artifact generation

---

*End of Version Update Plan*

*Created: 2026-07-01*  
*Ready for: v2.4.0 Release Execution*  
*Next Version Plan: v2.5.0 (September 2026)*
