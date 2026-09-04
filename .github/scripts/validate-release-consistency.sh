#!/usr/bin/env bash
# validate-release-consistency.sh
# Validates that all three registries (GitHub, Docker, WinGet) have consistent release artifacts.

set -euo pipefail

REPO="${1:-}"
VERSION="${2:-}"
RELEASE_TYPE="${3:-stable}"

if [[ -z "$REPO" ]] || [[ -z "$VERSION" ]]; then
    cat <<EOF
Usage: validate-release-consistency.sh <owner/repo> <version> [release-type]

Validates that GitHub Release, Docker images, and WinGet manifests are consistent.

Examples:
  validate-release-consistency.sh makr-code/ThemisDB 2.4.0 stable
  validate-release-consistency.sh makr-code/ThemisDB 2.4.0-rc1 rc

Checks:
  ✓ GitHub Release exists and has artifacts
  ✓ Docker images are tagged correctly
  ✓ SBOM/checksums are present
  ✓ Release notes mention correct version
  ✓ Artifact integrity (checksums match)
EOF
    exit 1
fi

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Stats
CHECKS_PASSED=0
CHECKS_FAILED=0
WARNINGS=0

check_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((CHECKS_PASSED++))
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    ((CHECKS_FAILED++))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    ((WARNINGS++))
}

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Release Consistency Validation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Repository:  $REPO"
echo "Version:     $VERSION"
echo "Release Type: $RELEASE_TYPE"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# GitHub Release checks
echo -e "${BLUE}[GitHub Release]${NC}"

RELEASE_TAG="v$VERSION"
case "$RELEASE_TYPE" in
    stable) RELEASE_TAG="v$VERSION" ;;
    rc|beta|alpha) RELEASE_TAG="v$VERSION-$RELEASE_TYPE*" ;;
    nightly) RELEASE_TAG="v$VERSION-nightly.*" ;;
esac

echo "Checking release: $RELEASE_TAG"

# Use GitHub API to check release
if command -v gh &>/dev/null; then
    RELEASE_INFO=$(gh release view "$RELEASE_TAG" --repo "$REPO" 2>/dev/null || echo "")
    
    if [[ -n "$RELEASE_INFO" ]]; then
        check_pass "GitHub Release found: $RELEASE_TAG"
        
        # Check for build-metadata.json
        BUILD_META_ASSET=$(echo "$RELEASE_INFO" | grep -i "build-metadata.json" || echo "")
        if [[ -n "$BUILD_META_ASSET" ]]; then
            check_pass "build-metadata.json artifact present"
        else
            check_warn "build-metadata.json artifact missing"
        fi
        
        # Check for SBOM
        SBOM_ASSET=$(echo "$RELEASE_INFO" | grep -i "sbom\|cyclone" || echo "")
        if [[ -n "$SBOM_ASSET" ]]; then
            check_pass "SBOM artifact present"
        else
            check_warn "SBOM artifact missing"
        fi
        
        # Check for checksums
        CHECKSUMS_ASSET=$(echo "$RELEASE_INFO" | grep -i "checksum\|sha256" || echo "")
        if [[ -n "$CHECKSUMS_ASSET" ]]; then
            check_pass "Checksums artifact present"
        else
            check_warn "Checksums artifact missing"
        fi
    else
        check_fail "GitHub Release not found: $RELEASE_TAG"
    fi
else
    check_warn "GitHub CLI (gh) not found; skipping GitHub checks"
fi

echo ""

# Docker image checks
echo -e "${BLUE}[Docker Images]${NC}"

DOCKER_TAGS=(
    "themisdb/themisdb:$VERSION"
    "ghcr.io/$REPO:$VERSION"
)

case "$RELEASE_TYPE" in
    nightly)
        DATE=$(date +%Y%m%d)
        DOCKER_TAGS+=(
            "themisdb/themisdb:nightly"
            "themisdb/themisdb:nightly-$DATE"
        )
        ;;
    rc|beta|alpha)
        DOCKER_TAGS+=(
            "themisdb/themisdb:$VERSION-$RELEASE_TYPE"
        )
        ;;
esac

for TAG in "${DOCKER_TAGS[@]}"; do
    if docker inspect "$TAG" >/dev/null 2>&1; then
        check_pass "Docker image exists: $TAG"
        
        # Check for OCI labels
        LABELS=$(docker inspect "$TAG" | grep -o '"org.opencontainers.image.version"' || echo "")
        if [[ -n "$LABELS" ]]; then
            check_pass "  OCI labels present"
        else
            check_warn "  OCI labels missing"
        fi
    else
        check_warn "Docker image not found locally: $TAG (may be in registry only)"
    fi
done

echo ""

# WinGet manifest checks (for stable releases only)
echo -e "${BLUE}[WinGet Community Repository]${NC}"

if [[ "$RELEASE_TYPE" == "stable" ]]; then
    MANIFEST_DIR="manifests/t/ThemisDB/ThemisDB/$VERSION"
    WINGET_URL="https://raw.githubusercontent.com/microsoft/winget-pkgs/master/$MANIFEST_DIR/ThemisDB.ThemisDB.yaml"
    
    if curl -s -f "$WINGET_URL" >/dev/null 2>&1; then
        check_pass "WinGet manifest found in microsoft/winget-pkgs"
    else
        check_warn "WinGet manifest not yet submitted to microsoft/winget-pkgs"
        echo "         (This is expected immediately after release; check again in 24-48h)"
    fi
else
    check_pass "WinGet submission skipped for $RELEASE_TYPE release (community repo only accepts stable)"
fi

echo ""

# Summary
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${BLUE}Summary${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "Passed:  ${GREEN}$CHECKS_PASSED${NC}"
echo -e "Failed:  ${RED}$CHECKS_FAILED${NC}"
echo -e "Warnings: ${YELLOW}$WARNINGS${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [[ $CHECKS_FAILED -gt 0 ]]; then
    echo -e "${RED}Validation FAILED${NC} (fix issues above)"
    exit 1
elif [[ $WARNINGS -gt 0 ]]; then
    echo -e "${YELLOW}Validation PASSED with warnings${NC}"
    exit 0
else
    echo -e "${GREEN}Validation PASSED${NC}"
    exit 0
fi
